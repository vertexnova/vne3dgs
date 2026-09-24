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
#include "vertexnova/gs/io/ply_writer.h"
#include "config.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <ostream>
#include <fstream>
#include <limits>
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
    cloud.resize(count);
    const int coeffs = cloud.shCoeffCount();
    for (std::size_t i = 0; i < count; ++i) {
        cloud.positions()[i] = vne::math::Vec3f(1.0f, 2.0f, 3.0f);
        cloud.scales()[i] = vne::math::Vec3f(2.0f, 1.0f, 0.5f);
        cloud.rotations()[i] = vne::math::Quatf::identity();
        cloud.opacities()[i] = 0.5f;
        cloud.setShCoefficient(i, 0, vne::math::Vec3f(0.1f, -0.2f, 0.3f));
        for (int k = 1; k < coeffs; ++k) {
            cloud.setShCoefficient(
                i,
                k,
                vne::math::Vec3f(static_cast<float>(k), static_cast<float>(k + 10), static_cast<float>(k + 20)));
        }
    }
    return cloud;
}

/** @brief Writes the 14 body floats of one minimal, finite Gaussian. */
void writeOneVertex(std::ostream& out, float x = 1.0f) {
    const float values[] = {x,
                            2.0f,
                            3.0f,  // position
                            0.1f,
                            0.2f,
                            0.3f,  // f_dc
                            0.0f,  // opacity -> 0.5
                            0.0f,
                            0.0f,
                            0.0f,  // log scale -> 1
                            1.0f,
                            0.0f,
                            0.0f,
                            0.0f};
    out.write(reinterpret_cast<const char*>(values), sizeof(values));
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

TEST(PlyReader, RejectsNegativeVertexCount) {
    const auto path = tempPlyPath("negative_count.ply");
    {
        std::ofstream out(path);
        out << "ply\nformat binary_little_endian 1.0\nelement vertex -1\nend_header\n";
    }
    vne::gs::GaussianCloud cloud;
    std::string error;
    EXPECT_FALSE(vne::gs::readGaussianPly(path.string(), cloud, &error));
    EXPECT_NE(error.find("element count"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(PlyReader, RejectsTruncatedBody) {
    const auto path = tempPlyPath("truncated_body.ply");
    {
        std::ofstream out(path, std::ios::binary);
        writeAsciiHeader(out, 1, 0);
        // Header claims one vertex but no body floats follow.
    }
    vne::gs::GaussianCloud cloud;
    std::string error;
    EXPECT_FALSE(vne::gs::readGaussianPly(path.string(), cloud, &error));
    EXPECT_NE(error.find("truncated"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(PlyReader, SkipsNonFiniteGaussiansByDefault) {
    const auto path = tempPlyPath("nan_body.ply");
    {
        std::ofstream out(path, std::ios::binary);
        writeAsciiHeader(out, 3, 0);
        writeOneVertex(out, 1.0f);
        // Middle Gaussian has a NaN position; the other two are fine.
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float values[] = {nan, 2.0f, 3.0f, 0.1f, 0.2f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
        out.write(reinterpret_cast<const char*>(values), sizeof(values));
        writeOneVertex(out, 5.0f);
    }

    vne::gs::PlyReader reader;
    vne::gs::GaussianCloud cloud;
    ASSERT_TRUE(reader.read(path.string(), cloud)) << reader.error();

    EXPECT_EQ(reader.stats().declared_vertex_count, 3u);
    EXPECT_EQ(reader.stats().skipped_non_finite, 1u);
    EXPECT_EQ(reader.stats().loaded_vertex_count, 2u);
    ASSERT_EQ(cloud.size(), 2u);
    EXPECT_TRUE(cloud.isConsistent());
    // The survivors keep their order, with the bad one squeezed out.
    EXPECT_FLOAT_EQ(cloud.positions()[0].x(), 1.0f);
    EXPECT_FLOAT_EQ(cloud.positions()[1].x(), 5.0f);
    std::filesystem::remove(path);
}

TEST(PlyReader, RejectsNonFiniteWhenPolicySaysSo) {
    const auto path = tempPlyPath("nan_strict.ply");
    {
        std::ofstream out(path, std::ios::binary);
        writeAsciiHeader(out, 1, 0);
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float values[] = {nan, 2.0f, 3.0f, 0.1f, 0.2f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
        out.write(reinterpret_cast<const char*>(values), sizeof(values));
    }

    vne::gs::PlyReadOptions options;
    options.skip_non_finite = false;
    vne::gs::PlyReader reader(options);
    vne::gs::GaussianCloud cloud;

    EXPECT_FALSE(reader.read(path.string(), cloud));
    EXPECT_NE(reader.error().find("non-finite"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(PlyReader, LeavesTheDestinationUntouchedOnFailure) {
    const auto path = tempPlyPath("truncated_keeps_dest.ply");
    {
        std::ofstream out(path, std::ios::binary);
        writeAsciiHeader(out, 4, 0);
        writeOneVertex(out);  // one vertex where the header promised four
    }

    // A cloud the caller already owns must survive a failed load intact.
    vne::gs::GaussianCloud cloud = makeCloud(1, 2);
    std::string error;
    EXPECT_FALSE(vne::gs::readGaussianPly(path.string(), cloud, &error));
    EXPECT_EQ(cloud.size(), 2u);
    EXPECT_EQ(cloud.shDegree(), 1);
    EXPECT_TRUE(cloud.isConsistent());
    EXPECT_FLOAT_EQ(cloud.positions()[0].x(), 1.0f);
    std::filesystem::remove(path);
}

TEST(PlyReader, SkipsElementsDeclaredBeforeVertex) {
    // A camera element ahead of vertex shifts the body. Reading the vertex data
    // from the wrong offset used to succeed and return the other element's bytes.
    const auto path = tempPlyPath("preceding_element.ply");
    {
        std::ofstream out(path, std::ios::binary);
        out << "ply\nformat binary_little_endian 1.0\n";
        out << "element camera 1\nproperty float px\nproperty float py\nproperty float pz\n";
        out << "element vertex 1\n";
        out << "property float x\nproperty float y\nproperty float z\n";
        out << "property float f_dc_0\nproperty float f_dc_1\nproperty float f_dc_2\n";
        out << "property float opacity\n";
        out << "property float scale_0\nproperty float scale_1\nproperty float scale_2\n";
        out << "property float rot_0\nproperty float rot_1\nproperty float rot_2\nproperty float rot_3\n";
        out << "end_header\n";
        const float camera[] = {111.0f, 222.0f, 333.0f};
        out.write(reinterpret_cast<const char*>(camera), sizeof(camera));
        writeOneVertex(out, 1.0f);
    }

    vne::gs::GaussianCloud cloud;
    std::string error;
    ASSERT_TRUE(vne::gs::readGaussianPly(path.string(), cloud, &error)) << error;

    ASSERT_EQ(cloud.size(), 1u);
    EXPECT_FLOAT_EQ(cloud.positions()[0].x(), 1.0f);
    EXPECT_FLOAT_EQ(cloud.positions()[0].y(), 2.0f);
    EXPECT_FLOAT_EQ(cloud.positions()[0].z(), 3.0f);
    std::filesystem::remove(path);
}

TEST(PlyReader, RejectsAListElementBeforeVertexRatherThanGuessing) {
    const auto path = tempPlyPath("list_before_vertex.ply");
    {
        std::ofstream out(path, std::ios::binary);
        out << "ply\nformat binary_little_endian 1.0\n";
        out << "element face 2\nproperty list uchar int vertex_index\n";
        out << "element vertex 1\n";
        out << "property float x\nproperty float y\nproperty float z\n";
        out << "property float f_dc_0\nproperty float f_dc_1\nproperty float f_dc_2\n";
        out << "property float opacity\n";
        out << "property float scale_0\nproperty float scale_1\nproperty float scale_2\n";
        out << "property float rot_0\nproperty float rot_1\nproperty float rot_2\nproperty float rot_3\n";
        out << "end_header\n";
        writeOneVertex(out);
    }

    vne::gs::GaussianCloud cloud;
    std::string error;
    EXPECT_FALSE(vne::gs::readGaussianPly(path.string(), cloud, &error));
    EXPECT_NE(error.find("list"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(PlyReader, IgnoresExtraPropertiesOfOtherTypes) {
    // Some tools add their own columns. Finding properties by name and offset
    // means those are skipped instead of shifting every value after them.
    const auto path = tempPlyPath("extra_properties.ply");
    {
        std::ofstream out(path, std::ios::binary);
        out << "ply\nformat binary_little_endian 1.0\nelement vertex 1\n";
        out << "property uchar red\n";
        out << "property float x\nproperty float y\nproperty float z\n";
        out << "property double confidence\n";
        out << "property float f_dc_0\nproperty float f_dc_1\nproperty float f_dc_2\n";
        out << "property float opacity\n";
        out << "property float scale_0\nproperty float scale_1\nproperty float scale_2\n";
        out << "property float rot_0\nproperty float rot_1\nproperty float rot_2\nproperty float rot_3\n";
        out << "property int cluster\n";
        out << "end_header\n";
        const std::uint8_t red = 200u;
        out.write(reinterpret_cast<const char*>(&red), 1);
        const float position[] = {1.0f, 2.0f, 3.0f};
        out.write(reinterpret_cast<const char*>(position), sizeof(position));
        const double confidence = 0.75;
        out.write(reinterpret_cast<const char*>(&confidence), sizeof(confidence));
        const float rest[] = {0.1f, 0.2f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
        out.write(reinterpret_cast<const char*>(rest), sizeof(rest));
        const std::int32_t cluster = 7;
        out.write(reinterpret_cast<const char*>(&cluster), sizeof(cluster));
    }

    vne::gs::GaussianCloud cloud;
    std::string error;
    ASSERT_TRUE(vne::gs::readGaussianPly(path.string(), cloud, &error)) << error;

    ASSERT_EQ(cloud.size(), 1u);
    EXPECT_FLOAT_EQ(cloud.positions()[0].x(), 1.0f);
    EXPECT_FLOAT_EQ(cloud.positions()[0].z(), 3.0f);
    EXPECT_NEAR(cloud.opacities()[0], 0.5f, 1e-5f);
    EXPECT_NEAR(cloud.scales()[0].x(), 1.0f, 1e-5f);
    std::filesystem::remove(path);
}

TEST(PlyReader, RejectsAHeaderThatOverstatesTheVertexCount) {
    // The count sizes an allocation, so it must be checked against the file.
    const auto path = tempPlyPath("huge_count.ply");
    {
        std::ofstream out(path, std::ios::binary);
        writeAsciiHeader(out, 100000000, 0);
        writeOneVertex(out);
    }

    vne::gs::GaussianCloud cloud;
    std::string error;
    EXPECT_FALSE(vne::gs::readGaussianPly(path.string(), cloud, &error));
    EXPECT_NE(error.find("truncated"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(PlyReader, ReadsAcrossChunkBoundaries) {
    // A chunk size below one vertex record must still make progress, and the
    // result must match a single-chunk read exactly.
    const auto path = tempPlyPath("chunked.ply");
    const vne::gs::GaussianCloud original = makeCloud(3, 37);
    std::string error;
    ASSERT_TRUE(vne::gs::writeGaussianPly(path.string(), original, &error)) << error;

    vne::gs::PlyReadOptions tiny;
    tiny.chunk_bytes = 1;
    vne::gs::PlyReader tiny_reader(tiny);
    vne::gs::GaussianCloud chunked;
    ASSERT_TRUE(tiny_reader.read(path.string(), chunked)) << tiny_reader.error();

    vne::gs::GaussianCloud whole;
    ASSERT_TRUE(vne::gs::readGaussianPly(path.string(), whole, &error)) << error;

    ASSERT_EQ(chunked.size(), 37u);
    ASSERT_EQ(chunked.sh().size(), whole.sh().size());
    for (std::size_t i = 0; i < chunked.size(); ++i) {
        EXPECT_FLOAT_EQ(chunked.positions()[i].x(), whole.positions()[i].x());
        EXPECT_FLOAT_EQ(chunked.opacities()[i], whole.opacities()[i]);
    }
    for (std::size_t i = 0; i < chunked.sh().size(); ++i) {
        EXPECT_FLOAT_EQ(chunked.sh()[i], whole.sh()[i]);
    }
    std::filesystem::remove(path);
}

TEST(PlyWriter, RejectsAnInconsistentCloud) {
    // The cloud class prevents desync, so a consistent cloud always writes.
    const vne::gs::GaussianCloud cloud = makeCloud(2, 3);
    ASSERT_TRUE(cloud.isConsistent());

    const auto path = tempPlyPath("writer_consistent.ply");
    vne::gs::PlyWriter writer;
    EXPECT_TRUE(writer.write(path.string(), cloud)) << writer.error();
    EXPECT_TRUE(writer.error().empty());
    std::filesystem::remove(path);
}

TEST(PlyWriter, CanOmitNormals) {
    vne::gs::PlyWriteOptions options;
    options.write_normals = false;
    vne::gs::PlyWriter writer(options);

    const auto path = tempPlyPath("no_normals.ply");
    const vne::gs::GaussianCloud original = makeCloud(1, 2);
    ASSERT_TRUE(writer.write(path.string(), original)) << writer.error();

    vne::gs::GaussianCloud loaded;
    std::string error;
    ASSERT_TRUE(vne::gs::readGaussianPly(path.string(), loaded, &error)) << error;
    EXPECT_EQ(loaded.size(), 2u);
    EXPECT_NEAR(loaded.positions()[0].x(), original.positions()[0].x(), 1e-6f);
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
