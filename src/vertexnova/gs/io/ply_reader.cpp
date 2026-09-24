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

#include "vertexnova/gs/io/ply_format.h"
#include "vertexnova/logging/logging.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

CREATE_VNE_LOGGER_CATEGORY("vne.gs.ply")

using vne::gs::detail::PlyElement;
using vne::gs::detail::PlyHeader;
using vne::gs::detail::PlyProperty;
using vne::gs::detail::PlyType;

/** @brief Number of `f_rest` properties each supported SH degree implies. */
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

[[nodiscard]] float sigmoid(float value) noexcept {
    return 1.0f / (1.0f + std::exp(-value));
}

/**
 * @brief Byte offsets of the Gaussian properties inside one vertex record.
 *
 * Resolved once per file; the per-vertex loop then does pointer arithmetic
 * only. `rest` is in file order (`f_rest_0`, `f_rest_1`, ...), which is
 * channel-major.
 */
struct VertexLayout {
    std::size_t x = 0;
    std::size_t y = 0;
    std::size_t z = 0;
    std::size_t scale[3] = {0, 0, 0};
    std::size_t rot[4] = {0, 0, 0, 0};
    std::size_t opacity = 0;
    std::size_t dc[3] = {0, 0, 0};
    std::vector<std::size_t> rest;
    int sh_degree = 0;
};

/**
 * @brief Looks up a required float property and records its offset.
 *
 * Gaussian parameters must be `float`; a file that stores them at another
 * width is not a 3DGS scene, and silently converting would hide that.
 */
[[nodiscard]] bool requireFloatProperty(const PlyElement& element,
                                        const std::string& name,
                                        std::size_t& out_offset,
                                        std::string& out_error) {
    const PlyProperty* property = element.find(name);
    if (property == nullptr) {
        out_error = "missing property: " + name;
        return false;
    }
    if (property->type != PlyType::eFloat32) {
        out_error = "property " + name + " must be float32";
        return false;
    }
    out_offset = property->offset;
    return true;
}

/** @brief Counts contiguous `f_rest_N` properties starting at 0. */
[[nodiscard]] int countRestProperties(const PlyElement& element) noexcept {
    int count = 0;
    while (element.find("f_rest_" + std::to_string(count)) != nullptr) {
        ++count;
    }
    return count;
}

/**
 * @brief Rejects `f_rest` properties that are not part of the contiguous run.
 *
 * A file with `f_rest_0..8` plus a stray `f_rest_20` would otherwise load as
 * degree 1 and quietly drop a coefficient.
 */
