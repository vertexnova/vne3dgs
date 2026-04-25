"""
demo/render_scene.py
--------------------
First render demo — load a pre-trained 3DGS .ply and save a PNG.

Usage:
    python demo/render_scene.py
    python demo/render_scene.py --ply /path/to/point_cloud.ply --output output/render.png

DGX Spark:
    python demo/render_scene.py \
        --ply /workspace/datasets/garden/point_cloud/iteration_30000/point_cloud.ply \
        --output /workspace/projects/vne3dgs/output/garden_render.png
"""

import argparse
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

import imageio
import torch

from vne3dgs import load_ply, render, make_intrinsics, orbit_camera, scene_center

DEFAULT_PLY = "/workspace/datasets/garden/point_cloud/iteration_30000/point_cloud.ply"
DEFAULT_OUT = "/workspace/projects/vne3dgs/output/garden_render.png"


def main():
    parser = argparse.ArgumentParser(description="vne3dgs — first render demo")
    parser.add_argument("--ply",      default=DEFAULT_PLY, help="Path to point_cloud.ply")
    parser.add_argument("--output",   default=DEFAULT_OUT, help="Output PNG path")
    parser.add_argument("--width",    type=int, default=1280)
    parser.add_argument("--height",   type=int, default=720)
    parser.add_argument("--fov",      type=float, default=60.0, help="Field of view degrees")
    parser.add_argument("--elevation",type=float, default=20.0, help="Camera elevation degrees")
    parser.add_argument("--azimuth",  type=float, default=45.0, help="Camera azimuth degrees")
    parser.add_argument("--radius",   type=float, default=None, help="Orbit radius (auto if not set)")
    parser.add_argument("--device",   default="cuda")
    args = parser.parse_args()

    os.makedirs(os.path.dirname(args.output), exist_ok=True)

    print("=" * 50)
    print("vne3dgs — first render demo")
    print("=" * 50)
    print(f"PLY:    {args.ply}")
    print(f"Output: {args.output}")
    print(f"Size:   {args.width}x{args.height}")
    print(f"Device: {args.device}")
    print()

    # Load scene
    t0 = time.time()
    print("Loading scene...")
    scene = load_ply(args.ply, device=args.device)
    print(scene)
    print(f"Load time: {time.time() - t0:.2f}s")
    print()

    # Print stats
    stats = scene.stats()
    print("Scene stats:")
    for k, v in stats.items():
        print(f"  {k}: {v}")
    print()

    # Camera setup
    center = scene_center(scene.means)
    spread = (scene.means - center).norm(dim=-1).mean().item()
    radius = args.radius if args.radius else spread * 2.0

    print(f"Scene center: {center.cpu().numpy().round(3)}")
    print(f"Scene spread: {spread:.3f}")
    print(f"Orbit radius: {radius:.3f}")
    print()

    K = make_intrinsics(args.width, args.height, args.fov, device=args.device)
    viewmat = orbit_camera(
        center=center,
        radius=radius,
        elevation_deg=args.elevation,
        azimuth_deg=args.azimuth,
        device=args.device,
    )

    # Render
    print("Rendering...")
    t1 = time.time()
    image, alpha = render(scene, viewmat, K, args.width, args.height)
    render_time = time.time() - t1

    print(f"Render time: {render_time*1000:.1f}ms")
    print(f"Image shape: {image.shape}")
    print(f"Alpha coverage: {alpha.mean().item()*100:.1f}%")
    print()

    # Save
    imageio.imwrite(args.output, image)
    print(f"Saved: {args.output}")
    print()
    print("=" * 50)
    print("DONE — first 3DGS render complete!")
    print("=" * 50)


if __name__ == "__main__":
    main()
