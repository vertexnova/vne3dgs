# Task 05 — Projecting the Ellipsoid (EWA Splatting)

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 3 — CPU reference renderer | [04](04_camera_and_points.md) | [06](06_sort_and_blend.md) | [ ] |

> **Goal:** turn a 3D Gaussian (position + covariance) into a 2D screen-space Gaussian: pixel center,
> conic, radius, depth.

## Why this task

This is the step that makes 3DGS 3D. Task 01 can blend 2D Gaussians and Task 04 can place centers. This
task produces each splat's **2D covariance** from its 3D covariance and the camera. It is the most
math-heavy step of the forward pass, and the one its backward pass (Task 18) depends on most.

```
 (μ, Σ₃D) ─[view]→ (t, W Σ Wᵀ) ─[linearized projection J]→ (μ₂D, Σ₂D) ─[+0.3 I, invert]→ conic, radius
```

## Learn

### 1. The problem

Perspective projection `(x, y, z) → (fx·x/z + cx, fy·y/z + cy)` is **nonlinear**, and a Gaussian pushed
through a nonlinear map is not a Gaussian. But under an **affine** map `y = A·x + b`:

```
x ~ N(μ, Σ)   ⇒   y ~ N(A·μ + b, A·Σ·Aᵀ)
```

**EWA splatting** (Zwicker et al. 2001) linearizes the projection around each Gaussian's own center
(first-order Taylor expansion). Locally the map is affine, so the projected shape is a Gaussian.

### 2. The three steps

**(a) Into camera space.** With `W` the 3×3 rotation of the world→camera transform (Task 04):

```
t      = W · μ + translation             camera-space mean (tx, ty, tz)
Σ_cam  = W · Σ₃D · Wᵀ
```

**(b) The Jacobian of the projection at `t`.**

```
u = fx · x/z + cx,   v = fy · y/z + cy

J = ∂(u, v)/∂(x, y, z) = [ fx/tz    0       −fx·tx/tz² ]
                         [ 0        fy/tz   −fy·ty/tz² ]        (2×3)
```

The third column shows that off-axis Gaussians get **stretched** away from the image center: moving along
z shifts their projection sideways.

**(c) Project the covariance.**

```
Σ₂D = J · Σ_cam · Jᵀ = J · W · Σ₃D · Wᵀ · Jᵀ          (2×2)
μ₂D = (fx · tx/tz + cx,  fy · ty/tz + cy)             (exact projection of the center, not linearized)
```

### 3. Two stability details from the reference code

**Frustum clamp.** Before computing `J`, clamp the direction used for the Jacobian:

```
limx = 1.3 · tan(fovx / 2),   limy = 1.3 · tan(fovy / 2)
tx_j = clamp(tx/tz, −limx, limx) · tz
ty_j = clamp(ty/tz, −limy, limy) · tz
```

For Gaussians far outside the view, `tx/tz` is large, the linearization is poor and `J` explodes, giving
giant, garbage splats. The clamp limits the damage. Only `J` uses the clamped values; `μ₂D` does not.

**Low-pass dilation.** Add `0.3` to the diagonal:

```
Σ₂D ← Σ₂D + 0.3 · I
```

The signal-processing view: EWA's *resampling filter* is the reconstruction kernel (the projected
Gaussian) convolved with a screen-space anti-aliasing prefilter. Convolving Gaussians **adds their
covariances**, so this is a Gaussian prefilter with variance 0.3 px² (σ ≈ 0.55 px). It guarantees every
splat covers about a pixel, so tiny or flat Gaussians don't fall between pixel centers and flicker.

Its flaw (see Mip-Splatting, Task 14): the filter is fixed in *screen* space, so zooming out makes
splats relatively fatter, and scenes look brighter and blurrier than in training.

### 4. From `Σ₂D` to what the rasterizer needs

Reuse Task 01:

```
det = Σ₂D[0][0] · Σ₂D[1][1] − Σ₂D[0][1]²       if det ≤ 0: cull
conic = (Σ₂D[1][1], −Σ₂D[0][1], Σ₂D[0][0]) / det
mid = ½ (Σ₂D[0][0] + Σ₂D[1][1])
λ₁ = mid + sqrt(max(0.1, mid² − det))           (the 0.1 guards against rounding)
radius = ceil(3 · sqrt(λ₁))                      pixels; cull if 0 or the 3σ box misses the image
depth  = tz                                      for sorting (Task 06)
```

### 5. Worked examples

**On the axis.** Isotropic Gaussian, `σ = 0.1` (`Σ₃D = 0.01·I`), camera-space `t = (0, 0, 5)`,
`fx = fy = 500`, image 1000×1000 (so `tan(fov/2) = 1` and the 1.3 clamp never engages here), `W = I`:

```
J = [[100, 0, 0], [0, 100, 0]]
Σ₂D = 100² · 0.01 · I = 100 · I   →  + 0.3  →  100.3 · I
σ₂D = √100.3 ≈ 10.01 px           (compare f·σ/z = 500 · 0.1 / 5 = 10 ✓)
radius = ceil(3 · 10.015) = 31
```

**Off axis.** Same Gaussian at `t = (2, 0, 5)`:

