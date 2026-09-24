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
 * @file conic.h
 * @brief Inverse 2x2 covariance (conic) and Gaussian power.
 * @ingroup vne::gs
 *
 * @details `Sigma^-1 = [[a, b], [b, c]]` is evaluated as
 * `power = -1/2 * (a*dx^2 + c*dy^2) - b*dx*dy`. The name is from the conic
 * section `a*dx^2 + 2b*dx*dy + c*dy^2 = k` (here an ellipse).
 *
 * @note Header-only on purpose. `power()` runs once per covered pixel per
 *       splat, so it must inline; an exported out-of-line call would cost a
 *       PLT jump in the innermost loop of the CPU rasterizers.
 */

#include "vertexnova/math/core/mat.h"
#include "vertexnova/math/core/vec.h"

#include <optional>

namespace vne::gs {

namespace detail {

/**
 * @brief Upper triangle of a symmetric 2x2 matrix as `(a, b, c)`.
 *
 * Off-diagonals are averaged so a slightly asymmetric input (round-off from a
 * projection Jacobian) still yields a symmetric result.
 */
[[nodiscard]] constexpr math::Vec3f symmetric2(const math::Mat2f& m) noexcept {
    return {m[0][0], 0.5f * (m[0][1] + m[1][0]), m[1][1]};
}

/** @brief Determinant of a symmetric 2x2 packed by `symmetric2`. */
[[nodiscard]] constexpr float symmetric2Det(const math::Vec3f& e) noexcept {
    return e.x() * e.z() - e.y() * e.y();
}

/**
 * @brief True when a symmetric 2x2 packed by `symmetric2` is positive definite.
 *
 * Sylvester's criterion: leading minor and determinant both positive. Written
 * as `!(x > 0)` so a NaN entry is rejected rather than accepted.
 */
[[nodiscard]] constexpr bool isSymmetric2PositiveDefinite(const math::Vec3f& e) noexcept {
    return (e.x() > 0.0f) && (symmetric2Det(e) > 0.0f);
}

}  // namespace detail

/**
 * @brief Inverse of a 2x2 covariance: `Sigma^-1 = [[a, b], [b, c]]`.
 *
 * Construct from a covariance with `fromCovariance`, which rejects matrices
 * that are not positive definite, or from three stored floats when the values
 * come back from a GPU buffer.
 */
class Conic {
   public:
    constexpr Conic() noexcept = default;

    /** @brief Wraps three already-inverted coefficients. No validation. */
    constexpr Conic(float a, float b, float c) noexcept
        : a_(a)
        , b_(b)
        , c_(c) {}

    /**
     * @brief Inverts a 2x2 covariance.
     * @return The conic, or nullopt if `cov` is not positive definite (which
     *         includes any NaN entry). The splat is skipped in that case.
     */
    [[nodiscard]] static constexpr std::optional<Conic> fromCovariance(const math::Mat2f& cov) noexcept {
        const math::Vec3f e = detail::symmetric2(cov);
        if (!detail::isSymmetric2PositiveDefinite(e)) {
            return std::nullopt;
        }
        const float inv_det = 1.0f / detail::symmetric2Det(e);
        return Conic{e.z() * inv_det, -e.y() * inv_det, e.x() * inv_det};
    }

    [[nodiscard]] constexpr float a() const noexcept { return a_; }
    [[nodiscard]] constexpr float b() const noexcept { return b_; }
    [[nodiscard]] constexpr float c() const noexcept { return c_; }

    /**
     * @brief Gaussian power at offset `d = pixel - mean`.
     * @return `-1/2 * d^T * Sigma^-1 * d`. Zero at the mean, so `G(mean) = 1`.
     */
    [[nodiscard]] constexpr float power(const math::Vec2f& d) const noexcept {
        const float dx = d.x();
        const float dy = d.y();
        return -0.5f * (a_ * dx * dx + c_ * dy * dy) - b_ * dx * dy;
    }

   private:
    float a_ = 0.0f;
    float b_ = 0.0f;
    float c_ = 0.0f;
};

}  // namespace vne::gs
