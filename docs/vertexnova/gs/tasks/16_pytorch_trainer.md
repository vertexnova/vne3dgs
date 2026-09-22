# Task 16 — A PyTorch Trainer You Can Read

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 5 — Training | [08](08_spherical_harmonics.md), [15](15_colmap_cameras.md) | [17](17_gsplat_training.md), [18](18_cpp_backward.md) | [ ] |

> **Goal:** write a small, slow, fully readable 3DGS trainer in PyTorch: pure-PyTorch rendering, autograd
> gradients, densification. Train a small scene and view the result in your C++ viewer.

## Why this task

Training is where 3DGS stops being "a fancy renderer". Using autograd means you write only the forward
pass (which you already know in detail from Tasks 01–08) and get every gradient for free. You'll learn
the loss, the optimizer setup and densification, which a big library would hide.

This is the start of `python/` in the repo (see the roadmap principles).

## Learn

### 1. The training loop

```
init Gaussians from COLMAP points (Task 15)
for it in 1..30000:
    cam, photo = random training view
    image = render(gaussians, cam)                  # your PyTorch forward pass
    loss  = 0.8 · L1(image, photo) + 0.2 · (1 − SSIM(image, photo))
    loss.backward()
    record the 2D position-gradient magnitude per Gaussian   (for densification)
    optimizer.step(); optimizer.zero_grad()
    update position LR (exponential decay); every 1000 its: sh_degree = min(sh_degree + 1, 3)
    densify / prune / opacity reset on schedule (§4)
```

### 2. Parameters and initialization

Store **raw** (pre-activation) values as `nn.Parameter`s, exactly the quantities in the PLY:

