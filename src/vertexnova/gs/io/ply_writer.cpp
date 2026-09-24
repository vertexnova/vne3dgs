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

#include "vertexnova/gs/io/ply_writer.h"

#include "vertexnova/gs/io/ply_format.h"
#include "vertexnova/logging/logging.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <span>
#include <vector>

namespace {

CREATE_VNE_LOGGER_CATEGORY("vne.gs.ply")

/// Keeps `logit` finite for opacities that have saturated to exactly 0 or 1.
constexpr float kLogitEps = 1e-6f;
/// Smallest scale that survives a `log` round-trip as a finite number.
constexpr float kMinScale = 1e-20f;

/** @brief Inverse of the sigmoid the reader applies to opacity. */
[[nodiscard]] float logit(float opacity) noexcept {
    const float clamped = std::clamp(opacity, kLogitEps, 1.0f - kLogitEps);
    return std::log(clamped / (1.0f - clamped));
}

/** @brief Inverse of the `exp` the reader applies to scale. */
[[nodiscard]] float logScale(float scale) noexcept {
    return std::log(std::max(scale, kMinScale));
}

/** @brief Appends one little-endian float to a byte cursor. */
void push(std::uint8_t*& cursor, float value) noexcept {
    vne::gs::detail::writeFloatLE(cursor, value);
    cursor += sizeof(float);
}

}  // namespace

namespace vne::gs {

PlyWriter::PlyWriter(const PlyWriteOptions& options)
    : options_(options) {}

const PlyWriteOptions& PlyWriter::options() const noexcept {
    return options_;
}

void PlyWriter::setOptions(const PlyWriteOptions& options) noexcept {
    options_ = options;
}

const std::string& PlyWriter::error() const noexcept {
    return error_;
}

bool PlyWriter::write(const std::string& path, const GaussianCloud& cloud) {
    error_.clear();

    const auto fail = [this](std::string message) -> bool {
        error_ = std::move(message);
        VNE_LOG_ERROR << error_;
        return false;
    };

    if (cloud.shDegree() < 0 || cloud.shDegree() > GaussianCloud::kMaxShDegree) {
        return fail("unsupported sh_degree " + std::to_string(cloud.shDegree()));
    }
    if (!cloud.isConsistent()) {
        return fail("GaussianCloud attribute sizes do not match");
    }

    const int coeffs = cloud.shCoeffCount();
    const int rest_per_channel = coeffs - 1;
    const int rest_count = 3 * rest_per_channel;
    const std::size_t normals = options_.write_normals ? 3u : 0u;
    // x y z [nx ny nz] f_dc*3 f_rest* opacity scale*3 rot*4
    const std::size_t floats_per_vertex = 3u + normals + 3u + static_cast<std::size_t>(rest_count) + 1u + 3u + 4u;
    const std::size_t stride = floats_per_vertex * sizeof(float);

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return fail("failed to open for write: " + path);
    }

    out << "ply\n";
    out << "format binary_little_endian 1.0\n";
    out << "element vertex " << cloud.size() << "\n";
    out << "property float x\n";
    out << "property float y\n";
    out << "property float z\n";
    if (options_.write_normals) {
        out << "property float nx\n";
        out << "property float ny\n";
        out << "property float nz\n";
    }
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
    if (!out) {
        return fail("failed while writing the PLY header: " + path);
    }

    const std::span<const math::Vec3f> positions = cloud.positions();
    const std::span<const math::Vec3f> scales = cloud.scales();
    const std::span<const math::Quatf> rotations = cloud.rotations();
    const std::span<const float> opacities = cloud.opacities();
    const std::span<const float> sh = cloud.sh();

    // Batch the body: one ostream::write per chunk instead of one per float.
    const std::size_t vertices_per_chunk = std::max<std::size_t>(1u, options_.chunk_bytes / stride);
    std::vector<std::uint8_t> buffer(vertices_per_chunk * stride);

    std::size_t written = 0;
    while (written < cloud.size()) {
        const std::size_t batch = std::min(vertices_per_chunk, cloud.size() - written);
        std::uint8_t* cursor = buffer.data();

        for (std::size_t n = 0; n < batch; ++n) {
            const std::size_t i = written + n;

            push(cursor, positions[i].x());
            push(cursor, positions[i].y());
            push(cursor, positions[i].z());
            if (options_.write_normals) {
                push(cursor, 0.0f);
                push(cursor, 0.0f);
                push(cursor, 0.0f);
            }

            const std::size_t sh_base = i * static_cast<std::size_t>(coeffs) * GaussianCloud::kChannels;
            push(cursor, sh[sh_base + 0]);
            push(cursor, sh[sh_base + 1]);
            push(cursor, sh[sh_base + 2]);

            // Back to channel-major: all red coefficients, then green, then blue.
            for (std::size_t channel = 0; channel < GaussianCloud::kChannels; ++channel) {
                for (int rest_index = 0; rest_index < rest_per_channel; ++rest_index) {
                    const std::size_t k = static_cast<std::size_t>(rest_index) + 1u;
                    push(cursor, sh[sh_base + k * GaussianCloud::kChannels + channel]);
                }
            }

            push(cursor, logit(opacities[i]));
            push(cursor, logScale(scales[i].x()));
            push(cursor, logScale(scales[i].y()));
            push(cursor, logScale(scales[i].z()));

            // Quatf is (x, y, z, w); the file wants w first.
            const math::Quatf rotation = rotations[i].normalized();
            push(cursor, rotation.w);
            push(cursor, rotation.x);
            push(cursor, rotation.y);
            push(cursor, rotation.z);
        }

        const std::size_t bytes = batch * stride;
        out.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(bytes));
        if (!out) {
            return fail("failed while writing: " + path);
        }
        written += batch;
    }

    out.flush();
    if (!out) {
        return fail("failed to flush: " + path);
    }
    return true;
}

bool writeGaussianPly(const std::string& path, const GaussianCloud& cloud, std::string* error) {
    PlyWriter writer;
    if (writer.write(path, cloud)) {
        return true;
    }
    if (error != nullptr) {
        *error = writer.error();
    }
    return false;
}

}  // namespace vne::gs
