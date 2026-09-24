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
 * This class holds the splat and turns its covariance into a conic and a
 * 3-sigma radius, and its opacity into a per-pixel alpha. Conic evaluation
 * lives in `conic.h`; compositing lives in `front_to_back_blender.h`.
 *
 * Pixel convention (see `camera/conventions.h`): pixel `(i, j)` covers
 * `[i, i+1) x [j, j+1)` and is sampled at its center `(i + 0.5, j + 0.5)`.
 *
 * @note Header-only on purpose. `alphaFromPower` and `alphaAt` run once per
 *       covered pixel per splat and must inline. The static members carry no
 *       state so they port unchanged to a GPU kernel.
 */

#include "vertexnova/gs/core/conic.h"

#include "vertexnova/math/core/mat.h"
#include "vertexnova/math/core/vec.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace vne::gs {

/**
 * @brief A 2D splat: screen-space mean, covariance, linear RGB and opacity.
 *
 * The covariance should be symmetric positive definite. `conic()` returns
 * nullopt otherwise, which is the signal to skip the splat.
 */
class Gaussian2D {
   public:
    /// Alpha is capped here so `1 - alpha` never reaches 0; the backward pass divides by it.
    static constexpr float kMaxAlpha = 0.99f;
    /// Alpha below this cannot change an 8-bit pixel, so the splat is skipped.
    static constexpr float kMinAlpha = 1.0f / 255.0f;
    /// Footprint cutoff along the major axis. Beyond it `G < exp(-4.5)`, about 1.1%.
    static constexpr float kRadiusSigma = 3.0f;

    constexpr Gaussian2D() noexcept = default;

    constexpr Gaussian2D(const math::Vec2f& mean,
                         const math::Mat2f& cov,
                         const math::Vec3f& color,
                         float opacity) noexcept
        : mean_(mean)
        , cov_(cov)
        , color_(color)
        , opacity_(opacity) {}

    [[nodiscard]] constexpr const math::Vec2f& mean() const noexcept { return mean_; }
    void setMean(const math::Vec2f& mean) noexcept { mean_ = mean; }

    [[nodiscard]] constexpr const math::Mat2f& covariance() const noexcept { return cov_; }
    void setCovariance(const math::Mat2f& cov) noexcept { cov_ = cov; }

    /** @brief Linear RGB radiance, not gamma-encoded and not premultiplied. */
    [[nodiscard]] constexpr const math::Vec3f& color() const noexcept { return color_; }
    void setColor(const math::Vec3f& color) noexcept { color_ = color; }

    [[nodiscard]] constexpr float opacity() const noexcept { return opacity_; }
    void setOpacity(float opacity) noexcept { opacity_ = opacity; }

    /** @brief Inverse covariance, or nullopt when the covariance is degenerate. */
    [[nodiscard]] constexpr std::optional<Conic> conic() const noexcept { return Conic::fromCovariance(cov_); }

    /** @brief `(lambda1, lambda2)` of this splat's covariance, largest first. */
    [[nodiscard]] math::Vec2f eigenvalues() const noexcept { return eigenvaluesOf(cov_); }

    /** @brief Radius in pixels of the 3-sigma footprint, 0 when degenerate. */
    [[nodiscard]] int radius() const noexcept { return radiusOf(cov_); }

    /**
     * @brief Alpha of this splat at a pixel center, given its conic.
     *
     * Pass the conic in rather than recomputing it: a splat is inverted once
     * and then evaluated over its whole footprint.
     *
     * @return 0 when the pixel contributes nothing (see `alphaFromPower`).
     */
    [[nodiscard]] float alphaAt(const Conic& conic, const math::Vec2f& pixel) const noexcept {
        return alphaFromPower(opacity_, conic.power(pixel - mean_));
    }

    /**
     * @brief Eigenvalues of a symmetric 2x2 covariance, largest first.
     * @return `(lambda1, lambda2)` with `lambda1 >= lambda2`. Algebraic; not
     *         filtered for `det <= 0`.
     */
    [[nodiscard]] static math::Vec2f eigenvaluesOf(const math::Mat2f& cov) noexcept {
        const math::Vec3f e = detail::symmetric2(cov);
        const float mid = 0.5f * (e.x() + e.z());
        const float disc = std::max(0.0f, mid * mid - detail::symmetric2Det(e));
        const float root = std::sqrt(disc);
        return {mid + root, mid - root};
    }

    /**
     * @brief Screen-space radius of the 3-sigma footprint: `ceil(3 * sqrt(lambda1))`.
     * @return 0 if the covariance is not positive definite.
     */
    [[nodiscard]] static int radiusOf(const math::Mat2f& cov) noexcept {
        if (!detail::isSymmetric2PositiveDefinite(detail::symmetric2(cov))) {
            return 0;
        }
        const float lambda1 = std::max(0.0f, eigenvaluesOf(cov).x());
        return static_cast<int>(std::ceil(kRadiusSigma * std::sqrt(lambda1)));
    }

    /**
     * @brief Opacity times falloff, with the reference rasterizer's skip rules.
     * @return `min(kMaxAlpha, opacity * exp(power))`, or 0 if `power > 0` (a
     *         numerical guard) or the result is below `kMinAlpha`.
     */
    [[nodiscard]] static float alphaFromPower(float opacity, float power) noexcept {
        if (power > 0.0f) {
            return 0.0f;
        }
        const float alpha = std::min(kMaxAlpha, opacity * std::exp(power));
        return (alpha < kMinAlpha) ? 0.0f : alpha;
    }

   private:
    math::Vec2f mean_{};
    math::Mat2f cov_{};
    math::Vec3f color_{};
    float opacity_ = 0.0f;
};

}  // namespace vne::gs
