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
 * @file conventions.h
 * @brief Coordinate conventions for `vne::gs` and the OpenGL bridge.
 * @ingroup vne::gs
 *
 * @details Inside `vne::gs`, cameras use the **OpenCV / COLMAP** frame: +X right,
 * +Y down, +Z forward. That matches training data and the reference rasterizer, so
 * projection and EWA formulas apply unchanged.
 *
 * vnescene (and vneinteraction) use an **OpenGL-style** right-handed camera by
 * default: +Y up, looking along −Z, with GraphicsApi-aware clip projection from
 * vnemath. Convert once at the viewer boundary (Task 09 `toGsCamera`); do not
 * pull vnescene into the core library.
 *
 * OpenGL camera space ↔ OpenCV camera space is a flip of Y and Z:
 * `diag(1, −1, −1)`. It is its own inverse. For a 4×4 view matrix,
 * `V_cv = diag(1, −1, −1, 1) · V_gl`.
 *
 * Pixel `(i, j)` covers `[i, i+1) × [j, j+1)`; its center is `(i + 0.5, j + 0.5)`.
 */

#include "vertexnova/gs/export.h"

#include "vertexnova/math/core/mat.h"

namespace vne::gs {

/**
 * @brief 3×3 map from OpenGL camera axes to OpenCV camera axes: `diag(1, −1, −1)`.
 *
 * Left-multiply a camera-space vector, or left-multiply the upper-left of an
 * OpenGL view matrix. Applying twice yields the identity.
 */
[[nodiscard]] inline math::Mat3f openGLToOpenCV() noexcept {
    math::Mat3f m(0.0f);
    m[0][0] = 1.0f;
    m[1][1] = -1.0f;
    m[2][2] = -1.0f;
    return m;
}

}  // namespace vne::gs
