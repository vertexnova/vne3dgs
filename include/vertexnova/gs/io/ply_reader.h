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
 * @brief Load and save trained 3DGS scenes as binary little-endian PLY.
 * @ingroup vne::gs
 *
 * @details The reader finds properties by name, activates scale / opacity /
 * rotation, and transposes channel-major `f_rest` into coefficient-major RGB.
 * The writer undoes those activations so round-trips and fixtures are exact.
 */

#include "vertexnova/gs/core/gaussian_cloud.h"
#include "vertexnova/gs/export.h"

#include <string>

namespace vne::gs {

/**
 * @brief Load a 3DGS PLY into an activated `GaussianCloud`.
 * @return true on success. On failure, logs with `VNE_LOG_ERROR` and, if
 *         `error` is non-null, writes the same message there.
 */
[[nodiscard]] VNE_GS_API bool readGaussianPly(const std::string& path,
                                              GaussianCloud& out,
                                              std::string* error = nullptr);

/**
 * @brief Write an activated cloud as binary little-endian 3DGS PLY.
 * @return true on success. On failure, logs with `VNE_LOG_ERROR` and, if
 *         `error` is non-null, writes the same message there.
 */
[[nodiscard]] VNE_GS_API bool writeGaussianPly(const std::string& path,
                                               const GaussianCloud& cloud,
                                               std::string* error = nullptr);

}  // namespace vne::gs
