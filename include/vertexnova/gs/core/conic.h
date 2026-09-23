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
 * @file conic.h
 * @brief Inverse 2x2 covariance (conic) and Gaussian power.
 * @ingroup vne::gs
 *
 * @details `Sigma^-1 = [[a, b], [b, c]]` is evaluated as
 * `power = -1/2 * (a*dx^2 + c*dy^2) - b*dx*dy`. The name is from the conic
 * section `a*dx^2 + 2b*dx*dy + c*dy^2 = k` (here an ellipse).
 */

#include "vertexnova/gs/export.h"

#include "vertexnova/math/core/mat.h"
#include "vertexnova/math/core/vec.h"

#include <optional>

namespace vne::gs {

/**
 * @brief Inverse of a 2x2 covariance: `Sigma^-1 = [[a, b], [b, c]]`.
 */
struct Conic {
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;
};

/**
 * @brief Inverts a 2x2 covariance to a conic.
 * @return The conic, or nullopt if `det(Sigma) <= 0` (not positive definite).
 */
[[nodiscard]] VNE_GS_API std::optional<Conic> computeConic(const math::Mat2f& cov) noexcept;

/**
 * @brief Gaussian power at offset `d = pixel - mu`.
 * @return `-1/2 * d^T * Sigma^-1 * d`. Zero at the mean, so `G(mu) = exp(0) = 1`.
 */
[[nodiscard]] VNE_GS_API float computePower(const Conic& conic, const math::Vec2f& d) noexcept;

}  // namespace vne::gs
