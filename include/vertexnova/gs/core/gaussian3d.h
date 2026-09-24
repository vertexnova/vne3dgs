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
 * `covariance()` builds `Sigma = R * S * S^T * R^T`, which is positive
 * semi-definite for every `(scale, rotation)`; that is why 3DGS optimizes
 * those instead of the six covariance entries. Color is replaced by spherical
 * harmonics in Task 08.
 *
 * Matrices are column-major (`m[col][row]`). The columns of `R` are the
 * ellipsoid axes in world space; `sx, sy, sz` are the 1-sigma lengths along
 * them.
 *
 * @note This is the single-splat type, for math and tests. A scene lives in
 *       `GaussianCloud`, which stores the same fields as parallel arrays.
 */

#include "vertexnova/gs/export.h"

#include "vertexnova/math/core/mat.h"
#include "vertexnova/math/core/quat.h"
#include "vertexnova/math/core/vec.h"

#include <array>

namespace vne::gs {

/**
 * @brief One 3D Gaussian in world space, holding activated parameters.
 *
 * `scale` is linear and positive and `rotation` is unit length: the stored
 * training values (`log s`, raw `q`, `logit o`) are activated before they land
 * here, which `PlyReader` does at load time.
 */
class VNE_GS_API Gaussian3D {
   public:
    Gaussian3D() noexcept = default;

    Gaussian3D(const math::Vec3f& position,
               const math::Vec3f& scale,
               const math::Quatf& rotation,
               float opacity,
               const math::Vec3f& color) noexcept
        : position_(position)
        , scale_(scale)
        , rotation_(rotation)
        , opacity_(opacity)
        , color_(color) {}

    [[nodiscard]] const math::Vec3f& position() const noexcept { return position_; }
    void setPosition(const math::Vec3f& position) noexcept { position_ = position; }

    /** @brief Linear 1-sigma extents along the local axes. */
    [[nodiscard]] const math::Vec3f& scale() const noexcept { return scale_; }
    void setScale(const math::Vec3f& scale) noexcept { scale_ = scale; }

    [[nodiscard]] const math::Quatf& rotation() const noexcept { return rotation_; }
    void setRotation(const math::Quatf& rotation) noexcept { rotation_ = rotation; }

    /** @brief Peak alpha in (0, 1). */
    [[nodiscard]] float opacity() const noexcept { return opacity_; }
    void setOpacity(float opacity) noexcept { opacity_ = opacity; }

    /** @brief Linear RGB placeholder until spherical harmonics (Task 08). */
    [[nodiscard]] const math::Vec3f& color() const noexcept { return color_; }
    void setColor(const math::Vec3f& color) noexcept { color_ = color; }

    /** @brief Rotation matrix of this splat's quaternion. */
    [[nodiscard]] math::Mat3f rotationMatrix() const noexcept { return rotationMatrixOf(rotation_); }

    /** @brief World-space covariance `Sigma = R * S * S^T * R^T`. */
    [[nodiscard]] math::Mat3f covariance() const noexcept { return covarianceOf(scale_, rotation_); }

    /** @brief This splat's covariance as six floats; see `packSymmetric`. */
    [[nodiscard]] std::array<float, 6> packedCovariance() const noexcept { return packSymmetric(covariance()); }

    /**
     * @brief Rotation matrix of a quaternion.
     *
     * @details Normalizes first, because stored quaternions are not unit
     *          length. A zero quaternion yields the identity. `q` and `-q`
     *          give the same matrix, since every entry is quadratic in `q`.
     *          `math::Quatf` stores `(x, y, z, w)` with `w` last; PLY files
     *          store `w` first, and `PlyReader` remaps it.
     */
    [[nodiscard]] static math::Mat3f rotationMatrixOf(const math::Quatf& rotation) noexcept;

    /**
     * @brief World-space covariance `Sigma = R * S * S^T * R^T`.
     *
     * @param scale Linear scales along the local axes. Not required to be
     *              positive for the product to be symmetric; a negative scale
     *              flips an axis and squares away.
     * @param rotation Rotation quaternion; normalized internally.
     */
    [[nodiscard]] static math::Mat3f covarianceOf(const math::Vec3f& scale, const math::Quatf& rotation) noexcept;

    /**
     * @brief Symmetric matrix as six floats: `(00, 01, 02, 11, 12, 22)`.
     *
     * @details A symmetric covariance repeats its off-diagonals, so the
     *          reference rasterizer (`strip_symmetric`) stores six floats
     *          instead of nine. Off-diagonals are read from the upper triangle
     *          (`m[col][row]` with `col >= row`).
     */
    [[nodiscard]] static std::array<float, 6> packSymmetric(const math::Mat3f& matrix) noexcept;

   private:
    math::Vec3f position_{};
    math::Vec3f scale_{};
    math::Quatf rotation_{};
    float opacity_ = 1.0f;
    math::Vec3f color_{};
};

}  // namespace vne::gs
