# vne3dgs

**3D Gaussian Splatting module for the VertexNova Engine**

A Python + CUDA module for loading, rendering, and experimenting with 3D Gaussian Splatting scenes — built as a research companion to [VertexNova](https://learnvertexnova.com), the multi-backend C++20 rendering engine.

## What this is

`vne3dgs` is a focused research module that:
- Loads pre-trained 3DGS scenes from `.ply` files
- Renders them using [gsplat](https://docs.gsplat.studio) — the CUDA-accelerated differentiable rasterizer
- Provides a clean Python API for experimenting with Gaussian scene representations
- Serves as the Python-side research companion before the C++/Vulkan integration in VertexNova

## Platform

Built and tested on **NVIDIA DGX Spark** (GB10 Grace Blackwell):
- ARM64 / aarch64
- CUDA 13.0 · SM 12.0
- PyTorch 2.7 · gsplat 1.5.3
- Container: `vertexnova/gsplat-spark:v1`

## Quick start

```bash
# Clone
git clone https://github.com/vertexnova/vne3dgs.git
cd vne3dgs

# Install dependencies
pip install gsplat plyfile numpy imageio --no-build-isolation

# Run first render demo
python demo/render_scene.py \
  --ply /path/to/point_cloud.ply \
  --output output/render.png
```

## Project structure

```
vne3dgs/
├── src/
│   └── vne3dgs/
│       ├── __init__.py       # public API
│       ├── loader.py         # .ply scene loader
│       ├── renderer.py       # gsplat-based renderer
│       └── camera.py         # camera utilities
├── demo/
│   └── render_scene.py       # first render demo
├── tests/
│   └── test_loader.py        # smoke tests
├── docs/
│   └── notes.md              # research notes
├── output/                   # rendered images (gitignored)
├── requirements.txt
└── README.md
```

## Part of VertexNova

- Engine: [learnvertexnova.com](https://learnvertexnova.com)
- GitHub org: [github.com/vertexnova](https://github.com/vertexnova)
- Research site: [vertexnova.github.io](https://vertexnova.github.io)

## Author

Ajeet Yadav · Principal Engineer · [linkedin.com/in/ajeet-yadav-b1133991](https://linkedin.com/in/ajeet-yadav-b1133991)
