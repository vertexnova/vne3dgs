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

#include "vertexnova/gs/io/ply_reader.h"

#include "vertexnova/logging/logging.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

CREATE_VNE_LOGGER_CATEGORY("vne.gs.ply")

constexpr float kLogitEps = 1e-6f;

void reportError(std::string* error, const std::string& message) {
    VNE_LOG_ERROR << message;
    if (error != nullptr) {
        *error = message;
    }
}

[[nodiscard]] float sigmoid(float value) noexcept {
    return 1.0f / (1.0f + std::exp(-value));
}

[[nodiscard]] float logit(float opacity) noexcept {
    const float clamped = std::clamp(opacity, kLogitEps, 1.0f - kLogitEps);
    return std::log(clamped / (1.0f - clamped));
}

[[nodiscard]] int degreeFromRestCount(int rest_count) noexcept {
    switch (rest_count) {
        case 0:
            return 0;
        case 9:
            return 1;
        case 24:
            return 2;
        case 45:
            return 3;
        default:
            return -1;
    }
}

[[nodiscard]] float readFloatLE(const std::uint8_t* bytes) noexcept {
    float value = 0.0f;
    std::memcpy(&value, bytes, sizeof(float));
    return value;
}

void writeFloatLE(std::ostream& out, float value) {
    std::uint8_t bytes[sizeof(float)];
    std::memcpy(bytes, &value, sizeof(float));
    out.write(reinterpret_cast<const char*>(bytes), sizeof(float));
}

struct PlyHeader {
    std::size_t vertex_count = 0;
    std::vector<std::string> property_names;
    bool binary_little_endian = false;
};

[[nodiscard]] bool parseHeader(std::istream& in, PlyHeader& header, std::string* error) {
    std::string line;
    if (!std::getline(in, line)) {
        reportError(error, "empty PLY file");
        return false;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    if (line != "ply") {
        reportError(error, "missing ply magic");
        return false;
    }

    bool in_vertex = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
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
            std::string version;
            tokens >> format >> version;
            if (format == "ascii") {
                reportError(error, "ASCII PLY is not supported; expected binary_little_endian");
                return false;
            }
            if (format == "binary_big_endian") {
                reportError(error, "big-endian PLY is not supported; expected binary_little_endian");
                return false;
            }
            if (format != "binary_little_endian") {
                reportError(error, "unsupported PLY format: " + format);
                return false;
            }
            header.binary_little_endian = true;
            continue;
        }
        if (keyword == "element") {
            std::string name;
            std::size_t count = 0;
            tokens >> name >> count;
            in_vertex = (name == "vertex");
            if (in_vertex) {
                header.vertex_count = count;
                header.property_names.clear();
            }
            continue;
        }
        if (keyword == "property") {
            if (!in_vertex) {
                continue;
            }
            std::string type;
            std::string name;
            tokens >> type >> name;
            if (type == "list") {
                reportError(error, "list properties are not supported in Gaussian PLY");
                return false;
            }
            if (type != "float" && type != "float32") {
                reportError(error, "unsupported property type: " + type + " for " + name);
                return false;
            }
            header.property_names.push_back(name);
            continue;
        }
        if (keyword == "end_header") {
            if (!header.binary_little_endian) {
                reportError(error, "missing binary_little_endian format");
                return false;
            }
            return true;
        }
    }

    reportError(error, "missing end_header");
    return false;
}

