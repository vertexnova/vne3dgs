# 3D Gaussian Splatting in One Page

Read this before [Task 01](../tasks/01_gaussians_2d.md). Every term in **bold** is in the
[glossary](glossary.md).

## The problem

You walk around an object or a room taking 100–300 photos. You want to see it from **any new viewpoint**,
photorealistically, in real time. This is **novel view synthesis**.

Two earlier answers, and why 3DGS replaced them for this job:

| Approach | Representation | Rendering | Weakness |
|----------|----------------|-----------|----------|
| Photogrammetry mesh | Triangles + textures | Rasterization (fast) | Hard to reconstruct fine, fuzzy or shiny things (hair, foliage, reflections) |
| NeRF (2020) | A neural network: position + direction → color + density | Ray marching: hundreds of network queries per pixel | Slow to train, slow to render |
| **3DGS (2023)** | Millions of explicit, fuzzy **3D Gaussians** | Project + sort + blend (rasterizer-like) | Large files; artifacts far from the training views |

3DGS keeps NeRF's idea of a soft, semi-transparent scene trained by gradient descent, but swaps the
neural network for explicit primitives that a GPU can rasterize fast. Real-time at 1080p, with training
in minutes to an hour.

## The representation

A scene is a list of Gaussians, typically 1–6 million. Each one has:

| Parameter | Size | Stored in the `.ply` as | Turned into a usable value by |
|-----------|------|-------------------------|-------------------------------|
| Position (mean) μ | 3 | `x y z` | used as-is |
| Scale s | 3 | `scale_0..2` (log) | `exp` |
| Rotation q | 4 | `rot_0..3` (quaternion, w first, not normalized) | normalize |
| Opacity o | 1 | `opacity` (logit) | `sigmoid` |
| Color | 48 | `f_dc_0..2` + `f_rest_0..44` (**spherical harmonics**, 16 per RGB channel) | evaluate SH along the view direction |

That is 59 numbers per Gaussian. Scale and rotation together define its **covariance**: the shape
and orientation of an ellipsoid. Think of each Gaussian as a soft, colored, semi-transparent blob whose
density falls off smoothly from its center.

## Rendering (Tasks 04–13)

```
 Gaussians (world space)
      │  1. PREPROCESS, one per Gaussian (parallel)
      │     cull behind camera / outside view
      │     project center to pixels                          (Task 04)
      │     project 3D covariance → 2D ellipse (EWA)          (Task 05)
      │     evaluate SH → RGB for this view direction         (Task 08)
      ▼
 2D splats: (pixel center, ellipse "conic", radius, depth, RGB, opacity)
      │  2. BIN + SORT
      │     which 16×16 tiles does each splat touch?          (Task 07)
      │     one key per (tile, splat): [tile id | depth]
      │     sort all keys → each tile gets a depth-sorted list
      ▼
 per-tile, front-to-back lists
      │  3. BLEND, one per pixel (parallel)
      │     walk the tile's list front to back:
      │       α = opacity · gaussian falloff at this pixel
      │       color += T · α · rgb ;  T *= (1 − α)            (Tasks 01, 06)
      │     stop when T (light still getting through) ≈ 0
      ▼
 image
```

No triangles, no rays, no neural network. The expensive parts are the sort and the blending, which is why
the GPU tasks (10–13) focus on those.

## Training (Tasks 15–18)

```
 photos ──COLMAP──▶ camera poses + sparse 3D points
                                   │ initialize one Gaussian per point
                                   ▼
            ┌──────────── repeat ~30,000 times ────────────┐
            │ pick a training photo and its camera           │
            │ render the Gaussians from that camera          │
            │ loss = 0.8·L1 + 0.2·D-SSIM (render vs photo)   │
            │ backpropagate → gradient for every parameter   │
            │ Adam step on all 59 parameters of all Gaussians│
            │ every 100 iters: DENSIFY (clone / split        │
            │   Gaussians in under-reconstructed areas) and  │
            │   PRUNE (remove near-transparent ones)          │
            └────────────────────────────────────────────────┘
                                   ▼
                           trained .ply file
```

Training works because every rendering step above is **differentiable**: a small change in a Gaussian's
position, shape, opacity or color produces a predictable change in pixel colors.

## How the tasks map onto this page

| Stage | Tasks |
|-------|-------|
| One Gaussian: the falloff function and blending | 01, 02 |
| Reading real trained data | 03 |
| Preprocess: camera, projection, color | 04, 05, 08 |
| Sort + blend on the CPU | 06, 07 |
| The same pipeline on the GPU | 09–13 |
| Training | 15–18 |
