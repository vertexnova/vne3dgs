# Task 01 — 2D Gaussians and Alpha Blending

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 1 — One Gaussian, by hand | [00](00_scaffold.md) | [02](02_gaussian_3d.md) | [x] |

> **Goal:** draw a few overlapping 2D Gaussians into an image, with the exact per-pixel math the real
> renderer uses.

## Why this task

After projection (Task 05), **every 3D Gaussian becomes a 2D Gaussian on screen**, and all rendering
happens in 2D. This task is that last stage in isolation: evaluate a 2D Gaussian at a pixel, turn it into
an alpha, and blend a sorted stack of them. Everything you write here is reused unchanged in Tasks 06, 07
and 12.

```
 3D Gaussians → project → [ 2D Gaussians → alpha → blend ] → image
                           └──────── this task ────────┘
```

## Learn

### 1. The Gaussian as a falloff, not a probability

```
G(x) = exp( -½ · (x − μ)ᵀ · Σ⁻¹ · (x − μ) )
```

- `μ` (2-vector) is the center in pixels, and `Σ` (2×2, symmetric, positive definite) is the covariance.
- **No `1/(2π√det Σ)` normalization.** `G(μ) = 1` always. A separate **opacity** `o ∈ (0,1)` sets the
  peak. If Gaussians were normalized, big splats would be dim and small ones blinding, and brightness would
  be tied to size. 3DGS keeps size and brightness independent.

### 2. Covariance ↔ ellipse

For `Σ = [[a, b], [b, c]]`:

```
det = a·c − b²            (must be > 0)
mid = (a + c) / 2
λ₁,₂ = mid ± sqrt(mid² − det)            eigenvalues (λ₁ ≥ λ₂ > 0)
e₁ ∝ (b, λ₁ − a)   (or (1, 0) if b = 0 and a ≥ c)   eigenvector of λ₁
```

The level set `G = e^(−½)` is an ellipse with semi-axes `√λ₁` and `√λ₂` along `e₁` and `e₂`: the 1σ
ellipse. Going the other way, from axes to covariance:

```
Σ = R(θ) · diag(σ₁², σ₂²) · R(θ)ᵀ
```

This "rotation × scale² × rotationᵀ" form is exactly how 3DGS builds 3D covariances in Task 02.

**Screen-space radius.** A splat's footprint is cut off at 3σ along the longest axis:
`radius = ceil(3 · √λ₁)` pixels. Beyond that, `G < e^(−4.5) ≈ 0.011`.

### 3. The conic: what the renderer actually stores

The renderer needs `Σ⁻¹`, not `Σ`. For a symmetric 2×2 it's three numbers:

```
Σ⁻¹ = (1/det) · [[ c, −b],
                 [−b,  a]]         →  conic (A, B, C) = (c/det, −b/det, a/det)

power(d) = −½ · (A·dx² + C·dy²) − B·dx·dy          where d = pixel − μ
G = exp(power)
```

The name comes from `A·dx² + 2B·dx·dy + C·dy² = k`, a conic section (here an ellipse).

### 4. From Gaussian to alpha

```
α = min(0.99, o · exp(power))
skip the splat at this pixel if power > 0 (numerical guard) or α < 1/255
```

- **`1/255` cutoff:** anything smaller can't change an 8-bit pixel; skipping it saves work.
- **`0.99` cap:** keeps `1 − α ≥ 0.01`, so transmittance never hits exactly zero. The backward pass
  (Task 18) divides by `1 − α`.

### 5. Front-to-back compositing

Sort splats nearest first. Walk them keeping the **transmittance** `T`, the fraction of light from behind
that still reaches the eye:

```
C = (0,0,0);  T = 1
for each splat i, nearest first:
    test_T = T · (1 − αᵢ)
    if test_T < 1e-4: stop            (early termination; note this splat is NOT added)
    C += T · αᵢ · cᵢ
    T = test_T
C += T · background
```

The reference rasterizer checks *before* adding, so the splat that would push `T` under `1e-4` is dropped.
The visual difference is nil, but matching it exactly lets your CPU and GPU images agree bit for bit later.

In closed form: `C = Σᵢ cᵢ · αᵢ · Tᵢ` with `Tᵢ = Πⱼ₍ⱼ<ᵢ₎ (1 − αⱼ)`. This is the discrete form of the
volume-rendering integral NeRF uses. Back-to-front (`C = αᵢ·cᵢ + (1−αᵢ)·C`, farthest first) gives the same
image, but front-to-back can **stop early**, which is why tile renderers use it.

### 6. Worked examples (do these on paper first)

**Axis-aligned.** `μ = (10, 10)`, `Σ = [[4, 0], [0, 1]]` (σx = 2, σy = 1), `o = 0.8`, pixel `(12, 10)`:

