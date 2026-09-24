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

#include <array>
#include <cmath>
#include <cstddef>
#include <random>

#include <gtest/gtest.h>

namespace {

[[nodiscard]] bool nearMatrix(const vne::math::Mat3f& actual, const vne::math::Mat3f& expected, float tolerance) {
    for (std::size_t col = 0; col < 3; ++col) {
        for (std::size_t row = 0; row < 3; ++row) {
            if (std::abs(actual[col][row] - expected[col][row]) > tolerance) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] vne::math::Mat3f diagonal(float x, float y, float z) {
    vne::math::Mat3f matrix;
    matrix[0][0] = x;
    matrix[1][1] = y;
    matrix[2][2] = z;
    return matrix;
}

[[nodiscard]] float dot(const vne::math::Vec3f& a, const vne::math::Vec3f& b) {
    return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

}  // namespace

TEST(Gaussian3D, IdentityRotationIsAxisAligned) {
    const vne::math::Vec3f scale(2.0f, 1.0f, 0.5f);
    const vne::math::Mat3f sigma = vne::gs::Gaussian3D::covarianceOf(scale, vne::math::Quatf::identity());

    EXPECT_TRUE(nearMatrix(sigma, diagonal(4.0f, 1.0f, 0.25f), 1e-6f));

    const std::array<float, 6> packed = vne::gs::Gaussian3D::packSymmetric(sigma);
    EXPECT_NEAR(packed[0], 4.0f, 1e-6f);
    EXPECT_NEAR(packed[1], 0.0f, 1e-6f);
    EXPECT_NEAR(packed[2], 0.0f, 1e-6f);
    EXPECT_NEAR(packed[3], 1.0f, 1e-6f);
    EXPECT_NEAR(packed[4], 0.0f, 1e-6f);
    EXPECT_NEAR(packed[5], 0.25f, 1e-6f);
}

TEST(Gaussian3D, QuarterTurnAboutZSwapsTheLongAxis) {
    // 90 degrees about +z: q = (cos 45, 0, 0, sin 45), scalar part first.
    constexpr float kHalfTurn = 0.70710678118f;
    const vne::math::Quatf rotation(0.0f, 0.0f, kHalfTurn, kHalfTurn);
    const vne::math::Mat3f sigma = vne::gs::Gaussian3D::covarianceOf(vne::math::Vec3f(2.0f, 1.0f, 0.5f), rotation);

    EXPECT_TRUE(nearMatrix(sigma, diagonal(1.0f, 4.0f, 0.25f), 1e-5f));

    const std::array<float, 6> packed = vne::gs::Gaussian3D::packSymmetric(sigma);
    EXPECT_NEAR(packed[0], 1.0f, 1e-5f);
    EXPECT_NEAR(packed[3], 4.0f, 1e-5f);
    EXPECT_NEAR(packed[5], 0.25f, 1e-5f);
}

TEST(Gaussian3D, RandomCovariancesAreSymmetricAndPositiveSemiDefinite) {
    std::mt19937 rng(0x3D65u);
    std::uniform_real_distribution<float> signed_unit(-1.0f, 1.0f);
    std::uniform_real_distribution<float> positive_scale(0.05f, 4.0f);

    for (int sample = 0; sample < 100; ++sample) {
        const vne::math::Vec3f scale(positive_scale(rng), positive_scale(rng), positive_scale(rng));
        vne::math::Quatf rotation(signed_unit(rng), signed_unit(rng), signed_unit(rng), signed_unit(rng));
        if (rotation.length() < 1e-3f) {
            rotation = vne::math::Quatf::identity();
        }

        const vne::math::Mat3f sigma = vne::gs::Gaussian3D::covarianceOf(scale, rotation);
        for (std::size_t col = 0; col < 3; ++col) {
            for (std::size_t row = 0; row < 3; ++row) {
                EXPECT_NEAR(sigma[col][row], sigma[row][col], 1e-5f);
            }
        }

        vne::math::Vec3f direction(signed_unit(rng), signed_unit(rng), signed_unit(rng));
        if (direction.length() < 1e-3f) {
            direction = vne::math::Vec3f(1.0f, 0.0f, 0.0f);
        }
        direction = direction.normalized();
        const float quadratic = dot(direction, sigma * direction);
        EXPECT_GE(quadratic, -1e-4f);
    }
}

TEST(Gaussian3D, ScaleAxesAreEigenvectors) {
    std::mt19937 rng(0xC0A1u);
    std::uniform_real_distribution<float> signed_unit(-1.0f, 1.0f);
    std::uniform_real_distribution<float> positive_scale(0.05f, 4.0f);

    for (int sample = 0; sample < 50; ++sample) {
        const vne::math::Vec3f scale(positive_scale(rng), positive_scale(rng), positive_scale(rng));
        const vne::math::Quatf rotation(signed_unit(rng), signed_unit(rng), signed_unit(rng), signed_unit(rng));
        if (rotation.length() < 1e-3f) {
            continue;
        }

        const vne::math::Mat3f axes = vne::gs::Gaussian3D::rotationMatrixOf(rotation);
        const vne::math::Mat3f sigma = vne::gs::Gaussian3D::covarianceOf(scale, rotation);
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const vne::math::Vec3f column = axes[axis];
            const vne::math::Vec3f applied = sigma * column;
            const float scale_squared = scale[axis] * scale[axis];
            EXPECT_NEAR(applied.x(), static_cast<double>(scale_squared) * static_cast<double>(column.x()), 1e-4f);
            EXPECT_NEAR(applied.y(), static_cast<double>(scale_squared) * static_cast<double>(column.y()), 1e-4f);
            EXPECT_NEAR(applied.z(), static_cast<double>(scale_squared) * static_cast<double>(column.z()), 1e-4f);
        }
    }
}

TEST(Gaussian3D, MatchesQuatToMatrixOnRandomQuaternions) {
    std::mt19937 rng(0x0A11u);
    std::uniform_real_distribution<float> signed_unit(-2.0f, 2.0f);

    int compared = 0;
    while (compared < 100) {
        const float x = signed_unit(rng);
        const float y = signed_unit(rng);
        const float z = signed_unit(rng);
        const float w = signed_unit(rng);
        const vne::math::Quatf rotation(x, y, z, w);
        if (rotation.length() <= vne::math::kEpsilon<float>) {
            continue;
        }

        const vne::math::Mat3f ours = vne::gs::Gaussian3D::rotationMatrixOf(rotation);
        const vne::math::Mat3f reference = rotation.normalized().toMatrix3();
        EXPECT_TRUE(nearMatrix(ours, reference, 1e-5f));
        ++compared;
    }
}

TEST(Gaussian3D, NegatedQuaternionIsTheSameRotation) {
    const vne::math::Quatf positive_q(-0.4f, 0.5f, 0.1f, 0.2f);
    const vne::math::Quatf negative_q(0.4f, -0.5f, -0.1f, -0.2f);
    const vne::math::Mat3f positive = vne::gs::Gaussian3D::rotationMatrixOf(positive_q);
    const vne::math::Mat3f negative = vne::gs::Gaussian3D::rotationMatrixOf(negative_q);

    EXPECT_TRUE(nearMatrix(positive, negative, 1e-6f));
}

TEST(Gaussian3D, UnnormalizedRealQuaternionIsIdentity) {
    const vne::math::Quatf unnormalized(0.0f, 0.0f, 0.0f, 2.0f);
    const vne::math::Mat3f rotation = vne::gs::Gaussian3D::rotationMatrixOf(unnormalized);

    EXPECT_TRUE(nearMatrix(rotation, vne::math::Mat3f{}, 1e-6f));
}

TEST(Gaussian3D, ActivatedScaleAndRotationBuildCovariance) {
    const vne::gs::Gaussian3D gaussian(vne::math::Vec3f(1.0f, 2.0f, 3.0f),
                                       vne::math::Vec3f(2.0f, 1.0f, 0.5f),
                                       vne::math::Quatf::identity(),
                                       0.8f,
                                       vne::math::Vec3f(1.0f, 0.0f, 0.0f));

    EXPECT_TRUE(nearMatrix(gaussian.covariance(), diagonal(4.0f, 1.0f, 0.25f), 1e-6f));
    EXPECT_TRUE(nearMatrix(gaussian.rotationMatrix(), vne::math::Mat3f{}, 1e-6f));
    EXPECT_FLOAT_EQ(gaussian.opacity(), 0.8f);
    EXPECT_FLOAT_EQ(gaussian.position().y(), 2.0f);
    EXPECT_FLOAT_EQ(gaussian.color().x(), 1.0f);

    const std::array<float, 6> packed = gaussian.packedCovariance();
    EXPECT_NEAR(packed[0], 4.0f, 1e-6f);
    EXPECT_NEAR(packed[3], 1.0f, 1e-6f);
    EXPECT_NEAR(packed[5], 0.25f, 1e-6f);
}

TEST(Gaussian3D, SettersRoundTrip) {
    vne::gs::Gaussian3D gaussian;
    gaussian.setPosition(vne::math::Vec3f(1.0f, 2.0f, 3.0f));
    gaussian.setScale(vne::math::Vec3f(2.0f, 1.0f, 0.5f));
    gaussian.setRotation(vne::math::Quatf::identity());
    gaussian.setOpacity(0.25f);
    gaussian.setColor(vne::math::Vec3f(0.1f, 0.2f, 0.3f));

    EXPECT_FLOAT_EQ(gaussian.position().z(), 3.0f);
    EXPECT_FLOAT_EQ(gaussian.scale().x(), 2.0f);
    EXPECT_FLOAT_EQ(gaussian.opacity(), 0.25f);
    EXPECT_FLOAT_EQ(gaussian.color().z(), 0.3f);
    EXPECT_TRUE(nearMatrix(gaussian.covariance(), diagonal(4.0f, 1.0f, 0.25f), 1e-6f));
}