[[nodiscard]] bool checkRestContiguity(const PlyElement& element, int rest_count, std::string& out_error) {
    for (const PlyProperty& property : element.properties) {
        if (property.name.rfind("f_rest_", 0) != 0) {
            continue;
        }
        const std::string suffix = property.name.substr(7);
        int index = 0;
        const char* begin = suffix.data();
        const char* end = begin + suffix.size();
        const auto [ptr, ec] = std::from_chars(begin, end, index);
        if (suffix.empty() || ec != std::errc{} || ptr != end) {
            out_error = "invalid f_rest property name: " + property.name;
            return false;
        }
        if (index >= rest_count) {
            out_error = "non-contiguous f_rest property: " + property.name;
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool resolveVertexLayout(const PlyElement& element, VertexLayout& out_layout, std::string& out_error) {
    if (element.has_list) {
        out_error = "vertex element has a list property, which a Gaussian PLY never does";
        return false;
    }

    if (!requireFloatProperty(element, "x", out_layout.x, out_error)
        || !requireFloatProperty(element, "y", out_layout.y, out_error)
        || !requireFloatProperty(element, "z", out_layout.z, out_error)
        || !requireFloatProperty(element, "opacity", out_layout.opacity, out_error)) {
        return false;
    }
    for (int i = 0; i < 3; ++i) {
        if (!requireFloatProperty(element, "scale_" + std::to_string(i), out_layout.scale[i], out_error)
            || !requireFloatProperty(element, "f_dc_" + std::to_string(i), out_layout.dc[i], out_error)) {
            return false;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (!requireFloatProperty(element, "rot_" + std::to_string(i), out_layout.rot[i], out_error)) {
            return false;
        }
    }

    const int rest_count = countRestProperties(element);
    if (!checkRestContiguity(element, rest_count, out_error)) {
        return false;
    }
    out_layout.sh_degree = degreeFromRestCount(rest_count);
    if (out_layout.sh_degree < 0) {
        out_error = "unsupported f_rest count " + std::to_string(rest_count) + "; expected 0, 9, 24, or 45";
        return false;
    }

    out_layout.rest.resize(static_cast<std::size_t>(rest_count));
    for (int i = 0; i < rest_count; ++i) {
        if (!requireFloatProperty(element,
                                  "f_rest_" + std::to_string(i),
                                  out_layout.rest[static_cast<std::size_t>(i)],
                                  out_error)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Total bytes occupied by the elements declared before `vertex`.
 *
 * Those bytes sit between `end_header` and the vertex data. Skipping them is
 * what keeps a file that declares, say, a camera element first from being read
 * as if its data were the first Gaussian.
 */
[[nodiscard]] bool bytesBeforeVertex(const PlyHeader& header,
                                     std::size_t vertex_index,
                                     std::size_t& out_bytes,
                                     std::string& out_error) {
    out_bytes = 0;
    for (std::size_t i = 0; i < vertex_index; ++i) {
        const PlyElement& element = header.elements[i];
        if (element.has_list) {
            out_error =
                "cannot skip element '" + element.name + "' before vertex: it has a list property of unknown length";
            return false;
        }
        const std::size_t bytes = element.byteCount();
        if (element.stride != 0 && element.count > std::numeric_limits<std::size_t>::max() / element.stride) {
            out_error = "element '" + element.name + "' size overflows size_t";
            return false;
        }
        if (bytes > std::numeric_limits<std::size_t>::max() - out_bytes) {
            out_error = "PLY body offset overflows size_t";
            return false;
        }
        out_bytes += bytes;
    }
    return true;
}

}  // namespace

namespace vne::gs {

PlyReader::PlyReader(const PlyReadOptions& options)
    : options_(options) {}

const PlyReadOptions& PlyReader::options() const noexcept {
    return options_;
}

void PlyReader::setOptions(const PlyReadOptions& options) noexcept {
    options_ = options;
}

const PlyReadStats& PlyReader::stats() const noexcept {
    return stats_;
}

const std::string& PlyReader::error() const noexcept {
    return error_;
}

bool PlyReader::read(const std::string& path, GaussianCloud& out_cloud) {
    stats_ = PlyReadStats{};
    error_.clear();

    const auto fail = [this](std::string message) -> bool {
        error_ = std::move(message);
        VNE_LOG_ERROR << error_;
        return false;
    };

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return fail("failed to open: " + path);
    }

    detail::PlyHeader header;
    std::string error;
    if (!detail::parsePlyHeader(in, header, error)) {
        return fail(std::move(error));
    }

    const std::size_t vertex_index = header.indexOf("vertex");
    if (vertex_index == header.elements.size()) {
        return fail("PLY has no vertex element");
    }
    const detail::PlyElement& vertex_element = header.elements[vertex_index];

    VertexLayout layout;
    if (!resolveVertexLayout(vertex_element, layout, error)) {
        return fail(std::move(error));
    }

    std::size_t skip_bytes = 0;
    if (!bytesBeforeVertex(header, vertex_index, skip_bytes, error)) {
        return fail(std::move(error));
    }

    const std::size_t stride = vertex_element.stride;
    const std::size_t declared = vertex_element.count;
    stats_.declared_vertex_count = declared;
    stats_.sh_degree = layout.sh_degree;

    if (stride == 0 && declared > 0) {
        return fail("vertex element declares no properties");
    }
    if (stride != 0 && declared > std::numeric_limits<std::size_t>::max() / stride) {
        return fail("PLY body size overflows size_t");
    }

    // Check the file is actually big enough before trusting the header's count
    // to size an allocation: a corrupt or hostile header must not be able to
    // ask for gigabytes that the file cannot back.
    const std::streampos body_start = in.tellg();
    if (body_start < 0) {
        return fail("failed to locate PLY body");
    }
    in.seekg(0, std::ios::end);
    const std::streampos file_end = in.tellg();
    if (!in || file_end < body_start) {
        return fail("failed to measure PLY file");
    }
    const std::size_t available = static_cast<std::size_t>(file_end - body_start);
    // Reject oversized preceding elements before budgeting vertices or seeking:
    // with declared == 0, needed is 0 and would otherwise pass a short file.
    if (skip_bytes > available) {
        return fail("truncated PLY body: preceding elements need " + std::to_string(skip_bytes) + " bytes, file has "
                    + std::to_string(available));
    }
    const std::size_t vertex_budget = available - skip_bytes;
    const std::size_t needed = declared * stride;
    if (needed > vertex_budget) {
        return fail("truncated PLY body: header declares " + std::to_string(needed) + " bytes of vertex data, file has "
                    + std::to_string(vertex_budget));
    }
    in.seekg(body_start + static_cast<std::streamoff>(skip_bytes));
    if (!in) {
        return fail("failed to seek to the vertex body");
    }

    // Assembled separately so a mid-file failure cannot leave the caller's
    // cloud looking populated.
    GaussianCloud cloud;
    cloud.setShDegree(layout.sh_degree);
    cloud.resize(declared);

    const int coeffs = cloud.shCoeffCount();
    const int rest_per_channel = coeffs - 1;
    const std::span<math::Vec3f> positions = cloud.positions();
    const std::span<math::Vec3f> scales = cloud.scales();
    const std::span<math::Quatf> rotations = cloud.rotations();
    const std::span<float> opacities = cloud.opacities();
    const std::span<float> sh = cloud.sh();

    const std::size_t vertices_per_chunk =
        (stride == 0) ? 1u : std::max<std::size_t>(1u, options_.chunk_bytes / stride);
    std::vector<std::uint8_t> buffer(vertices_per_chunk * stride);

    std::size_t written = 0;
    std::size_t remaining = declared;
    std::size_t vertex_index_in_file = 0;

    while (remaining > 0) {
        const std::size_t batch = std::min(vertices_per_chunk, remaining);
        const std::size_t bytes = batch * stride;
        in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(bytes));
        if (static_cast<std::size_t>(in.gcount()) != bytes) {
            return fail("truncated PLY body");
        }

        for (std::size_t v = 0; v < batch; ++v, ++vertex_index_in_file) {
            const std::uint8_t* record = buffer.data() + v * stride;

            const auto value = [record](std::size_t offset) noexcept { return detail::readFloatLE(record + offset); };

            const float x = value(layout.x);
            const float y = value(layout.y);
            const float z = value(layout.z);
            const float scale0 = value(layout.scale[0]);
            const float scale1 = value(layout.scale[1]);
            const float scale2 = value(layout.scale[2]);
            const float opacity = value(layout.opacity);
            const float rot0 = value(layout.rot[0]);
            const float rot1 = value(layout.rot[1]);
            const float rot2 = value(layout.rot[2]);
            const float rot3 = value(layout.rot[3]);

            bool is_finite = std::isfinite(x) && std::isfinite(y) && std::isfinite(z) && std::isfinite(scale0)
                             && std::isfinite(scale1) && std::isfinite(scale2) && std::isfinite(opacity)
                             && std::isfinite(rot0) && std::isfinite(rot1) && std::isfinite(rot2)
                             && std::isfinite(rot3);

            const std::size_t sh_base = written * static_cast<std::size_t>(coeffs) * GaussianCloud::kChannels;

            // Gather SH before committing, so a non-finite coefficient can
            // still drop the whole Gaussian.
            std::array<float, 3> dc{value(layout.dc[0]), value(layout.dc[1]), value(layout.dc[2])};
            is_finite = is_finite && std::isfinite(dc[0]) && std::isfinite(dc[1]) && std::isfinite(dc[2]);

            if (is_finite && rest_per_channel > 0) {
                for (int k = 1; k < coeffs && is_finite; ++k) {
                    const std::size_t rest_index = static_cast<std::size_t>(k - 1);
                    const std::size_t per_channel = static_cast<std::size_t>(rest_per_channel);
                    // f_rest is channel-major in the file: all red coefficients,
                    // then all green, then all blue. Renderers want RGB triplets
                    // per coefficient, so transpose here.
                    const float r = value(layout.rest[rest_index]);
                    const float g = value(layout.rest[per_channel + rest_index]);
                    const float b = value(layout.rest[2u * per_channel + rest_index]);
                    if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b)) {
                        is_finite = false;
                        break;
                    }
                    const std::size_t dst = sh_base + static_cast<std::size_t>(k) * GaussianCloud::kChannels;
                    sh[dst + 0] = r;
                    sh[dst + 1] = g;
                    sh[dst + 2] = b;
                }
            }

            if (!is_finite) {
                if (!options_.skip_non_finite) {
                    return fail("non-finite value at vertex " + std::to_string(vertex_index_in_file));
                }
                ++stats_.skipped_non_finite;
                continue;
            }

            positions[written] = {x, y, z};
            scales[written] = {std::exp(scale0), std::exp(scale1), std::exp(scale2)};
            opacities[written] = sigmoid(opacity);
            // File order is (w, x, y, z); Quatf is (x, y, z, w).
            rotations[written] = math::Quatf(rot1, rot2, rot3, rot0).normalized();
            sh[sh_base + 0] = dc[0];
            sh[sh_base + 1] = dc[1];
            sh[sh_base + 2] = dc[2];
            ++written;
        }

        remaining -= batch;
    }

    if (written != declared) {
        // Drops the tail left by skipped Gaussians; earlier entries keep their
        // offsets, so the SH block stays aligned.
        cloud.resize(written);
        VNE_LOG_WARN << "dropped " << stats_.skipped_non_finite << " non-finite Gaussians from " << path;
    }

    stats_.loaded_vertex_count = written;
    out_cloud = std::move(cloud);
    return true;
}

bool readGaussianPly(const std::string& path, GaussianCloud& out, std::string* error) {
    PlyReader reader;
    if (reader.read(path, out)) {
        return true;
    }
    if (error != nullptr) {
        *error = reader.error();
    }
    return false;
}

}  // namespace vne::gs
