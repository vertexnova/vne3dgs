# Task 06 — Sort and Blend: the First Real Image

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 3 — CPU reference renderer | [05](05_ewa_projection.md) | [07](07_tile_renderer.md), [08](08_spherical_harmonics.md) | [ ] |

> **Goal:** render a trained scene into a real image by projecting every Gaussian, sorting by depth and
> blending front to back. This image is the golden reference for everything after it.

## Why this task

Tasks 01–05 each built one piece. This task wires them into a complete forward renderer, the whole 3DGS
rendering algorithm without the performance tricks. It's slow, and that's fine: it's the **reference**
every faster renderer (Task 07 CPU tiles, Tasks 12–13 GPU) is tested against.

```
 cloud → [project (T05)] → [sort by depth] → [blend per pixel (T01)] → image
          └───────────────── this task ─────────────────┘
```

## Learn

### 1. The algorithm

```
visible = []
for each Gaussian g:
    p = projectGaussian(g, camera)                 # Task 05, color = DC color for now
    if p.radius > 0: visible.append(p)
sort visible by (depth, index)                     # nearest first; index breaks ties deterministically

per-pixel state: C = 0, T = 1, done = false
for each p in visible (nearest first):             # "splat order"
    for each pixel whose center lies in [p.mean − r, p.mean + r]²:
        if done[pixel]: continue
        d = pixel_center − p.mean
        power = −½(A·dx² + C·dy²) − B·dx·dy
        if power > 0: continue
        α = min(0.99, p.opacity · exp(power))
        if α < 1/255: continue
        test_T = T[pixel] · (1 − α)
        if test_T < 1e-4: done[pixel] = true; continue
        C[pixel] += p.color · α · T[pixel]
        T[pixel] = test_T
image = C + T · background
```

Every constant comes from the reference rasterizer: the `power > 0` guard, the 0.99 cap, the 1/255
skip and the 1e-4 stop.

**Two loop orders give the same answer.** *Pixel order* (for each pixel, walk the sorted list) is what the
GPU does. *Splat order* (for each splat, touch its pixels; per-pixel state in buffers) is much faster on a
CPU because each splat touches only its own box. Because every pixel still sees splats in the same depth
order, the results are identical. Use splat order here.

### 2. One sort for all pixels: an approximation

3DGS sorts Gaussians by their **center's** depth once, and every pixel uses that order. Two large,
overlapping Gaussians can be in the "wrong" order for some pixels, and when the camera moves and their
centers swap depth, you can see **popping**. It's a deliberate speed-vs-correctness trade.
(*StopThePop*, Radl et al. 2024, fixes it with per-pixel sorting, at a cost.)

### 3. Link to volume rendering

`C = Σᵢ cᵢ αᵢ Tᵢ` is the same sum NeRF uses to approximate the volume-rendering integral along a ray. The
difference is where the samples come from: NeRF queries a network at points along the ray; 3DGS uses the
Gaussians that overlap the pixel.

### 4. Color and background

- **Color:** DC only for now: `max(0, 0.5 + C0 · f_dc)`. It will look "flat" (no highlights). Task 08
  adds view dependence.
- **Background:** the reference trainer uses **black** unless `--white_background` was set. The Inria
  pre-trained Mip-NeRF 360 scenes were trained on black, so render them on black to compare.
- **Output:** clamp to [0, 1] and write 8-bit. 3DGS works in the photos' color space with no gamma step,
  so write the values as-is.

### 5. Making it fast enough to iterate

5.8M Gaussians at 1080p on one CPU thread is slow. While developing:

- render at **320×180** or **640×360** (scale `fx, fy, cx, cy` with the resolution),
- use a filtered cloud (Task 03 *Going further*: drop opacity < 0.05, crop to the 99th-percentile box),
- build in **Release**.

Task 07 makes it fast properly.

### 6. The golden reference

