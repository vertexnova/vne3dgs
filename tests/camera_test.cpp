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

#include "vertexnova/gs/camera/camera.h"
#include "vertexnova/gs/camera/conventions.h"

#include "vertexnova/math/core/constants.h"
#include "vertexnova/math/core/types.h"

#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>

#include <gtest/gtest.h>

namespace {

[[nodiscard]] bool nearVec2(const vne::math::Vec2f& a, const vne::math::Vec2f& b, float tol) {
    return std::abs(a.x() - b.x()) <= tol && std::abs(a.y() - b.y()) <= tol;
}

[[nodiscard]] bool nearVec3(const vne::math::Vec3f& a, const vne::math::Vec3f& b, float tol) {
    return std::abs(a.x() - b.x()) <= tol && std::abs(a.y() - b.y()) <= tol && std::abs(a.z() - b.z()) <= tol;
}

[[nodiscard]] vne::gs::Camera identityCamera(const vne::gs::Intrinsics& k) {
    return vne::gs::Camera(vne::math::Mat3f{}, vne::math::Vec3f(0.0f, 0.0f, 0.0f), k);
}

}  // namespace

TEST(Camera, FromFovYNinetyDegrees) {
    const vne::gs::Intrinsics k = vne::gs::Intrinsics::fromFovY(vne::math::degToRad(90.0f), 200, 100);
    EXPECT_NEAR(k.fy(), 50.0f, 1e-4f);
    EXPECT_NEAR(k.fx(), 50.0f, 1e-4f);
    EXPECT_NEAR(k.cx(), 100.0f, 1e-4f);
    EXPECT_NEAR(k.cy(), 50.0f, 1e-4f);
    EXPECT_EQ(k.width(), 200u);
    EXPECT_EQ(k.height(), 100u);
}

TEST(Camera, IdentityProjectsAxisAlignedPoints) {
    const vne::gs::Intrinsics k{50.0f, 50.0f, 100.0f, 50.0f, 200, 100};
    const vne::gs::Camera cam = identityCamera(k);

    const auto center = cam.projectToPixel(vne::math::Vec3f(0.0f, 0.0f, 5.0f));
    ASSERT_TRUE(center.has_value());
    EXPECT_TRUE(nearVec2(*center, vne::math::Vec2f(100.0f, 50.0f), 1e-4f));

    const auto right = cam.projectToPixel(vne::math::Vec3f(5.0f, 0.0f, 5.0f));
    ASSERT_TRUE(right.has_value());
    EXPECT_NEAR(right->x(), 100.0f + 50.0f, 1e-4f);
    EXPECT_NEAR(right->y(), 50.0f, 1e-4f);

    // +Y is down in OpenCV camera space → below image center.
    const auto down = cam.projectToPixel(vne::math::Vec3f(0.0f, 5.0f, 5.0f));
    ASSERT_TRUE(down.has_value());
    EXPECT_NEAR(down->x(), 100.0f, 1e-4f);
    EXPECT_NEAR(down->y(), 50.0f + 50.0f, 1e-4f);
}

TEST(Camera, NearAndBehindAreCulled) {
    const vne::gs::Intrinsics k{50.0f, 50.0f, 100.0f, 50.0f, 200, 100};
    const vne::gs::Camera cam = identityCamera(k);

    EXPECT_FALSE(cam.projectToPixel(vne::math::Vec3f(0.0f, 0.0f, 0.1f)).has_value());
    EXPECT_FALSE(cam.projectToPixel(vne::math::Vec3f(0.0f, 0.0f, -1.0f)).has_value());
    EXPECT_FALSE(cam.projectToPixel(vne::math::Vec3f(0.0f, 0.0f, 0.2f)).has_value());
}

