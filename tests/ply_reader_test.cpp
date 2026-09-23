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
#include "vertexnova/gs/io/ply_reader.h"
#include "config.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

namespace {

[[nodiscard]] std::filesystem::path testdataPath(const char* name) {
    return std::filesystem::path(VNE_ROOT_DIR) / "testdata" / name;
}

[[nodiscard]] std::filesystem::path tempPlyPath(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

void writeAsciiHeader(std::ostream& out, int vertex_count, int rest_count, bool include_opacity = true) {
    out << "ply\nformat binary_little_endian 1.0\nelement vertex " << vertex_count << "\n";
    out << "property float x\nproperty float y\nproperty float z\n";
    out << "property float f_dc_0\nproperty float f_dc_1\nproperty float f_dc_2\n";
    for (int i = 0; i < rest_count; ++i) {
        out << "property float f_rest_" << i << "\n";
    }
    if (include_opacity) {
        out << "property float opacity\n";
    }
    out << "property float scale_0\nproperty float scale_1\nproperty float scale_2\n";
    out << "property float rot_0\nproperty float rot_1\nproperty float rot_2\nproperty float rot_3\n";
    out << "end_header\n";
}

[[nodiscard]] vne::gs::GaussianCloud makeCloud(int degree, std::size_t count = 1) {
    vne::gs::GaussianCloud cloud;
    cloud.setShDegree(degree);
    const int coeffs = vne::gs::shCoeffCount(degree);
    cloud.positions().assign(count, vne::math::Vec3f(1.0f, 2.0f, 3.0f));
    cloud.scales().assign(count, vne::math::Vec3f(2.0f, 1.0f, 0.5f));
    cloud.rotations().assign(count, vne::math::Quatf::identity());
    cloud.opacities().assign(count, 0.5f);
    cloud.sh().assign(count * static_cast<std::size_t>(coeffs) * 3u, 0.0f);
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t base = i * static_cast<std::size_t>(coeffs) * 3u;
        cloud.sh()[base + 0] = 0.1f;
        cloud.sh()[base + 1] = -0.2f;
        cloud.sh()[base + 2] = 0.3f;
        for (int k = 1; k < coeffs; ++k) {
            cloud.sh()[base + static_cast<std::size_t>(k) * 3u + 0] = static_cast<float>(k);
            cloud.sh()[base + static_cast<std::size_t>(k) * 3u + 1] = static_cast<float>(k + 10);
            cloud.sh()[base + static_cast<std::size_t>(k) * 3u + 2] = static_cast<float>(k + 20);
        }
    }
    return cloud;
}

}  // namespace

TEST(PlyReader, LoadsThreeGaussiansFixture) {
    vne::gs::GaussianCloud cloud;
    std::string error;
    ASSERT_TRUE(vne::gs::readGaussianPly(testdataPath("three_gaussians.ply").string(), cloud, &error)) << error;

    EXPECT_EQ(cloud.size(), 3u);
    EXPECT_EQ(cloud.shDegree(), 3);
    EXPECT_EQ(vne::gs::shCoeffCount(cloud.shDegree()), 16);

    EXPECT_NEAR(cloud.scales()[0].x(), 2.0f, 1e-5f);
    EXPECT_NEAR(cloud.opacities()[0], 0.5f, 1e-5f);

    // rot = (2,0,0,0) file order -> identity after normalize
    EXPECT_NEAR(cloud.rotations()[0].w, 1.0f, 1e-5f);
    EXPECT_NEAR(cloud.rotations()[0].x, 0.0f, 1e-5f);
    EXPECT_NEAR(cloud.rotations()[0].y, 0.0f, 1e-5f);
    EXPECT_NEAR(cloud.rotations()[0].z, 0.0f, 1e-5f);

    // rot = (0,1,0,0) file order -> Quatf(1,0,0,0)
    EXPECT_NEAR(cloud.rotations()[1].x, 1.0f, 1e-5f);
    EXPECT_NEAR(cloud.rotations()[1].y, 0.0f, 1e-5f);
    EXPECT_NEAR(cloud.rotations()[1].z, 0.0f, 1e-5f);
    EXPECT_NEAR(cloud.rotations()[1].w, 0.0f, 1e-5f);

    // f_rest_k = k, channel-major -> coefficient-major
    // sh[1] = (0, 15, 30), sh[15] = (14, 29, 44)
    const int coeffs = vne::gs::shCoeffCount(3);
    const std::size_t base0 = 0u * static_cast<std::size_t>(coeffs) * 3u;
    EXPECT_NEAR(cloud.sh()[base0 + 1 * 3 + 0], 0.0f, 1e-5f);
    EXPECT_NEAR(cloud.sh()[base0 + 1 * 3 + 1], 15.0f, 1e-5f);
    EXPECT_NEAR(cloud.sh()[base0 + 1 * 3 + 2], 30.0f, 1e-5f);
    EXPECT_NEAR(cloud.sh()[base0 + 15 * 3 + 0], 14.0f, 1e-5f);
    EXPECT_NEAR(cloud.sh()[base0 + 15 * 3 + 1], 29.0f, 1e-5f);
    EXPECT_NEAR(cloud.sh()[base0 + 15 * 3 + 2], 44.0f, 1e-5f);

    const vne::math::Vec3f dc = cloud.dcColor(0);
    constexpr float kShC0 = 0.28209479177387814f;
    EXPECT_NEAR(dc.x(), std::max(0.0f, 0.5f + kShC0 * 0.1f), 1e-5f);
    EXPECT_NEAR(dc.y(), std::max(0.0f, 0.5f + kShC0 * -0.2f), 1e-5f);
    EXPECT_NEAR(dc.z(), std::max(0.0f, 0.5f + kShC0 * 0.3f), 1e-5f);
}

