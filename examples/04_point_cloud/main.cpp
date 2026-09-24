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

#include "vertexnova/gs/camera/camera.h"
#include "vertexnova/gs/core/gaussian_cloud.h"
#include "vertexnova/gs/io/ply_reader.h"
#include "vertexnova/gs/render/cpu/point_renderer.h"
#include "vertexnova/gs/render/image.h"
#include "vertexnova/gs/version.h"
#include "vertexnova/io/image/image.h"
#include "vertexnova/math/core/types.h"
#include "config.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace {

[[nodiscard]] std::filesystem::path defaultPlyPath() {
    return std::filesystem::path(VNE_ROOT_DIR) / "testdata" / "three_gaussians.ply";
}

[[nodiscard]] bool parseUp(const std::string& token, vne::math::Vec3f& out) {
    if (token == "+y" || token == "y" || token == "+Y" || token == "Y") {
        out = vne::math::Vec3f(0.0f, 1.0f, 0.0f);
        return true;
    }
    if (token == "-y" || token == "-Y") {
        out = vne::math::Vec3f(0.0f, -1.0f, 0.0f);
        return true;
    }
    if (token == "+z" || token == "z" || token == "+Z" || token == "Z") {
        out = vne::math::Vec3f(0.0f, 0.0f, 1.0f);
        return true;
    }
    if (token == "-z" || token == "-Z") {
        out = vne::math::Vec3f(0.0f, 0.0f, -1.0f);
        return true;
    }
    return false;
}

[[nodiscard]] vne::math::Vec3f medianPosition(const vne::gs::GaussianCloud& cloud) {
    if (cloud.size() == 0) {
        return vne::math::Vec3f(0.0f, 0.0f, 0.0f);
    }
    std::vector<float> xs;
    std::vector<float> ys;
    std::vector<float> zs;
    xs.reserve(cloud.size());
    ys.reserve(cloud.size());
    zs.reserve(cloud.size());
    for (const auto& p : cloud.positions()) {
        xs.push_back(p.x());
        ys.push_back(p.y());
        zs.push_back(p.z());
    }
    auto median = [](std::vector<float>& v) {
        std::sort(v.begin(), v.end());
        return v[v.size() / 2];
    };
    return vne::math::Vec3f(median(xs), median(ys), median(zs));
}

[[nodiscard]] float defaultRadius(const vne::gs::GaussianCloud& cloud, const vne::math::Vec3f& center) {
    if (cloud.size() == 0) {
        return 5.0f;
    }
    std::vector<float> dists;
    dists.reserve(cloud.size());
    for (const auto& p : cloud.positions()) {
        dists.push_back((p - center).length());
    }
    std::sort(dists.begin(), dists.end());
    const float p99 = dists[static_cast<std::size_t>(0.99f * static_cast<float>(dists.size() - 1))];
    return std::max(1.0f, 1.5f * p99);
}

void printUsage(const char* argv0) {
    VNE_LOG_INFO << "Usage: " << argv0
                 << " <file.ply> [--up +y|-y|+z|-z] [--az deg] [--el deg] [--radius r] "
                    "[--width W] [--height H] [--out points.png]";
}

}  // namespace

int main(int argc, char** argv) {
    vne::gs::examples::LoggingGuard logging_guard;

    std::string ply_path;
    vne::math::Vec3f up(0.0f, 1.0f, 0.0f);
    float az_deg = 0.0f;
    float el_deg = 15.0f;
    float radius = -1.0f;
    std::uint32_t width = 800;
    std::uint32_t height = 600;
    std::string out_path = "points.png";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                VNE_LOG_ERROR << "Missing value for " << name;
                return nullptr;
            }
            return argv[++i];
        };
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "--up") {
            const char* v = need("--up");
            if (!v || !parseUp(v, up)) {
                VNE_LOG_ERROR << "Invalid --up (use +y|-y|+z|-z)";
                return 1;
            }
        } else if (arg == "--az") {
            const char* v = need("--az");
            if (!v) {
                return 1;
            }
            az_deg = std::strtof(v, nullptr);
        } else if (arg == "--el") {
            const char* v = need("--el");
            if (!v) {
                return 1;
            }
            el_deg = std::strtof(v, nullptr);
        } else if (arg == "--radius") {
            const char* v = need("--radius");
            if (!v) {
                return 1;
            }
            radius = std::strtof(v, nullptr);
        } else if (arg == "--width") {
            const char* v = need("--width");
            if (!v) {
                return 1;
            }
            width = static_cast<std::uint32_t>(std::strtoul(v, nullptr, 10));
        } else if (arg == "--height") {
            const char* v = need("--height");
            if (!v) {
                return 1;
            }
            height = static_cast<std::uint32_t>(std::strtoul(v, nullptr, 10));
        } else if (arg == "--out") {
            const char* v = need("--out");
            if (!v) {
                return 1;
            }
            out_path = v;
        } else if (!arg.empty() && arg[0] == '-') {
            VNE_LOG_ERROR << "Unknown flag: " << arg;
            printUsage(argv[0]);
            return 1;
        } else if (ply_path.empty()) {
            ply_path = arg;
        } else {
            VNE_LOG_ERROR << "Unexpected argument: " << arg;
            printUsage(argv[0]);
            return 1;
        }
    }

    if (ply_path.empty()) {
        ply_path = defaultPlyPath().string();
        VNE_LOG_INFO << "vne3dgs version: " << vne::gs::getVersion();
        VNE_LOG_INFO << "No PLY path given; using default fixture: " << ply_path;
    }

    vne::gs::GaussianCloud cloud;
    std::string error;
    if (!vne::gs::readGaussianPly(ply_path, cloud, &error)) {
        VNE_LOG_ERROR << "Failed to load " << ply_path << ": " << error;
        return 1;
    }

    const vne::math::Vec3f center = medianPosition(cloud);
    if (radius <= 0.0f) {
        radius = defaultRadius(cloud, center);
    }

    const vne::gs::Intrinsics K = vne::gs::Intrinsics::fromFovY(vne::math::degToRad(60.0f), width, height);
    const vne::gs::Camera cam =
        vne::gs::Camera::orbit(center, radius, vne::math::degToRad(az_deg), vne::math::degToRad(el_deg), up, K);

    VNE_LOG_INFO << "gaussians: " << cloud.size();
    VNE_LOG_INFO << "center: (" << center.x() << ", " << center.y() << ", " << center.z() << ")";
    VNE_LOG_INFO << "eye: (" << cam.position().x() << ", " << cam.position().y() << ", " << cam.position().z() << ")";
    VNE_LOG_INFO << "radius: " << radius << " az_deg: " << az_deg << " el_deg: " << el_deg;

    const vne::gs::ImageRGBf image = vne::gs::renderPoints(cloud, cam, vne::math::Vec3f(0.0f, 0.0f, 0.0f));
    const std::vector<std::uint8_t> rgba = vne::gs::image_utils::toRGBA8(image);
    if (!vne::image::image_utils::saveImage(out_path,
                                            rgba.data(),
                                            static_cast<int>(image.width()),
                                            static_cast<int>(image.height()),
                                            4)) {
        VNE_LOG_ERROR << "Failed to write " << out_path;
        return 1;
    }
    VNE_LOG_INFO << "Wrote " << out_path;
    return 0;
}
