#pragma once
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

/**
 * @file camera.h
 * @brief OpenCV-convention pinhole camera for the 3DGS core.
 * @ingroup vne::gs
 *
 * @details Extrinsics map world to camera as `p_cam = R * p_world + t` with +X
 * right, +Y down, +Z forward. Intrinsics map camera to pixels:
 * `u = fx * (x / z) + cx`, `v = fy * (y / z) + cy`.
 *
 * This type is intentionally separate from vnescene's GraphicsApi cameras; see
 * `conventions.h`. Viewer conversion lands in Task 09.
 *
 * @note The projection methods are inline: they run once per Gaussian per
 *       frame, millions of times, so an exported out-of-line call would show up
 *       in a profile. `lookAt`, `orbit` and `fromFovY` run once per frame and
 *       live in the .cpp.
 */

#include "vertexnova/gs/export.h"

#include "vertexnova/math/core/mat.h"
#include "vertexnova/math/core/vec.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace vne::gs {

/**
 * @brief A Gaussian center projected to the image: pixel plus camera-space depth.
 *
 * Depth is kept because a renderer needs it for z-ordering, and recovering it
 * would mean transforming the point a second time.
 */
struct ProjectedPoint {
    math::Vec2f pixel{};
    float depth = 0.0f;
};

/**
 * @brief Pinhole intrinsics in pixels.
 *
 * `fromFovY` sets `fy = height / (2 * tan(fovy / 2))` and `fx = fy` (square
 * pixels), with the principal point at the image center.
 */
class VNE_GS_API Intrinsics {
   public:
    constexpr Intrinsics() noexcept = default;

    constexpr Intrinsics(float fx, float fy, float cx, float cy, std::uint32_t width, std::uint32_t height) noexcept
        : fx_(fx)
        , fy_(fy)
        , cx_(cx)
        , cy_(cy)
        , width_(width)
        , height_(height) {}

    /**
     * @brief Square-pixel intrinsics from a vertical field of view.
     *
     * @param fovy_rad Vertical field of view in radians. A value at or below
     *                 zero (or at or above pi) leaves the focal lengths at 0,
     *                 which `isValid()` then reports as unusable.
     */
    [[nodiscard]] static Intrinsics fromFovY(float fovy_rad, std::uint32_t width, std::uint32_t height);

    [[nodiscard]] constexpr float fx() const noexcept { return fx_; }
    [[nodiscard]] constexpr float fy() const noexcept { return fy_; }
    [[nodiscard]] constexpr float cx() const noexcept { return cx_; }
    [[nodiscard]] constexpr float cy() const noexcept { return cy_; }
    [[nodiscard]] constexpr std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] constexpr std::uint32_t height() const noexcept { return height_; }

    void setFocalLength(float fx, float fy) noexcept {
        fx_ = fx;
        fy_ = fy;
    }
    void setPrincipalPoint(float cx, float cy) noexcept {
        cx_ = cx;
        cy_ = cy;
    }
    void setResolution(std::uint32_t width, std::uint32_t height) noexcept {
        width_ = width;
        height_ = height;
    }

    /** @brief `width * height`, widened so large images cannot overflow. */
    [[nodiscard]] constexpr std::size_t pixelCount() const noexcept {
        return static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
    }

    /**
     * @brief True when both focal lengths and both image dimensions are positive.
     *
     * A camera that fails this projects every point onto the principal point,
     * so renderers check it and return an empty image rather than a black one.
     */
    [[nodiscard]] constexpr bool isValid() const noexcept {
        return fx_ > 0.0f && fy_ > 0.0f && width_ > 0u && height_ > 0u;
    }

    /** @brief Vertical field of view in radians implied by `fy` and `height`. */
    [[nodiscard]] float fovYRad() const noexcept;

   private:
    float fx_ = 0.0f;
    float fy_ = 0.0f;
    float cx_ = 0.0f;
    float cy_ = 0.0f;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
};

/**
 * @brief World-to-camera pose plus intrinsics (OpenCV / COLMAP convention).
 */
class VNE_GS_API Camera {
   public:
    /**
     * @brief Near cull distance in camera space, the reference rasterizer's value.
     *
     * Closer than this the divide by z explodes and the projection Jacobian
     * (Task 05) becomes unstable, so the point is dropped.
     */
    static constexpr float kDefaultNearZ = 0.2f;

    constexpr Camera() noexcept = default;

