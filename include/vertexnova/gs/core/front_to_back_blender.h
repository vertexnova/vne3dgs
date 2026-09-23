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
 * would push `T` under `kTransmittanceEps` is not composited so CPU and GPU
 * images can agree bit-for-bit later.
 */

#include "vertexnova/gs/export.h"

#include "vertexnova/math/core/vec.h"

namespace vne::gs {

/// Stop compositing once `T * (1 - alpha)` would fall below this (splat not composited).
VNE_GS_API extern const float kTransmittanceEps;

class VNE_GS_API FrontToBackBlender {
   public:
    /**
     * @brief Front-to-back over: `C += T * alpha * radiance`, `T *= (1 - alpha)`.
     * @return false if this splat would saturate transmittance, or if an earlier
     *         splat already did; radiance is unchanged.
     */
    bool composite(const math::Vec3f& radiance, float alpha) noexcept;

    /** @brief Resolved pixel: `C + T * background`. */
    [[nodiscard]] math::Vec3f resolve(const math::Vec3f& background) const noexcept;

    /** @brief Remaining transmittance `T` (1 at the start, product of `(1 - alpha)`). */
    [[nodiscard]] float transmittance() const noexcept;

   private:
    math::Vec3f radiance_{};
    float transmittance_{1.0f};
    bool is_saturated_{false};
};

}  // namespace vne::gs
