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
 * @file gaussian_cloud.h
 * @brief Struct-of-arrays storage for a trained 3DGS scene.
 * @ingroup vne::gs
 *
 * @details Each attribute is a contiguous array so a GPU upload can bind one
 * buffer per column. Values are activated: linear scales, unit quaternions,
 * opacities in (0, 1), and coefficient-major RGB spherical harmonics.
 *
 * SH layout for Gaussian `i`, coefficient `k`, channel `c`:
 * `sh[(i * shCoeffCount(shDegree()) + k) * 3 + c]`.
 */

#include "vertexnova/gs/export.h"

#include "vertexnova/math/core/quat.h"
#include "vertexnova/math/core/vec.h"

#include <cstddef>
#include <vector>

namespace vne::gs {

/// Number of SH coefficients per channel for a given degree: `(degree + 1)^2`.
[[nodiscard]] VNE_GS_API int shCoeffCount(int degree) noexcept;

/**
 * @brief A trained scene: one array per Gaussian attribute.
 *
 * @note Prefer this over `std::vector<Gaussian3D>`. The single-splat type is for
 *       math and tests; the cloud is what loaders and renderers share.
 */
class VNE_GS_API GaussianCloud {
   public:
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] const std::vector<math::Vec3f>& positions() const noexcept;
    [[nodiscard]] std::vector<math::Vec3f>& positions() noexcept;

    [[nodiscard]] const std::vector<math::Vec3f>& scales() const noexcept;
    [[nodiscard]] std::vector<math::Vec3f>& scales() noexcept;

    [[nodiscard]] const std::vector<math::Quatf>& rotations() const noexcept;
    [[nodiscard]] std::vector<math::Quatf>& rotations() noexcept;

    [[nodiscard]] const std::vector<float>& opacities() const noexcept;
    [[nodiscard]] std::vector<float>& opacities() noexcept;

    /** @brief `size() * shCoeffCount(shDegree()) * 3`, coefficient-major RGB. */
    [[nodiscard]] const std::vector<float>& sh() const noexcept;
    [[nodiscard]] std::vector<float>& sh() noexcept;

    [[nodiscard]] int shDegree() const noexcept;
    void setShDegree(int degree) noexcept;

    /**
     * @brief Degree-0 RGB for Gaussian `i`: `max(0, 0.5 + C0 * f_dc)`.
     */
    [[nodiscard]] math::Vec3f dcColor(std::size_t i) const noexcept;

    /** @brief Clears all attribute arrays and resets SH degree to 0. */
    void clear() noexcept;

   private:
    std::vector<math::Vec3f> positions_;
    std::vector<math::Vec3f> scales_;
    std::vector<math::Quatf> rotations_;
    std::vector<float> opacities_;
    std::vector<float> sh_;
    int sh_degree_ = 0;
};

}  // namespace vne::gs
