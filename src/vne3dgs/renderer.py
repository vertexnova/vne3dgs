"""
vne3dgs.renderer
----------------
Thin wrapper around gsplat.rasterization for rendering GaussianScene objects.
"""

import torch
import numpy as np
from typing import Optional, Tuple
from gsplat import rasterization

from .loader import GaussianScene
from .camera import make_intrinsics, orbit_camera, scene_center


def render(
    scene: GaussianScene,
    viewmat: torch.Tensor,
    K: torch.Tensor,
    width: int,
    height: int,
    background: Optional[torch.Tensor] = None,
    near_plane: float = 0.01,
    far_plane: float = 1000.0,
) -> Tuple[torch.Tensor, torch.Tensor]:
    if background is None:
        background = torch.ones(3, device=scene.means.device)

    # Render without background — composite manually after
    renders, alphas, _ = rasterization(
        means=scene.means,
        quats=scene.quats,
        scales=scene.scales,
        opacities=scene.opacities,
        colors=scene.colors,
        viewmats=viewmat,
        Ks=K,
        width=width,
        height=height,
        near_plane=near_plane,
        far_plane=far_plane,
    )

    # renders: [1, H, W, 3], alphas: [1, H, W, 1]
    # Composite: out = render * alpha + background * (1 - alpha)
    alpha = alphas[0]                          # [H, W, 1]
    bg = background.view(1, 1, 3)              # [1, 1, 3]
    composited = renders[0] * alpha + bg * (1.0 - alpha)  # [H, W, 3]

    image = (composited.clamp(0, 1).cpu().numpy() * 255).astype(np.uint8)

    return image, alpha[:, :, 0].cpu()


def render_orbit(
    scene: GaussianScene,
    width: int = 1280,
    height: int = 720,
    n_frames: int = 36,
    radius_scale: float = 1.5,
    elevation_deg: float = 20.0,
    fov_deg: float = 60.0,
) -> list:
    center = scene_center(scene.means)
    spread = (scene.means - center).norm(dim=-1).mean().item()
    radius = spread * radius_scale

    K = make_intrinsics(width, height, fov_deg, device=scene.means.device)

    frames = []
    for i in range(n_frames):
        azimuth = (360.0 / n_frames) * i
        viewmat = orbit_camera(center, radius, elevation_deg, azimuth, device=scene.means.device)
        image, _ = render(scene, viewmat, K, width, height)
        frames.append(image)
        if (i + 1) % 9 == 0:
            print(f"  Rendered {i + 1}/{n_frames} frames")

    return frames
