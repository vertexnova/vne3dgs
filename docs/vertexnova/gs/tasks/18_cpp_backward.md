# Task 18 — Your Own Backward Pass in C++ (Capstone)

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 5 — Training | [13](13_tile_rasterizer_gpu.md), [16](16_pytorch_trainer.md) | — | [ ] |

> **Goal:** derive and implement 3DGS gradients by hand, first for 2D Gaussians fitting a single image on the
> CPU, then (stretch) the full 3D backward pass in compute shaders, verified against PyTorch.

## Why this task

Autograd (Task 16) hides the most instructive part of 3DGS: *how* a pixel error flows back through blending,
the conic, the projection and SH into each Gaussian's parameters. Writing it yourself is the deepest
understanding you can get, and it's what the reference CUDA code (`backward.cu`) does. Treat it as a
capstone: the 2D part is very doable; the full 3D GPU trainer is a serious project.

## Learn

### 1. Part A: 2D image fitting (CPU)

Fit `N` 2D Gaussians to one image. Parameters per Gaussian: mean `(2)`, `log σ₁, log σ₂, θ` (covariance =
`R(θ)·diag(σ²)·R(θ)ᵀ`, Task 01 §2), color `(3)`, opacity logit `(1)`. The forward pass is Task 01/06's
blend. Loss: L2 or L1 over pixels. This contains every hard gradient except the 3D projection.

### 2. Backprop through front-to-back blending

For one pixel with splats `i = 1..n` (front to back):

```
C = Σᵢ cᵢ αᵢ Tᵢ + T_final · bg,       Tᵢ = Πⱼ<ᵢ (1 − αⱼ),   T_final = Πᵢ (1 − αᵢ)

∂C/∂cᵢ = αᵢ Tᵢ
∂C/∂αᵢ = Tᵢ · cᵢ − (1 / (1 − αᵢ)) · ( Σⱼ>ᵢ cⱼ αⱼ Tⱼ + T_final · bg )
```

The second term is "everything behind `i`, which `i` now occludes a bit more". Walking **back to front**
lets you accumulate that sum incrementally, and recover each `Tᵢ` from the stored final transmittance:
`Tᵢ = Tᵢ₊₁ / (1 − αᵢ)`. That's exactly why the forward pass stores `final_T` and `n_contrib` per pixel
(Task 13).

### 3. The rest of the chain

```
α = o · exp(power)                   ∂α/∂o = exp(power),  ∂α/∂power = α
                                     (if α hit the 0.99 cap or was skipped: gradient 0)
power = −½(A dx² + C dy²) − B dx dy  ∂power/∂(A, B, C), ∂power/∂μ₂D (via d = pixel − μ₂D)
conic = Σ₂D⁻¹                        dΣ⁻¹ = −Σ⁻¹ · dΣ · Σ⁻¹
Σ₂D (2D fit) = R(θ) diag(σ²) R(θ)ᵀ   ∂/∂σ, ∂/∂θ;  σ = exp(log σ)  → multiply by σ
o = sigmoid(ℓ)                       ∂o/∂ℓ = o(1 − o)
```

For **3D** (Part B) continue through Task 05 (`Σ₂D = J W Σ₃D Wᵀ Jᵀ`, and `J` depends on the mean `t`),
Task 02 (`Σ₃D` from scale and quaternion, including the quaternion normalization), and Task 08 (color
depends on SH coefficients **and** on the position, through the view direction).

### 4. Verify every piece

- **Finite differences:** central differences in **double precision**, `h ≈ 1e-6`, relative error < 1e-5.
  Test each function on its own (conic, α, blend, covariance, projection, SH) before chaining them.
- **PyTorch:** Task 16's `dump_reference.py` gives autograd gradients for a fixed scene and camera.
  Compare your C++ gradients with them in gtest.

### 5. Adam

```
m = β₁ m + (1 − β₁) g
v = β₂ v + (1 − β₂) g²
m̂ = m / (1 − β₁ᵗ),  v̂ = v / (1 − β₂ᵗ)
θ -= lr · m̂ / (√v̂ + ε)          β₁ = 0.9, β₂ = 0.999, ε = 1e-15 (the reference uses a tiny ε)
```

### 6. Part B: the 3D backward pass on the GPU (stretch)

