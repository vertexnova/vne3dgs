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

#include "vertexnova/gs/core/front_to_back_blender.h"

namespace vne::gs {

VNE_GS_API const float kTransmittanceEps = 1e-4f;

bool FrontToBackBlender::composite(const math::Vec3f& radiance, float alpha) noexcept {
    const float next_transmittance = transmittance_ * (1.0f - alpha);
    if (next_transmittance < kTransmittanceEps) {
        return false;
    }
    radiance_ += transmittance_ * alpha * radiance;
    transmittance_ = next_transmittance;
    return true;
}

math::Vec3f FrontToBackBlender::resolve(const math::Vec3f& background) const noexcept {
    return radiance_ + transmittance_ * background;
}

float FrontToBackBlender::transmittance() const noexcept {
    return transmittance_;
}

}  // namespace vne::gs
