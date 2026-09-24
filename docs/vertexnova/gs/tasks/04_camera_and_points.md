# Task 04 — Cameras and Projecting Centers

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 3 — CPU reference renderer | [03](03_ply_loader.md) | [05](05_ewa_projection.md), [09](09_viewer_shell.md), [15](15_colmap_cameras.md) | [x] |

> **Goal:** put a camera in the scene and draw every Gaussian center as a dot. Fix the library's coordinate
> conventions for good.

## Why this task

Coordinate conventions are the #1 source of 3DGS bugs: upside-down images, mirrored scenes, empty frames.
This task deals with them while the "renderer" is still one dot per Gaussian and every bug is easy to see.

```
 world point ─[view matrix]→ camera space ─[intrinsics]→ pixel
 └──────────────────────── this task ──────────────────────┘
```

## Learn

### 1. The pinhole camera

```
p_cam = R · p_world + t                 extrinsics (world → camera), a.k.a. the view matrix
u = fx · (x / z) + cx                   intrinsics (camera → pixel)
v = fy · (y / z) + cy
```

- `fx, fy`: focal lengths in pixels. From a field of view: `fx = width / (2 · tan(fovx / 2))`.
- `cx, cy`: principal point, usually the image center (`width/2`, `height/2`).
- **Cull** anything with `z ≤ 0.2`, the reference rasterizer's near threshold. Behind or too close to the
  camera, the divide by z explodes.

### 2. Conventions: pick one and convert at the edges

| Convention | +X | +Y | Camera looks along | Used by |
|------------|----|----|-------------------|---------|
| **OpenCV / COLMAP** | right | **down** | **+Z** | 3DGS data and training cameras |
| OpenGL | right | up | **−Z** | many engines, glm::lookAt |
| Vulkan clip space | right | down | +Z into depth [0, 1] | vnerhi Vulkan backend |
| Metal NDC | right | up | +Z into depth [0, 1] | vnerhi Metal backend |

**Recommendation:** use **OpenCV camera space** inside `vne::gs` (it matches the data and the reference
code, so every formula in the papers applies unchanged). Convert once where you meet other systems:
vnescene / vneinteraction cameras in Task 09, and GPU clip space in Task 12.

OpenGL camera ↔ OpenCV camera is a flip of Y and Z: `diag(1, −1, −1)`. It's its own inverse.

