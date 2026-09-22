# Task 12 — Splats as Instanced Quads

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 4 — Real-time GPU viewer | [11](11_gpu_sort.md) | [13](13_tile_rasterizer_gpu.md), [14](14_polish.md), [19](19_vnegfx_integration.md) | [ ] |

> **Goal:** real-time 3DGS: draw each projected Gaussian as a screen-aligned quad, evaluate the Gaussian in
> the fragment shader, and blend back to front with the fixed-function blender.

## Why this task

This is how most web and engine viewers render 3DGS. It uses the hardware rasterizer and blender
instead of a custom compute rasterizer, so it's the quickest way to real time: preprocess (Task 10) → sort
(Task 11) → draw. Task 13 then builds the paper's compute rasterizer, and you'll compare the two.

```
 projected + sorted indices → [ vertex shader: place quad ] → [ fragment: Gaussian → α ] → [ blend: over ]
```

## Learn

### 1. One quad per Gaussian

Draw `N` instances of a 4-vertex quad (triangle strip, or 6 indices).

**Vertex shader**, for instance `i`:

```
g = sorted_index[N − 1 − i]            # back to front: farthest first
p = projected[g]
if p.radius == 0: emit a degenerate position (all corners equal, or outside clip) and return
corner = (±1, ±1)
pixel  = p.mean + corner · p.radius    # square covering the 3σ box
clip   = pixelToClip(pixel)            # see §4
pass to fragment: p.mean, conic, opacity, color  (flat)
```

**Fragment shader:**

```
d = fragCoord.xy − mean                # both in pixel coords, center at +0.5 (Task 04)
power = −½(A·dx² + C·dy²) − B·dx·dy
if power > 0: discard
α = min(0.99, opacity · exp(power))
if α < 1/255: discard
out = vec4(color · α, α)               # premultiplied alpha
```

### 2. Back-to-front "over" blending with premultiplied alpha

```
dst = src + (1 − src.a) · dst           blend factors: src = ONE, dst = ONE_MINUS_SRC_ALPHA
```

Clear the target to the background color, then draw farthest first. Unrolling the recursion gives exactly
`Σ cᵢ αᵢ Tᵢ + T·background`, the same image as front-to-back. No depth test and no depth write: order
comes from the sort, not the z-buffer.

(Front to back is also possible with the "under" operator, `src·(1 − dst.a) + dst` and blend factors
`ONE_MINUS_DST_ALPHA, ONE`, which needs a cleared alpha of 0 and the background composited at the end.
Back to front is simpler; start there.)

### 3. How this differs from the CPU reference, and why the diff is tiny

| Aspect | CPU (Tasks 06/07) | Quads |
|--------|------------------|-------|
| Early termination (`T < 1e-4`) | yes | no: everything behind still blends with weight < 1e-4 |
| Coverage | radius box / tile rect | the quad = radius box (same as Task 06) |
| Order | depth sort | same depth sort |
| Precision | fp32 | render-target format (fp16 or fp32) |

So differences should be under 1/255 almost everywhere. If you see more, suspect a convention bug (a
half-pixel offset or a Y flip) before precision.

### 4. Pixel → clip space, per backend

```
x_ndc = 2 · pixel.x / width − 1
y_ndc = 2 · pixel.y / height − 1        # if NDC +Y points down (Vulkan)
y_ndc = 1 − 2 · pixel.y / height        # if NDC +Y points up (Metal)
```

Framebuffer origin and `gl_FragCoord` are top-left on both Vulkan and Metal, so the fragment math
doesn't change. Only the vertex shader's NDC mapping does. vnemath already encodes per-API conventions
(`GraphicsApiTraits` in `vnemath/include/vertexnova/math/core/types.h`), and vnerhi samples show how the
flip is handled. Follow that rather than hard-coding one backend.

### 5. Cost and precision

- **Overdraw** is the cost: every quad pixel runs the fragment shader and blends, even behind a saturated
  pixel. Big splats near the camera × many layers = slow. Add an overdraw heat-map mode.
- **Tighter quads:** a square around a thin ellipse wastes most of its area. Build an oriented quad from
  the 2D covariance eigenvectors (`±3√λ₁·e₁ ± 3√λ₂·e₂`), which is a big win for flat splats.
- **Render-target format:** start with RGBA16F. Thousands of tiny contributions accumulated in half
  precision can band; if you see banding compared with the CPU image, try RGBA32F where the backend can
  blend it.

### 6. The frame

```
compute: preprocess (T10) → compact + sort by depth (T11)
render:  clear(background) → draw N instanced quads → (ImGui) → present
```

Time each stage with GPU timestamp queries (`IGpuQuery`, `writeTimestamp`) and show them in the panel.

## Read

- **Code**, [antimatter15/splat](https://github.com/antimatter15/splat) `main.js`: the vertex/fragment
  shader pair and how it builds oriented quads from the 2D covariance. It's WebGL, but the idea
  transfers directly.
- `../vnerhi/samples/33_oit_weighted_blended` and `29_blending`: blend state setup in vnerhi.
- `../vnerhi/samples/06_instancing`: instanced draws.
- `../vnerhi/samples/41_gpu_debug`: GPU timestamps and debug markers.

## Build

- [ ] `shaders/splat.vert.glsl`, `shaders/splat.frag.glsl` + manifest.
- [ ] `include/vertexnova/gs/render/gpu/splat_quad_renderer.h`: owns the pipeline; records draw calls;
  exposes timings.
- [ ] Viewer: GPU path as default. Panel shows per-stage GPU ms, visible count, SH degree, background,
  "square vs oriented quads" and an overdraw view.
- [ ] Offscreen render helper for tests (render to texture, read back).
- [ ] `tests/gpu/splat_quad_gpu_test.cpp`

## Test

| Case | Expected |
|------|----------|
| One Gaussian on the axis (Task 06's case), offscreen, 101×101 | center pixel within 1/255 of the CPU value |
| Synthetic 1,000-Gaussian scene | max abs diff vs `renderTiled` ≤ 2/255; PSNR > 45 dB |
| Two overlapping splats in both input orders | same image (the sort decides, not draw order) |
| Garden, golden camera | PSNR vs the Task 06/07 golden image > 40 dB |

## Done when

- [ ] Tests pass on Metal and Vulkan.
- [ ] The garden runs in real time in the viewer on the Mac and on the Spark; the FPS and per-stage timings
  are recorded in *My notes*.

## Check yourself

1. Why premultiplied alpha, and what are the blend factors?
2. Why no depth test?
3. Where do the (small) differences from the CPU image come from?
4. Why are oriented quads faster than square quads?
5. When would a quad renderer be slower than a tile rasterizer?

<details><summary>Answers</summary>

1. Premultiplied "over" is associative and composes correctly; the factors are `ONE, ONE_MINUS_SRC_ALPHA`.
2. The splats are semi-transparent; order comes from the sort. A depth test would drop partially-covered
   splats behind others.
3. No early termination, render-target precision, and fast-math `exp`.
4. Far fewer fragments for thin/flat splats, so less fragment work and blending.
5. With heavy overdraw (the camera inside or near large splats), where the tile rasterizer's per-pixel
   early termination skips most of the work.

</details>

## Going further

- Sort only every other frame (or asynchronously) and watch for artifacts during fast rotation.
- Add a "splat scale" slider (multiply the covariance) to see how much each Gaussian contributes.

## My notes

_Fill in after finishing._
