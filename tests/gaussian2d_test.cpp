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

    const std::optional<vne::gs::Conic> conic = vne::gs::Conic::fromCovariance(cov);
    ASSERT_TRUE(conic.has_value());
    const float power = conic->power(vne::math::Vec2f(2.0f, 0.0f));

    EXPECT_NEAR(vne::gs::Gaussian2D::alphaFromPower(0.8f, power), 0.4852f, 1e-4f);
    EXPECT_EQ(vne::gs::Gaussian2D::radiusOf(cov), 6);
}

TEST(Gaussian2D, RotatedEigenvalues) {
    const vne::math::Vec2f eigenvalues = vne::gs::Gaussian2D::eigenvaluesOf(cov2(2.5f, 1.5f, 2.5f));

    EXPECT_NEAR(eigenvalues.x(), 4.0f, 1e-5f);
    EXPECT_NEAR(eigenvalues.y(), 1.0f, 1e-5f);
}

TEST(Gaussian2D, RadiusZeroWhenNonPositiveDefinite) {
    EXPECT_EQ(vne::gs::Gaussian2D::radiusOf(cov2(1.0f, 2.0f, 1.0f)), 0);
}

TEST(Gaussian2D, RadiusZeroWhenNegativeDefinite) {
    EXPECT_EQ(vne::gs::Gaussian2D::radiusOf(cov2(-1.0f, 0.0f, -1.0f)), 0);
}

TEST(Gaussian2D, AlphaIsCapped) {
    EXPECT_NEAR(vne::gs::Gaussian2D::alphaFromPower(1.0f, 0.0f), vne::gs::Gaussian2D::kMaxAlpha, 1e-6f);
}

TEST(Gaussian2D, AlphaBelowCutoffIsSkipped) {
    EXPECT_FLOAT_EQ(vne::gs::Gaussian2D::alphaFromPower(0.003f, 0.0f), 0.0f);
}

TEST(Gaussian2D, AlphaSkippedWhenPowerPositive) {
    EXPECT_FLOAT_EQ(vne::gs::Gaussian2D::alphaFromPower(1.0f, 0.1f), 0.0f);
}

TEST(Gaussian2D, SplatReportsItsOwnFootprintAndAlpha) {
    const vne::gs::Gaussian2D splat(vne::math::Vec2f(10.0f, 10.0f),
                                    cov2(4.0f, 0.0f, 1.0f),
                                    vne::math::Vec3f(1.0f, 0.0f, 0.0f),
                                    0.8f);

    EXPECT_EQ(splat.radius(), 6);
    EXPECT_NEAR(splat.eigenvalues().x(), 4.0f, 1e-5f);
    EXPECT_NEAR(splat.eigenvalues().y(), 1.0f, 1e-5f);

    const std::optional<vne::gs::Conic> conic = splat.conic();
    ASSERT_TRUE(conic.has_value());

    // Pixel 2 px right of the mean is 1 sigma along x: alpha = 0.8 * exp(-0.5).
    EXPECT_NEAR(splat.alphaAt(*conic, vne::math::Vec2f(12.0f, 10.0f)), 0.4852f, 1e-4f);
    // At the mean the falloff is 1, so alpha is the opacity itself.
    EXPECT_NEAR(splat.alphaAt(*conic, splat.mean()), 0.8f, 1e-5f);
}

TEST(Gaussian2D, DegenerateSplatHasNoConic) {
    const vne::gs::Gaussian2D splat(vne::math::Vec2f(0.0f, 0.0f),
                                    cov2(1.0f, 2.0f, 1.0f),
                                    vne::math::Vec3f(1.0f, 1.0f, 1.0f),
                                    1.0f);

    EXPECT_FALSE(splat.conic().has_value());
    EXPECT_EQ(splat.radius(), 0);
}

TEST(Gaussian2D, SettersRoundTrip) {
    vne::gs::Gaussian2D splat;
    splat.setMean(vne::math::Vec2f(3.0f, 4.0f));
    splat.setCovariance(cov2(2.0f, 0.0f, 2.0f));
    splat.setColor(vne::math::Vec3f(0.1f, 0.2f, 0.3f));
    splat.setOpacity(0.5f);

    EXPECT_FLOAT_EQ(splat.mean().x(), 3.0f);
    EXPECT_FLOAT_EQ(splat.mean().y(), 4.0f);
    EXPECT_FLOAT_EQ(splat.covariance()[0][0], 2.0f);
    EXPECT_FLOAT_EQ(splat.color().y(), 0.2f);
    EXPECT_FLOAT_EQ(splat.opacity(), 0.5f);
}
