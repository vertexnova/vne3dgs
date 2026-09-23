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
#include "vertexnova/gs/core/gaussian2d.h"

#include <optional>

#include <gtest/gtest.h>

namespace {

[[nodiscard]] vne::math::Mat2f cov2(float a, float b, float c) {
    return {vne::math::Vec2f(a, b), vne::math::Vec2f(b, c)};
}

}  // namespace

TEST(Gaussian2D, AxisAlignedWorkedExample) {
    const vne::math::Mat2f cov = cov2(4.0f, 0.0f, 1.0f);

    const std::optional<vne::gs::Conic> conic = vne::gs::computeConic(cov);
    ASSERT_TRUE(conic.has_value());
    const float power = vne::gs::computePower(*conic, vne::math::Vec2f(2.0f, 0.0f));
    const float alpha = vne::gs::computeAlpha(0.8f, power);

    EXPECT_NEAR(alpha, 0.4852f, 1e-4f);
    EXPECT_EQ(vne::gs::computeRadius(cov), 6);
}

TEST(Gaussian2D, RotatedEigenvalues) {
    const vne::math::Mat2f cov = cov2(2.5f, 1.5f, 2.5f);

    const vne::math::Vec2f eigenvalues = vne::gs::computeEigenvalues(cov);

    EXPECT_NEAR(eigenvalues.x(), 4.0f, 1e-5f);
    EXPECT_NEAR(eigenvalues.y(), 1.0f, 1e-5f);
}

TEST(Gaussian2D, RadiusZeroWhenNonPositiveDefinite) {
    const int radius = vne::gs::computeRadius(cov2(1.0f, 2.0f, 1.0f));

    EXPECT_EQ(radius, 0);
}

TEST(Gaussian2D, RadiusZeroWhenNegativeDefinite) {
    const int radius = vne::gs::computeRadius(cov2(-1.0f, 0.0f, -1.0f));

    EXPECT_EQ(radius, 0);
}

TEST(Gaussian2D, AlphaIsCapped) {
    const float alpha = vne::gs::computeAlpha(1.0f, 0.0f);

    EXPECT_NEAR(alpha, 0.99f, 1e-6f);
}

TEST(Gaussian2D, AlphaBelowCutoffIsSkipped) {
    const float alpha = vne::gs::computeAlpha(0.003f, 0.0f);

    EXPECT_FLOAT_EQ(alpha, 0.0f);
}

TEST(Gaussian2D, AlphaSkippedWhenPowerPositive) {
    const float alpha = vne::gs::computeAlpha(1.0f, 0.1f);

    EXPECT_FLOAT_EQ(alpha, 0.0f);
}
