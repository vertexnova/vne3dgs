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

#include "vertexnova/gs/camera/camera.h"

#include "vertexnova/math/core/constants.h"

#include <cmath>

namespace vne::gs {

namespace {

constexpr float kEps = 1e-6f;

[[nodiscard]] math::Vec3f orthonormalPerpendicular(const math::Vec3f& axis) noexcept {
    // Pick a world axis least aligned with `axis`, then cross for a stable right.
    const math::Vec3f ref = (std::abs(axis.y()) < 0.9f) ? math::Vec3f(0.0f, 1.0f, 0.0f) : math::Vec3f(1.0f, 0.0f, 0.0f);
    math::Vec3f right = axis.cross(ref);
    if (right.length() < kEps) {
        right = axis.cross(math::Vec3f(0.0f, 0.0f, 1.0f));
    }
    return right.normalized();
}

/** @brief Normalizes `v`, falling back to `fallback` when it has no direction. */
[[nodiscard]] math::Vec3f normalizedOr(const math::Vec3f& v, const math::Vec3f& fallback) noexcept {
    const float len = v.length();
    return (len < kEps) ? fallback : (v / len);
}

}  // namespace

Intrinsics Intrinsics::fromFovY(float fovy_rad, std::uint32_t width, std::uint32_t height) {
    Intrinsics k;
    k.setResolution(width, height);
    k.setPrincipalPoint(0.5f * static_cast<float>(width), 0.5f * static_cast<float>(height));
    // Strictly (0, π): zero/negative and ≥π make tan(fovy/2) non-positive or undefined.
    if (!(fovy_rad > 0.0f && fovy_rad < math::kPi)) {
        k.setFocalLength(0.0f, 0.0f);
        return k;
    }
    const float tan_half = std::tan(0.5f * fovy_rad);
    const float fy = (tan_half > kEps) ? (static_cast<float>(height) / (2.0f * tan_half)) : 0.0f;
    k.setFocalLength(fy, fy);
    return k;
}

float Intrinsics::fovYRad() const noexcept {
    if (fy_ <= 0.0f || height_ == 0u) {
        return 0.0f;
    }
    return 2.0f * std::atan(0.5f * static_cast<float>(height_) / fy_);
}

math::Vec3f Camera::position() const {
    return -(rotation_.transpose() * translation_);
}

Camera Camera::lookAt(const math::Vec3f& eye,
                      const math::Vec3f& target,
                      const math::Vec3f& up,
                      const Intrinsics& intrinsics) {
    const math::Vec3f forward = normalizedOr(target - eye, math::Vec3f(0.0f, 0.0f, -1.0f));
    const math::Vec3f up_n = normalizedOr(up, math::Vec3f(0.0f, 1.0f, 0.0f));

    math::Vec3f right = forward.cross(up_n);
    if (right.length() < kEps) {
        // View direction parallel to `up`: no unique right, pick any perpendicular.
        right = orthonormalPerpendicular(forward);
    } else {
        right = right.normalized();
    }

    const math::Vec3f down = forward.cross(right);

    // Rows of R are right, down, forward; Mat3f takes columns, so transpose.
    const math::Mat3f rotation = math::Mat3f(right, down, forward).transpose();
    return Camera(rotation, -(rotation * eye), intrinsics);
}

Camera Camera::orbit(const math::Vec3f& center,
                     float radius,
                     float azimuth_rad,
                     float elevation_rad,
                     const math::Vec3f& up,
                     const Intrinsics& intrinsics) {
    const math::Vec3f up_n = normalizedOr(up, math::Vec3f(0.0f, 1.0f, 0.0f));
    const math::Vec3f right = orthonormalPerpendicular(up_n);
    const math::Vec3f forward = right.cross(up_n);  // horizontal "north", unit

    const float cos_el = std::cos(elevation_rad);
    const float sin_el = std::sin(elevation_rad);
    const float cos_az = std::cos(azimuth_rad);
    const float sin_az = std::sin(azimuth_rad);

    // Local spherical offset: azimuth about up, elevation toward up.
    const math::Vec3f offset = (right * (sin_az * cos_el) + forward * (cos_az * cos_el) + up_n * sin_el) * radius;
    return lookAt(center + offset, center, up_n, intrinsics);
}

}  // namespace vne::gs
