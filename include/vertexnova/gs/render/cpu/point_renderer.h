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
 * @file point_renderer.h
 * @brief CPU point-cloud view: one DC-colored dot per Gaussian center.
 * @ingroup vne::gs
 */

#include "vertexnova/gs/camera/camera.h"
#include "vertexnova/gs/core/gaussian_cloud.h"
#include "vertexnova/gs/export.h"
#include "vertexnova/gs/render/image.h"

#include "vertexnova/math/core/vec.h"

#include <cstdint>

namespace vne::gs {

/**
 * @brief Project each Gaussian center, keep the nearest per pixel (z-buffer),
 *        and color it with `dcColor`. Background fills uncovered pixels.
 *
 * @details This is the reference "point cloud" view that every later renderer
 * is compared against: no footprint, no blending, one stamped disc per
 * Gaussian. The projected center lands at `floor(u), floor(v)`, matching the
 * half-pixel-center convention in `camera/conventions.h`.
 *
 * @param point_radius Half-width of the axis-aligned square stamp in pixels
 *                     (0 = a single pixel). Use a few pixels so sparse fixtures
 *                     like `three_gaussians.ply` are visible on large images.
 *
 * @return An image of the camera's resolution, or an empty image if the
 *         camera's intrinsics are not usable (`Intrinsics::isValid()`).
 */
[[nodiscard]] VNE_GS_API ImageRGBf renderPoints(const GaussianCloud& cloud,
                                                const Camera& camera,
                                                const math::Vec3f& background,
                                                std::uint32_t point_radius = 0);

}  // namespace vne::gs
