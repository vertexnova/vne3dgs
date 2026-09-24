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

#include "vertexnova/gs/core/conic.h"

#include <cmath>
#include <limits>
#include <optional>

#include <gtest/gtest.h>

namespace {

[[nodiscard]] vne::math::Mat2f cov2(float a, float b, float c) {
    return {vne::math::Vec2f(a, b), vne::math::Vec2f(b, c)};
}

}  // namespace

TEST(Conic, PowerAtMeanIsZero) {
    const std::optional<vne::gs::Conic> conic = vne::gs::Conic::fromCovariance(cov2(4.0f, 0.0f, 1.0f));

    ASSERT_TRUE(conic.has_value());
    EXPECT_FLOAT_EQ(conic->power(vne::math::Vec2f(0.0f, 0.0f)), 0.0f);
}

TEST(Conic, AxisAlignedWorkedExample) {
    const std::optional<vne::gs::Conic> conic = vne::gs::Conic::fromCovariance(cov2(4.0f, 0.0f, 1.0f));

    ASSERT_TRUE(conic.has_value());
    EXPECT_NEAR(conic->a(), 0.25f, 1e-5f);
    EXPECT_NEAR(conic->b(), 0.0f, 1e-5f);
    EXPECT_NEAR(conic->c(), 1.0f, 1e-5f);
    EXPECT_NEAR(conic->power(vne::math::Vec2f(2.0f, 0.0f)), -0.5f, 1e-4f);
}

TEST(Conic, RotatedWorkedExample) {
    const std::optional<vne::gs::Conic> conic = vne::gs::Conic::fromCovariance(cov2(2.5f, 1.5f, 2.5f));

    ASSERT_TRUE(conic.has_value());
    EXPECT_NEAR(conic->a(), 0.625f, 1e-5f);
    EXPECT_NEAR(conic->b(), -0.375f, 1e-5f);
    EXPECT_NEAR(conic->c(), 0.625f, 1e-5f);

    // 2 px along the major axis e1 = (1,1)/sqrt(2) is exactly 1 sigma.
    const float sqrt2 = std::sqrt(2.0f);
    EXPECT_NEAR(conic->power(vne::math::Vec2f(sqrt2, sqrt2)), -0.5f, 1e-4f);
}

TEST(Conic, RejectsNonPositiveDefinite) {
    EXPECT_FALSE(vne::gs::Conic::fromCovariance(cov2(1.0f, 2.0f, 1.0f)).has_value());
}

TEST(Conic, RejectsNegativeDefinite) {
    EXPECT_FALSE(vne::gs::Conic::fromCovariance(cov2(-1.0f, 0.0f, -1.0f)).has_value());
}

TEST(Conic, RejectsNaNCovariance) {
    const float nan = std::numeric_limits<float>::quiet_NaN();

    EXPECT_FALSE(vne::gs::Conic::fromCovariance(cov2(nan, 0.0f, 1.0f)).has_value());
    EXPECT_FALSE(vne::gs::Conic::fromCovariance(cov2(1.0f, nan, 1.0f)).has_value());
    EXPECT_FALSE(vne::gs::Conic::fromCovariance(cov2(1.0f, 0.0f, nan)).has_value());
}

TEST(Conic, AveragesSlightlyAsymmetricOffDiagonals) {
    // A projection Jacobian can leave the two off-diagonals differing by
    // round-off; the conic must still come out symmetric.
    const vne::math::Mat2f skewed{vne::math::Vec2f(2.5f, 1.4f), vne::math::Vec2f(1.6f, 2.5f)};

    const std::optional<vne::gs::Conic> conic = vne::gs::Conic::fromCovariance(skewed);

    ASSERT_TRUE(conic.has_value());
    EXPECT_NEAR(conic->a(), 0.625f, 1e-5f);
    EXPECT_NEAR(conic->b(), -0.375f, 1e-5f);
    EXPECT_NEAR(conic->c(), 0.625f, 1e-5f);
}

TEST(Conic, ConstructsFromStoredCoefficients) {
    // A conic read back from a GPU buffer is already inverted.
    constexpr vne::gs::Conic conic(0.25f, 0.0f, 1.0f);

    static_assert(conic.a() == 0.25f, "accessors are constexpr");
    EXPECT_NEAR(conic.power(vne::math::Vec2f(2.0f, 0.0f)), -0.5f, 1e-4f);
}
