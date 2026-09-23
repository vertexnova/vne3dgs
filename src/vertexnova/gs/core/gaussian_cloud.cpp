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
    return (degree + 1) * (degree + 1);
}

std::size_t GaussianCloud::size() const noexcept {
    return positions_.size();
}

const std::vector<math::Vec3f>& GaussianCloud::positions() const noexcept {
    return positions_;
}

std::vector<math::Vec3f>& GaussianCloud::positions() noexcept {
    return positions_;
}

const std::vector<math::Vec3f>& GaussianCloud::scales() const noexcept {
    return scales_;
}

std::vector<math::Vec3f>& GaussianCloud::scales() noexcept {
    return scales_;
}

const std::vector<math::Quatf>& GaussianCloud::rotations() const noexcept {
    return rotations_;
}

std::vector<math::Quatf>& GaussianCloud::rotations() noexcept {
    return rotations_;
}

const std::vector<float>& GaussianCloud::opacities() const noexcept {
    return opacities_;
}

std::vector<float>& GaussianCloud::opacities() noexcept {
    return opacities_;
}

const std::vector<float>& GaussianCloud::sh() const noexcept {
    return sh_;
}

std::vector<float>& GaussianCloud::sh() noexcept {
    return sh_;
}

int GaussianCloud::shDegree() const noexcept {
    return sh_degree_;
}

void GaussianCloud::setShDegree(int degree) noexcept {
    sh_degree_ = degree;
}

math::Vec3f GaussianCloud::dcColor(std::size_t i) const noexcept {
    const int coeffs = shCoeffCount(sh_degree_);
    const std::size_t base = i * static_cast<std::size_t>(coeffs) * 3u;
    if (base + 2 >= sh_.size()) {
        return {};
    }
    return {std::max(0.0f, 0.5f + kShC0 * sh_[base + 0]),
            std::max(0.0f, 0.5f + kShC0 * sh_[base + 1]),
            std::max(0.0f, 0.5f + kShC0 * sh_[base + 2])};
}

void GaussianCloud::clear() noexcept {
    positions_.clear();
    scales_.clear();
    rotations_.clear();
    opacities_.clear();
    sh_.clear();
    sh_degree_ = 0;
}

}  // namespace vne::gs
