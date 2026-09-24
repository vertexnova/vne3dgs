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

#include "vertexnova/gs/core/gaussian_cloud.h"

#include <cstddef>

#include <gtest/gtest.h>

TEST(GaussianCloud, StartsEmptyAndConsistent) {
    const vne::gs::GaussianCloud cloud;

    EXPECT_EQ(cloud.size(), 0u);
    EXPECT_TRUE(cloud.isEmpty());
    EXPECT_EQ(cloud.shDegree(), 0);
    EXPECT_EQ(cloud.shCoeffCount(), 1);
    EXPECT_TRUE(cloud.isConsistent());
}

TEST(GaussianCloud, ResizeKeepsEveryColumnInLockstep) {
    vne::gs::GaussianCloud cloud;
    cloud.setShDegree(3);
    cloud.resize(7);

    EXPECT_EQ(cloud.size(), 7u);
    EXPECT_EQ(cloud.positions().size(), 7u);
    EXPECT_EQ(cloud.scales().size(), 7u);
    EXPECT_EQ(cloud.rotations().size(), 7u);
    EXPECT_EQ(cloud.opacities().size(), 7u);
    EXPECT_EQ(cloud.sh().size(), 7u * 16u * 3u);
    EXPECT_TRUE(cloud.isConsistent());
}

TEST(GaussianCloud, ShrinkingKeepsTheSurvivingPrefix) {
    vne::gs::GaussianCloud cloud;
    cloud.setShDegree(1);
    cloud.resize(4);
    cloud.positions()[1] = vne::math::Vec3f(1.0f, 2.0f, 3.0f);
    cloud.setShCoefficient(1, 2, vne::math::Vec3f(0.5f, 0.6f, 0.7f));

    cloud.resize(2);

    EXPECT_EQ(cloud.size(), 2u);
    EXPECT_TRUE(cloud.isConsistent());
    EXPECT_FLOAT_EQ(cloud.positions()[1].y(), 2.0f);
    EXPECT_FLOAT_EQ(cloud.shCoefficient(1, 2).y(), 0.6f);
}

TEST(GaussianCloud, SetShDegreeReshapesTheCoefficientArray) {
    vne::gs::GaussianCloud cloud;
    cloud.resize(3);
    EXPECT_EQ(cloud.sh().size(), 3u * 1u * 3u);

    cloud.setShDegree(2);

    EXPECT_EQ(cloud.shDegree(), 2);
    EXPECT_EQ(cloud.shCoeffCount(), 9);
    EXPECT_EQ(cloud.sh().size(), 3u * 9u * 3u);
    EXPECT_TRUE(cloud.isConsistent());
}

TEST(GaussianCloud, ShDegreeIsClampedToTheFormatRange) {
    vne::gs::GaussianCloud cloud;
    cloud.resize(2);

    cloud.setShDegree(9);
    EXPECT_EQ(cloud.shDegree(), vne::gs::GaussianCloud::kMaxShDegree);
    EXPECT_TRUE(cloud.isConsistent());

    cloud.setShDegree(-4);
    EXPECT_EQ(cloud.shDegree(), 0);
    EXPECT_TRUE(cloud.isConsistent());

    EXPECT_EQ(vne::gs::shCoeffCount(-1), 1);
    EXPECT_EQ(vne::gs::shCoeffCount(99), 16);
}

TEST(GaussianCloud, ShCoefficientAccessIsBoundsChecked) {
    vne::gs::GaussianCloud cloud;
    cloud.setShDegree(1);
    cloud.resize(2);

    cloud.setShCoefficient(0, 3, vne::math::Vec3f(1.0f, 2.0f, 3.0f));
    EXPECT_FLOAT_EQ(cloud.shCoefficient(0, 3).z(), 3.0f);

    // Out of range in either index is ignored on write and zero on read.
    cloud.setShCoefficient(5, 0, vne::math::Vec3f(9.0f, 9.0f, 9.0f));
    cloud.setShCoefficient(0, 99, vne::math::Vec3f(9.0f, 9.0f, 9.0f));
    EXPECT_FLOAT_EQ(cloud.shCoefficient(5, 0).x(), 0.0f);
    EXPECT_FLOAT_EQ(cloud.shCoefficient(0, 99).x(), 0.0f);
    EXPECT_FLOAT_EQ(cloud.shCoefficient(0, -1).x(), 0.0f);
    EXPECT_TRUE(cloud.isConsistent());
}

TEST(GaussianCloud, DcColorAppliesTheDegreeZeroConstant) {
    constexpr float kShC0 = 0.28209479177387814f;
    vne::gs::GaussianCloud cloud;
    cloud.resize(1);
    cloud.setShCoefficient(0, 0, vne::math::Vec3f(1.0f, 0.0f, -10.0f));

    const vne::math::Vec3f color = cloud.dcColor(0);

    EXPECT_NEAR(color.x(), 0.5f + kShC0, 1e-6f);
    EXPECT_NEAR(color.y(), 0.5f, 1e-6f);
    // Strongly negative DC clamps at zero rather than going negative.
    EXPECT_FLOAT_EQ(color.z(), 0.0f);
    // Out of range is black, not a read past the end.
    EXPECT_FLOAT_EQ(cloud.dcColor(42).x(), 0.0f);
}

TEST(GaussianCloud, GaussianGathersOneSplat) {
    vne::gs::GaussianCloud cloud;
    cloud.resize(2);
    cloud.positions()[1] = vne::math::Vec3f(1.0f, 2.0f, 3.0f);
    cloud.scales()[1] = vne::math::Vec3f(2.0f, 1.0f, 0.5f);
    cloud.rotations()[1] = vne::math::Quatf::identity();
    cloud.opacities()[1] = 0.25f;

    const vne::gs::Gaussian3D splat = cloud.gaussian(1);

    EXPECT_FLOAT_EQ(splat.position().z(), 3.0f);
    EXPECT_FLOAT_EQ(splat.scale().x(), 2.0f);
    EXPECT_FLOAT_EQ(splat.opacity(), 0.25f);
    EXPECT_FLOAT_EQ(splat.color().x(), cloud.dcColor(1).x());

    // Out of range yields a default splat instead of reading past the end.
    EXPECT_FLOAT_EQ(cloud.gaussian(99).position().x(), 0.0f);
}

TEST(GaussianCloud, ClearResetsDegreeAndColumns) {
    vne::gs::GaussianCloud cloud;
    cloud.setShDegree(3);
    cloud.resize(5);

    cloud.clear();

    EXPECT_EQ(cloud.size(), 0u);
    EXPECT_EQ(cloud.shDegree(), 0);
    EXPECT_TRUE(cloud.sh().empty());
    EXPECT_TRUE(cloud.isConsistent());
}
