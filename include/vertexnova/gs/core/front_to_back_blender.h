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
 * @file front_to_back_blender.h
 * @brief Front-to-back compositor for a single pixel.
 * @ingroup vne::gs
 *
 * @details Walks splats nearest-first, accumulating radiance
 * `C += T * alpha * c` and transmittance `T *= (1 - alpha)`. The splat that
 * would push `T` under `kTransmittanceEps` is not composited, matching the
 * reference rasterizer so CPU and GPU images can agree bit-for-bit later.
 *
 * @note Header-only on purpose: one instance per pixel, `composite()` called
 *       once per splat covering it. It must inline.
 */

#include "vertexnova/math/core/vec.h"

namespace vne::gs {

/**
 * @brief Accumulates radiance and transmittance for one pixel, nearest first.
 */
class FrontToBackBlender {
   public:
    /// Stop compositing once `T * (1 - alpha)` would fall below this (splat not composited).
    static constexpr float kTransmittanceEps = 1e-4f;

    /**
     * @brief Composite one splat: `C += T * alpha * radiance`, `T *= (1 - alpha)`.
     * @return false if this splat would saturate transmittance, or if an earlier
     *         splat already did; radiance is unchanged in both cases. A false
     *         return means the caller can stop walking this pixel's splats.
     */
    bool composite(const math::Vec3f& radiance, float alpha) noexcept {
        if (is_saturated_) {
            return false;
        }
        const float next_transmittance = transmittance_ * (1.0f - alpha);
        if (next_transmittance < kTransmittanceEps) {
            is_saturated_ = true;
            return false;
        }
        radiance_ += transmittance_ * alpha * radiance;
        transmittance_ = next_transmittance;
        return true;
    }

    /** @brief Resolved pixel: `C + T * background`. */
    [[nodiscard]] math::Vec3f resolve(const math::Vec3f& background) const noexcept {
        return radiance_ + transmittance_ * background;
    }

    /** @brief Remaining transmittance `T` (1 at the start, product of `(1 - alpha)`). */
    [[nodiscard]] float transmittance() const noexcept { return transmittance_; }

    /** @brief True once a splat was dropped for saturating transmittance. */
    [[nodiscard]] bool isSaturated() const noexcept { return is_saturated_; }

   private:
    math::Vec3f radiance_{};
    float transmittance_ = 1.0f;
    bool is_saturated_ = false;
};

}  // namespace vne::gs