TEST(PlyReader, DegreesFromRestCount) {
    for (const int degree : {0, 1, 2, 3}) {
        const auto path = tempPlyPath(("degree_" + std::to_string(degree) + ".ply").c_str());
        vne::gs::GaussianCloud cloud = makeCloud(degree);
        std::string error;
        ASSERT_TRUE(vne::gs::writeGaussianPly(path.string(), cloud, &error)) << error;

        vne::gs::GaussianCloud loaded;
        ASSERT_TRUE(vne::gs::readGaussianPly(path.string(), loaded, &error)) << error;
        EXPECT_EQ(loaded.shDegree(), degree);
        std::filesystem::remove(path);
    }
}

TEST(PlyReader, RejectsTenRestProperties) {
    const auto path = tempPlyPath("bad_rest_count.ply");
    {
        std::ofstream out(path, std::ios::binary);
        writeAsciiHeader(out, 0, 10);
    }
    vne::gs::GaussianCloud cloud;
    std::string error;
    EXPECT_FALSE(vne::gs::readGaussianPly(path.string(), cloud, &error));
    EXPECT_NE(error.find("10"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(PlyReader, RejectsAsciiFormat) {
    const auto path = tempPlyPath("ascii.ply");
    {
        std::ofstream out(path);
        out << "ply\nformat ascii 1.0\nelement vertex 0\nend_header\n";
    }
    vne::gs::GaussianCloud cloud;
    std::string error;
    EXPECT_FALSE(vne::gs::readGaussianPly(path.string(), cloud, &error));
    EXPECT_NE(error.find("ASCII"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(PlyReader, RejectsBigEndian) {
    const auto path = tempPlyPath("be.ply");
    {
        std::ofstream out(path);
        out << "ply\nformat binary_big_endian 1.0\nelement vertex 0\nend_header\n";
    }
    vne::gs::GaussianCloud cloud;
    std::string error;
    EXPECT_FALSE(vne::gs::readGaussianPly(path.string(), cloud, &error));
    EXPECT_NE(error.find("big-endian"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(PlyReader, RejectsMissingOpacity) {
    const auto path = tempPlyPath("no_opacity.ply");
    {
        std::ofstream out(path, std::ios::binary);
        writeAsciiHeader(out, 0, 0, false);
    }
    vne::gs::GaussianCloud cloud;
    std::string error;
    EXPECT_FALSE(vne::gs::readGaussianPly(path.string(), cloud, &error));
    EXPECT_NE(error.find("opacity"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(PlyReader, RoundTripPreservesValues) {
    const auto path = tempPlyPath("round_trip.ply");
    vne::gs::GaussianCloud original = makeCloud(3, 2);
    original.rotations()[1] = vne::math::Quatf(1.0f, 0.0f, 0.0f, 0.0f);
    original.opacities()[1] = 0.8f;
    original.scales()[1] = vne::math::Vec3f(3.0f, 2.0f, 1.0f);

    std::string error;
    ASSERT_TRUE(vne::gs::writeGaussianPly(path.string(), original, &error)) << error;

    vne::gs::GaussianCloud loaded;
    ASSERT_TRUE(vne::gs::readGaussianPly(path.string(), loaded, &error)) << error;
    ASSERT_EQ(loaded.size(), original.size());
    EXPECT_EQ(loaded.shDegree(), original.shDegree());

    for (std::size_t i = 0; i < original.size(); ++i) {
        EXPECT_NEAR(loaded.positions()[i].x(), original.positions()[i].x(), 1e-5f);
        EXPECT_NEAR(loaded.scales()[i].x(), original.scales()[i].x(), 1e-5f);
        EXPECT_NEAR(loaded.opacities()[i], original.opacities()[i], 1e-5f);
        EXPECT_NEAR(loaded.rotations()[i].x, original.rotations()[i].x, 1e-5f);
        EXPECT_NEAR(loaded.rotations()[i].w, original.rotations()[i].w, 1e-5f);
    }
    ASSERT_EQ(loaded.sh().size(), original.sh().size());
    for (std::size_t i = 0; i < original.sh().size(); ++i) {
        EXPECT_NEAR(loaded.sh()[i], original.sh()[i], 1e-5f);
    }
    std::filesystem::remove(path);
}
