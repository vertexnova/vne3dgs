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

#include <algorithm>

namespace vne::gs {

namespace {

/** @brief Clamps to [0, 1] and quantizes to 8 bits, rounding half up. */
[[nodiscard]] std::uint8_t quantize(float value) noexcept {
    return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

}  // namespace

ImageRGBf::ImageRGBf(std::uint32_t width, std::uint32_t height) {
    resize(width, height);
}

ImageRGBf::ImageRGBf(std::uint32_t width, std::uint32_t height, const math::Vec3f& color) {
    resize(width, height);
    fill(color);
}

std::uint32_t ImageRGBf::width() const noexcept {
    return width_;
}

std::uint32_t ImageRGBf::height() const noexcept {
    return height_;
}

std::size_t ImageRGBf::pixelCount() const noexcept {
    return static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
}

bool ImageRGBf::isEmpty() const noexcept {
    return width_ == 0u || height_ == 0u;
}

void ImageRGBf::resize(std::uint32_t width, std::uint32_t height) {
    width_ = width;
    height_ = height;
    rgb_.assign(pixelCount() * kChannels, 0.0f);
}

void ImageRGBf::fill(const math::Vec3f& color) noexcept {
    for (std::size_t i = 0; i < rgb_.size(); i += kChannels) {
        rgb_[i + 0] = color.x();
        rgb_[i + 1] = color.y();
        rgb_[i + 2] = color.z();
    }
}

std::span<const float> ImageRGBf::data() const noexcept {
    return rgb_;
}

std::span<float> ImageRGBf::data() noexcept {
    return rgb_;
}

std::size_t ImageRGBf::offsetOf(std::uint32_t x, std::uint32_t y) const noexcept {
    return (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)) * kChannels;
}

math::Vec3f ImageRGBf::pixel(std::uint32_t x, std::uint32_t y) const noexcept {
    if (x >= width_ || y >= height_) {
        return {};
    }
    const std::size_t offset = offsetOf(x, y);
    return {rgb_[offset + 0], rgb_[offset + 1], rgb_[offset + 2]};
}

void ImageRGBf::setPixel(std::uint32_t x, std::uint32_t y, const math::Vec3f& color) noexcept {
    if (x >= width_ || y >= height_) {
        return;
    }
    const std::size_t offset = offsetOf(x, y);
    rgb_[offset + 0] = color.x();
    rgb_[offset + 1] = color.y();
    rgb_[offset + 2] = color.z();
}

namespace image_utils {

std::vector<std::uint8_t> toRGBA8(const ImageRGBf& image) {
    const std::span<const float> rgb = image.data();
    const std::size_t pixels = image.pixelCount();
    std::vector<std::uint8_t> rgba(pixels * 4u);
    for (std::size_t i = 0; i < pixels; ++i) {
        rgba[i * 4u + 0u] = quantize(rgb[i * ImageRGBf::kChannels + 0u]);
        rgba[i * 4u + 1u] = quantize(rgb[i * ImageRGBf::kChannels + 1u]);
        rgba[i * 4u + 2u] = quantize(rgb[i * ImageRGBf::kChannels + 2u]);
        rgba[i * 4u + 3u] = 255u;
    }
    return rgba;
}

std::vector<std::uint8_t> toRGB8(const ImageRGBf& image) {
    const std::span<const float> rgb = image.data();
    std::vector<std::uint8_t> out(rgb.size());
    for (std::size_t i = 0; i < rgb.size(); ++i) {
        out[i] = quantize(rgb[i]);
    }
    return out;
}

}  // namespace image_utils

}  // namespace vne::gs
