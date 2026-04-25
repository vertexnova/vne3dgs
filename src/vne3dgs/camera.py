"""
vne3dgs.camera
--------------
Camera utilities for 3DGS rendering.

Conventions (following gsplat):
  - viewmat: world-to-camera transform [4, 4]
  - K:       camera intrinsics [3, 3]
  - +X right, +Y down, +Z forward (OpenCV convention)
"""

import torch
import numpy as np
import math


def make_intrinsics(
    width: int,
    height: int,
    fov_deg: float = 60.0,
    device: str = "cuda",
) -> torch.Tensor:
    """
    Build a simple pinhole camera intrinsics matrix K.

    Args:
        width:    image width in pixels
        height:   image height in pixels
        fov_deg:  horizontal field of view in degrees (default 60)
        device:   torch device

    Returns:
        K [1, 3, 3] camera intrinsics
    """
    fov_rad = math.radians(fov_deg)
    fx = (width / 2.0) / math.tan(fov_rad / 2.0)
    fy = fx
    cx = width / 2.0
    cy = height / 2.0

    K = torch.tensor([[
        [fx,  0, cx],
        [ 0, fy, cy],
        [ 0,  0,  1],
    ]], dtype=torch.float32, device=device)

    return K


def look_at(
    eye: list,
    target: list = [0, 0, 0],
    up: list = [0, -1, 0],
    device: str = "cuda",
) -> torch.Tensor:
    """
    Build a world-to-camera view matrix using look-at convention.

    Args:
        eye:    camera position in world space [x, y, z]
        target: point to look at [x, y, z]
        up:     world up vector (default [0, -1, 0] for OpenCV Y-down)
        device: torch device

    Returns:
        viewmat [1, 4, 4] world-to-camera transform
    """
    eye = np.array(eye, dtype=np.float32)
    target = np.array(target, dtype=np.float32)
    up = np.array(up, dtype=np.float32)

    z = target - eye
    z = z / np.linalg.norm(z)

    x = np.cross(z, up)
    x_norm = np.linalg.norm(x)
    if x_norm < 1e-6:
        # Handle degenerate case — camera looking straight up/down
        up = np.array([0, 0, 1], dtype=np.float32)
        x = np.cross(z, up)
        x_norm = np.linalg.norm(x)
    x = x / x_norm

    y = np.cross(z, x)
    y = y / np.linalg.norm(y)

    # World-to-camera rotation
    R = np.stack([x, y, z], axis=0)  # [3, 3]

    # Translation in camera space
    t = -R @ eye  # [3]

    viewmat = np.eye(4, dtype=np.float32)
    viewmat[:3, :3] = R
    viewmat[:3,  3] = t

    return torch.tensor(viewmat, dtype=torch.float32, device=device).unsqueeze(0)


def scene_center(means: torch.Tensor) -> torch.Tensor:
    """Estimate scene center as median of Gaussian positions."""
    return means.median(dim=0).values


def orbit_camera(
    center: torch.Tensor,
    radius: float,
    elevation_deg: float = 20.0,
    azimuth_deg: float = 45.0,
    device: str = "cuda",
) -> torch.Tensor:
    """
    Position a camera on a sphere orbiting around center.

    Args:
        center:        scene center [3]
        radius:        distance from center
        elevation_deg: camera elevation above horizon
        azimuth_deg:   camera rotation around vertical axis
        device:        torch device

    Returns:
        viewmat [1, 4, 4]
    """
    el = math.radians(elevation_deg)
    az = math.radians(azimuth_deg)

    eye = center.cpu().numpy() + radius * np.array([
        math.cos(el) * math.sin(az),
        -math.sin(el),
        math.cos(el) * math.cos(az),
    ], dtype=np.float32)

    return look_at(
        eye=eye.tolist(),
        target=center.cpu().tolist(),
        up=[0, -1, 0],
        device=device,
    )