```
J row 0 = [100, 0, −500·2/25] = [100, 0, −40]
Σ₂D[0][0] = 0.01 · (100² + 40²) = 116   (+0.3 = 116.3)
Σ₂D[1][1] = 100                          (+0.3 = 100.3)
Σ₂D[0][1] = 0
```

A sphere near the edge of the view projects to an ellipse stretched **horizontally** (radially) by
`√(116.3/100.3) ≈ 1.08`. That's real perspective, not a bug.

### 6. Pitfalls

- `W` must be the rotation of **world→camera**, not camera→world (they're transposes of each other).
- Column-major GLM: `J · W` written in the wrong order silently computes `(W · J)`-like garbage. Test each
  product with the worked example.
- Don't use the clamped `tx, ty` for `μ₂D`.
- `radius` is an integer number of pixels; keep `μ₂D` as float.

## Read

- **Paper** §4, the paragraph with `Σ' = J W Σ Wᵀ Jᵀ` (equation 5 in the arXiv version).
- **Math supplement** (Ye & Kanazawa), the "Projection" section: the same derivation, in the notation
  used in Task 18.
- **Zwicker et al., EWA Splatting**, §3–4: the resampling-filter view (skip the volume-rendering parts).
- **Code**, `forward.cu`: `computeCov2D` (find the `1.3f` clamp and the `0.3f` dilation) and
  `preprocessCUDA` (conic, `lambda1`, `my_radius`, `getRect`).

## Build

- [ ] `include/vertexnova/gs/render/projection.h` + `.cpp`

  ```cpp
  namespace vne::gs {
  struct ProjectedGaussian {
      math::Vec2f mean;      // pixels, continuous coords (Task 04 convention)
      Conic conic;           // Task 01
      float depth = 0.0f;    // camera-space z
      int radius = 0;        // pixels; 0 = culled
      float opacity = 0.0f;
      math::Vec3f color;
  };

  [[nodiscard]] math::Mat2f computeCovariance2D(const math::Vec3f& t_cam, const math::Mat3f& cov3d_world,
                                                const math::Mat3f& world_to_cam_rot, const Intrinsics& intr,
                                                float dilation = 0.3f);
  [[nodiscard]] ProjectedGaussian projectGaussian(const math::Vec3f& position, const math::Mat3f& cov3d,
                                                  float opacity, const math::Vec3f& color, const Camera& cam);
  }  // namespace vne::gs
  ```

- [ ] `tests/projection_test.cpp`
- [ ] `examples/05_ellipses/`: a synthetic scene (a 5×5 grid of spheres plus a few rotated cigars and
  discs), each drawn on its own with the Task 01 falloff (no sorting yet). Look for the radial stretching
  toward the edges.

## Test

| Case | Expected |
|------|----------|
| On-axis worked example | `Σ₂D = 100.3·I`, radius `31`, `μ₂D = (cx, cy)` |
| Off-axis worked example | `Σ₂D = [[116.3, 0], [0, 100.3]]` |
| Isotropic Gaussian, any rotation `q` | same `Σ₂D` as `q = identity` |
| Cigar along the view axis (`s = (0.01, 0.01, 1)`, facing camera) | small near-circular footprint |
| Same cigar rotated 90° (lying across the view) | long thin ellipse; major axis ≈ `f·1/z` σ |
| `dilation = 0`, degenerate flat disc seen edge-on | `det ≈ 0` → culled (no NaN) |
| Gaussian at `t = (1000, 0, 5)` (far outside the view) | finite `Σ₂D`, no NaN, culled by the image-bounds check |
| Behind the camera | culled (`radius == 0`) |

## Done when

- [ ] Tests pass, including both worked examples to 1e-3.
- [ ] `05_ellipses.png` shows spheres getting radially stretched near the image edges and cigars/discs
  with the orientations you set up.

## Check yourself

1. Why is the projected shape only *approximately* Gaussian, and where is the approximation worst?
2. What does the third column of `J` do?
3. Why *add* `0.3·I` instead of clamping the eigenvalues to at least 0.3?
4. Why sort by `tz`, not by distance to the camera?
5. What goes wrong without the 1.3× frustum clamp?

<details><summary>Answers</summary>

1. Perspective isn't affine; EWA uses the tangent (Taylor) approximation at the center. It's worst for
   large Gaussians close to the camera and near the edges of a wide field of view.
2. Moving along depth shifts the projection sideways for off-axis points, which spreads the footprint
   radially.
3. Adding is convolving with a Gaussian prefilter, the principled anti-aliasing step, and it's smooth and
   differentiable. Clamping eigenvalues is neither.
4. That's the reference choice: it matches what the rasterizer and its backward pass assume, and
   camera-space z is cheap. Both are approximations of true per-pixel ordering.
5. Gaussians just outside the view get huge `J` entries, so enormous splats smear across the image.

</details>

## Going further

- Compare the EWA ellipse with the "true" footprint: sample 10k points from the 3D Gaussian, project each
  one exactly, and compute their 2D covariance. Where does it diverge?
- Derive `∂Σ₂D/∂Σ₃D` and `∂Σ₂D/∂t` (needed in Task 18).

## My notes

_Fill in after finishing._
