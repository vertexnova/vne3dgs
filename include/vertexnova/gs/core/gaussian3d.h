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
 * @file gaussian3d.h
 * @brief One world-space 3D Gaussian and its covariance.
 * @ingroup vne::gs
 *
 * @details A trained splat stores a position, an activated scale (linear,
 * positive), a unit quaternion, an opacity in (0, 1) and a color placeholder.
 * `computeCovariance3D` turns scale and rotation into `Sigma =
 * R * S * S^T * R^T`. Color is replaced by spherical harmonics in Task 08.
 *
 * Matrices are column-major (`m[col][row]`). The columns of `R` are the
 * ellipsoid axes in world space; `sx, sy, sz` are the 1-sigma lengths along
 * them.
 */

#include "vertexnova/gs/export.h"

#include "vertexnova/math/core/mat.h"
#include "vertexnova/math/core/quat.h"
#include "vertexnova/math/core/vec.h"

#include <array>

namespace vne::gs {

/**
 * @brief One 3D Gaussian in world space.
 *
 * @note `scale` and `rotation` are the activated parameters: linear scale
 *       greater than zero, and a unit quaternion. The stored training values
 *       (`log s`, raw `q`) are applied before they land here (Task 03).
 */
struct Gaussian3D {
    math::Vec3f position{};
    math::Vec3f scale{};
    math::Quatf rotation{};
    float opacity = 1.0f;
    math::Vec3f color{};
};

/**
 * @brief Rotation matrix of a quaternion.
 *
 * @details Normalizes, then calls `Quatf::toMatrix3()`. That conversion assumes
 *          a unit quaternion. A zero quaternion returns the identity. `q` and
 *          `-q` produce the same matrix. `math::Quatf` stores `(x, y, z, w)`
 *          with `w` last; PLY files store `w` first (Task 03).
 */
[[nodiscard]] VNE_GS_API math::Mat3f quatToRotationMatrix(const math::Quatf& rotation) noexcept;

/**
 * @brief World-space covariance `Sigma = R * S * S^T * R^T`.
 *
 * @param scale Linear scales along the local axes. Not required to be positive
 *              for the product to be symmetric; a negative scale flips an axis
 *              and squares away.
 * @param rotation Rotation quaternion. Normalized inside `quatToRotationMatrix`.
 */
[[nodiscard]] VNE_GS_API math::Mat3f computeCovariance3D(const math::Vec3f& scale,
                                                         const math::Quatf& rotation) noexcept;

/**
 * @brief Symmetric covariance as six floats: `(00, 01, 02, 11, 12, 22)`.
 *
 * @details A symmetric covariance repeats its off-diagonals, so the reference
 *          rasterizer (`strip_symmetric`) stores six floats instead of nine.
 *          Off-diagonals are the upper triangle (`m[col][row]` with `col >= row`).
 */
[[nodiscard]] VNE_GS_API std::array<float, 6> packSymmetricCovariance(const math::Mat3f& matrix) noexcept;

}  // namespace vne::gs