```
conic = (1/4, 0, 1/1) = (0.25, 0, 1)
d = (2, 0) → power = −½ · 0.25 · 4 = −0.5 → G = e^(−0.5) = 0.6065
α = 0.8 · 0.6065 = 0.4852
radius = ceil(3 · √4) = 6
```

**Rotated.** `Σ = [[2.5, 1.5], [1.5, 2.5]]`:

```
det = 6.25 − 2.25 = 4;  mid = 2.5;  λ = 2.5 ± 1.5 → 4, 1;  e₁ = (1, 1)/√2
conic = (2.5/4, −1.5/4, 2.5/4) = (0.625, −0.375, 0.625)
d = (√2, √2)   (2 px along e₁ = exactly 1σ, since σ₁ = √4 = 2)
power = −½(0.625·2 + 0.625·2) − (−0.375)·2 = −1.25 + 0.75 = −0.5  ✓ (1σ again)
```

**Blending.** Front splat red `(1,0,0)` with `α = 0.5`, back splat green `(0,1,0)` with `α = 0.8`,
black background:

```
C = 0.5·(1,0,0) + (1−0.5)·0.8·(0,1,0) = (0.5, 0.4, 0);   T_final = 0.5 · 0.2 = 0.1
swap the order:  C = 0.8·(0,1,0) + 0.2·0.5·(1,0,0) = (0.1, 0.8, 0);   T_final = 0.1
```

Order changes the **color** but not the final **transmittance**: a product doesn't care about order, but
the weights do. That is why 3DGS must sort.

### 7. Pitfalls

- A non-positive-definite `Σ` (det ≤ 0) has no inverse; skip that splat.
- **Pixel centers:** decide now whether pixel `(i, j)` is sampled at `(i, j)` or `(i + 0.5, j + 0.5)`.
  Task 04 fixes the convention for the whole library; for now, sample at `(i + 0.5, j + 0.5)` and write it down.
- Evaluate only inside the 3σ bounding box. Looping every pixel for every splat is what Task 07 exists to avoid.

## Read

- **Paper** (Kerbl et al. 2023), §6 last two paragraphs: blending, the 1/255 and 0.99 details, early stop.
- **Code**, `diff-gaussian-rasterization/cuda_rasterizer/forward.cu`, function `renderCUDA`: find the lines
  computing `power`, `alpha` and `test_T`. You'll recognize every one of them after this task.
- **Code**, same file, function `preprocessCUDA`: the conic and `my_radius` computation (ignore the 3D
  parts for now).

## Build

- [x] One type per file (`conic.h` / `Conic`, `gaussian2d.h` / `Gaussian2D`,
      `front_to_back_blender.h` / `FrontToBackBlender`). All three are **header-only**: their
      per-pixel math has to inline, and an exported out-of-line definition would put a cross-library
      call in the innermost loop of Tasks 06, 07 and 13.

  ```cpp
  // core/conic.h
  class Conic {                                    // Σ⁻¹ = [[a, b], [b, c]]
   public:
      constexpr Conic(float a, float b, float c);  // from stored / GPU-side coefficients
      static constexpr std::optional<Conic> fromCovariance(const math::Mat2f& cov);  // nullopt if not SPD
      constexpr float a() const, b() const, c() const;
      constexpr float power(const math::Vec2f& d) const;   // −½ dᵀ Σ⁻¹ d, 0 at the mean
  };

  // core/gaussian2d.h
  class Gaussian2D {
   public:
      static constexpr float kMaxAlpha = 0.99f, kMinAlpha = 1.0f / 255.0f, kRadiusSigma = 3.0f;
      Gaussian2D(mean, cov, color, opacity);
      // accessors: mean/covariance/color/opacity + setMean/setCovariance/setColor/setOpacity
      std::optional<Conic> conic() const;                  // nullopt when degenerate
      math::Vec2f eigenvalues() const;                     // (λ₁, λ₂), λ₁ >= λ₂
      int radius() const;                                  // ceil(3·√λ₁), 0 when degenerate
      float alphaAt(const Conic& conic, const math::Vec2f& pixel) const;
      // stateless, GPU-portable primitives:
      static math::Vec2f eigenvaluesOf(const math::Mat2f& cov);
      static int radiusOf(const math::Mat2f& cov);
      static float alphaFromPower(float opacity, float power);   // 0 when skipped
  };

  // core/front_to_back_blender.h
  class FrontToBackBlender {                               // one pixel
   public:
      static constexpr float kTransmittanceEps = 1e-4f;
      bool composite(const math::Vec3f& radiance, float alpha);  // false once saturated
      [[nodiscard]] math::Vec3f resolve(const math::Vec3f& background) const;
      [[nodiscard]] float transmittance() const;
      [[nodiscard]] bool isSaturated() const;
  };
  ```

  Pass the conic into `alphaAt` rather than letting it recompute: a splat is inverted once and then
  evaluated over its whole footprint.

