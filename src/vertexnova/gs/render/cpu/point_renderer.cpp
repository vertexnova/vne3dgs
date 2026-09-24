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

#include "vertexnova/gs/render/cpu/point_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace vne::gs {

ImageRGBf renderPoints(const GaussianCloud& cloud,
                       const Camera& camera,
                       const math::Vec3f& background,
                       std::uint32_t point_radius) {
    const Intrinsics& intrinsics = camera.intrinsics();
    if (!intrinsics.isValid()) {
        return {};
    }

    ImageRGBf image(intrinsics.width(), intrinsics.height(), background);

    const std::size_t pixel_count = image.pixelCount();
    std::vector<float> depth(pixel_count, std::numeric_limits<float>::infinity());

    // Wider signed type so width + radius / cx + radius cannot overflow int.
    const std::int64_t width = static_cast<std::int64_t>(image.width());
    const std::int64_t height = static_cast<std::int64_t>(image.height());
    // Cap the stamp so a hostile radius cannot dominate the frame walk.
    constexpr std::uint32_t kMaxPointRadius = 1024u;
    const std::int64_t radius = static_cast<std::int64_t>(std::min(point_radius, kMaxPointRadius));

    const std::span<const math::Vec3f> positions = cloud.positions();
    for (std::size_t i = 0; i < positions.size(); ++i) {
        // One transform per Gaussian: project() hands back the depth the
        // z-buffer needs instead of making us re-apply the extrinsics.
        const std::optional<ProjectedPoint> projected = camera.project(positions[i]);
        if (!projected.has_value()) {
            continue;
        }

        // Round to the pixel covering [i, i+1). Float compare first so a huge
        // near-plane-grazing coordinate does not overflow the cast.
        const float px = std::floor(projected->pixel.x());
        const float py = std::floor(projected->pixel.y());
        if (!(px >= static_cast<float>(-radius) && px < static_cast<float>(width + radius)
              && py >= static_cast<float>(-radius) && py < static_cast<float>(height + radius))) {
            continue;
        }

        const std::int64_t cx = static_cast<std::int64_t>(px);
        const std::int64_t cy = static_cast<std::int64_t>(py);
        const math::Vec3f color = cloud.dcColor(i);
        const float z = projected->depth;

        const std::int64_t x0 = std::max<std::int64_t>(0, cx - radius);
        const std::int64_t y0 = std::max<std::int64_t>(0, cy - radius);
        const std::int64_t x1 = std::min(width - 1, cx + radius);
        const std::int64_t y1 = std::min(height - 1, cy + radius);

        for (std::int64_t y = y0; y <= y1; ++y) {
            for (std::int64_t x = x0; x <= x1; ++x) {
                const std::size_t index =
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
                if (!(z < depth[index])) {
                    continue;
                }
                depth[index] = z;
                image.setPixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), color);
            }
        }
    }

    return image;
}

}  // namespace vne::gs
