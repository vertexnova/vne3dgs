# Task 08 — Spherical Harmonics (View-Dependent Color)

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 3 — CPU reference renderer | [06](06_sort_and_blend.md) | [10](10_gpu_preprocess.md), [16](16_pytorch_trainer.md) | [ ] |

> **Goal:** evaluate each Gaussian's color as a function of viewing direction, so the renders get
> highlights, reflections and sky that change as you move.

## Why this task

With DC color only, every Gaussian looks the same from every direction, and shiny objects look like
matte paint. The other 45 numbers per Gaussian store *how color changes with direction*. This task turns
them on, which completes the CPU forward renderer.

## Learn

### 1. The idea

Color seen from direction `d` (a unit vector) is a function on the sphere, `c(d)`. Store it in a basis, the
way a Fourier series stores a periodic signal:

```
c(d) = Σ_{l=0..L} Σ_{m=−l..l}  k_{lm} · Y_{lm}(d)
```

**Spherical harmonics** `Y_lm` are the Fourier basis of the sphere: orthonormal, ordered by angular
frequency. Degree `l` has `2l + 1` functions; up to degree `L` there are `(L + 1)²`. 3DGS uses `L = 3`,
so 16 coefficients per channel and 48 per Gaussian. Low degrees give smooth variation, like a matte
surface lit from one side; degree 3 can express broad highlights but not mirror reflections.

### 2. The exact evaluation 3DGS uses

With `dir = normalize(position − camera_position)` (from the camera **to** the Gaussian) and
`(x, y, z) = dir`:

```
C0 = 0.28209479177387814
C1 = 0.4886025119029199
C2 = [ 1.0925484305920792, −1.0925484305920792, 0.31539156525252005,
      −1.0925484305920792,  0.5462742152960396 ]
C3 = [−0.5900435899266435,  2.890611442640554,  −0.4570457994644658,
       0.3731763325901154, −0.4570457994644658,  1.445305721320277,
      −0.5900435899266435 ]

result = C0 · sh[0]
if L ≥ 1:
    result += −C1·y·sh[1] + C1·z·sh[2] − C1·x·sh[3]
if L ≥ 2:   (xx = x², yy = y², zz = z², xy = x·y, yz = y·z, xz = x·z)
    result += C2[0]·xy·sh[4] + C2[1]·yz·sh[5] + C2[2]·(2zz − xx − yy)·sh[6]
            + C2[3]·xz·sh[7] + C2[4]·(xx − yy)·sh[8]
if L ≥ 3:
    result += C3[0]·y·(3xx − yy)·sh[9]  + C3[1]·xy·z·sh[10]
            + C3[2]·y·(4zz − xx − yy)·sh[11] + C3[3]·z·(2zz − 3xx − 3yy)·sh[12]
            + C3[4]·x·(4zz − xx − yy)·sh[13] + C3[5]·z·(xx − yy)·sh[14]
            + C3[6]·x·(xx − 3yy)·sh[15]
color = max(result + 0.5, 0)            (per channel; no upper clamp here)
```

Each `sh[k]` is an RGB triplet (Task 03's coefficient-major layout), so this runs once per channel, or
once on 3-vectors.

- **`+ 0.5`**: with all coefficients zero the Gaussian is mid-gray, a convenient neutral start for training.
- **Clamp at 0 only**: negative light makes no sense; values above 1 are clamped when writing the image.
- The constants are the SH normalization factors, e.g. `C0 = 1/(2√π)` and `C1 = √(3/(4π))`. The negative
  signs are a sign convention; they don't affect orthonormality.

### 3. Worked example

DC only, `sh[0] = (1, 0, −1)`: `color = 0.5 + 0.2821 · (1, 0, −1) = (0.782, 0.5, 0.218)`.

Add degree 1 with `sh[3] = (1, 1, 1)` (the x-term) and every other coefficient 0:

- looking along `+x` (`dir = (1, 0, 0)`): `result += −C1 · 1 · 1 = −0.4886` → `(0.293, 0.011, 0)` after the clamp
- looking along `−x`: `+0.4886` → `(1.271, 0.989, 0.707)`

The same Gaussian is dark from one side and bright from the other.

### 4. The training schedule

The trainer starts at degree 0 and adds one SH degree every 1,000 iterations up to 3 (Task 16), so the
coarse color is learned before view-dependent detail. Your renderer's `max_sh_degree` setting lets you
view any stage.

### 5. A trap for later: SH lives in world space

The direction is measured in the scene's world frame. If you **rotate the scene** (say, to fix the "up"
problem from Task 04), view-dependent color breaks unless you also rotate the SH coefficients (Wigner-D
matrices per degree) or evaluate with directions in the original frame. Transform the camera, not the
scene.