- **Per-pixel kernel** (one workgroup per tile, like Task 13): walk each pixel's list back to front,
  compute per-splat gradients with respect to `μ₂D`, the conic, opacity and color.
- **Accumulate** into per-Gaussian gradient buffers: many pixels touch the same Gaussian, so you need
  **atomic float adds**. Portability warning: Metal has atomic float add on recent GPUs (MSL 3.0),
  Vulkan needs `VK_EXT_shader_atomic_float`, and WGSL has none (fall back to a compare-and-swap loop, or
  per-tile partial sums followed by a reduction pass). Decide per backend.
- **Per-Gaussian kernel:** chain `μ₂D`/conic/color gradients back to position, scale, rotation and SH
  (Tasks 05, 02, 08 in reverse), then Adam.
- **Densification** needs the per-Gaussian 2D mean-gradient norms: accumulate them in the same pass.

## Read

- **Math supplement** (Ye & Kanazawa): the backward sections, which give every formula above in one notation.
- **Code**, `diff-gaussian-rasterization/cuda_rasterizer/backward.cu`: `renderCUDA` (back-to-front walk,
  the `T = T / (1 − alpha)` recovery, the atomics), `computeCov2DCUDA`, `computeCov3D`,
  `computeColorFromSH` (backward versions).
- **Kingma & Ba**, *Adam*, Algorithm 1.

## Build

- [ ] **Part A:** `include/vertexnova/gs/train/fit2d.h` + `.cpp`: forward, backward, Adam for 2D Gaussians;
  `examples/18_fit_image/`: fit N Gaussians to a PNG, writing a snapshot every 100 steps.
- [ ] `tests/fit2d_grad_test.cpp`: finite-difference checks for each function.
- [ ] **Part B (stretch):** `shaders/backward_*.comp.glsl`, `include/vertexnova/gs/train/`
  (`adam.h`, `gpu_trainer.h`); `tests/gpu/backward_gpu_test.cpp` vs PyTorch dumps.

## Test

| Case | Expected |
|------|----------|
| `∂C/∂αᵢ` for a 3-splat pixel | matches finite differences (double, rel. err < 1e-5) |
| Gradients through conic inversion, covariance-from-(σ, θ), sigmoid | match finite differences |
| α at the 0.99 cap, or below 1/255 | gradient exactly 0 (and finite differences agree away from the kink) |
| Whole 2D pipeline on a 16×16 image, 5 Gaussians | matches finite differences for every parameter |
| (Part B) 3D gradients for Task 16's reference scene | match PyTorch within 1e-4 relative (float32) |

## Done when

- [ ] **Part A:** gradient tests pass; `18_fit_image` reproduces a 256×256 photo with a few thousand
  Gaussians to a PSNR you're happy with (record it and the loss curve).
- [ ] **Part B (stretch):** your GPU trainer reconstructs a small scene from COLMAP to within a few dB of
  Task 16/17 on the same data.

## Check yourself

1. Why walk back to front in the backward pass?
2. Why does the forward pass store `final_T` per pixel?
3. `∂C/∂αᵢ` has two terms. Explain each physically.
4. Why do color gradients also flow into the Gaussian's **position**?
5. Why are atomics needed in Part B, and what can you do on backends without float atomics?

<details><summary>Answers</summary>

1. The derivative for splat `i` needs the sum of contributions *behind* it; walking back to front builds
   that sum incrementally, in O(1) per splat.
2. To recover each `Tᵢ = Tᵢ₊₁ / (1 − αᵢ)` walking backwards, without storing every intermediate `T`.
3. First: more α shows more of `i`'s own color (weighted by what reaches it). Second: more α hides
   everything behind `i` (scaled by `1/(1 − αᵢ)` because each `Tⱼ` for `j > i` contains `(1 − αᵢ)`).
4. SH color depends on the view direction `normalize(position − camera)`.
5. Many pixels (threads) update the same Gaussian's gradient concurrently. Without float atomics: a CAS
   loop on the bit pattern, or per-tile partial sums reduced in a second pass.

</details>

## Going further

- Add densification to your 2D fitter (clone/split on the 2D mean gradient) and watch detail appear
  where the image is complex.
- Replace L2 with L1 + D-SSIM; derive SSIM's gradient or check it with finite differences.

## My notes

_Fill in after finishing._