[[nodiscard]] bool resolvePropertyMap(const std::vector<std::string>& names,
                                      std::unordered_map<std::string, int>& index_of,
                                      int& rest_count,
                                      int& degree,
                                      std::string* error) {
    index_of.clear();
    for (int i = 0; i < static_cast<int>(names.size()); ++i) {
        if (index_of.contains(names[static_cast<std::size_t>(i)])) {
            reportError(error, "duplicate property: " + names[static_cast<std::size_t>(i)]);
            return false;
        }
        index_of.emplace(names[static_cast<std::size_t>(i)], i);
    }

    const auto require = [&](const char* name) -> bool {
        if (!index_of.contains(name)) {
            reportError(error, std::string("missing property: ") + name);
            return false;
        }
        return true;
    };

    if (!require("x") || !require("y") || !require("z") || !require("opacity") || !require("scale_0")
        || !require("scale_1") || !require("scale_2") || !require("rot_0") || !require("rot_1") || !require("rot_2")
        || !require("rot_3") || !require("f_dc_0") || !require("f_dc_1") || !require("f_dc_2")) {
        return false;
    }

    rest_count = 0;
    for (int i = 0;; ++i) {
        const std::string name = "f_rest_" + std::to_string(i);
        if (!index_of.contains(name)) {
            break;
        }
        ++rest_count;
    }
    for (const auto& [name, _] : index_of) {
        if (name.rfind("f_rest_", 0) != 0) {
            continue;
        }
        const std::string suffix = name.substr(7);
        if (suffix.empty()
            || !std::all_of(suffix.begin(), suffix.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) {
            reportError(error, "invalid f_rest property name: " + name);
            return false;
        }
        const int index = std::stoi(suffix);
        if (index < 0 || index >= rest_count) {
            reportError(error, "non-contiguous or out-of-range f_rest property: " + name);
            return false;
        }
    }

    degree = degreeFromRestCount(rest_count);
    if (degree < 0) {
        reportError(error, "unsupported f_rest count " + std::to_string(rest_count) + "; expected 0, 9, 24, or 45");
        return false;
    }
    return true;
}

}  // namespace

namespace vne::gs {

bool readGaussianPly(const std::string& path, GaussianCloud& out, std::string* error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        reportError(error, "failed to open: " + path);
        return false;
    }

    PlyHeader header;
    if (!parseHeader(in, header, error)) {
        return false;
    }

    std::unordered_map<std::string, int> index_of;
    int rest_count = 0;
    int degree = 0;
    if (!resolvePropertyMap(header.property_names, index_of, rest_count, degree, error)) {
        return false;
    }

    const int property_count = static_cast<int>(header.property_names.size());
    const std::size_t floats_per_vertex = static_cast<std::size_t>(property_count);
    const std::size_t bytes_per_vertex = floats_per_vertex * sizeof(float);
    const std::size_t body_bytes = header.vertex_count * bytes_per_vertex;

    std::vector<std::uint8_t> body(body_bytes);
    if (body_bytes > 0) {
        in.read(reinterpret_cast<char*>(body.data()), static_cast<std::streamsize>(body_bytes));
        if (static_cast<std::size_t>(in.gcount()) != body_bytes) {
            reportError(error, "truncated PLY body");
            return false;
        }
    }

    const int coeffs = shCoeffCount(degree);
    const int rest_per_channel = coeffs - 1;

    out.clear();
    out.setShDegree(degree);
    out.positions().resize(header.vertex_count);
    out.scales().resize(header.vertex_count);
    out.rotations().resize(header.vertex_count);
    out.opacities().resize(header.vertex_count);
    out.sh().assign(header.vertex_count * static_cast<std::size_t>(coeffs) * 3u, 0.0f);

    const auto at = [&](std::size_t vertex, const char* name) -> float {
        const int prop = index_of.at(name);
        const std::size_t offset = (vertex * floats_per_vertex + static_cast<std::size_t>(prop)) * sizeof(float);
        return readFloatLE(body.data() + offset);
    };

    for (std::size_t i = 0; i < header.vertex_count; ++i) {
        out.positions()[i] = {at(i, "x"), at(i, "y"), at(i, "z")};
        out.scales()[i] = {std::exp(at(i, "scale_0")), std::exp(at(i, "scale_1")), std::exp(at(i, "scale_2"))};
        out.opacities()[i] = sigmoid(at(i, "opacity"));

        // File order is (w, x, y, z). Quatf is (x, y, z, w).
        const float w = at(i, "rot_0");
        const float x = at(i, "rot_1");
        const float y = at(i, "rot_2");
        const float z = at(i, "rot_3");
        out.rotations()[i] = math::Quatf(x, y, z, w).normalized();

        const std::size_t sh_base = i * static_cast<std::size_t>(coeffs) * 3u;
        out.sh()[sh_base + 0] = at(i, "f_dc_0");
        out.sh()[sh_base + 1] = at(i, "f_dc_1");
        out.sh()[sh_base + 2] = at(i, "f_dc_2");

        if (rest_per_channel > 0) {
            std::vector<float> rest(static_cast<std::size_t>(rest_count));
            for (int r = 0; r < rest_count; ++r) {
                const std::string rest_name = "f_rest_" + std::to_string(r);
                rest[static_cast<std::size_t>(r)] = at(i, rest_name.c_str());
            }
            // Channel-major file -> coefficient-major RGB.
            for (int k = 1; k < coeffs; ++k) {
                const int rest_index = k - 1;
                const std::size_t dst = sh_base + static_cast<std::size_t>(k) * 3u;
                out.sh()[dst + 0] = rest[static_cast<std::size_t>(rest_index)];
                out.sh()[dst + 1] = rest[static_cast<std::size_t>(rest_per_channel + rest_index)];
                out.sh()[dst + 2] = rest[static_cast<std::size_t>(2 * rest_per_channel + rest_index)];
            }
        }
    }

