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
#include <optional>

#include <gtest/gtest.h>

namespace {

[[nodiscard]] vne::math::Mat2f cov2(float a, float b, float c) {
    return {vne::math::Vec2f(a, b), vne::math::Vec2f(b, c)};
}

}  // namespace

TEST(Conic, PowerAtMeanIsZero) {
    const std::optional<vne::gs::Conic> conic = vne::gs::computeConic(cov2(4.0f, 0.0f, 1.0f));

    ASSERT_TRUE(conic.has_value());
    EXPECT_FLOAT_EQ(vne::gs::computePower(*conic, vne::math::Vec2f(0.0f, 0.0f)), 0.0f);
}

TEST(Conic, AxisAlignedWorkedExample) {
    const vne::math::Mat2f cov = cov2(4.0f, 0.0f, 1.0f);

    const std::optional<vne::gs::Conic> conic = vne::gs::computeConic(cov);

    ASSERT_TRUE(conic.has_value());
    EXPECT_NEAR(conic->a, 0.25f, 1e-5f);
    EXPECT_NEAR(conic->b, 0.0f, 1e-5f);
    EXPECT_NEAR(conic->c, 1.0f, 1e-5f);

    const float power = vne::gs::computePower(*conic, vne::math::Vec2f(2.0f, 0.0f));
    EXPECT_NEAR(power, -0.5f, 1e-4f);
}

TEST(Conic, RotatedWorkedExample) {
    const vne::math::Mat2f cov = cov2(2.5f, 1.5f, 2.5f);

    const std::optional<vne::gs::Conic> conic = vne::gs::computeConic(cov);

    ASSERT_TRUE(conic.has_value());
    const float sqrt2 = std::sqrt(2.0f);
    const float power = vne::gs::computePower(*conic, vne::math::Vec2f(sqrt2, sqrt2));
    EXPECT_NEAR(conic->a, 0.625f, 1e-5f);
    EXPECT_NEAR(conic->b, -0.375f, 1e-5f);
    EXPECT_NEAR(conic->c, 0.625f, 1e-5f);
    EXPECT_NEAR(power, -0.5f, 1e-4f);
}

TEST(Conic, RejectsNonPositiveDefinite) {
    const std::optional<vne::gs::Conic> conic = vne::gs::computeConic(cov2(1.0f, 2.0f, 1.0f));

    EXPECT_FALSE(conic.has_value());
}
