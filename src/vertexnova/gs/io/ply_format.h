#pragma once
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

/**
 * @file ply_format.h
 * @brief Internal PLY header grammar shared by the reader and the writer.
 * @ingroup vne::gs
 *
 * @details Not a public header: it lives under `src/` and is not installed.
 * It models the whole header, every element and every scalar type, not just
 * the Gaussian properties, because the byte offset of the `vertex` body
 * depends on the elements declared before it.
 */

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace vne::gs::detail {

/** @brief Scalar types a binary PLY property can have. */
enum class PlyType {
    eInt8 = 0,
    eUint8,
    eInt16,
    eUint16,
    eInt32,
    eUint32,
    eFloat32,
    eFloat64,
};

/** @brief Byte width of a scalar PLY type. */
[[nodiscard]] std::size_t plyTypeSize(PlyType type) noexcept;

/**
 * @brief Maps a PLY type keyword to its enumerator.
 * @return false when the keyword is not a known scalar type.
 */
[[nodiscard]] bool plyTypeFromName(const std::string& name, PlyType& out_type) noexcept;

/** @brief One scalar property and where it sits inside an element record. */
struct PlyProperty {
    std::string name;
    PlyType type = PlyType::eFloat32;
    std::size_t offset = 0;  //!< Byte offset within one record of the element.
};

/** @brief One element: a name, a record count and the properties per record. */
struct PlyElement {
    std::string name;
    std::size_t count = 0;
    std::vector<PlyProperty> properties;
    std::size_t stride = 0;  //!< Bytes per record. Meaningless when `has_list`.
    bool has_list = false;   //!< A list property makes records variable-length.

    /**
     * @brief Finds a property by name.
     * @return Pointer into `properties`, or nullptr when absent.
     */
    [[nodiscard]] const PlyProperty* find(const std::string& property_name) const noexcept;

    /** @brief Total bytes this element occupies. Only valid when `!has_list`. */
    [[nodiscard]] std::size_t byteCount() const noexcept { return count * stride; }
};

/** @brief A parsed binary-little-endian PLY header. */
struct PlyHeader {
    std::vector<PlyElement> elements;

    /**
     * @brief Finds an element by name.
     * @return Pointer into `elements`, or nullptr when absent.
     */
    [[nodiscard]] const PlyElement* find(const std::string& name) const noexcept;

    /** @brief Index of an element by name, or `elements.size()` when absent. */
    [[nodiscard]] std::size_t indexOf(const std::string& name) const noexcept;
};

/**
 * @brief Parses a PLY header from an open binary stream.
 *
 * On success the stream is positioned at the first body byte. Only
 * `binary_little_endian` is accepted; ASCII and big-endian files are rejected
 * with a message naming the format.
 *
 * @param in Stream opened in binary mode, positioned at the start of the file.
 * @param out_header Receives the parsed elements.
 * @param out_error Receives a human-readable reason on failure.
 * @return true on success.
 */
[[nodiscard]] bool parsePlyHeader(std::istream& in, PlyHeader& out_header, std::string& out_error);

/**
 * @brief Reads a little-endian float from four bytes, byte-swapping if needed.
 */
[[nodiscard]] float readFloatLE(const std::uint8_t* bytes) noexcept;

/**
 * @brief Writes a float as four little-endian bytes into `out`.
 */
void writeFloatLE(std::uint8_t* out, float value) noexcept;

}  // namespace vne::gs::detail