TEST(Camera, LookAtFromPositiveZ) {
    const vne::gs::Intrinsics k{50.0f, 50.0f, 100.0f, 50.0f, 200, 100};
    const vne::gs::Camera cam = vne::gs::Camera::lookAt(vne::math::Vec3f(0.0f, 0.0f, 5.0f),
                                                        vne::math::Vec3f(0.0f, 0.0f, 0.0f),
                                                        vne::math::Vec3f(0.0f, 1.0f, 0.0f),
                                                        k);

    EXPECT_TRUE(nearVec3(cam.position(), vne::math::Vec3f(0.0f, 0.0f, 5.0f), 1e-4f));

    // World +Y is opposite camera down → projects above center (v < cy).
    const auto above = cam.projectToPixel(vne::math::Vec3f(0.0f, 1.0f, 0.0f));
    ASSERT_TRUE(above.has_value());
    EXPECT_LT(above->y(), k.cy());

    const auto right = cam.projectToPixel(vne::math::Vec3f(1.0f, 0.0f, 0.0f));
    ASSERT_TRUE(right.has_value());
    EXPECT_GT(right->x(), k.cx());
}

TEST(Camera, LookAtFromNegativeZMirrorsX) {
    const vne::gs::Intrinsics k{50.0f, 50.0f, 100.0f, 50.0f, 200, 100};
    const vne::gs::Camera cam = vne::gs::Camera::lookAt(vne::math::Vec3f(0.0f, 0.0f, -5.0f),
                                                        vne::math::Vec3f(0.0f, 0.0f, 0.0f),
                                                        vne::math::Vec3f(0.0f, 1.0f, 0.0f),
                                                        k);

    // Looking along +Z: world +X projects left of center.
    const auto left = cam.projectToPixel(vne::math::Vec3f(1.0f, 0.0f, 0.0f));
    ASSERT_TRUE(left.has_value());
    EXPECT_LT(left->x(), k.cx());
}

TEST(Camera, OpenGLToOpenCVIsInvolution) {
    const vne::math::Mat3f m = vne::gs::openGLToOpenCV();
    const vne::math::Mat3f twice = m * m;
    for (std::size_t c = 0; c < 3; ++c) {
        for (std::size_t r = 0; r < 3; ++r) {
            const float expected = (c == r) ? 1.0f : 0.0f;
            EXPECT_NEAR(twice[c][r], expected, 1e-6f);
        }
    }
    EXPECT_NEAR(m[0][0], 1.0f, 1e-6f);
    EXPECT_NEAR(m[1][1], -1.0f, 1e-6f);
    EXPECT_NEAR(m[2][2], -1.0f, 1e-6f);
}

TEST(Camera, OrbitPositionMatchesLookAt) {
    const vne::gs::Intrinsics k{50.0f, 50.0f, 100.0f, 50.0f, 200, 100};
    const vne::math::Vec3f center(1.0f, 2.0f, 3.0f);
    const float radius = 4.0f;
    const float az = vne::math::degToRad(30.0f);
    const float el = vne::math::degToRad(20.0f);
    const vne::math::Vec3f up(0.0f, 1.0f, 0.0f);

    const vne::gs::Camera cam = vne::gs::Camera::orbit(center, radius, az, el, up, k);
    EXPECT_NEAR((cam.position() - center).length(), radius, 1e-4f);

    // Looking at center: center projects near principal point.
    const auto mid = cam.projectToPixel(center);
    ASSERT_TRUE(mid.has_value());
    EXPECT_NEAR(mid->x(), k.cx(), 1e-3f);
    EXPECT_NEAR(mid->y(), k.cy(), 1e-3f);
}

TEST(Camera, ProjectKeepsDepthFromASingleTransform) {
    const vne::gs::Intrinsics k{50.0f, 50.0f, 100.0f, 50.0f, 200, 100};
    const vne::gs::Camera cam = identityCamera(k);

    const auto projected = cam.project(vne::math::Vec3f(5.0f, 0.0f, 5.0f));
    ASSERT_TRUE(projected.has_value());
    EXPECT_NEAR(projected->pixel.x(), 150.0f, 1e-4f);
    EXPECT_NEAR(projected->pixel.y(), 50.0f, 1e-4f);
    EXPECT_NEAR(projected->depth, 5.0f, 1e-4f);

    // Same answer as the pixel-only overload, which the renderers rely on.
    const auto pixel_only = cam.projectToPixel(vne::math::Vec3f(5.0f, 0.0f, 5.0f));
    ASSERT_TRUE(pixel_only.has_value());
    EXPECT_NEAR(projected->pixel.x(), pixel_only->x(), 1e-6f);
}

