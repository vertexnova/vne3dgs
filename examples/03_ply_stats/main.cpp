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

#include "vertexnova/gs/core/gaussian_cloud.h"
#include "vertexnova/gs/io/ply_reader.h"
#include "vertexnova/gs/version.h"
#include "config.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace {

[[nodiscard]] float percentile(std::vector<float> values, float fraction) {
    if (values.empty()) {
        return 0.0f;
    }
    std::sort(values.begin(), values.end());
    const float index = fraction * static_cast<float>(values.size() - 1);
    const std::size_t lo = static_cast<std::size_t>(index);
    const std::size_t hi = std::min(lo + 1, values.size() - 1);
    const float t = index - static_cast<float>(lo);
    return values[lo] * (1.0f - t) + values[hi] * t;
}

[[nodiscard]] float median(std::vector<float> values) {
    return percentile(std::move(values), 0.5f);
}

[[nodiscard]] std::filesystem::path defaultPlyPath() {
    return std::filesystem::path(VNE_ROOT_DIR) / "testdata" / "three_gaussians.ply";
}

}  // namespace

int main(int argc, char** argv) {
    vne::gs::examples::LoggingGuard logging_guard;

    std::string path;
    if (argc >= 2) {
        path = argv[1];
    } else {
        path = defaultPlyPath().string();
        VNE_LOG_INFO << "vne3dgs version: " << vne::gs::getVersion();
        VNE_LOG_INFO << "No PLY path given; using default fixture: " << path;
        VNE_LOG_INFO << "Pass a path to load another scene: " << argv[0] << " <file.ply>";
    }

    vne::gs::GaussianCloud cloud;
    std::string error;

    const auto start = std::chrono::steady_clock::now();
    if (!vne::gs::readGaussianPly(path, cloud, &error)) {
        VNE_LOG_ERROR << "Failed to load " << path << ": " << error;
        return 1;
    }
    const auto elapsed_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

    VNE_LOG_INFO << "file: " << path;
    VNE_LOG_INFO << "gaussians: " << cloud.size();
    VNE_LOG_INFO << "sh_degree: " << cloud.shDegree();
    VNE_LOG_INFO << "load_ms: " << elapsed_ms;

    if (cloud.size() == 0) {
        return 0;
    }

    std::vector<float> xs;
    std::vector<float> ys;
    std::vector<float> zs;
    std::vector<float> opacities = cloud.opacities();
    std::vector<float> log_scales;
    xs.reserve(cloud.size());
    ys.reserve(cloud.size());
    zs.reserve(cloud.size());
    log_scales.reserve(cloud.size() * 3);

    float min_x = cloud.positions()[0].x();
    float max_x = min_x;
    float min_y = cloud.positions()[0].y();
    float max_y = min_y;
    float min_z = cloud.positions()[0].z();
    float max_z = min_z;

    for (const vne::math::Vec3f& p : cloud.positions()) {
        xs.push_back(p.x());
        ys.push_back(p.y());
        zs.push_back(p.z());
        min_x = std::min(min_x, p.x());
        max_x = std::max(max_x, p.x());
        min_y = std::min(min_y, p.y());
        max_y = std::max(max_y, p.y());
        min_z = std::min(min_z, p.z());
        max_z = std::max(max_z, p.z());
    }
    for (const vne::math::Vec3f& s : cloud.scales()) {
        log_scales.push_back(std::log(std::max(s.x(), 1e-8f)));
        log_scales.push_back(std::log(std::max(s.y(), 1e-8f)));
        log_scales.push_back(std::log(std::max(s.z(), 1e-8f)));
    }

    VNE_LOG_INFO << "bounds_min: " << min_x << " " << min_y << " " << min_z;
    VNE_LOG_INFO << "bounds_max: " << max_x << " " << max_y << " " << max_z;
    VNE_LOG_INFO << "p01: " << percentile(xs, 0.01f) << " " << percentile(ys, 0.01f) << " " << percentile(zs, 0.01f);
    VNE_LOG_INFO << "p99: " << percentile(xs, 0.99f) << " " << percentile(ys, 0.99f) << " " << percentile(zs, 0.99f);
    VNE_LOG_INFO << "median: " << median(xs) << " " << median(ys) << " " << median(zs);

    constexpr int kBins = 10;
    std::array<std::size_t, kBins> hist{};
    for (float opacity : opacities) {
        int bin = static_cast<int>(opacity * static_cast<float>(kBins));
        bin = std::clamp(bin, 0, kBins - 1);
        ++hist[static_cast<std::size_t>(bin)];
    }
    std::ostringstream histogram;
    histogram << "opacity_histogram:";
    for (std::size_t count : hist) {
        histogram << " " << count;
    }
    VNE_LOG_INFO << histogram.str();

    double log_sum = 0.0;
    double log_sum_sq = 0.0;
    for (float value : log_scales) {
        log_sum += static_cast<double>(value);
        log_sum_sq += static_cast<double>(value) * static_cast<double>(value);
    }
    const double n = static_cast<double>(log_scales.size());
    const double mean = log_sum / n;
    const double variance = std::max(0.0, log_sum_sq / n - mean * mean);
    VNE_LOG_INFO << "log_scale_mean: " << mean;
    VNE_LOG_INFO << "log_scale_std: " << std::sqrt(variance);
    VNE_LOG_INFO << "log_scale_p01: " << percentile(log_scales, 0.01f);
    VNE_LOG_INFO << "log_scale_p99: " << percentile(log_scales, 0.99f);

    return 0;
}
