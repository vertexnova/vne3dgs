# vne3dgs

**3D Gaussian Splatting module for the VertexNova Engine**

A Python + CUDA library for loading and rendering 3D Gaussian Splatting scenes —
built as a research companion to [VertexNova](https://learnvertexnova.com),
the multi-backend C++20 rendering engine.

## First render — garden scene

> 5,834,784 Gaussians · rendered in **133ms** · 1280×720 · DGX Spark GB10

```
Scene center: [-0.024  1.781  1.432]
Scene spread: 9.215
Load time:    0.60s
Render time:  133.3ms
Alpha coverage: 64.4%
```

## What this is

`vne3dgs` provides a clean Python API to:

- Load pre-trained 3DGS scenes from standard `.ply` files
- Render them using [gsplat](https://docs.gsplat.studio) — the CUDA-accelerated differentiable rasterizer
- Orbit cameras, control viewpoints, composite backgrounds
- Serve as the Python research layer before the C++/Vulkan integration in VertexNova

## Platform

Built and verified on **NVIDIA DGX Spark** (GB10 Grace Blackwell):

| Component | Version |
|-----------|---------|
| OS | DGX OS 7.4.0 (Ubuntu 24.04) |
| GPU | NVIDIA GB10 · SM 12.0 |
| CUDA | 13.0.2 |
| Driver | 580.142 |
| Architecture | ARM64 / aarch64 |
| PyTorch | 2.7.0 (NVIDIA build) |
| gsplat | 1.5.3 |
| Container | `vertexnova/gsplat-spark:v2` |

## Install

```bash
# Requires gsplat — on DGX Spark build from source
export TORCH_CUDA_ARCH_LIST="12.0"
pip install git+https://github.com/nerfstudio-project/gsplat.git --no-build-isolation

# Install vne3dgs
pip install plyfile numpy imageio
git clone https://github.com/vertexnova/vne3dgs.git
cd vne3dgs
```

## Quick start

```python
from vne3dgs import load_ply, render, make_intrinsics, orbit_camera, scene_center
import imageio

# Load a pre-trained scene
scene = load_ply("/path/to/point_cloud.ply")
print(scene)
# GaussianScene(n=5,834,784 Gaussians, device=cuda:0)

# Set up camera
center = scene_center(scene.means)
K = make_intrinsics(1280, 720, fov_deg=60.0)
viewmat = orbit_camera(center, radius=18.0, elevation_deg=20.0, azimuth_deg=45.0)

# Render
image, alpha = render(scene, viewmat, K, width=1280, height=720)

# Save
imageio.imwrite("render.png", image)
```

## Demo

```bash
# Render garden scene (download pre-trained models first)
python demo/render_scene.py \
  --ply /path/to/garden/point_cloud/iteration_30000/point_cloud.ply \
  --output output/garden_render.png \
  --width 1280 \
  --height 720 \
  --elevation 20 \
  --azimuth 45
```

All arguments:

| Argument | Default | Description |
|----------|---------|-------------|
| `--ply` | garden scene | Path to `.ply` file |
| `--output` | `output/garden_render.png` | Output PNG path |
| `--width` | 1280 | Image width |
| `--height` | 720 | Image height |
| `--fov` | 60.0 | Field of view (degrees) |
| `--elevation` | 20.0 | Camera elevation (degrees) |
| `--azimuth` | 45.0 | Camera azimuth (degrees) |
| `--radius` | auto | Orbit radius (auto = 2× scene spread) |
| `--device` | cuda | PyTorch device |

## Tests

```bash
python tests/test_loader.py
```

```
========================================
vne3dgs smoke tests
========================================
Device: cuda (NVIDIA GB10)

test_load_ply ...        PASS
test_scene_stats ...     PASS
test_make_intrinsics ... PASS
test_orbit_camera ...    PASS
test_render_smoke ...    PASS

All tests passed.
========================================
```

## Project structure

```
vne3dgs/
├── src/
│   └── vne3dgs/
│       ├── __init__.py       # public API
│       ├── loader.py         # .ply scene loader → GaussianScene
│       ├── renderer.py       # gsplat rasterization wrapper
│       └── camera.py         # intrinsics, look_at, orbit_camera
├── demo/
│   └── render_scene.py       # CLI render demo
├── tests/
│   └── test_loader.py        # smoke tests
├── output/                   # rendered images (gitignored)
├── requirements.txt
└── README.md
```

## How it works

3DGS represents a scene as millions of 3D Gaussian ellipsoids — each with a position,
scale, rotation, opacity, and color. Rendering projects these Gaussians onto the image
plane and alpha-composites them front-to-back using tile-based rasterization.

gsplat implements this pipeline in CUDA with differentiable gradients,
enabling both inference (viewing) and training (optimizing a scene from photos).

`vne3dgs` wraps gsplat's `rasterization()` API with a clean interface that loads
standard `.ply` files and handles the coordinate/data-type conventions automatically
(log-space scales, logit opacities, SH DC to RGB conversion).

## Roadmap

- [ ] Orbit animation — render full 360° as video
- [ ] Better camera placement — auto-fit to scene bounds
- [ ] Spherical harmonics — view-dependent color (beyond DC term)
- [ ] Vulkan port — real-time C++ renderer in VertexNova
- [ ] Surgical scene reconstruction — COLMAP + 3DGS on medical data

## Part of VertexNova

| Link | |
|------|-|
| Engine docs | [learnvertexnova.com](https://learnvertexnova.com) |
| Research site | [vertexnova.github.io](https://vertexnova.github.io) |
| GitHub org | [github.com/vertexnova](https://github.com/vertexnova) |
| Engine repo | [github.com/vertexnova/vnetestbed](https://github.com/vertexnova/vnetestbed) |

## Author

**Ajeet Yadav** · Principal Engineer · Stryker  
M.Tech Signal Processing · IIT Kanpur  
[linkedin.com/in/ajeet-yadav-b1133991](https://linkedin.com/in/ajeet-yadav-b1133991) ·
[learnvertexnova.com](https://learnvertexnova.com)

## License

MIT