Save the garden image **together with the exact camera** (eye, target, up, fov, resolution) in a small
JSON next to it under `data/golden/` (git-ignored; real scenes aren't committed). Later renderers must
reproduce it. Unit tests use small synthetic scenes with hand-computable answers instead.

## Read

- **Paper** §6 "Fast Differentiable Rasterizer for Gaussians": the whole section. You now know every
  piece; notice the order they describe it in.
- **Code**, `forward.cu`: `renderCUDA`, line by line. Map each line to a line of your renderer.
- **Optional**, *StopThePop* (Radl et al., SIGGRAPH 2024), just the teaser figure and §1, to see popping.

## Build

- [ ] `include/vertexnova/gs/render/cpu/naive_renderer.h` + `.cpp`

  ```cpp
  namespace vne::gs {
  struct RenderSettings {
      math::Vec3f background{0.0f, 0.0f, 0.0f};
      int max_sh_degree = 0;                 // used from Task 08
  };
  struct RenderStats {
      std::size_t visible = 0;               // after culling
      double avg_splats_per_pixel = 0.0;     // contributions actually blended
      double milliseconds = 0.0;
  };
  [[nodiscard]] ImageRGBf renderNaive(const GaussianCloud& cloud, const Camera& cam,
                                      const RenderSettings& settings, RenderStats* stats = nullptr);
  }  // namespace vne::gs
  ```

- [ ] `tests/naive_renderer_test.cpp`
- [ ] `examples/06_first_render/`: `example_06_first_render <file.ply> [camera flags from Task 04]
  [--width W --height H]` → `render.png`, prints `RenderStats`, and optionally writes the golden JSON.

## Test

Use an odd image size (e.g. 101×101, `cx = cy = 50.5`) so a pixel center lands exactly on the principal
point.

| Case | Expected |
|------|----------|
| One Gaussian on the axis, `o = 0.8`, red, blue background; center pixel | `(0.8, 0, 0.2)` |
| Same, `o = 1.0` | `α` capped: `(0.99, 0, 0.01)` |
| Two overlapping splats (Task 01 blending example) given in **reverse** input order | same result as Task 01: sorting works |
| 10 opaque (`o = 1`) splats stacked on the axis | only the 2 nearest contribute; the rest are skipped once `T` would drop below 1e-4 |
| Gaussian with `o = 0.003` | contributes nothing (below 1/255) |
| Gaussian behind the camera | contributes nothing |
| Empty cloud | image == background |

## Done when

- [ ] Tests pass.
- [ ] The garden renders as a recognizable (flat-colored) image; the golden image and camera JSON are
  saved; `RenderStats` numbers are written in *My notes*.

## Check yourself

1. Why front to back instead of back to front?
2. What artifact does "one global sort per frame" cause, and when do you notice it?
3. Why is the splat that would push `T` below 1e-4 dropped rather than added?
4. Your render is too bright everywhere. List three likely causes.
5. Why do DC-only colors look flat?

<details><summary>Answers</summary>

1. Early termination: once a pixel is saturated, nothing behind it needs evaluating. It also matches what
   the GPU tile renderer does.
2. Popping: overlapping Gaussians switch order as the camera moves because their center depths swap.
   It shows most on large Gaussians during rotation.
3. To match the reference exactly (its backward pass assumes it). Visually it makes no difference.
4. A wrong background (white instead of black), opacity not passed through the sigmoid, or colors not
   passed through `0.5 + C0·f_dc` (using raw `f_dc`), or scales not `exp`'d, which inflates the splats.
5. Without the degree-1–3 SH terms, color doesn't change with viewing direction: no highlights or
   reflections.

</details>

## Going further

- Render a **depth image** (`Σ Tᵢ αᵢ zᵢ`) and an **alpha image** (`1 − T`) alongside color. You'll want
  both later (Task 14 and Task 19).
- Render a heat map of "splats blended per pixel": where is the scene expensive?

## My notes

_Fill in after finishing._
