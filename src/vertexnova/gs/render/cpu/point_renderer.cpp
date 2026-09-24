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

    const std::span<const math::Vec3f> positions = cloud.positions();
    const int width = static_cast<int>(image.width());
    const int height = static_cast<int>(image.height());
    const int radius = static_cast<int>(point_radius);

    for (std::size_t i = 0; i < positions.size(); ++i) {
        // One transform per Gaussian: project() hands back the depth the
        // z-buffer needs instead of making us re-apply the extrinsics.
        const std::optional<ProjectedPoint> projected = camera.project(positions[i]);
        if (!projected.has_value()) {
            continue;
        }

        // Round to the nearest pixel covering [i, i+1). Compare as floats first:
        // a huge coordinate from a near-plane-grazing point would overflow int.
        const float px = std::floor(projected->pixel.x());
        const float py = std::floor(projected->pixel.y());
        if (!(px >= -static_cast<float>(radius) && px < static_cast<float>(width + radius) &&
              py >= -static_cast<float>(radius) && py < static_cast<float>(height + radius))) {
            continue;
        }

        const int cx = static_cast<int>(px);
        const int cy = static_cast<int>(py);
        const math::Vec3f color = cloud.dcColor(i);
        const float z = projected->depth;

        const int x0 = std::max(0, cx - radius);
        const int y0 = std::max(0, cy - radius);
        const int x1 = std::min(width - 1, cx + radius);
        const int y1 = std::min(height - 1, cy + radius);

        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                const std::size_t index =
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                    static_cast<std::size_t>(x);
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
