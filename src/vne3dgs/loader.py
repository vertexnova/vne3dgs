"""
vne3dgs.loader
--------------
Load pre-trained 3DGS scenes from .ply files.

A standard 3DGS .ply file stores per-Gaussian attributes:
  - Position:   x, y, z
  - Opacity:    opacity (logit — apply sigmoid)
  - Scale:      scale_0, scale_1, scale_2 (log — apply exp)
  - Rotation:   rot_0, rot_1, rot_2, rot_3 (quaternion wxyz)
  - SH DC:      f_dc_0, f_dc_1, f_dc_2 (base color)
  - SH rest:    f_rest_0 ... f_rest_N (higher order SH, optional)
"""

import numpy as np
import torch
from plyfile import PlyData
from dataclasses import dataclass
from pathlib import Path


@dataclass
class GaussianScene:
    """
    A loaded 3DGS scene — all tensors on the specified device.

    Attributes:
        means:      [N, 3]  Gaussian centers in world space
        scales:     [N, 3]  scale per axis (after exp)
        quats:      [N, 4]  rotation quaternions (wxyz, normalized)
        opacities:  [N]     opacity values in [0, 1] (after sigmoid)
        colors:     [N, 3]  RGB colors from SH DC term, clamped [0, 1]
        n:          int     number of Gaussians
        source:     str     path to source .ply file
    """
    means: torch.Tensor
    scales: torch.Tensor
    quats: torch.Tensor
    opacities: torch.Tensor
    colors: torch.Tensor
    n: int
    source: str

    def __repr__(self):
        return (
            f"GaussianScene(\n"
            f"  n={self.n:,} Gaussians\n"
            f"  means:     {tuple(self.means.shape)}\n"
            f"  scales:    {tuple(self.scales.shape)}\n"
            f"  quats:     {tuple(self.quats.shape)}\n"
            f"  opacities: {tuple(self.opacities.shape)}\n"
            f"  colors:    {tuple(self.colors.shape)}\n"
            f"  device:    {self.means.device}\n"
            f"  source:    {self.source}\n"
            f")"
        )

    def to(self, device: str) -> "GaussianScene":
        """Move all tensors to device."""
        return GaussianScene(
            means=self.means.to(device),
            scales=self.scales.to(device),
            quats=self.quats.to(device),
            opacities=self.opacities.to(device),
            colors=self.colors.to(device),
            n=self.n,
            source=self.source,
        )

    def stats(self) -> dict:
        """Return scene statistics useful for debugging."""
        return {
            "n_gaussians": self.n,
            "means_range": (self.means.min().item(), self.means.max().item()),
            "scale_range": (self.scales.min().item(), self.scales.max().item()),
            "opacity_mean": self.opacities.mean().item(),
            "color_range": (self.colors.min().item(), self.colors.max().item()),
        }


def load_ply(path: str, device: str = "cuda") -> GaussianScene:
    """
    Load a 3DGS scene from a .ply file.

    Args:
        path:   Path to point_cloud.ply (e.g. from iteration_30000/)
        device: PyTorch device — 'cuda' or 'cpu'

    Returns:
        GaussianScene with all attributes as tensors on device

    Example:
        scene = load_ply("/workspace/datasets/garden/point_cloud/iteration_30000/point_cloud.ply")
        print(scene)
    """
    path = str(path)
    ply = PlyData.read(path)
    v = ply['vertex']

    means = torch.tensor(
        np.stack([v['x'], v['y'], v['z']], axis=1),
        dtype=torch.float32
    )

    # Scales are stored as log values — exponentiate to get actual scale
    scales = torch.tensor(
        np.exp(np.stack([v['scale_0'], v['scale_1'], v['scale_2']], axis=1)),
        dtype=torch.float32
    )

    # Quaternion — wxyz convention, already unit length in well-formed files
    quats = torch.tensor(
        np.stack([v['rot_0'], v['rot_1'], v['rot_2'], v['rot_3']], axis=1),
        dtype=torch.float32
    )
    # Normalize just in case
    quats = quats / quats.norm(dim=-1, keepdim=True).clamp(min=1e-6)

    # Opacity stored as logit — apply sigmoid
    opacities = torch.sigmoid(
        torch.tensor(np.array(v['opacity']), dtype=torch.float32)
    )

    # Colors from SH DC (degree 0) term — convert to RGB
    # SH DC to color: color = 0.5 + SH_C0 * f_dc
    # where SH_C0 = 0.28209479177387814
    sh_dc = torch.tensor(
        np.stack([v['f_dc_0'], v['f_dc_1'], v['f_dc_2']], axis=1),
        dtype=torch.float32
    )
    colors = (0.5 + 0.28209479177387814 * sh_dc).clamp(0.0, 1.0)

    scene = GaussianScene(
        means=means,
        scales=scales,
        quats=quats,
        opacities=opacities,
        colors=colors,
        n=len(v),
        source=path,
    )

    return scene.to(device)
