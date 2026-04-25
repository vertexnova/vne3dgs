"""
tests/test_loader.py
--------------------
Smoke tests for vne3dgs — verifies core functionality without needing a real .ply file.
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

import torch
import numpy as np
import tempfile
from plyfile import PlyData, PlyElement

from vne3dgs import load_ply, render, make_intrinsics, orbit_camera, scene_center
from vne3dgs.loader import GaussianScene


def make_fake_ply(path: str, n: int = 100):
    """Create a minimal valid 3DGS .ply file for testing."""
    rng = np.random.default_rng(42)
    dtype = [
        ('x', 'f4'), ('y', 'f4'), ('z', 'f4'),
        ('opacity', 'f4'),
        ('scale_0', 'f4'), ('scale_1', 'f4'), ('scale_2', 'f4'),
        ('rot_0', 'f4'), ('rot_1', 'f4'), ('rot_2', 'f4'), ('rot_3', 'f4'),
        ('f_dc_0', 'f4'), ('f_dc_1', 'f4'), ('f_dc_2', 'f4'),
    ]
    data = np.zeros(n, dtype=dtype)
    data['x'] = rng.uniform(-1, 1, n)
    data['y'] = rng.uniform(-1, 1, n)
    data['z'] = rng.uniform(-1, 1, n)
    data['opacity'] = rng.uniform(-2, 2, n)   # logit space
    data['scale_0'] = rng.uniform(-3, -1, n)  # log space
    data['scale_1'] = rng.uniform(-3, -1, n)
    data['scale_2'] = rng.uniform(-3, -1, n)
    # Unit quaternion
    q = rng.standard_normal((n, 4)).astype(np.float32)
    q /= np.linalg.norm(q, axis=1, keepdims=True)
    data['rot_0'] = q[:, 0]
    data['rot_1'] = q[:, 1]
    data['rot_2'] = q[:, 2]
    data['rot_3'] = q[:, 3]
    data['f_dc_0'] = rng.uniform(-1, 1, n)
    data['f_dc_1'] = rng.uniform(-1, 1, n)
    data['f_dc_2'] = rng.uniform(-1, 1, n)

    el = PlyElement.describe(data, 'vertex')
    PlyData([el]).write(path)


def test_load_ply():
    print("test_load_ply ... ", end="")
    with tempfile.NamedTemporaryFile(suffix='.ply', delete=False) as f:
        path = f.name
    make_fake_ply(path, n=500)

    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    scene = load_ply(path, device=device)

    assert isinstance(scene, GaussianScene)
    assert scene.n == 500
    assert scene.means.shape == (500, 3)
    assert scene.scales.shape == (500, 3)
    assert scene.quats.shape == (500, 4)
    assert scene.opacities.shape == (500,)
    assert scene.colors.shape == (500, 3)
    assert scene.means.device.type == device.split(':')[0]
    assert scene.opacities.min() >= 0.0
    assert scene.opacities.max() <= 1.0
    assert scene.colors.min() >= 0.0
    assert scene.colors.max() <= 1.0

    os.unlink(path)
    print("PASS")


def test_scene_stats():
    print("test_scene_stats ... ", end="")
    with tempfile.NamedTemporaryFile(suffix='.ply', delete=False) as f:
        path = f.name
    make_fake_ply(path, n=200)

    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    scene = load_ply(path, device=device)
    stats = scene.stats()

    assert 'n_gaussians' in stats
    assert stats['n_gaussians'] == 200
    assert 'means_range' in stats
    assert 'opacity_mean' in stats

    os.unlink(path)
    print("PASS")


def test_make_intrinsics():
    print("test_make_intrinsics ... ", end="")
    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    K = make_intrinsics(1280, 720, fov_deg=60.0, device=device)
    assert K.shape == (1, 3, 3)
    assert K[0, 2, 2].item() == 1.0
    assert K[0, 0, 2].item() == 640.0  # cx = width/2
    assert K[0, 1, 2].item() == 360.0  # cy = height/2
    print("PASS")


def test_orbit_camera():
    print("test_orbit_camera ... ", end="")
    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    center = torch.zeros(3, device=device)
    viewmat = orbit_camera(center, radius=5.0, elevation_deg=20.0, azimuth_deg=45.0, device=device)
    assert viewmat.shape == (1, 4, 4)
    print("PASS")


def test_render_smoke():
    print("test_render_smoke ... ", end="")
    with tempfile.NamedTemporaryFile(suffix='.ply', delete=False) as f:
        path = f.name
    make_fake_ply(path, n=1000)

    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    scene = load_ply(path, device=device)

    W, H = 320, 240
    center = scene_center(scene.means)
    K = make_intrinsics(W, H, fov_deg=60.0, device=device)
    viewmat = orbit_camera(center, radius=3.0, device=device)

    image, alpha = render(scene, viewmat, K, width=W, height=H)

    assert image.shape == (H, W, 3)
    assert image.dtype.name == 'uint8'
    assert alpha.shape == (H, W)

    os.unlink(path)
    print("PASS")


def run_all():
    print()
    print("=" * 40)
    print("vne3dgs smoke tests")
    print("=" * 40)
    import torch
    print(f"Device: {'cuda (' + torch.cuda.get_device_name(0) + ')' if torch.cuda.is_available() else 'cpu'}")
    print()

    test_load_ply()
    test_scene_stats()
    test_make_intrinsics()
    test_orbit_camera()
    test_render_smoke()

    print()
    print("All tests passed.")
    print("=" * 40)


if __name__ == "__main__":
    run_all()
