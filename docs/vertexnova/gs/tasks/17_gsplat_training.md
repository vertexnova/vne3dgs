# Task 17 — Full-Scale Training with gsplat

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 5 — Training | [16](16_pytorch_trainer.md) | — | [ ] |

> **Goal:** train full-size scenes with a production trainer on the DGX Spark, and use your own viewer to
> understand how a scene evolves during training.

## Why this task

Your Task 16 trainer is readable but slow. gsplat is fast (CUDA) and well tested, so you can train the real
Mip-NeRF 360 scenes in minutes, and even your own captures. You already know what every setting means,
so this is now a tool rather than a black box.

## Learn

### 1. What to watch

| Metric | Typical for 3DGS on Mip-NeRF 360 outdoor | Meaning |
|--------|------------------------------------------|---------|
| PSNR (test views) | ~24–27 dB | pixel accuracy |
| SSIM | ~0.7–0.85 | structural similarity |
| LPIPS | ~0.2–0.3 (lower is better) | perceptual difference |
| Gaussian count | grows until 15k iterations, then flat | densification at work |

Always evaluate on **held-out** views (every 8th image is the usual split); training-view PSNR flatters.

### 2. Checkpoints tell the story

Export at iterations **1k, 7k and 30k** (the reference trainer evaluates at 7k and 30k by default), open
each in your viewer, and look:

- **1k:** few, large, blurry Gaussians around the COLMAP points; mostly DC color (SH degree 1 has just started).
- **7k:** most of the structure present; densification active; some floaters.
- **30k:** sharp; full SH; floaters mostly pruned.

Use your Task 14 debug views (scale, opacity, splats per pixel) on each.

### 3. Densification strategies

- **Default (the paper's):** clone/split/prune as in Task 16.
- **MCMC** (*3D Gaussian Splatting as Markov Chain Monte Carlo*, Kheradmand et al., NeurIPS 2024): a
  fixed Gaussian budget, "relocating" dead Gaussians instead of heuristic densification. gsplat offers it
  as a strategy. Compare the two on one scene: quality, count, speed.

### 4. Anti-aliased training

gsplat's `antialiased` rasterize mode trains with the opacity compensation from Task 14 item C. Models
trained this way look correct when zoomed out in a viewer using the same mode.

## Read

- **gsplat docs**: `rasterization()` and the example trainer (`examples/simple_trainer.py`) options.
- **gsplat paper** (Ye et al. 2024): §3–4 for what the library adds beyond the original.
- **MCMC paper**, §3 (optional).

## Build

- [ ] `python/scripts/train_gsplat.sh`: runs gsplat's trainer on the Spark (the prototype's
  `vertexnova/gsplat-spark:v2` container has gsplat built for GB10 / SM 12.0).
- [ ] A converter from gsplat checkpoints to Inria-layout PLY (reuse Task 16's exporter), if gsplat's own
  export doesn't already produce that layout.
- [ ] Viewer: load several PLYs and flip between them (keys 1/2/3) at the same camera.

## Test

| Case | Expected |
|------|----------|
| gsplat-trained garden at 30k, test views | PSNR within ~0.5 dB of published numbers for the same settings |
| Exported PLY → `example_03_ply_stats` | loads; count matches the trainer's log |
| Same PLY in your viewer vs gsplat's own render, same camera | PSNR > 40 dB (small differences from AA mode or version details are fine; document them) |

## Done when

- [ ] Garden (or another Mip-NeRF 360 scene) trained on the Spark; metrics and training time recorded.
- [ ] The 1k / 7k / 30k checkpoints in your viewer, with a paragraph per checkpoint in *My notes*
  explaining what you see and why.
- [ ] (Optional) Your own capture: photos → COLMAP → gsplat → your viewer.

## Check yourself

1. Why evaluate on held-out views?
2. Why does the Gaussian count stop growing after iteration 15k?
3. What's the practical difference between default densification and MCMC?

<details><summary>Answers</summary>

1. The model can overfit the training photos (especially with many Gaussians); held-out views measure
   novel-view quality, which is the actual goal.
2. Densification is scheduled to stop at 15k; after that the trainer only refines existing Gaussians
   (and prunes).
3. Default grows the count heuristically based on gradients and can explode on some scenes; MCMC keeps a
   fixed budget and moves low-value Gaussians to where they're needed, so memory and performance are
   predictable.

</details>

## My notes

_Fill in after finishing._