**First sub-step:** read `../vnescene/include/vertexnova/scene/camera/` and write down which convention
vnescene cameras use (and whether vnemath's projection helpers assume GL or Vulkan clip space). Record it
in the Conventions table in [gs.md](../gs.md).

### 3. Pixel centers

Use continuous image coordinates where pixel `(i, j)` covers `[i, i+1) × [j, j+1)` and its **center is
`(i + 0.5, j + 0.5)`**. That matches `gl_FragCoord` on the GPU (Task 12).

The reference rasterizer uses the equivalent "center at integer `i`" convention: its
`ndc2Pix(v, S) = ((v + 1) · S − 1) · 0.5` works out to `u − 0.5`. If you ever compare pixel-by-pixel
against its output, remember the half-pixel shift. Record your choice in [gs.md](../gs.md).

### 4. A camera for looking at a scene

You don't have training cameras yet (Task 15), so build test cameras:

- **Look-at (OpenCV):** forward `f = normalize(target − eye)`, right `r = normalize(f × up)`,
  down `d = f × r`. Rows of `R` are `r, d, f`; `t = −R · eye`. Check: `r × d = f` (right-handed).
  Example: eye `(0, 0, 5)`, target origin, up `+Y` → `f = (0,0,−1)`, `r = (1,0,0)`, `d = (0,−1,0)`.
- **Orbit:** eye = center + radius · (spherical direction from azimuth, elevation). Use the median position
  from Task 03 as the center.

**The "up" problem:** a COLMAP world frame has **no guaranteed up direction**. It's whatever the
reconstruction produced. For now, give the example an `--up` flag and try `+Y`, `−Y`, `+Z`. Task 15 gets it
properly from the training cameras.

### 5. Drawing points

For each Gaussian: transform, cull, project, round to a pixel, and keep the **nearest** one per pixel
(a z-buffer). Color it with the DC color. That's all: this is the "point cloud" view you'll compare every
later renderer against.

### 6. Debugging table

| You see | Likely cause |
|---------|--------------|
| Upside-down image | Y convention flipped (GL vs CV) |
| Mirror image | Handedness: right = up × forward instead of forward × up |
| Empty image | Camera looking the wrong way (Z sign), or near-cull rejecting everything |
| Everything in one spot | Forgot to divide by z |
| Squashed | `fx` vs `fy` / aspect-ratio mix-up |
| Scene looks sideways | Wrong up vector (the COLMAP frame problem) |

## Read

- **Code**, `gaussian-splatting/utils/graphics_utils.py`: `getWorld2View2`, `getProjectionMatrix`,
  `focal2fov`, `fov2focal`.
- **Code**, `diff-gaussian-rasterization/cuda_rasterizer/auxiliary.h`: `in_frustum` (the 0.2 near cull)
  and `ndc2Pix`.
- **COLMAP docs**, "Output format" ([references](../learn/references.md#formats)): the paragraph on the
  camera coordinate system. You'll need it in Task 15.

## Build

- [x] `include/vertexnova/gs/camera/camera.h` + `.cpp`

  ```cpp
  namespace vne::gs {
  struct ProjectedPoint { math::Vec2f pixel; float depth; };

  class Intrinsics {
   public:
      constexpr Intrinsics(float fx, float fy, float cx, float cy, std::uint32_t w, std::uint32_t h);
      static Intrinsics fromFovY(float fovy_rad, std::uint32_t width, std::uint32_t height);
      constexpr float fx() const, fy() const, cx() const, cy() const;
      constexpr std::uint32_t width() const, height() const;
      void setFocalLength(float fx, float fy);
      void setPrincipalPoint(float cx, float cy);
      void setResolution(std::uint32_t width, std::uint32_t height);
      constexpr std::size_t pixelCount() const;     // widened; large images cannot overflow
      constexpr bool isValid() const;               // fx, fy, width, height all > 0
      float fovYRad() const;                        // inverse of fromFovY
  };

  class Camera {                                    // OpenCV convention
   public:
      static constexpr float kDefaultNearZ = 0.2f;
      constexpr Camera(const math::Mat3f& rotation, const math::Vec3f& translation, const Intrinsics&);
      static Camera lookAt(eye, target, up, intrinsics);
      static Camera orbit(center, radius, azimuth_rad, elevation_rad, up, intrinsics);
      // accessors: rotation(), translation(), intrinsics() + setters
      math::Vec3f position() const;                                   // −Rᵀ t
      math::Vec3f worldToCamera(const math::Vec3f& p_world) const;     // R p + t
      std::optional<math::Vec2f> cameraToPixel(const math::Vec3f& p_cam, float near_z = kDefaultNearZ) const;
      std::optional<math::Vec2f> projectToPixel(const math::Vec3f& p_world, float near_z = kDefaultNearZ) const;
      std::optional<ProjectedPoint> project(const math::Vec3f& p_world, float near_z = kDefaultNearZ) const;
  };
  }  // namespace vne::gs
  ```

  Renderers call `project()`, not `projectToPixel()`: it transforms the point **once** and hands back
  the depth a z-buffer or sort key needs. Applying the extrinsics and then re-projecting doubles the
  transform cost over millions of Gaussians. The near test is written `!(z > near_z)` so a NaN depth is
  culled rather than projected. `isValid()` exists because a degenerate field of view silently
  collapses every point onto the principal point; `renderPoints` returns an empty image instead.

- [x] `include/vertexnova/gs/camera/conventions.h`: `openGLToOpenCV()` (`diag(1, −1, −1)`) and a
  one-paragraph comment stating the library's conventions.
- [x] `include/vertexnova/gs/render/image.h`: `class ImageRGBf` plus a `image_utils` namespace for
  the conversions. Every CPU renderer returns one.

  ```cpp
  class ImageRGBf {
   public:
      static constexpr std::size_t kChannels = 3;
      ImageRGBf(std::uint32_t width, std::uint32_t height);
      ImageRGBf(std::uint32_t width, std::uint32_t height, const math::Vec3f& color);
      std::uint32_t width() const, height() const;
      std::size_t pixelCount() const;  bool isEmpty() const;
      void resize(std::uint32_t width, std::uint32_t height);
      void fill(const math::Vec3f& color);
      std::span<const float> data() const;  std::span<float> data();
      math::Vec3f pixel(std::uint32_t x, std::uint32_t y) const;          // bounds-checked
      void setPixel(std::uint32_t x, std::uint32_t y, const math::Vec3f&); // bounds-checked
  };

  namespace image_utils {
  std::vector<std::uint8_t> toRGBA8(const ImageRGBf& image);   // clamp to [0,1], 8-bit, opaque alpha
  std::vector<std::uint8_t> toRGB8(const ImageRGBf& image);
  }
  ```

  The class owns the relationship between its dimensions and its buffer, which is the whole reason it
  is a class: a plain struct with public `width`, `height` and `rgb` lets a caller set dimensions that
  disagree with the buffer, and `toRGBA8` then reads past the end.
- [x] `include/vertexnova/gs/render/cpu/point_renderer.h`: `ImageRGBf renderPoints(const GaussianCloud&,
  const Camera&, const math::Vec3f& background)`.
- [x] `tests/camera_test.cpp`
- [x] `examples/04_point_cloud/`: `example_04_point_cloud <file.ply> [--up +y|-y|+z] [--az deg]
  [--el deg] [--radius r]` → `points.png`.

## Test

| Case | Expected |
|------|----------|
| Identity camera, point `(0, 0, 5)` | pixel `(cx, cy)` |
| Identity camera, point `(5, 0, 5)` | `u = cx + fx` |
| Identity camera, point `(0, 5, 5)` | `v = cy + fy` (below center: +Y is down) |
| Point at `z = 0.1` or behind | `nullopt` |
| `lookAt(eye=(0,0,5), target=0, up=(0,1,0))`, point `(0, 1, 0)` | projects **above** center (`v < cy`) |
| Same camera, point `(1, 0, 0)` | projects **right** of center (`u > cx`); catches mirroring |
| `lookAt(eye=(0,0,−5), ...)` (looking along +Z), point `(1, 0, 0)` | projects **left** of center: a right-handed world seen from the other side. If this surprises you, work it out with `r = f × up` |
| `position()` of `lookAt(eye, ...)` | equals `eye` |
| `openGLToOpenCV()²` | identity |
| `fromFovY(90°, 200, 100)` | `fy = 50`, `fx = 50`, `cx = 100`, `cy = 50` |
| `project()` vs `projectToPixel()` | same pixel; `project()` also returns camera-space `z` |
| `project()` with a NaN depth | `nullopt` |
| `fromFovY(0°, …)` or a zero focal length | `isValid()` false |
| `lookAt` with `up` parallel to the view direction | finite orthonormal `R` (`R·Rᵀ = I`), no NaNs |
| `lookAt(eye == target, up == 0)` | no NaNs in `R` |
| `fromFovY(fovy).fovYRad()` | round-trips to `fovy` |

## Done when

- [x] Tests pass.
- [x] `points.png` shows the garden as a recognizable point cloud (table, vase, grass), right side up,
  from at least two viewpoints.
- [x] Your chosen conventions are written in [gs.md](../gs.md).

## Check yourself

1. Why cull at `z ≤ 0.2` rather than `z ≤ 0`?
2. Convert an OpenGL view matrix to an OpenCV one.
3. What's the camera position, given `R` and `t`?
4. Why does a COLMAP scene not have a well-defined "up"?
5. Pixel `(3, 7)`: where is its center in your convention?

<details><summary>Answers</summary>

1. Near `z = 0` the projection divides by almost nothing: huge coordinates and unstable Jacobians
   (Task 05). 0.2 is the reference value.
2. Left-multiply by `diag(1, −1, −1)` (flip camera Y and Z): `V_cv = diag(1, −1, −1, 1) · V_gl`.
3. `C = −Rᵀ · t`.
4. Structure-from-motion recovers geometry only up to a similarity transform; the first camera or an
   arbitrary frame sets the axes.
5. `(3.5, 7.5)`.

</details>

## Going further

- Color each dot by depth instead of DC color, to see the scene's depth structure.
- Draw the median center and the 99th-percentile box as lines to understand the scene's extent.

## My notes

Chose a separate `gs::Camera` (OpenCV R/t + fx/fy) rather than reusing vnescene's perspective/ortho
cameras: wrong convention (GL −Z look), wrong params (FOV/clip vs intrinsics), and would pull vnescene
into the headless core. vnescene reuse is deferred to Task 09 via `toGsCamera` / `scene_camera_bridge.h`.
vnescene default = OpenGL RH; bridge = `openGLToOpenCV()` = `diag(1,−1,−1)`.

A point lands in pixel `(floor(u), floor(v))`, which is what the half-pixel-center convention implies:
pixel `i` covers `[i, i+1)`, so `u = 3.6` belongs to pixel 3. Rounding to the *nearest* integer instead
would put it in pixel 4 and shift the whole image half a pixel against the stated convention.