    constexpr Camera(const math::Mat3f& rotation, const math::Vec3f& translation, const Intrinsics& intrinsics) noexcept
        : rotation_(rotation)
        , translation_(translation)
        , intrinsics_(intrinsics) {}

    /**
     * @brief OpenCV look-at: forward `f = normalize(target - eye)`, right
     *        `r = normalize(f x up)`, down `d = f x r`; the rows of `R` are
     *        `r, d, f`.
     *
     * Degenerate inputs are handled rather than producing NaNs: a zero-length
     * `target - eye` falls back to looking along -Z, a zero `up` to +Y, and an
     * `up` parallel to the view direction to an arbitrary perpendicular axis.
     */
    [[nodiscard]] static Camera lookAt(const math::Vec3f& eye,
                                       const math::Vec3f& target,
                                       const math::Vec3f& up,
                                       const Intrinsics& intrinsics);

    /**
     * @brief Orbit the eye around `center` at `radius`, then look at `center`.
     *
     * Azimuth rotates about `up`; elevation tilts toward `up`. At elevation 0
     * the eye sits in the plane through `center` perpendicular to `up`.
     */
    [[nodiscard]] static Camera orbit(const math::Vec3f& center,
                                      float radius,
                                      float azimuth_rad,
                                      float elevation_rad,
                                      const math::Vec3f& up,
                                      const Intrinsics& intrinsics);

    /** @brief World-to-camera rotation. */
    [[nodiscard]] constexpr const math::Mat3f& rotation() const noexcept { return rotation_; }
    void setRotation(const math::Mat3f& rotation) noexcept { rotation_ = rotation; }

    /** @brief World-to-camera translation. */
    [[nodiscard]] constexpr const math::Vec3f& translation() const noexcept { return translation_; }
    void setTranslation(const math::Vec3f& translation) noexcept { translation_ = translation; }

    [[nodiscard]] constexpr const Intrinsics& intrinsics() const noexcept { return intrinsics_; }
    void setIntrinsics(const Intrinsics& intrinsics) noexcept { intrinsics_ = intrinsics; }

    /** @brief Camera center in world space: `-R^T * t`. */
    [[nodiscard]] math::Vec3f position() const;

    /** @brief Applies the extrinsics: `R * p_world + t`. */
    [[nodiscard]] math::Vec3f worldToCamera(const math::Vec3f& p_world) const noexcept {
        return rotation_ * p_world + translation_;
    }

    /**
     * @brief Applies the intrinsics to an already-transformed point.
     * @return Continuous pixel coordinates, or nullopt if `p_cam.z() <= near_z`.
     */
    [[nodiscard]] std::optional<math::Vec2f> cameraToPixel(const math::Vec3f& p_cam,
                                                           float near_z = kDefaultNearZ) const noexcept {
        if (!(p_cam.z() > near_z)) {  // also rejects NaN
            return std::nullopt;
        }
        const float inv_z = 1.0f / p_cam.z();
        return math::Vec2f(intrinsics_.fx() * (p_cam.x() * inv_z) + intrinsics_.cx(),
                           intrinsics_.fy() * (p_cam.y() * inv_z) + intrinsics_.cy());
    }

    /**
     * @brief Projects a world point to continuous pixel coordinates.
     * @return nullopt if the point is behind or too close to the camera.
     */
    [[nodiscard]] std::optional<math::Vec2f> projectToPixel(const math::Vec3f& p_world,
                                                            float near_z = kDefaultNearZ) const noexcept {
        return cameraToPixel(worldToCamera(p_world), near_z);
    }

    /**
     * @brief Projects a world point, keeping camera-space depth.
     *
     * Prefer this in renderers: it transforms the point once and hands back the
     * depth a z-buffer or a sort key needs.
     */
    [[nodiscard]] std::optional<ProjectedPoint> project(const math::Vec3f& p_world,
                                                        float near_z = kDefaultNearZ) const noexcept {
        const math::Vec3f p_cam = worldToCamera(p_world);
        const std::optional<math::Vec2f> pixel = cameraToPixel(p_cam, near_z);
        if (!pixel.has_value()) {
            return std::nullopt;
        }
        return ProjectedPoint{*pixel, p_cam.z()};
    }

   private:
    math::Mat3f rotation_{};
    math::Vec3f translation_{};
    Intrinsics intrinsics_{};
};

}  // namespace vne::gs
