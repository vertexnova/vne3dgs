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

#include "common/logging_guard.h"

#include "vertexnova/gs/core/conic.h"
#include "vertexnova/gs/core/front_to_back_blender.h"
#include "vertexnova/gs/core/gaussian2d.h"
#include "vertexnova/gs/render/image.h"
#include "vertexnova/io/image/image.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {

constexpr int kWidth = 256;
constexpr int kHeight = 256;
constexpr int kChannels = 3;
constexpr float kPixelCenterOffset = 0.5f;

[[nodiscard]] vne::math::Mat2f covarianceFromAxes(float sigma1, float sigma2, float theta_rad) {
    const float cos_t = std::cos(theta_rad);
    const float sin_t = std::sin(theta_rad);
    const float s1 = sigma1 * sigma1;
    const float s2 = sigma2 * sigma2;
    const float a = cos_t * cos_t * s1 + sin_t * sin_t * s2;
    const float b = cos_t * sin_t * (s1 - s2);
    const float c = sin_t * sin_t * s1 + cos_t * cos_t * s2;
    return {vne::math::Vec2f(a, b), vne::math::Vec2f(b, c)};
}

void splatInto(std::vector<vne::gs::FrontToBackBlender>& pixels, const vne::gs::Gaussian2D& gaussian) {
    // Invert the covariance once per splat, then evaluate over its footprint.
    const std::optional<vne::gs::Conic> conic = gaussian.conic();
    if (!conic.has_value()) {
        return;
    }
    const float radius = static_cast<float>(gaussian.radius());
    const int x0 = std::max(0, static_cast<int>(std::floor(gaussian.mean().x() - radius)));
    const int y0 = std::max(0, static_cast<int>(std::floor(gaussian.mean().y() - radius)));
    const int x1 = std::min(kWidth, static_cast<int>(std::ceil(gaussian.mean().x() + radius)));
    const int y1 = std::min(kHeight, static_cast<int>(std::ceil(gaussian.mean().y() + radius)));

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const vne::math::Vec2f pixel(static_cast<float>(x) + kPixelCenterOffset,
                                         static_cast<float>(y) + kPixelCenterOffset);
            const float alpha = gaussian.alphaAt(*conic, pixel);
            if (alpha <= 0.0f) {
                continue;
            }
            pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(kWidth) + static_cast<std::size_t>(x)]
                .composite(gaussian.color(), alpha);
        }
    }
}

[[nodiscard]] std::vector<std::uint8_t> render(const std::vector<vne::gs::Gaussian2D>& splats) {
    std::vector<vne::gs::FrontToBackBlender> pixels(static_cast<std::size_t>(kWidth * kHeight));
    for (const vne::gs::Gaussian2D& splat : splats) {
        splatInto(pixels, splat);
    }

    const vne::math::Vec3f background(0.0f, 0.0f, 0.0f);
    vne::gs::ImageRGBf image(static_cast<std::uint32_t>(kWidth), static_cast<std::uint32_t>(kHeight));
    for (std::uint32_t y = 0; y < image.height(); ++y) {
        for (std::uint32_t x = 0; x < image.width(); ++x) {
            const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(kWidth) + x;
            image.setPixel(x, y, pixels[index].resolve(background));
        }
    }
    return vne::gs::image_utils::toRGB8(image);
}

bool writePng(const std::string& path, const std::vector<std::uint8_t>& rgb) {
    return vne::image::image_utils::saveImage(path, rgb.data(), kWidth, kHeight, kChannels);
}

}  // namespace

int main(int argc, char** argv) {
    vne::gs::examples::LoggingGuard logging_guard;

    const std::string out_dir = (argc > 1) ? argv[1] : ".";
    const std::string forward_path = out_dir + "/gaussians_2d.png";
    const std::string reversed_path = out_dir + "/gaussians_2d_reversed.png";

    // Five hand-placed ellipses. 0 and 1 overlap (red over green); 3 and 4 overlap
    // (yellow over magenta). 1 and 4 are rotated. Pixel centers at i+0.5.
    std::vector<vne::gs::Gaussian2D> splats = {
        {vne::math::Vec2f(80.0f, 90.0f),
         covarianceFromAxes(40.0f, 16.0f, 0.0f),
         vne::math::Vec3f(1.0f, 0.15f, 0.10f),
         0.75f},
        {vne::math::Vec2f(115.0f, 105.0f),
         covarianceFromAxes(36.0f, 14.0f, 0.78539816f),
         vne::math::Vec3f(0.15f, 1.0f, 0.20f),
         0.80f},
        {vne::math::Vec2f(196.0f, 78.0f),
         covarianceFromAxes(14.0f, 38.0f, 0.0f),
         vne::math::Vec3f(0.20f, 0.40f, 1.0f),
         0.70f},
        {vne::math::Vec2f(88.0f, 188.0f),
         covarianceFromAxes(22.0f, 22.0f, 0.0f),
         vne::math::Vec3f(1.0f, 0.90f, 0.15f),
         0.80f},
        {vne::math::Vec2f(170.0f, 186.0f),
         covarianceFromAxes(34.0f, 13.0f, -0.5235988f),
         vne::math::Vec3f(0.90f, 0.20f, 0.85f),
         0.75f},
    };

    const std::vector<std::uint8_t> forward = render(splats);
    std::reverse(splats.begin(), splats.end());
    const std::vector<std::uint8_t> reversed = render(splats);

    if (!writePng(forward_path, forward) || !writePng(reversed_path, reversed)) {
        VNE_LOG_ERROR << "Failed to write PNG files";
        return 1;
    }

    VNE_LOG_INFO << "Wrote " << forward_path << " (front-to-back as listed)";
    VNE_LOG_INFO << "Wrote " << reversed_path << " (draw order reversed)";
    VNE_LOG_INFO << "Images differ only where splats overlap.";
    return 0;
}
