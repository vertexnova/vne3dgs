# References

Every task links back here. Entries say **what to use them for**, so you know what to skip.

## Core papers

| Paper | Use it for | Tasks |
|-------|-----------|-------|
| Kerbl, Kopanas, Leimkühler, Drettakis. *3D Gaussian Splatting for Real-Time Radiance Field Rendering.* ACM TOG (SIGGRAPH) 2023. [arXiv:2308.04079](https://arxiv.org/abs/2308.04079) · [project page](https://repo-sam.inria.fr/fungraph/3d-gaussian-splatting/) | The method. §4 representation, §5 optimization + densification, §6 tile rasterizer. Short; read it twice, at Task 02 and again at Task 06. | 02–08, 16 |
| Ye, Kanazawa. *Mathematical Supplement for the gsplat Library.* 2023. [arXiv:2312.02121](https://arxiv.org/abs/2312.02121) | Every forward formula (projection, covariance, blending) and every gradient, with consistent notation. The best single math reference. | 05, 06, 18 |
| Ye et al. *gsplat: An Open-Source Library for Gaussian Splatting.* 2024. [arXiv:2409.06765](https://arxiv.org/abs/2409.06765) | How a production library organizes the pipeline; features beyond the original paper. | 13, 17 |
| Zwicker, Pfister, van Baar, Gross. *EWA Volume Splatting.* IEEE Visualization 2001; and *EWA Splatting*, IEEE TVCG 2002. | Where `Σ₂D = J W Σ Wᵀ Jᵀ` comes from, framed as resampling/anti-aliasing. | 05 |
| Yu, Chen, Huang, Sattler, Geiger. *Mip-Splatting: Alias-free 3D Gaussian Splatting.* CVPR 2024. [arXiv:2311.16493](https://arxiv.org/abs/2311.16493) | Why the `+0.3` dilation causes aliasing when zooming, and the principled fix. | 05, 14 |

## Background

| Reference | Use it for | Tasks |
|-----------|-----------|-------|
| Mildenhall et al. *NeRF.* ECCV 2020. [arXiv:2003.08934](https://arxiv.org/abs/2003.08934) | Context: the volume-rendering equation that 3DGS blending approximates. §4 only. | overview, 06 |
| Barron et al. *Mip-NeRF 360.* CVPR 2022. [arXiv:2111.12077](https://arxiv.org/abs/2111.12077) | The dataset you'll use; skim for how scenes were captured. | 15–17 |
| Schönberger, Frahm. *Structure-from-Motion Revisited.* CVPR 2016. | How COLMAP gets poses and points. | 15 |
| Sloan. *Stupid Spherical Harmonics (SH) Tricks.* GDC 2008. | The most practical SH introduction for graphics programmers; basis functions and constants. | 08 |
| Ramamoorthi, Hanrahan. *An Efficient Representation for Irradiance Environment Maps.* SIGGRAPH 2001. | SH in rendering, and why low orders are enough for smooth lighting. | 08 |
| Kingma, Ba. *Adam: A Method for Stochastic Optimization.* [arXiv:1412.6980](https://arxiv.org/abs/1412.6980) | The optimizer, Algorithm 1 (one page). | 16, 18 |
| Wang, Bovik, Sheikh, Simoncelli. *Image Quality Assessment: From Error Visibility to Structural Similarity.* IEEE TIP 2004. | SSIM, used in the loss. | 16 |

## GPU sorting and scan

| Reference | Use it for | Tasks |
|-----------|-----------|-------|
| Blelloch. *Prefix Sums and Their Applications.* CMU-CS-90-190, 1990. | Up-sweep / down-sweep scan, clearly explained. | 11 |
| Harris, Sengupta, Owens. *Parallel Prefix Sum (Scan) with CUDA.* GPU Gems 3, ch. 39. | Work-efficient scan in shared memory, bank conflicts. Ideas carry over to GLSL compute. | 11 |
| Merrill, Grimshaw. *High Performance and Scalable Radix Sorting.* Parallel Processing Letters, 2011. | The classic GPU radix sort structure (upsweep / scan / downsweep per digit). | 11 |
| Adinets, Merrill. *Onesweep: A Faster Least Significant Digit Radix Sort for GPUs.* 2022. [arXiv:2206.01784](https://arxiv.org/abs/2206.01784) | State of the art. Needs forward-progress guarantees that not every backend gives; read, don't start here. | 11 |

## Reference code

Read these; don't copy them. Each task names the specific files to open.

| Code | Use it for |
|------|-----------|
| [graphdeco-inria/gaussian-splatting](https://github.com/graphdeco-inria/gaussian-splatting) | The original training code: `scene/gaussian_model.py` (parameters, PLY save/load, densification), `train.py` (loop, loss, schedule), `utils/sh_utils.py` (SH constants). |
| [graphdeco-inria/diff-gaussian-rasterization](https://github.com/graphdeco-inria/diff-gaussian-rasterization) | The original CUDA rasterizer: `cuda_rasterizer/forward.cu` (preprocess + blend), `rasterizer_impl.cu` (key duplication, sort, tile ranges), `auxiliary.h` (constants, helpers), `backward.cu` (gradients). |
| [nerfstudio-project/gsplat](https://github.com/nerfstudio-project/gsplat) · [docs](https://docs.gsplat.studio) | Clean modern implementation; the trainer used in Task 17. |
| [antimatter15/splat](https://github.com/antimatter15/splat) | A tiny WebGL viewer that draws splats as instanced quads with back-to-front blending: Task 12's approach in about 1,000 lines. |
| vnerhi samples `16_compute`, `17_compute_to_render`, `35_point_rendering`, `33_oit_weighted_blended` (in `../vnerhi/samples/`) | How compute, storage buffers, point rendering and blending are done on your own RHI. |

## Friendly introductions

- Hugging Face blog: *Introduction to 3D Gaussian Splatting* (2023). A visual walkthrough; good right after
  [overview.md](overview.md).
- The paper's project page has videos comparing 3DGS with NeRF variants; worth five minutes.

## Datasets

Links as of 2026; if one moves, check the gaussian-splatting repository README.

| Data | What | Size | Link |
|------|------|------|------|
| Inria pre-trained models | Trained `.ply` scenes (Mip-NeRF 360, Tanks & Temples, Deep Blending). Use `garden/point_cloud/iteration_30000/point_cloud.ply` from Task 03. | ~14 GB | [models.zip](https://repo-sam.inria.fr/fungraph/3d-gaussian-splatting/datasets/pretrained/models.zip) |
| Mip-NeRF 360 | Photos + COLMAP for 9 scenes (garden, bicycle, stump, room, counter, kitchen, bonsai, …) | ~12 GB | [project page](https://jonbarron.info/mipnerf360/) (`360_v2.zip`) |
| Tanks & Temples + Deep Blending | Photos + COLMAP, as used in the paper | ~650 MB | [tandt_db.zip](https://repo-sam.inria.fr/fungraph/3d-gaussian-splatting/datasets/input/tandt_db.zip) |

Tips:

- For CPU tasks (04–08), crop or subsample the garden scene, or render at 320×180; a 5.8M-Gaussian scene at
  1080p is slow on the CPU.
- Tanks & Temples *truck* and *train* are smaller and train faster; good for Tasks 16–17.
- Keep data under the git-ignored `data/` folder or outside the repo.

## Formats

- COLMAP output formats (`cameras.bin`, `images.bin`, `points3D.bin` and text equivalents):
  [colmap.github.io/format.html](https://colmap.github.io/format.html)
- PLY format: Paul Bourke's description, [paulbourke.net/dataformats/ply](https://paulbourke.net/dataformats/ply/)
