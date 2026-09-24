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
 * @file ply_reader.h
 * @brief Load a trained 3DGS scene from a binary little-endian PLY.
 * @ingroup vne::gs
 *
 * @details Properties are found by name, never by byte offset, because other
 * tools reorder them and add their own. Values are activated on the way in:
 * `exp` for scale, a sigmoid for opacity, normalization for the quaternion,
 * and the channel-major `f_rest` block is transposed into coefficient-major
 * RGB triplets.
 *
 * The body is consumed in bounded chunks rather than being buffered whole, so
 * peak memory stays close to the size of the resulting cloud instead of twice
 * it. A 5.8M-Gaussian scene is about 1.4 GB of file and 1.4 GB of cloud.
 */

#include "vertexnova/gs/core/gaussian_cloud.h"
#include "vertexnova/gs/export.h"

#include <cstddef>
#include <string>

namespace vne::gs {

/** @brief Policy knobs for `PlyReader`. */
struct PlyReadOptions {
    /**
     * @brief Drop Gaussians with a non-finite value instead of failing the load.
     *
     * Trained scenes from third-party tools do contain the occasional NaN
     * splat. Dropping them keeps a usable scene; `PlyReadStats::skipped_non_finite`
     * reports how many went. Set false to treat the first one as an error.
     */
    bool skip_non_finite = true;

    /**
     * @brief Target size of one body read, in bytes. Rounded down to a whole
     *        number of vertex records, and never below one record.
     */
    std::size_t chunk_bytes = 4u * 1024u * 1024u;
};

/** @brief What a load produced, whether or not it succeeded. */
struct PlyReadStats {
    /// Vertices the header declared.
    std::size_t declared_vertex_count = 0;
    /// Gaussians actually stored (declared minus skipped).
    std::size_t loaded_vertex_count = 0;
    /// Gaussians dropped for holding a non-finite value.
    std::size_t skipped_non_finite = 0;
    /// SH degree inferred from the `f_rest` property count.
    int sh_degree = 0;
};

/**
 * @brief Reads 3DGS PLY files into an activated `GaussianCloud`.
 *
 * @details Construct once, call `read` as often as needed; `stats()` and
 * `error()` describe the most recent call. The reader is not thread-safe, but
 * separate instances are independent.
 *
 * @note On failure the destination cloud is left untouched. The cloud is
 *       assembled separately and only moved into place once the whole file has
 *       been consumed, so a half-read file cannot leave a caller holding a
 *       cloud that looks populated.
 */
class VNE_GS_API PlyReader {
   public:
    PlyReader() = default;
    explicit PlyReader(const PlyReadOptions& options);

    [[nodiscard]] const PlyReadOptions& options() const noexcept;
    void setOptions(const PlyReadOptions& options) noexcept;

    /**
     * @brief Loads a scene.
     *
     * @param path File to read.
     * @param out_cloud Receives the activated cloud on success; untouched on failure.
     * @return true on success. On failure the reason is in `error()` and is
     *         also logged with `VNE_LOG_ERROR`.
     */
    [[nodiscard]] bool read(const std::string& path, GaussianCloud& out_cloud);

    /** @brief Counts from the most recent `read`. */
    [[nodiscard]] const PlyReadStats& stats() const noexcept;

    /** @brief Reason the most recent `read` failed; empty after a success. */
    [[nodiscard]] const std::string& error() const noexcept;

   private:
    PlyReadOptions options_{};
    PlyReadStats stats_{};
    std::string error_;
};

/**
 * @brief Loads a 3DGS PLY with default options.
 *
 * Convenience wrapper over `PlyReader` for callers that need neither the
 * statistics nor the policy knobs.
 *
 * @param path File to read.
 * @param out Receives the activated cloud on success; untouched on failure.
 * @param error Optional; receives the failure reason.
 * @return true on success.
 */
[[nodiscard]] VNE_GS_API bool readGaussianPly(const std::string& path,
                                              GaussianCloud& out,
                                              std::string* error = nullptr);

}  // namespace vne::gs