## Read

- **Sloan, *Stupid Spherical Harmonics (SH) Tricks*** (GDC 2008), the first ~10 pages: the real SH basis
  and why it's the Fourier basis on the sphere.
- **Code**, `gaussian-splatting/utils/sh_utils.py`: constants and `eval_sh`, plus `RGB2SH` / `SH2RGB`.
- **Code**, `forward.cu`: `computeColorFromSH`. Note the `clamped` output used by the backward pass.

## Build

- [ ] `include/vertexnova/gs/core/sh.h` + `.cpp`

  ```cpp
  namespace vne::gs {
  // coeffs: shCoeffCount(degree) RGB triplets, coefficient-major (sh[0] = DC).
  [[nodiscard]] math::Vec3f evalSH(int degree, const float* coeffs, const math::Vec3f& dir);
  [[nodiscard]] float evalSHBasis(int index, const math::Vec3f& dir);   // Y_index(dir), for tests
  }  // namespace vne::gs
  ```

- [ ] Renderers (Tasks 06 and 07) compute color with `evalSH(min(settings.max_sh_degree, cloud.sh_degree), ...)`
  per visible Gaussian, using `dir` from the camera position.
- [ ] `tests/sh_test.cpp`
- [ ] `examples/08_sh_views/`: the golden view at degree 0 and at degree 3, plus their difference ×5;
  then three nearby viewpoints of a shiny area (the garden table) at degree 3.

## Test

| Case | Expected |
|------|----------|
| Degree 0, `sh[0] = (1, 0, −1)` | `(0.782, 0.5, 0.218)` for any `dir` |
| Worked degree-1 example, `dir = ±x` | the two colors above |
| All coefficients 0 | `(0.5, 0.5, 0.5)` |
| Orthonormality: `∫ Y_i·Y_j dω` over a 100k-point Fibonacci sphere, `i, j < 16` | `≈ δ_ij` (tolerance 1e-3) |
| `evalSH` degree 3 vs `evalSHBasis` summed by hand | equal |
| Channel layout: degree-3 file with Task 03's `f_rest_k = k` pattern, evaluated at degree 1, `dir = −y` | `result − 0.5 = C0·sh[0] + C1·sh[1]` with `sh[1] = (0, 15, 30)`: checks loader + evaluator together |

## Done when

- [ ] Tests pass.
- [ ] The degree-3 garden looks noticeably richer than degree 0 (the difference image highlights shiny and
  sky regions), and highlights move across the three nearby viewpoints.

## Check yourself

1. How many coefficients per channel for degree 2, and why that formula?
2. Why `+0.5`?
3. Which direction goes into `evalSH`, camera→Gaussian or Gaussian→camera? What breaks if you flip it?
4. Why can't degree-3 SH represent a mirror reflection?
5. You rotated the scene by 90° to fix "up" and colors look odd from some angles. Why?

<details><summary>Answers</summary>

1. 9 = (2 + 1)². Degree `l` adds `2l + 1` functions: 1 + 3 + 5 = 9.
2. So all-zero coefficients give mid-gray, a neutral starting point for training.
3. Camera → Gaussian (`position − camera`). Flipping negates odd degrees (1 and 3): each Gaussian shows the
   color it has when seen from behind.
4. SH is band-limited: degree 3 only captures low angular frequencies, and a mirror needs very high ones.
5. SH is in world space. You changed the direction frame without rotating the coefficients.

</details>

## Going further

- Visualize one Gaussian's SH color on a sphere (a mini "environment map" per Gaussian).
- Implement SH rotation for degree 1 (a 3×3 matrix; the coefficients transform like a vector) and verify
  that a rotated scene renders the same.

## My notes

_Fill in after finishing._
