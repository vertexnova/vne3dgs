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
 * @file ply_writer.h
 * @brief Save a `GaussianCloud` as a binary little-endian 3DGS PLY.
 * @ingroup vne::gs
 *
 * @details The writer undoes every activation the reader applies: `log` for
 * scale, `logit` for opacity, w-first quaternions, and a channel-major
 * `f_rest` block. Property order matches what the reference trainer emits, so
 * the files load in other 3DGS tools and round-trip exactly here.
 */

#include "vertexnova/gs/core/gaussian_cloud.h"
#include "vertexnova/gs/export.h"

#include <cstddef>
#include <string>

namespace vne::gs {

/** @brief Policy knobs for `PlyWriter`. */
struct PlyWriteOptions {
    /**
     * @brief Target size of one body write, in bytes. Rounded down to a whole
     *        number of vertex records, and never below one record.
     */
    std::size_t chunk_bytes = 4u * 1024u * 1024u;

    /**
     * @brief Emit the unused `nx, ny, nz` properties that the reference format
     *        carries. Trained files always contain them, and some third-party
     *        loaders expect them.
     */
    bool write_normals = true;
};

/**
 * @brief Writes 3DGS PLY files from an activated `GaussianCloud`.
 *
 * @details Construct once, call `write` as often as needed; `error()`
 * describes the most recent call. Not thread-safe; separate instances are
 * independent.
 */
class VNE_GS_API PlyWriter {
   public:
    PlyWriter() = default;
    explicit PlyWriter(const PlyWriteOptions& options);

    [[nodiscard]] const PlyWriteOptions& options() const noexcept;
    void setOptions(const PlyWriteOptions& options) noexcept;

    /**
     * @brief Saves a cloud.
     *
     * @param path File to create or overwrite.
     * @param cloud Activated cloud. Rejected if its SH degree is out of range
     *              or its columns disagree (`GaussianCloud::isConsistent()`).
     * @return true on success. On failure the reason is in `error()` and is
     *         also logged with `VNE_LOG_ERROR`.
     */
    [[nodiscard]] bool write(const std::string& path, const GaussianCloud& cloud);

    /** @brief Reason the most recent `write` failed; empty after a success. */
    [[nodiscard]] const std::string& error() const noexcept;

   private:
    PlyWriteOptions options_{};
    std::string error_;
};

/**
 * @brief Saves a cloud as 3DGS PLY with default options.
 *
 * @param path File to create or overwrite.
 * @param cloud Activated cloud.
 * @param error Optional; receives the failure reason.
 * @return true on success.
 */
[[nodiscard]] VNE_GS_API bool writeGaussianPly(const std::string& path,
                                               const GaussianCloud& cloud,
                                               std::string* error = nullptr);

}  // namespace vne::gs
