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

#include "vertexnova/gs/core/gaussian2d.h"

#include <algorithm>
#include <cmath>

namespace vne::gs {

namespace {

constexpr float kMaxAlpha = 0.99f;
constexpr float kMinAlpha = 1.0f / 255.0f;
constexpr float kRadiusSigma = 3.0f;

// Packed as (a, b, c) = (Sigma_00, averaged off-diagonal, Sigma_11).
[[nodiscard]] math::Vec3f unpackCov(const math::Mat2f& cov) noexcept {
    return {cov[0][0], 0.5f * (cov[0][1] + cov[1][0]), cov[1][1]};
}

[[nodiscard]] float covarianceDet(const math::Vec3f& e) noexcept {
    return e.x() * e.z() - e.y() * e.y();
}

}  // namespace

math::Vec2f computeEigenvalues(const math::Mat2f& cov) noexcept {
    const math::Vec3f e = unpackCov(cov);
    const float det = covarianceDet(e);
    const float mid = 0.5f * (e.x() + e.z());
    const float disc = std::max(0.0f, mid * mid - det);
    const float root = std::sqrt(disc);
    return {mid + root, mid - root};
}

int computeRadius(const math::Mat2f& cov) noexcept {
    const math::Vec3f e = unpackCov(cov);
    if (covarianceDet(e) <= 0.0f) {
        return 0;
    }
    const float lambda1 = std::max(0.0f, computeEigenvalues(cov).x());
    return static_cast<int>(std::ceil(kRadiusSigma * std::sqrt(lambda1)));
}

float computeAlpha(float opacity, float power) noexcept {
    if (power > 0.0f) {
        return 0.0f;
    }
    const float alpha = std::min(kMaxAlpha, opacity * std::exp(power));
    if (alpha < kMinAlpha) {
        return 0.0f;
    }
    return alpha;
}

}  // namespace vne::gs
