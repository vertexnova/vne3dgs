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

#include "vertexnova/gs/core/front_to_back_blender.h"

#include <gtest/gtest.h>

TEST(FrontToBackBlender, BlendRedThenGreen) {
    const vne::math::Vec3f red(1.0f, 0.0f, 0.0f);
    const vne::math::Vec3f green(0.0f, 1.0f, 0.0f);
    const vne::math::Vec3f black(0.0f, 0.0f, 0.0f);
    vne::gs::FrontToBackBlender compositor;

    ASSERT_TRUE(compositor.composite(red, 0.5f));
    ASSERT_TRUE(compositor.composite(green, 0.8f));
    const vne::math::Vec3f radiance = compositor.resolve(black);

    EXPECT_NEAR(radiance.x(), 0.5f, 1e-5f);
    EXPECT_NEAR(radiance.y(), 0.4f, 1e-5f);
    EXPECT_NEAR(radiance.z(), 0.0f, 1e-5f);
    EXPECT_NEAR(compositor.transmittance(), 0.1f, 1e-5f);
}

TEST(FrontToBackBlender, BlendGreenThenRed) {
    const vne::math::Vec3f red(1.0f, 0.0f, 0.0f);
    const vne::math::Vec3f green(0.0f, 1.0f, 0.0f);
    const vne::math::Vec3f black(0.0f, 0.0f, 0.0f);
    vne::gs::FrontToBackBlender compositor;

    ASSERT_TRUE(compositor.composite(green, 0.8f));
    ASSERT_TRUE(compositor.composite(red, 0.5f));
    const vne::math::Vec3f radiance = compositor.resolve(black);

    EXPECT_NEAR(radiance.x(), 0.1f, 1e-5f);
    EXPECT_NEAR(radiance.y(), 0.8f, 1e-5f);
    EXPECT_NEAR(radiance.z(), 0.0f, 1e-5f);
    EXPECT_NEAR(compositor.transmittance(), 0.1f, 1e-5f);
}

TEST(FrontToBackBlender, EarlyTerminationDropsSaturatingSplat) {
    const vne::math::Vec3f white(1.0f, 1.0f, 1.0f);
    const vne::math::Vec3f black(0.0f, 0.0f, 0.0f);
    vne::gs::FrontToBackBlender compositor;

    int composited = 0;
    for (int i = 0; i < 20; ++i) {
        if (compositor.composite(white, 0.95f)) {
            ++composited;
        }
    }
    const vne::math::Vec3f before = compositor.resolve(black);
    const bool accepted_fourth = compositor.composite(white, 0.95f);
    const vne::math::Vec3f after = compositor.resolve(black);

    EXPECT_EQ(composited, 3);
    EXPECT_NEAR(compositor.transmittance(), 1.25e-4f, 1e-8f);
    EXPECT_FALSE(accepted_fourth);
    EXPECT_FLOAT_EQ(before.x(), after.x());
    EXPECT_FLOAT_EQ(before.y(), after.y());
    EXPECT_FLOAT_EQ(before.z(), after.z());
}

TEST(FrontToBackBlender, SaturationRejectsLaterFaintSplat) {
    const vne::math::Vec3f white(1.0f, 1.0f, 1.0f);
    const vne::math::Vec3f black(0.0f, 0.0f, 0.0f);
    vne::gs::FrontToBackBlender compositor;

    ASSERT_TRUE(compositor.composite(white, 0.95f));
    ASSERT_TRUE(compositor.composite(white, 0.95f));
    ASSERT_TRUE(compositor.composite(white, 0.95f));
    ASSERT_FALSE(compositor.composite(white, 0.95f));
    const vne::math::Vec3f before = compositor.resolve(black);
    const bool accepted_faint = compositor.composite(white, 0.1f);
    const vne::math::Vec3f after = compositor.resolve(black);

    EXPECT_FALSE(accepted_faint);
    EXPECT_NEAR(compositor.transmittance(), 1.25e-4f, 1e-8f);
    EXPECT_FLOAT_EQ(before.x(), after.x());
    EXPECT_FLOAT_EQ(before.y(), after.y());
    EXPECT_FLOAT_EQ(before.z(), after.z());
}
