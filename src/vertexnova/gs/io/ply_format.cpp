/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   September 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include "vertexnova/gs/io/ply_format.h"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstring>
#include <istream>
#include <limits>
#include <sstream>

namespace vne::gs::detail {

namespace {

/** @brief Strips a trailing CR so CRLF headers parse on every platform. */
void stripCarriageReturn(std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

[[nodiscard]] bool parseCount(const std::string& text, std::size_t& out_count, std::string& out_error) {
    if (text.empty() || text[0] == '-') {
        out_error = "invalid element count: " + text;
        return false;
    }
    std::uint64_t parsed = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed);
    if (ec != std::errc{} || ptr != end) {
        out_error = "invalid element count: " + text;
        return false;
    }
    if (parsed > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        out_error = "element count exceeds size_t range: " + text;
        return false;
    }
    out_count = static_cast<std::size_t>(parsed);
    return true;
}

}  // namespace

std::size_t plyTypeSize(PlyType type) noexcept {
    switch (type) {
        case PlyType::eInt8:
        case PlyType::eUint8:
            return 1;
        case PlyType::eInt16:
        case PlyType::eUint16:
            return 2;
        case PlyType::eInt32:
        case PlyType::eUint32:
        case PlyType::eFloat32:
            return 4;
        case PlyType::eFloat64:
            return 8;
    }
    return 0;
}

bool plyTypeFromName(const std::string& name, PlyType& out_type) noexcept {
    // Both the short and the explicitly sized spellings appear in the wild.
    if (name == "char" || name == "int8") {
        out_type = PlyType::eInt8;
    } else if (name == "uchar" || name == "uint8") {
        out_type = PlyType::eUint8;
    } else if (name == "short" || name == "int16") {
        out_type = PlyType::eInt16;
    } else if (name == "ushort" || name == "uint16") {
        out_type = PlyType::eUint16;
    } else if (name == "int" || name == "int32") {
        out_type = PlyType::eInt32;
    } else if (name == "uint" || name == "uint32") {
        out_type = PlyType::eUint32;
    } else if (name == "float" || name == "float32") {
        out_type = PlyType::eFloat32;
    } else if (name == "double" || name == "float64") {
        out_type = PlyType::eFloat64;
    } else {
        return false;
    }
    return true;
}

const PlyProperty* PlyElement::find(const std::string& property_name) const noexcept {
    const auto it = std::find_if(properties.begin(), properties.end(), [&property_name](const PlyProperty& p) {
        return p.name == property_name;
    });
    return (it == properties.end()) ? nullptr : &(*it);
}

const PlyElement* PlyHeader::find(const std::string& name) const noexcept {
    const std::size_t index = indexOf(name);
    return (index == elements.size()) ? nullptr : &elements[index];
}

std::size_t PlyHeader::indexOf(const std::string& name) const noexcept {
    for (std::size_t i = 0; i < elements.size(); ++i) {
        if (elements[i].name == name) {
            return i;
        }
    }
    return elements.size();
}

bool parsePlyHeader(std::istream& in, PlyHeader& out_header, std::string& out_error) {
    out_header.elements.clear();

    std::string line;
    if (!std::getline(in, line)) {
        out_error = "empty PLY file";
        return false;
    }
    stripCarriageReturn(line);
    if (line != "ply") {
        out_error = "missing ply magic";
        return false;
    }

    bool has_format = false;
    while (std::getline(in, line)) {
        stripCarriageReturn(line);
        if (line.empty()) {
            continue;
        }

        std::istringstream tokens(line);
        std::string keyword;
        tokens >> keyword;

        if (keyword == "comment" || keyword == "obj_info") {
            continue;
        }

        if (keyword == "format") {
            std::string format;
            tokens >> format;
            if (format == "ascii") {
                out_error = "ASCII PLY is not supported; expected binary_little_endian";
                return false;
            }
            if (format == "binary_big_endian") {
                out_error = "big-endian PLY is not supported; expected binary_little_endian";
                return false;
            }
            if (format != "binary_little_endian") {
                out_error = "unsupported PLY format: " + format;
                return false;
            }
            has_format = true;
            continue;
        }

        if (keyword == "element") {
            PlyElement element;
            std::string count_text;
            tokens >> element.name >> count_text;
            if (element.name.empty()) {
                out_error = "element without a name";
                return false;
            }
            if (!parseCount(count_text, element.count, out_error)) {
                return false;
            }
            out_header.elements.push_back(std::move(element));
            continue;
        }

        if (keyword == "property") {
            if (out_header.elements.empty()) {
                out_error = "property declared before any element";
                return false;
            }
            PlyElement& element = out_header.elements.back();

            std::string type_name;
            tokens >> type_name;
            if (type_name == "list") {
                // Variable-length records: we cannot compute a stride, so any
                // element that has one can only be parsed, never skipped.
                element.has_list = true;
                std::string count_type;
                std::string value_type;
                std::string name;
                tokens >> count_type >> value_type >> name;
                continue;
            }

            PlyProperty property;
            tokens >> property.name;
            if (property.name.empty()) {
                out_error = "property without a name in element " + element.name;
                return false;
            }
            if (!plyTypeFromName(type_name, property.type)) {
                out_error = "unsupported property type '" + type_name + "' for " + property.name;
                return false;
            }
            if (element.find(property.name) != nullptr) {
                out_error = "duplicate property " + property.name + " in element " + element.name;
                return false;
            }
            property.offset = element.stride;
            element.stride += plyTypeSize(property.type);
            element.properties.push_back(std::move(property));
            continue;
        }

        if (keyword == "end_header") {
            if (!has_format) {
                out_error = "missing format line before end_header";
                return false;
            }
            if (out_header.elements.empty()) {
                out_error = "PLY header declares no elements";
                return false;
            }
            return true;
        }
    }

    out_error = "missing end_header";
    return false;
}

float readFloatLE(const std::uint8_t* bytes) noexcept {
    std::uint8_t ordered[sizeof(float)];
    if constexpr (std::endian::native == std::endian::little) {
        std::memcpy(ordered, bytes, sizeof(float));
    } else {
        ordered[0] = bytes[3];
        ordered[1] = bytes[2];
        ordered[2] = bytes[1];
        ordered[3] = bytes[0];
    }
    float value = 0.0f;
    std::memcpy(&value, ordered, sizeof(float));
    return value;
}

void writeFloatLE(std::uint8_t* out, float value) noexcept {
    std::uint8_t host[sizeof(float)];
    std::memcpy(host, &value, sizeof(float));
    if constexpr (std::endian::native == std::endian::little) {
        std::memcpy(out, host, sizeof(float));
    } else {
        out[0] = host[3];
        out[1] = host[2];
        out[2] = host[1];
        out[3] = host[0];
    }
}

}  // namespace vne::gs::detail
