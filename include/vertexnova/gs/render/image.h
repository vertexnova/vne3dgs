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
 * @file image.h
 * @brief Linear RGB float image returned by CPU renderers, plus conversions.
 * @ingroup vne::gs
 *
 * @details `ImageRGBf` owns the relationship between its dimensions and its
 * buffer: the buffer is always `width * height * 3` floats, so a conversion
 * cannot be handed a size that disagrees with the pixel count. Conversions and
 * other free helpers live in `image_utils`.
 *
 * Values are linear radiance in `[0, inf)`, not gamma-encoded. `toRGBA8`
 * clamps and quantizes; it does not apply a transfer function.
 */

#include "vertexnova/gs/export.h"

#include "vertexnova/math/core/vec.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace vne::gs {

/**
 * @brief Packed linear RGB float image, row-major, 3 floats per pixel.
 */
class VNE_GS_API ImageRGBf {
   public:
    /// Floats stored per pixel.
    static constexpr std::size_t kChannels = 3;

    ImageRGBf() noexcept = default;

    /** @brief Allocates `width * height` black pixels. */
    ImageRGBf(std::uint32_t width, std::uint32_t height);

    /** @brief Allocates `width * height` pixels filled with `color`. */
    ImageRGBf(std::uint32_t width, std::uint32_t height, const math::Vec3f& color);

    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;

    /** @brief `width * height`, widened so large images cannot overflow. */
    [[nodiscard]] std::size_t pixelCount() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;

    /** @brief Reallocates to `width * height` black pixels. */
    void resize(std::uint32_t width, std::uint32_t height);

    /** @brief Sets every pixel to `color`. */
    void fill(const math::Vec3f& color) noexcept;

    /** @brief Raw buffer, `pixelCount() * kChannels` floats, row-major. */
    [[nodiscard]] std::span<const float> data() const noexcept;
    [[nodiscard]] std::span<float> data() noexcept;

    /**
     * @brief Reads one pixel.
     * @return Black if `(x, y)` is outside the image.
     */
    [[nodiscard]] math::Vec3f pixel(std::uint32_t x, std::uint32_t y) const noexcept;

    /** @brief Writes one pixel. Out-of-range coordinates are ignored. */
    void setPixel(std::uint32_t x, std::uint32_t y, const math::Vec3f& color) noexcept;

   private:
    /** @brief Float offset of pixel `(x, y)`; only valid after a bounds check. */
    [[nodiscard]] std::size_t offsetOf(std::uint32_t x, std::uint32_t y) const noexcept;

    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::vector<float> rgb_;
};

namespace image_utils {

/**
 * @brief Clamps RGB to `[0, 1]`, quantizes to 8-bit, and appends opaque alpha.
 *
 * @return Row-major RGBA bytes, `width * height * 4` long; empty for an empty
 *         image. Rounding is round-half-up, matching what the reference
 *         implementation writes to PNG.
 */
[[nodiscard]] VNE_GS_API std::vector<std::uint8_t> toRGBA8(const ImageRGBf& image);

/**
 * @brief Clamps RGB to `[0, 1]` and quantizes to 8-bit, without an alpha channel.
 *
 * @return Row-major RGB bytes, `width * height * 3` long; empty for an empty image.
 */
[[nodiscard]] VNE_GS_API std::vector<std::uint8_t> toRGB8(const ImageRGBf& image);

}  // namespace image_utils

}  // namespace vne::gs