    return true;
}

bool writeGaussianPly(const std::string& path, const GaussianCloud& cloud, std::string* error) {
    if (cloud.shDegree() < 0 || cloud.shDegree() > 3) {
        reportError(error, "unsupported sh_degree " + std::to_string(cloud.shDegree()));
        return false;
    }
    const int coeffs = shCoeffCount(cloud.shDegree());
    const int rest_per_channel = coeffs - 1;
    const int rest_count = 3 * rest_per_channel;
    const std::size_t expected_sh = cloud.size() * static_cast<std::size_t>(coeffs) * 3u;
    if (cloud.scales().size() != cloud.size() || cloud.rotations().size() != cloud.size()
        || cloud.opacities().size() != cloud.size() || cloud.sh().size() != expected_sh) {
        reportError(error, "GaussianCloud attribute sizes do not match");
        return false;
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        reportError(error, "failed to open for write: " + path);
        return false;
    }

    out << "ply\n";
    out << "format binary_little_endian 1.0\n";
    out << "element vertex " << cloud.size() << "\n";
    out << "property float x\n";
    out << "property float y\n";
    out << "property float z\n";
    out << "property float nx\n";
    out << "property float ny\n";
    out << "property float nz\n";
    out << "property float f_dc_0\n";
    out << "property float f_dc_1\n";
    out << "property float f_dc_2\n";
    for (int r = 0; r < rest_count; ++r) {
        out << "property float f_rest_" << r << "\n";
    }
    out << "property float opacity\n";
    out << "property float scale_0\n";
    out << "property float scale_1\n";
    out << "property float scale_2\n";
    out << "property float rot_0\n";
    out << "property float rot_1\n";
    out << "property float rot_2\n";
    out << "property float rot_3\n";
    out << "end_header\n";

    for (std::size_t i = 0; i < cloud.size(); ++i) {
        const math::Vec3f& position = cloud.positions()[i];
        writeFloatLE(out, position.x());
        writeFloatLE(out, position.y());
        writeFloatLE(out, position.z());
        writeFloatLE(out, 0.0f);
        writeFloatLE(out, 0.0f);
        writeFloatLE(out, 0.0f);

        const std::size_t sh_base = i * static_cast<std::size_t>(coeffs) * 3u;
        writeFloatLE(out, cloud.sh()[sh_base + 0]);
        writeFloatLE(out, cloud.sh()[sh_base + 1]);
        writeFloatLE(out, cloud.sh()[sh_base + 2]);

        for (int channel = 0; channel < 3; ++channel) {
            for (int rest_index = 0; rest_index < rest_per_channel; ++rest_index) {
                const int k = rest_index + 1;
                const std::size_t src = sh_base + static_cast<std::size_t>(k) * 3u + static_cast<std::size_t>(channel);
                writeFloatLE(out, cloud.sh()[src]);
            }
        }

        writeFloatLE(out, logit(cloud.opacities()[i]));
        writeFloatLE(out, std::log(std::max(cloud.scales()[i].x(), kLogitEps)));
        writeFloatLE(out, std::log(std::max(cloud.scales()[i].y(), kLogitEps)));
        writeFloatLE(out, std::log(std::max(cloud.scales()[i].z(), kLogitEps)));

        const math::Quatf rotation = cloud.rotations()[i].normalized();
        writeFloatLE(out, rotation.w);
        writeFloatLE(out, rotation.x);
        writeFloatLE(out, rotation.y);
        writeFloatLE(out, rotation.z);
    }

    if (!out) {
        reportError(error, "failed while writing: " + path);
        return false;
    }
    return true;
}

}  // namespace vne::gs
