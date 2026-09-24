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

#include "vertexnova/gs/render/image.h"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

TEST(ImageRGBf, DefaultIsEmpty) {
    const vne::gs::ImageRGBf image;

    EXPECT_TRUE(image.isEmpty());
    EXPECT_EQ(image.width(), 0u);
    EXPECT_EQ(image.height(), 0u);
    EXPECT_EQ(image.pixelCount(), 0u);
    EXPECT_TRUE(image.data().empty());
}

TEST(ImageRGBf, BufferAlwaysMatchesTheDimensions) {
    const vne::gs::ImageRGBf image(4, 3);

    EXPECT_FALSE(image.isEmpty());
    EXPECT_EQ(image.pixelCount(), 12u);
    EXPECT_EQ(image.data().size(), 12u * vne::gs::ImageRGBf::kChannels);
}

TEST(ImageRGBf, FillConstructorSetsEveryPixel) {
    const vne::gs::ImageRGBf image(2, 2, vne::math::Vec3f(0.25f, 0.5f, 0.75f));

    for (std::uint32_t y = 0; y < 2; ++y) {
        for (std::uint32_t x = 0; x < 2; ++x) {
            const vne::math::Vec3f pixel = image.pixel(x, y);
            EXPECT_FLOAT_EQ(pixel.x(), 0.25f);
            EXPECT_FLOAT_EQ(pixel.y(), 0.5f);
            EXPECT_FLOAT_EQ(pixel.z(), 0.75f);
        }
    }
}

TEST(ImageRGBf, PixelAccessIsRowMajorAndBoundsChecked) {
    vne::gs::ImageRGBf image(3, 2);
    image.setPixel(2, 1, vne::math::Vec3f(1.0f, 2.0f, 3.0f));

    EXPECT_FLOAT_EQ(image.pixel(2, 1).y(), 2.0f);
    // Row-major: pixel (2,1) is the sixth pixel.
    EXPECT_FLOAT_EQ(image.data()[5u * 3u + 1u], 2.0f);

    // Out of range reads black and writes nothing.
    EXPECT_FLOAT_EQ(image.pixel(3, 1).x(), 0.0f);
    EXPECT_FLOAT_EQ(image.pixel(0, 2).x(), 0.0f);
    image.setPixel(99, 99, vne::math::Vec3f(9.0f, 9.0f, 9.0f));
    EXPECT_EQ(image.data().size(), 6u * 3u);
}

TEST(ImageRGBf, ResizeReallocatesAndClears) {
    vne::gs::ImageRGBf image(2, 2, vne::math::Vec3f(1.0f, 1.0f, 1.0f));
    image.resize(1, 5);

    EXPECT_EQ(image.width(), 1u);
    EXPECT_EQ(image.height(), 5u);
    EXPECT_EQ(image.data().size(), 5u * 3u);
    EXPECT_FLOAT_EQ(image.pixel(0, 0).x(), 0.0f);
}

TEST(ImageUtils, ToRGBA8ClampsAndQuantizes) {
    vne::gs::ImageRGBf image(2, 1);
    image.setPixel(0, 0, vne::math::Vec3f(0.0f, 0.5f, 1.0f));
    // Out-of-range radiance must clamp, not wrap around.
    image.setPixel(1, 0, vne::math::Vec3f(-1.0f, 2.0f, 0.25f));

    const std::vector<std::uint8_t> rgba = vne::gs::image_utils::toRGBA8(image);

    ASSERT_EQ(rgba.size(), 2u * 4u);
    EXPECT_EQ(rgba[0], 0u);
    EXPECT_EQ(rgba[1], 128u);
    EXPECT_EQ(rgba[2], 255u);
    EXPECT_EQ(rgba[3], 255u);
    EXPECT_EQ(rgba[4], 0u);
    EXPECT_EQ(rgba[5], 255u);
    EXPECT_EQ(rgba[6], 64u);
    EXPECT_EQ(rgba[7], 255u);
}

TEST(ImageUtils, ToRGB8HasNoAlphaChannel) {
    vne::gs::ImageRGBf image(2, 1, vne::math::Vec3f(1.0f, 0.0f, 0.5f));

    const std::vector<std::uint8_t> rgb = vne::gs::image_utils::toRGB8(image);

    ASSERT_EQ(rgb.size(), 2u * 3u);
    EXPECT_EQ(rgb[0], 255u);
    EXPECT_EQ(rgb[1], 0u);
    EXPECT_EQ(rgb[2], 128u);
}

TEST(ImageUtils, ConversionsHandleAnEmptyImage) {
    const vne::gs::ImageRGBf image;

    EXPECT_TRUE(vne::gs::image_utils::toRGBA8(image).empty());
    EXPECT_TRUE(vne::gs::image_utils::toRGB8(image).empty());
}
