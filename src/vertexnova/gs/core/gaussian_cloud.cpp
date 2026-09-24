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

#include "vertexnova/gs/core/gaussian_cloud.h"

#include <algorithm>

namespace {

// SH degree-0 constant: 0.5 * sqrt(1 / pi).
constexpr float kShC0 = 0.28209479177387814f;

}  // namespace

namespace vne::gs {

int shCoeffCount(int degree) noexcept {
    const int clamped = std::clamp(degree, 0, GaussianCloud::kMaxShDegree);
    return (clamped + 1) * (clamped + 1);
}

std::size_t GaussianCloud::size() const noexcept {
    return positions_.size();
}

bool GaussianCloud::isEmpty() const noexcept {
    return positions_.empty();
}

void GaussianCloud::resize(std::size_t count) {
    positions_.resize(count);
    scales_.resize(count);
    rotations_.resize(count);
    opacities_.resize(count);
    // resize, not assign: the layout is Gaussian-major with a fixed stride, so
    // existing coefficients keep their offsets and only the new tail is zeroed.
    sh_.resize(shElementCount(), 0.0f);
}

void GaussianCloud::reserve(std::size_t count) {
    positions_.reserve(count);
    scales_.reserve(count);
    rotations_.reserve(count);
    opacities_.reserve(count);
    sh_.reserve(count * static_cast<std::size_t>(shCoeffCount()) * kChannels);
}

void GaussianCloud::clear() noexcept {
    positions_.clear();
    scales_.clear();
    rotations_.clear();
    opacities_.clear();
    sh_.clear();
    sh_degree_ = 0;
}

int GaussianCloud::shDegree() const noexcept {
    return sh_degree_;
}

void GaussianCloud::setShDegree(int degree) {
    const int clamped = std::clamp(degree, 0, kMaxShDegree);
    if (clamped == sh_degree_) {
        return;
    }
    sh_degree_ = clamped;
    // A degree change moves every coefficient's offset, so old values cannot be
    // reinterpreted in place.
    sh_.assign(shElementCount(), 0.0f);
}

int GaussianCloud::shCoeffCount() const noexcept {
    return vne::gs::shCoeffCount(sh_degree_);
}

std::size_t GaussianCloud::shElementCount() const noexcept {
    return positions_.size() * static_cast<std::size_t>(shCoeffCount()) * kChannels;
}

std::span<const math::Vec3f> GaussianCloud::positions() const noexcept {
    return positions_;
}

std::span<math::Vec3f> GaussianCloud::positions() noexcept {
    return positions_;
}

std::span<const math::Vec3f> GaussianCloud::scales() const noexcept {
    return scales_;
}

std::span<math::Vec3f> GaussianCloud::scales() noexcept {
    return scales_;
}

std::span<const math::Quatf> GaussianCloud::rotations() const noexcept {
    return rotations_;
}

std::span<math::Quatf> GaussianCloud::rotations() noexcept {
    return rotations_;
}

std::span<const float> GaussianCloud::opacities() const noexcept {
    return opacities_;
}

std::span<float> GaussianCloud::opacities() noexcept {
    return opacities_;
}

std::span<const float> GaussianCloud::sh() const noexcept {
    return sh_;
}

std::span<float> GaussianCloud::sh() noexcept {
    return sh_;
}

math::Vec3f GaussianCloud::shCoefficient(std::size_t i, int k) const noexcept {
    const int coeffs = shCoeffCount();
    if (i >= positions_.size() || k < 0 || k >= coeffs) {
        return {};
    }
    const std::size_t base = (i * static_cast<std::size_t>(coeffs) + static_cast<std::size_t>(k)) * kChannels;
    return {sh_[base + 0], sh_[base + 1], sh_[base + 2]};
}

void GaussianCloud::setShCoefficient(std::size_t i, int k, const math::Vec3f& rgb) noexcept {
    const int coeffs = shCoeffCount();
    if (i >= positions_.size() || k < 0 || k >= coeffs) {
        return;
    }
    const std::size_t base = (i * static_cast<std::size_t>(coeffs) + static_cast<std::size_t>(k)) * kChannels;
    sh_[base + 0] = rgb.x();
    sh_[base + 1] = rgb.y();
    sh_[base + 2] = rgb.z();
}

math::Vec3f GaussianCloud::dcColor(std::size_t i) const noexcept {
    if (i >= positions_.size()) {
        return {};
    }
    const math::Vec3f dc = shCoefficient(i, 0);
    return {std::max(0.0f, 0.5f + kShC0 * dc.x()),
            std::max(0.0f, 0.5f + kShC0 * dc.y()),
            std::max(0.0f, 0.5f + kShC0 * dc.z())};
}

Gaussian3D GaussianCloud::gaussian(std::size_t i) const noexcept {
    if (i >= positions_.size()) {
        return {};
    }
    return Gaussian3D(positions_[i], scales_[i], rotations_[i], opacities_[i], dcColor(i));
}

bool GaussianCloud::isConsistent() const noexcept {
    const std::size_t count = positions_.size();
    return scales_.size() == count && rotations_.size() == count && opacities_.size() == count
           && sh_.size() == shElementCount();
}

}  // namespace vne::gs
