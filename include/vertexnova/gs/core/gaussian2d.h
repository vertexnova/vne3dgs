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
 * @file gaussian2d.h
 * @brief 2D Gaussian splat: footprint, eigenvalues and alpha.
 * @ingroup vne::gs
 *
 * @details After projection, every 3D Gaussian is a 2D Gaussian on screen.
 * This type holds the splat and the helpers that turn covariance into a
 * 3-sigma radius and opacity into a per-pixel alpha. Conic evaluation lives
 * in `conic.h`; compositing lives in `front_to_back_blender.h`.
 *
 * Pixel convention (until Task 04 freezes it library-wide): pixel `(i, j)` is
 * sampled at its center `(i + 0.5, j + 0.5)`.
 */

#include "vertexnova/gs/export.h"

#include "vertexnova/math/core/mat.h"
#include "vertexnova/math/core/vec.h"

namespace vne::gs {

/**
 * @brief A 2D splat: screen-space mean, covariance, linear RGB and opacity.
 *
 * @note Covariance must be symmetric positive definite. `computeConic` returns
 *       nullopt otherwise and the splat is skipped.
 */
struct Gaussian2D {
    math::Vec2f mean{};
    math::Mat2f cov{};
    math::Vec3f color{};
    float opacity = 0.0f;
};

/**
 * @brief Eigenvalues of a 2x2 covariance, largest first.
 * @return `(lambda1, lambda2)` with `lambda1 >= lambda2`. Algebraic; not filtered
 *         for `det <= 0`.
 */
[[nodiscard]] VNE_GS_API math::Vec2f computeEigenvalues(const math::Mat2f& cov) noexcept;

/**
 * @brief Screen-space radius of the 3-sigma footprint: `ceil(3 * sqrt(lambda1))`.
 * @return 0 if `Sigma` is not positive definite.
 */
[[nodiscard]] VNE_GS_API int computeRadius(const math::Mat2f& cov) noexcept;

/**
 * @brief Opacity x falloff, with the reference rasterizer's skip rules.
 * @return `min(0.99, opacity * exp(power))`, or 0 if `power > 0` or the
 *         result is below `1/255`.
 */
[[nodiscard]] VNE_GS_API float computeAlpha(float opacity, float power) noexcept;

}  // namespace vne::gs