TEST(Camera, ProjectRejectsNonFiniteDepth) {
    const vne::gs::Intrinsics k{50.0f, 50.0f, 100.0f, 50.0f, 200, 100};
    const vne::gs::Camera cam = identityCamera(k);
    const float nan = std::numeric_limits<float>::quiet_NaN();

    EXPECT_FALSE(cam.project(vne::math::Vec3f(0.0f, 0.0f, nan)).has_value());
}

TEST(Camera, IntrinsicsValidity) {
    EXPECT_TRUE((vne::gs::Intrinsics{50.0f, 50.0f, 100.0f, 50.0f, 200, 100}.isValid()));
    // Zero focal length: a degenerate field of view must not look usable.
    EXPECT_FALSE((vne::gs::Intrinsics{0.0f, 50.0f, 100.0f, 50.0f, 200, 100}.isValid()));
    EXPECT_FALSE((vne::gs::Intrinsics{50.0f, 50.0f, 100.0f, 50.0f, 0, 100}.isValid()));
    EXPECT_FALSE(vne::gs::Intrinsics::fromFovY(0.0f, 200, 100).isValid());
    EXPECT_FALSE(vne::gs::Intrinsics::fromFovY(-1.0f, 200, 100).isValid());
    EXPECT_FALSE(vne::gs::Intrinsics::fromFovY(vne::math::kPi, 200, 100).isValid());
    EXPECT_FALSE(vne::gs::Intrinsics::fromFovY(vne::math::kPi + 0.1f, 200, 100).isValid());
}

TEST(Camera, IntrinsicsFovYRoundTrips) {
    const float fovy = vne::math::degToRad(60.0f);
    const vne::gs::Intrinsics k = vne::gs::Intrinsics::fromFovY(fovy, 320, 240);

    EXPECT_NEAR(k.fovYRad(), fovy, 1e-4f);
    EXPECT_EQ(k.pixelCount(), 320u * 240u);
}

TEST(Camera, LookAtSurvivesUpParallelToView) {
    const vne::gs::Intrinsics k{50.0f, 50.0f, 100.0f, 50.0f, 200, 100};
    // Looking straight down the up axis: right is not unique, but the result
    // must still be a finite orthonormal frame rather than NaNs.
    const vne::gs::Camera cam = vne::gs::Camera::lookAt(vne::math::Vec3f(0.0f, 5.0f, 0.0f),
                                                        vne::math::Vec3f(0.0f, 0.0f, 0.0f),
                                                        vne::math::Vec3f(0.0f, 1.0f, 0.0f),
                                                        k);

    EXPECT_TRUE(nearVec3(cam.position(), vne::math::Vec3f(0.0f, 5.0f, 0.0f), 1e-4f));
    const vne::math::Mat3f r = cam.rotation();
    const vne::math::Mat3f should_be_identity = r * r.transpose();
    for (std::size_t c = 0; c < 3; ++c) {
        for (std::size_t row = 0; row < 3; ++row) {
            EXPECT_NEAR(should_be_identity[c][row], (c == row) ? 1.0f : 0.0f, 1e-4f);
        }
    }
}

TEST(Camera, LookAtSurvivesDegenerateInputs) {
    const vne::gs::Intrinsics k{50.0f, 50.0f, 100.0f, 50.0f, 200, 100};
    const vne::math::Vec3f origin(0.0f, 0.0f, 0.0f);

    // eye == target, and a zero up vector.
    const vne::gs::Camera cam = vne::gs::Camera::lookAt(origin, origin, origin, k);

    const vne::math::Mat3f r = cam.rotation();
    for (std::size_t c = 0; c < 3; ++c) {
        for (std::size_t row = 0; row < 3; ++row) {
            EXPECT_FALSE(std::isnan(r[c][row]));
        }
    }
}
