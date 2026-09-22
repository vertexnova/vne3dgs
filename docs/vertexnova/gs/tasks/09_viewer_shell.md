# Task 09 — Viewer Shell

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 4 — Real-time GPU viewer | [04](04_camera_and_points.md) (and ideally [07](07_tile_renderer.md)) | [10](10_gpu_preprocess.md), [15](15_colmap_cameras.md) | [ ] |

> **Goal:** an interactive window with an orbit camera and an ImGui panel, first showing the **CPU**
> renderer's output, on Metal (Mac) and Vulkan (DGX Spark).

## Why this task

Everything so far writes PNGs. From here on you want to *move around* the scene. This task sets up the
window, frame loop, camera control and UI, and deliberately shows the slow CPU renderer inside it:
seeing the low frame rate makes clear what Tasks 10–13 are for.

This task is mostly VertexNova plumbing, not 3DGS. Keep it small.

## Learn

### 1. The frame loop

```
init: window (vnewindow) → device + swapchain (vnerhi) → ImGui → load .ply
each frame:
    poll events (vneevents) → camera manipulator update (vneinteraction → vnescene camera)
    convert the vnescene camera → gs::Camera (OpenCV convention, Task 04)
    render the scene image                 (Task 09: CPU tile renderer → upload texture → draw)
    draw ImGui panel on top
    present
```

### 2. Two camera worlds, one conversion function

vneinteraction manipulators drive a `vne::scene` camera. vne3dgs renders with its own `gs::Camera` in
OpenCV convention. Write **one** function converting between them and test it. It's where every
"the scene is upside down in the viewer but fine in PNGs" bug will live. Use the vnescene convention you
recorded in Task 04.

Scene "up": until Task 15 gives you training cameras, add an "up axis" combo box (+Y / −Y / +Z / −Z) that
changes the manipulator's up vector. Don't rotate the scene; see Task 08 §5.

### 3. Where the app lives, and the GPU target

- **`vne3dgs_gpu` target** (alias `vne::gs::gpu`), under `src/vertexnova/gs/render/gpu/`: all vnerhi code
  from Tasks 10–13. Option `VNE_GS_GPU` (desktop default `ON`). The core `vne3dgs` target stays
  GPU-free and headless-testable.
- **`apps/viewer/`**: the viewer executable. Option `VNE_GS_VIEWER`.
- **vnerhi dependency:** add it as a submodule (`deps/internal/vnerhi`, which brings metal-cpp,
  Vulkan headers, etc.) or use the sibling checkout via `vne_use_dep`, following the vnemath pattern in
  the root `CMakeLists.txt`.

### 4. Reuse vnerhi's sample framework, or not?

vnerhi's samples share a static library, **`vne::rhi::samples`** (`../vnerhi/samples/framework/`), with
`Application`, a layer stack, `ImGuiLayer`, camera helpers and the `VNE_RHI_REGISTER_DEMO` pattern
(see `../vnerhi/samples/17_compute_to_render/sample_compute_to_render.cpp`).

**First sub-step:** find out whether that target is usable from outside vnerhi: which option builds it,
whether it drags in all the samples, and whether it installs. If it works, use it; it gives you ImGui
docking, viewports and camera controllers for free. If it doesn't, write a thin loop on vnewindow +
vnerhi yourself and note in *My notes* what vnerhi could export to make this easier.

### 5. The CPU renderer in a window

Render with Task 07's tiled renderer at a reduced resolution (a "resolution scale" slider, 0.1–1.0),
convert to RGBA8, upload into a texture each frame, and draw it full-screen (or into the ImGui viewport).
Expect a few frames per second on the garden. That's the point.

## Read

- `../vnerhi/samples/framework/` headers: `app/application.h`, `layers/`, `imgui/`, `camera/`.
- `../vnerhi/samples/09_texturing` and `14_camera_controller`: uploading a texture, orbiting a camera.
- `../vneinteraction/include/vertexnova/interaction/`: `camera_rig.h`, `trackball_manipulator.h`,
  `inspect_3d_controller.h`.
- `../vnescene/include/vertexnova/scene/camera/`: `camera.h`, `perspective_camera.h`.

## Build

- [ ] CMake: `VNE_GS_GPU`, `VNE_GS_VIEWER` options; vnerhi (and vnewindow, vneevents, vneinteraction,
  vnescene) wired in with `vne_use_dep`.
- [ ] `src/vertexnova/gs/render/gpu/` skeleton + the `vne3dgs_gpu` target (even if nearly empty for now).
- [ ] `include/vertexnova/gs/camera/scene_camera_bridge.h`: `gs::Camera toGsCamera(const vne::scene::ICamera&,
  std::uint32_t width, std::uint32_t height)` (in `vne3dgs_gpu` or the viewer, to keep vnescene out of the core).
- [ ] `apps/viewer/`: window, orbit/trackball camera, ImGui panel (FPS, frame time, resolution scale, SH
  degree, background color, up axis, `.ply` path + Load button, Gaussian count), CPU-render display.
- [ ] `tests/scene_camera_bridge_test.cpp`

## Test

| Case | Expected |
|------|----------|
| vnescene camera at `(0, 0, 5)` looking at the origin, up `+Y` → `toGsCamera` | same `R, t` as Task 04's `lookAt` for that pose |
| World point `(0, 1, 0)` through the converted camera | above the image center |
| Aspect ratio 16:9 with vertical FOV 60° | `fy = height / (2·tan 30°)`, `fx == fy` |

## Done when

- [ ] Tests pass.
- [ ] You can orbit, pan and zoom the garden (CPU-rendered, a few FPS at a low resolution scale) on macOS
  (Metal) and on the DGX Spark (Vulkan), with the image the same way up as your Task 06 golden PNG.

## Check yourself

1. Why keep the GPU code out of the core `vne3dgs` target?
2. Where would an "upside-down only in the viewer" bug be, and how would you catch it with a test?
3. Why change the manipulator's up vector instead of rotating the scene?

<details><summary>Answers</summary>

1. The core must build and test headless (CI runners have no GPU), and other users may want the CPU path
   or I/O without vnerhi.
2. In the vnescene → `gs::Camera` conversion. A test projecting a known world point through both camera
   types catches it.
3. Rotating the scene breaks view-dependent (SH) color unless you also rotate the coefficients (Task 08 §5).

</details>

## Going further

- Add a "golden view" button that jumps to the camera stored in Task 06's JSON, so you can compare
  viewer and PNG side by side.

## My notes

_Fill in after finishing._