| Parameter | Shape | Init |
|-----------|-------|------|
| `means` | N×3 | COLMAP point positions |
| `log_scales` | N×3 | `log(mean distance to the 3 nearest neighbours)`, the same on all 3 axes |
| `quats` | N×4 | `(1, 0, 0, 0)` |
| `opacity_logits` | N×1 | `logit(0.1)` |
| `sh_dc` | N×1×3 | `(rgb − 0.5) / C0` (inverse of Task 08's DC formula) |
| `sh_rest` | N×15×3 | zeros |

### 3. Optimizer: Adam with per-parameter learning rates

Values from the reference implementation (they've changed slightly between versions):

| Parameter | LR |
|-----------|----|
| means | `1.6e-4 × extent`, decaying exponentially to `1.6e-6 × extent` by iteration 30k (`extent` from Task 15) |
| sh_dc | `2.5e-3` |
| sh_rest | `2.5e-3 / 20` |
| opacity | `0.025` (the paper used 0.05) |
| scales | `5e-3` |
| rotations | `1e-3` |

### 4. Adaptive density control

The initial point cloud is sparse, so the trainer must **add** Gaussians where detail is missing and
**remove** useless ones:

- **Signal:** the running average of the magnitude of each Gaussian's gradient with respect to its
  **2D projected position**. Large means "this area is poorly fit".
- **Every 100 iterations, from 500 to 15,000:**
  - average gradient > `0.0002` and max scale ≤ `0.01 × extent` → **clone** (copy the Gaussian; the
    optimizer moves the copies apart)
  - average gradient > `0.0002` and max scale > `0.01 × extent` → **split** into 2: sample new centers
    from the Gaussian itself, divide scales by 1.6
  - **prune**: opacity < `0.005`, or too large (in screen space or world space)
- **Every 3,000 iterations:** reset every opacity to `min(opacity, 0.01)`. This clears floaters: only
  Gaussians the loss actually needs regain opacity.
- **Optimizer state:** when Gaussians are added or removed, the Adam moment tensors must be extended or
  masked the same way. The reference code does this carefully; so must you.

### 5. A readable renderer in PyTorch

Write the forward pass with tensor ops (projection → conic → per-pixel α → cumulative product for `T`):

- **Projection and SH:** vectorized over Gaussians, a direct port of Tasks 05 and 08.
- **Blending:** for an `H×W` image and K candidate Gaussians per pixel (or per tile), build α as an
  `[H·W, K]` tensor sorted by depth, then `T = cumprod(1 − α)` shifted by one, and `C = Σ T·α·c`.
- **Memory limits you:** keep it small (e.g. 200×150 images, ≤ 100k Gaussians), process tiles in chunks,
  or restrict K to each tile's top-K by depth. It's slow, and that's fine.
- gsplat ships **pure-PyTorch reference implementations** of its kernels (look for `_torch_impl.py`
  in the gsplat source). Use them to check yours.

### 6. Where it runs

The Mac (CPU or MPS) for tiny experiments; the DGX Spark (CUDA) for anything bigger. Keep a `--device` flag.

### 7. Exports that tie back to C++

- **PLY** in the Inria layout (Task 03's writer format) → open it in your viewer.
- **Reference dumps** for a fixed camera: projected means, conics, radii, colors, the rendered image and,
  for Task 18, the gradients with respect to every parameter. Write them as raw `.bin` files plus a JSON
  header, easy to read from gtest.

## Read

- **Paper** §5 (optimization) and §5.2 (adaptive density control).
- **Code**, `gaussian-splatting/train.py` (loop, loss, schedule) and `scene/gaussian_model.py`
  (`create_from_pcd`, `training_setup`, `densify_and_clone`, `densify_and_split`, `prune_points`,
  `reset_opacity`, and `replace_tensor_to_optimizer` / `cat_tensors_to_optimizer` for the Adam-state
  surgery).
- **Code**, `gaussian-splatting/utils/loss_utils.py`: `l1_loss`, `ssim` (11×11 Gaussian window, σ = 1.5).
- **Kingma & Ba**, *Adam*, Algorithm 1.

## Build

- [ ] `python/pyproject.toml` (torch, numpy, plyfile or your own writer, imageio), `python/README.md`
- [ ] `python/vne3dgs_train/`: `colmap.py`, `model.py` (parameters, activations, PLY export),
  `render.py` (pure-PyTorch forward), `losses.py` (L1, SSIM), `densify.py`, `train.py` (CLI)
- [ ] `python/tests/`: `test_render.py`, `test_losses.py`, `test_ply.py`
- [ ] `python/scripts/dump_reference.py`: writes reference values for the C++ tests

## Test

| Case | Expected |
|------|----------|
| `torch.autograd.gradcheck` on a 3-Gaussian, 8×8 render in float64 | passes |
| `SSIM(x, x)` | 1 |
| PLY export → C++ `readGaussianPly` (via `example_03_ply_stats`) | same count and parameters |
| Your forward pass vs the C++ tiled renderer on the same scene and camera | max abs diff < 1e-4 (float32) |
| Densify on a toy scene with one high-gradient Gaussian | clone vs split chosen by its scale; Adam state has the right length |

## Done when

- [ ] Tests pass.
- [ ] On a small scene (e.g. Tanks & Temples *truck* at 1/8 resolution, or a synthetic scene), the loss
  falls and the training-view PSNR rises steadily; Gaussian count vs iteration is plotted.
- [ ] The exported PLY opens and looks right in your C++ viewer.

## Check yourself

1. Why optimize raw (pre-activation) values?
2. What signal decides where to densify, and why that one?
3. Clone vs split: what decides it, and what does each fix?
4. What problem does the periodic opacity reset solve?
5. What goes wrong if you add Gaussians without extending Adam's state?

<details><summary>Answers</summary>

1. The raw values are unconstrained reals, so gradient steps can never produce invalid scales, opacities
   or rotations (Task 02 §2).
2. The gradient of the loss with respect to the 2D projected position: large where the image is poorly
   fit, meaning the Gaussians there "want" to move, which suggests missing geometry.
3. Scale. Small Gaussians are **cloned** (under-reconstruction: not enough Gaussians). Large ones are
   **split** (over-reconstruction: one blob covering too much detail).
4. Floaters and over-dense regions: after a reset only Gaussians the loss needs regain opacity; the rest
   fall below the prune threshold.
5. Shape mismatches, or worse, new Gaussians inheriting the wrong momentum: erratic training.

</details>

## Going further

- Plot a histogram of Gaussian count, opacity and scale every 1,000 iterations and watch densification
  work.
- Replace the pure-PyTorch blend with gsplat's `rasterization()` and compare speed. That's Task 17's
  starting point.

## My notes

_Fill in after finishing._
