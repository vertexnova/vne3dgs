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

#include "vertexnova/gs/core/conic.h"

#include <optional>

namespace vne::gs {

namespace {

// Packed as (a, b, c) = (Sigma_00, averaged off-diagonal, Sigma_11).
[[nodiscard]] math::Vec3f unpackCov(const math::Mat2f& cov) noexcept {
    return {cov[0][0], 0.5f * (cov[0][1] + cov[1][0]), cov[1][1]};
}

}  // namespace

std::optional<Conic> computeConic(const math::Mat2f& cov) noexcept {
    const math::Vec3f e = unpackCov(cov);
    const float det = e.x() * e.z() - e.y() * e.y();
    if (det <= 0.0f) {
        return std::nullopt;
    }
    const float inv_det = 1.0f / det;
    return Conic{e.z() * inv_det, -e.y() * inv_det, e.x() * inv_det};
}

float computePower(const Conic& conic, const math::Vec2f& d) noexcept {
    const float dx = d.x();
    const float dy = d.y();
    return -0.5f * (conic.a * dx * dx + conic.c * dy * dy) - conic.b * dx * dy;
}

}  // namespace vne::gs
