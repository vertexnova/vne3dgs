"""
vne3dgs — 3D Gaussian Splatting module for VertexNova
======================================================

Quick start:
    from vne3dgs import load_ply, render, make_intrinsics, orbit_camera

    scene = load_ply("/workspace/datasets/garden/point_cloud/iteration_30000/point_cloud.ply")
    K = make_intrinsics(1280, 720, fov_deg=60)
    viewmat = orbit_camera(scene.means.median(dim=0).values, radius=5.0)
    image, alpha = render(scene, viewmat, K, width=1280, height=720)
"""

__version__ = "0.1.0"

from .loader import load_ply, GaussianScene
from .renderer import render, render_orbit
from .camera import make_intrinsics, look_at, orbit_camera, scene_center

__all__ = [
    "load_ply",
    "GaussianScene",
    "render",
    "render_orbit",
    "make_intrinsics",
    "look_at",
    "orbit_camera",
    "scene_center",
]
