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
 * buffer per column, SIMD can walk one attribute at a time, and unused
 * attributes can be skipped entirely. Values are activated: linear scales,
 * unit quaternions, opacities in (0, 1), and coefficient-major RGB spherical
 * harmonics.
 *
 * SH layout for Gaussian `i`, coefficient `k`, channel `c`:
 * `sh()[(i * shCoeffCount() + k) * 3 + c]`, with the DC term at `k = 0`.
 */

#include "vertexnova/gs/core/gaussian3d.h"
#include "vertexnova/gs/export.h"

#include "vertexnova/math/core/quat.h"
#include "vertexnova/math/core/vec.h"

#include <cstddef>
#include <span>
#include <vector>

namespace vne::gs {

/// Number of SH coefficients per channel for a given degree: `(degree + 1)^2`.
[[nodiscard]] VNE_GS_API int shCoeffCount(int degree) noexcept;

/**
 * @brief A trained scene: one array per Gaussian attribute, kept in lockstep.
 *
 * @details The class owns the invariant that every column has the same length
 * and that `sh()` has exactly `size() * shCoeffCount() * 3` entries. Mutable
 * access is handed out as `std::span`, which allows in-place writes but not
 * resizing, so the columns cannot drift apart. Change the element count only
 * through `resize()` and the SH width only through `setShDegree()`.
 *
 * @note Prefer this over `std::vector<Gaussian3D>`. The single-splat type is
 *       for math and tests; the cloud is what loaders and renderers share.
 *       `gaussian(i)` bridges the two.
 */
class VNE_GS_API GaussianCloud {
   public:
    /// Highest SH degree the 3DGS format defines (16 coefficients per channel).
    static constexpr int kMaxShDegree = 3;
    /// RGB channels stored per SH coefficient.
    static constexpr std::size_t kChannels = 3;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;

    /**
     * @brief Sets the number of Gaussians, resizing every column together.
     *
     * New elements are value-initialized; the SH block for them is zeroed.
     */
    void resize(std::size_t count);

    /** @brief Reserves capacity in every column without changing `size()`. */
    void reserve(std::size_t count);

    /** @brief Clears all attribute arrays and resets the SH degree to 0. */
    void clear() noexcept;

    /** @brief SH degree in `[0, kMaxShDegree]`. */
    [[nodiscard]] int shDegree() const noexcept;

    /**
     * @brief Sets the SH degree and reshapes the SH array to match.
     *
     * @param degree Clamped to `[0, kMaxShDegree]`. Existing coefficients are
     *               dropped: the SH block is re-zeroed, because a degree change
     *               moves every coefficient's offset.
     */
    void setShDegree(int degree);

    /** @brief Coefficients per channel: `(shDegree() + 1)^2`. */
    [[nodiscard]] int shCoeffCount() const noexcept;

    [[nodiscard]] std::span<const math::Vec3f> positions() const noexcept;
    [[nodiscard]] std::span<math::Vec3f> positions() noexcept;

    /** @brief Linear 1-sigma extents, already through `exp`. */
    [[nodiscard]] std::span<const math::Vec3f> scales() const noexcept;
    [[nodiscard]] std::span<math::Vec3f> scales() noexcept;

    /** @brief Unit quaternions, already normalized. */
    [[nodiscard]] std::span<const math::Quatf> rotations() const noexcept;
    [[nodiscard]] std::span<math::Quatf> rotations() noexcept;

    /** @brief Peak alphas in (0, 1), already through the sigmoid. */
    [[nodiscard]] std::span<const float> opacities() const noexcept;
    [[nodiscard]] std::span<float> opacities() noexcept;

    /** @brief `size() * shCoeffCount() * 3` raw coefficients, coefficient-major RGB. */
    [[nodiscard]] std::span<const float> sh() const noexcept;
    [[nodiscard]] std::span<float> sh() noexcept;

    /**
     * @brief One SH coefficient as RGB.
     * @param i Gaussian index, `i < size()`.
     * @param k Coefficient index, `k < shCoeffCount()`.
     * @return The triplet, or zero if either index is out of range.
     */
    [[nodiscard]] math::Vec3f shCoefficient(std::size_t i, int k) const noexcept;

    /**
     * @brief Writes one SH coefficient. Out-of-range indices are ignored.
     */
    void setShCoefficient(std::size_t i, int k, const math::Vec3f& rgb) noexcept;

    /**
     * @brief Degree-0 RGB for Gaussian `i`: `max(0, 0.5 + C0 * f_dc)`.
     *
     * This is the view-independent base color. Full SH evaluation (Task 08)
     * adds the view-dependent terms on top of it.
     *
     * @return Black if `i >= size()`.
     */
    [[nodiscard]] math::Vec3f dcColor(std::size_t i) const noexcept;

    /**
     * @brief Gathers Gaussian `i` into the single-splat type, with DC color.
     * @return A default-constructed splat if `i >= size()`.
     */
    [[nodiscard]] Gaussian3D gaussian(std::size_t i) const noexcept;

    /**
     * @brief True when every column length agrees with `size()` and the SH
     *        array matches `size() * shCoeffCount() * 3`.
     *
     * The class maintains this; it is exposed so callers that receive a cloud
     * across an ABI boundary can assert on it.
     */
    [[nodiscard]] bool isConsistent() const noexcept;

   private:
    [[nodiscard]] std::size_t shElementCount() const noexcept;

    std::vector<math::Vec3f> positions_;
    std::vector<math::Vec3f> scales_;
    std::vector<math::Quatf> rotations_;
    std::vector<float> opacities_;
    std::vector<float> sh_;
    int sh_degree_ = 0;
};

}  // namespace vne::gs
