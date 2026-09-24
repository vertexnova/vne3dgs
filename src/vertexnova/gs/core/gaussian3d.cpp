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

#include "vertexnova/gs/core/gaussian3d.h"

namespace vne::gs {

math::Mat3f Gaussian3D::rotationMatrixOf(const math::Quatf& rotation) noexcept {
    // toMatrix3 assumes a unit quaternion. normalized() returns identity at length 0.
    return rotation.normalized().toMatrix3();
}

math::Mat3f Gaussian3D::covarianceOf(const math::Vec3f& scale, const math::Quatf& rotation) noexcept {
    const math::Mat3f rotation_matrix = rotationMatrixOf(rotation);

    math::Mat3f scaling;
    scaling[0][0] = scale.x();
    scaling[1][1] = scale.y();
    scaling[2][2] = scale.z();

    // Scale the unit sphere in its local frame, then rotate. S is diagonal, so
    // (R S) (R S)^T = R S S^T R^T.
    const math::Mat3f rotated_scale = rotation_matrix * scaling;
    return rotated_scale * rotated_scale.transpose();
}

std::array<float, 6> Gaussian3D::packSymmetric(const math::Mat3f& matrix) noexcept {
    return {matrix[0][0], matrix[1][0], matrix[2][0], matrix[1][1], matrix[2][1], matrix[2][2]};
}

}  // namespace vne::gs