- [x] `tests/conic_test.cpp`, `tests/gaussian2d_test.cpp`, `tests/front_to_back_blender_test.cpp`
- [x] `examples/01_gaussians_2d/`: five hand-placed Gaussians (at least one rotated, two overlapping) on a
  256×256 image, written to `gaussians_2d.png`, plus a second image with the draw order reversed.
- [x] **PNG output:** add vneio as an **examples-only** dependency, image component only
  (`-DVNEIO_BUILD_MESH=OFF`), and use `saveImage(path, data, width, height, channels)` from
  `vertexnova/io/image/image.h`. If vneio is too heavy to pull in, vendor `stb_image_write.h` under
  `deps/external/stb/` instead. The core library must not depend on either.

## Test

| Case | Expected |
|------|----------|
| `computePower` at `d = 0` | `0` (so `G = 1`) |
| Axis-aligned example above | power `−0.5`, α `0.4852 ± 1e-4`, radius `6` |
| Rotated example above | conic `(0.625, −0.375, 0.625)`, eigenvalues `(4, 1)`, power `−0.5` |
| `det ≤ 0` (e.g. `[[1, 2], [2, 1]]`) | `computeConic` returns `nullopt` |
| `computeAlpha(1.0, 0.0)` | `0.99` (cap) |
| `computeAlpha(0.003, 0.0)` | `0` (below 1/255, skipped) |
| Blending example, both orders | `(0.5, 0.4, 0)` / `(0.1, 0.8, 0)`, `T = 0.1` in both |
| 20 identical splats with `α = 0.95` | 3 are composited (`T` = 0.05, 0.0025, 1.25e-4); the 4th would give 6.25e-6 < 1e-4, so `composite` returns `false` and the radiance stops changing. (Avoid α values that land exactly on 1e-4; float rounding decides those.) |

## Done when

- [x] All tests pass, and you worked out the three examples on paper **before** running them.
- [x] `gaussians_2d.png` shows five ellipses with the orientations you intended, and the reversed-order
  image differs only where splats overlap.

## Check yourself

1. Why doesn't 3DGS normalize its Gaussians?
2. You have the conic but not `Σ`. How do you get the screen radius?
3. Swapping two splats changes the color but not `T_final`. Why?
4. What would go wrong without the 0.99 cap?
5. What does `T` mean physically at the moment you add splat *i*?

<details><summary>Answers</summary>

1. Opacity sets the peak. Normalizing would make brightness depend on size, so big splats would fade and
   small ones would saturate.
2. The eigenvalues of `Σ` are the reciprocals of the conic's eigenvalues: `λmax(Σ) = 1 / λmin(conic)`.
   Or invert the conic back to `Σ`.
3. `T_final = Π(1 − αᵢ)` is a product, which is order-independent. Each color's weight `αᵢTᵢ` depends on
   what is in front of it.
4. `1 − α` could reach 0, making `T` exactly zero. The backward pass divides by `1 − α` to recover earlier
   transmittances.
5. The fraction of light from splat *i* (and everything behind it) that survives all the splats in front.

</details>

## Going further

- Implement back-to-front blending and assert both orders give identical images (with early termination off).
- Plot `G` along the major axis and mark 1σ, 2σ, 3σ. How much energy is lost by the 3σ cutoff?
- What happens to a splat with `σ = 0.2 px`? Try it, then read ahead to the `+0.3` dilation in Task 05.

## My notes

Pixel centers are `(i + 0.5, j + 0.5)`. The worked examples
treat a pixel as an integer point (`d = (2, 0)` for pixel `(12, 10)` vs μ `(10, 10)`); the unit tests
call `computePower` with that `d` directly. The example renderer uses the half-pixel convention.

PNG output uses vneio's image component only (`VNEIO_BUILD_MESH=OFF`) via
`vne::image::image_utils::saveImage`. The core library does not link vneio. CMake looks in
`deps/internal/vneio`, then `deps/external/vneio`, then a sibling `../vneio`.

The compositor's "saturated" check is on `next_T = T · (1 − α)`, not on `T` itself. After three α=0.95
splats, `T = 1.25e-4` which is still above `1e-4`; the fourth is the one that is dropped.

`Gaussian2D::radiusOf` returns 0 when the covariance is not positive definite. You can also recover
the screen radius from the conic alone: `λmax(Σ) = 1 / λmin(conic)`.

`Conic::fromCovariance` tests positive definiteness with Sylvester's criterion (`a > 0` **and**
`det > 0`), written as `!(x > 0)` so a NaN entry is rejected instead of slipping through. It averages
the two off-diagonals, so a covariance left slightly asymmetric by a projection Jacobian's round-off
still yields a symmetric conic.

Order changes color weights `αᵢ Tᵢ` but not `T_final = Π(1 − αᵢ)`. That is why the reversed PNG
differs only in the overlap.
