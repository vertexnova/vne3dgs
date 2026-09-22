# Task 02 — The 3D Gaussian

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 1 — One Gaussian, by hand | [01](01_gaussians_2d.md) | [03](03_ply_loader.md) | [ ] |

> **Goal:** build a 3D covariance from a scale and a quaternion, and understand why 3DGS parametrizes it
> that way.

## Why this task

Each Gaussian in a trained scene stores a **scale** (3 numbers) and a **rotation** (4-number quaternion),
not a covariance matrix. Before you can project anything (Task 05), you need `Σ` from those two.

```
 [ scale + quaternion → Σ (3×3) ] → project to Σ₂D → alpha → blend
 └────────── this task ─────────┘
```

## Learn

### 1. The 3D Gaussian

```
G(x) = exp( -½ · (x − μ)ᵀ · Σ⁻¹ · (x − μ) )           x, μ ∈ R³,  Σ ∈ R³ˣ³
```

Level sets are **ellipsoids**. The eigenvectors of `Σ` are the ellipsoid's axes, and the eigenvalues are
the squared 1σ lengths along them. Like the 2D case, 3DGS never normalizes `G`.

### 2. Why not store `Σ` directly?

A covariance has 6 free numbers (it's symmetric) but must stay **positive semi-definite** (PSD). Training
moves parameters by gradient steps, and nothing stops a step on raw matrix entries from producing a matrix
with a negative eigenvalue, which is not an ellipsoid at all.

3DGS instead factors it:

```
Σ = R · S · Sᵀ · Rᵀ = (R·S)(R·S)ᵀ         S = diag(sx, sy, sz),  R = rotation from a unit quaternion
```

Anything of the form `M·Mᵀ` is PSD, so **every** value of `(s, q)` is a valid ellipsoid and the optimizer
can move freely. Read it right to left: scale a unit sphere along its local axes, then rotate it.

The parameters are stored with an activation that keeps them in range (Task 03):

| Parameter | Stored | Activation | Why |
|-----------|--------|-----------|-----|
| scale | `log s` | `exp` | always positive; a gradient step changes size by a *ratio*, which suits sizes spanning orders of magnitude |
| rotation | raw `q` | normalize | any 4-vector except 0 becomes a valid rotation |

### 3. Quaternion → rotation matrix

For a unit quaternion `q = (w, x, y, z)` (w is the scalar part):

```
R = [ 1 − 2(y² + z²)    2(xy − wz)       2(xz + wy)    ]
    [ 2(xy + wz)        1 − 2(x² + z²)   2(yz − wx)    ]
    [ 2(xz − wy)        2(yz + wx)       1 − 2(x² + y²) ]
```

- **Normalize first.** Stored quaternions are not unit length.
- `q` and `−q` give the same `R` (every entry is quadratic in q).
- The **columns** of `R` are the ellipsoid's axes in world space; `sx, sy, sz` are the 1σ lengths along them.

### 4. Worked example

`s = (2, 1, 0.5)`, rotation of 90° about +z: `q = (cos 45°, 0, 0, sin 45°) = (0.7071, 0, 0, 0.7071)`.

```
R = [ 1 − 2(0 + 0.5)   2(0 − 0.5)      0 ]   [ 0  −1  0 ]
    [ 2(0 + 0.5)       1 − 2(0 + 0.5)  0 ] = [ 1   0  0 ]
    [ 0                0               1 ]   [ 0   0  1 ]

S·Sᵀ = diag(4, 1, 0.25)
Σ = R · diag(4, 1, 0.25) · Rᵀ = diag(1, 4, 0.25)
```

The long axis (σ = 2, originally along x) now points along y. ✓

### 5. Storage

`Σ` is symmetric, so the reference rasterizer stores 6 floats: `(Σ00, Σ01, Σ02, Σ11, Σ12, Σ22)`. Keep a
`Mat3f` in the CPU code for clarity; the GPU tasks will pack it.

### 6. vnemath and GLM gotchas

- `vne::math::Quat` wraps GLM. Its constructor order is **`Quat(x, y, z, w)`**: w **last**. PLY files
  store `w` **first** (Task 03). Get this wrong and every splat is rotated incorrectly, but the image still
  "kind of" works, which makes it hard to spot.
- GLM matrices are **column-major**: `m[col][row]`. When you compare against a hand-written matrix, check
  you are indexing the way you think.
- Write your own `quatToRotationMatrix()` for learning, then test it against `Quatf::toMatrix3()`.

## Read

- **Paper** §4 "Differentiable 3D Gaussian Splatting": the paragraph on the covariance parametrization
  (equations 6 and 7 in the arXiv version).
- **Code**, `gaussian-splatting/utils/general_utils.py`: `build_rotation`, `build_scaling_rotation`,
  `strip_symmetric`. Note the `L @ L.transpose(1, 2)`.
- **Code**, `diff-gaussian-rasterization/cuda_rasterizer/forward.cu`: `computeCov3D`. Same math in CUDA,
  with GLM's column-major layout (`M = S * R`, then `Sigma = transpose(M) * M`). Work out why that
  equals `R·S·Sᵀ·Rᵀ`.

## Build

- [ ] `include/vertexnova/gs/core/gaussian.h`: one Gaussian, array-of-structs, for clarity.

  ```cpp
  namespace vne::gs {
  struct Gaussian {
      math::Vec3f position;
      math::Vec3f scale;        // activated: linear, > 0
      math::Quatf rotation;     // activated: unit length
      float opacity = 1.0f;     // activated: (0, 1)
      math::Vec3f color;        // placeholder until SH in Task 08
  };
  }  // namespace vne::gs
  ```

- [ ] `include/vertexnova/gs/core/covariance.h` + `src/vertexnova/gs/core/covariance.cpp`

  ```cpp
  [[nodiscard]] math::Mat3f quatToRotationMatrix(float w, float x, float y, float z);   // normalizes
  [[nodiscard]] math::Mat3f computeCovariance3D(const math::Vec3f& scale, const math::Quatf& rotation);
  [[nodiscard]] std::array<float, 6> packSymmetric(const math::Mat3f& m);  // (00, 01, 02, 11, 12, 22)
  ```

- [ ] `tests/covariance_test.cpp`

## Test

| Case | Expected |
|------|----------|
| Identity rotation, `s = (2, 1, 0.5)` | `Σ = diag(4, 1, 0.25)` |
| Worked example (90° about z) | `Σ = diag(1, 4, 0.25)` |
| Random `s`, `q` | `Σ == Σᵀ` (to 1e-6) |
| Random `s`, `q`, random unit `v` | `vᵀΣv ≥ 0` |
| Random `s`, `q`; `rᵢ` = column *i* of `R` | `Σ·rᵢ = sᵢ²·rᵢ` (eigen-pairs, no solver needed) |
| `quatToRotationMatrix(q)` vs `Quatf(x, y, z, w).toMatrix3()` | equal for 100 random quaternions |
| `q` vs `−q` | same `R` |
| Unnormalized `q = (2, 0, 0, 0)` | identity `R` |

## Done when

- [ ] All tests pass.
- [ ] You can explain, without notes, why 3DGS optimizes `(log s, q)` instead of `Σ`.

## Check yourself

1. Why `Σ = R·S·Sᵀ·Rᵀ` and not `S·R·Rᵀ·Sᵀ`?
2. How many degrees of freedom does a 3D covariance have, and how many numbers does 3DGS use for it?
3. Why a quaternion rather than Euler angles?
4. What happens when one scale goes to ~0? Is that a problem for rendering?
5. `Σ` has eigenvalues `(9, 1, 1)`. Describe the shape.

<details><summary>Answers</summary>

1. `S·R·Rᵀ·Sᵀ = S·Sᵀ` because `R·Rᵀ = I`: the rotation disappears and you get an axis-aligned ellipsoid.
   You must scale first (in the local frame), then rotate.
2. 6 (symmetric 3×3). 3DGS uses 7 (3 scales + 4 quaternion). The extra one is the quaternion's length,
   which normalization removes.
3. No gimbal lock, smooth everywhere (good for gradients), cheap to normalize, and no angle-wrapping.
4. The ellipsoid becomes a flat disk and `Σ` is singular in 3D. That's fine for rendering: the projected 2D
   covariance gets a `+0.3` dilation (Task 05). Trained scenes are full of these flat "surfels".
5. A cigar: σ = 3 along one axis, 1 along the other two.

</details>

## Going further

- Render a 3D Gaussian as a point cloud: sample 10,000 points from `N(μ, Σ)` (with `x = μ + R·S·z`,
  `z ~ N(0, I)`) and check that their sample covariance matches `Σ`.
- Derive `∂Σ/∂s` and `∂Σ/∂q`. You'll need them in Task 18.

## My notes

_Fill in after finishing._
