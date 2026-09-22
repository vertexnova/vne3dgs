# Task 10 — Preprocessing in a Compute Shader

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 4 — Real-time GPU viewer | [08](08_spherical_harmonics.md), [09](09_viewer_shell.md) | [11](11_gpu_sort.md) | [ ] |

> **Goal:** run per-Gaussian preprocessing (cull, project, conic, radius, SH → RGB) on the GPU, and prove it
> matches the CPU value for value.

## Why this task

Preprocessing is embarrassingly parallel: every Gaussian is independent. It's the easiest part of the
pipeline to move to the GPU and teaches the compute basics (storage buffers, workgroups, dispatch,
readback) that Tasks 11–13 build on.

```
 GaussianCloud ─upload→ [ preprocess.comp: one thread per Gaussian ] → projected buffer ─(readback in tests)
                         └──────────────────── this task ────────────────────┘
```

## Learn

### 1. Compute shader basics

- A **dispatch** launches `groups_x × groups_y × groups_z` **workgroups**; each has `local_size_x` threads
  (use 256). Thread id: `gl_GlobalInvocationID.x`.
- `groups_x = ceil(N / 256)`; the last group has spare threads, so **always** check `if (id >= N) return;`.
- 5.8M Gaussians / 256 ≈ 22,800 groups. The guaranteed minimum limit per dimension is 65,535, so bigger
  scenes need a 2D dispatch. Keep that in mind.

### 2. Storage buffers and the std430 trap

Gaussian data goes in **storage buffers** (SSBOs; `BufferBindFlags::eStorage` in vnerhi). GLSL `std430`
layout rules:

- `vec3` has **16-byte alignment**, so `vec3 data[]` has a 16-byte stride, not 12. Upload tightly packed
  `Vec3f` from C++ and every element after the first is garbage.
- Fix: use `vec4` (pad with something useful: `position.w` unused, `scale.w = opacity`) or plain
  `float data[]` with manual indexing.

A GPU-friendly layout (struct-of-arrays, like `GaussianCloud`):

| Buffer | Per Gaussian | Bytes |
|--------|--------------|-------|
| `positions` | `vec4(x, y, z, 0)` | 16 |
| `scale_opacity` | `vec4(sx, sy, sz, opacity)` | 16 |
| `rotations` | `vec4(x, y, z, w)` (or store `Σ₃D` as 6 floats instead) | 16 |
| `sh` | `float[48]` (degree 3, coefficient-major RGB) | 192 |
| **output** `projected` | `vec4(mean.x, mean.y, depth, radius)` + `vec4(conic.a, conic.b, conic.c, opacity)` + `vec4(rgb, 0)` | 48 |

SH dominates: about 1.1 GB for the garden. Apple Silicon and the GB10 share CPU/GPU memory, so it fits;
Task 14 compresses it.

**Uniforms** (one small buffer): world→camera matrix, camera position, `fx, fy, cx, cy`, image size,
`tan_fovx`, `tan_fovy` (for the 1.3× clamp), SH degree, Gaussian count.

### 3. The shader is Tasks 05 + 08 in GLSL

Port `projectGaussian()` and `evalSH()` directly: same constants, same order of operations. Resist
"optimizing" while porting; make it match first. `radius = 0` marks a culled Gaussian.

### 4. Shaders in the vne toolchain

Write GLSL, and let **vneshaderc** produce SPIR-V (Vulkan), MSL (Metal) and WGSL, using a manifest like
vnerhi's samples:

```json
{
  "name": "preprocess",
  "source_lang": "glsl",
  "stages": [ { "stage": "compute", "file": "preprocess.comp.glsl" } ],
  "targets": ["msl", "wgsl"]
}
```

See `../vnerhi/samples/17_compute_to_render/shaders/src/` for a working example and how the sample
CMake (`SHADER_DIR`) compiles them.

### 5. Readback and tolerances

For tests, copy `projected` into a host-visible buffer (`ResourceStorage::eShared`, or a blit to a
staging buffer), `map()` it and compare with the CPU results.

Expect **small** differences, not bit-exact ones:

- `exp`, `sqrt` and division precision differ between CPU libm and GPU hardware.
- **Metal compiles with fast math by default**, which relaxes IEEE rules. Check what vneshaderc/vnerhi use.
- `radius = ceil(...)` can flip by one pixel when the value sits near an integer.

Compare with relative tolerances (e.g. 1e-4 for mean and conic, 1e-3 for color) and allow `radius` to
differ by at most 1.

## Read

- `../vnerhi/samples/16_compute/` and `17_compute_to_render/`: buffers, compute pipeline creation,
  `bindResources`, `dispatch`.
- `../vnerhi/include/vertexnova/rhi/buffer.h` (bind flags, storage, `map`) and `compute_command_encoder.h`.
- The GLSL spec, section "Memory Layout / std430" (or any std430 cheat sheet).
- **Code**, `forward.cu`: `preprocessCUDA`, which is what you're porting.

## Build

- [ ] `shaders/preprocess.comp.glsl` + `shaders/preprocess.manifest.json`; CMake to compile them via vneshaderc.
- [ ] `include/vertexnova/gs/render/gpu/gpu_scene.h`: uploads a `GaussianCloud` into the buffers above;
  owns them.
- [ ] `include/vertexnova/gs/render/gpu/preprocess_pass.h`: records the dispatch into a command buffer;
  owns the uniforms and the `projected` buffer.
- [ ] `tests/gpu/preprocess_gpu_test.cpp` in a separate `vne3dgs_gpu_tests` executable. It
  `GTEST_SKIP()`s when no GPU device is available, so CI stays green.
- [ ] Viewer: an ImGui toggle "GPU preprocess" (still drawing with the CPU blend for now, or just showing
  preprocess timing).

## Test

| Case | Expected |
|------|----------|
| 10,000 random Gaussians (varied scales, rotations, positions incl. behind/outside the view), degree 3 | every field matches `projectGaussian` + `evalSH` within tolerance |
| N = 1, 255, 256, 257 | no out-of-bounds writes (check a guard value after the end) |
| All Gaussians behind the camera | all `radius == 0` |
| Garden, golden camera | visible count within 0.1% of the CPU count |

## Done when

- [ ] GPU tests pass on Metal (Mac) and Vulkan (Spark).
- [ ] Preprocess time for the garden (GPU timestamps) is recorded in *My notes*, next to the CPU time for the
  same step.

## Check yourself

1. Why the `if (id >= N) return;` guard?
2. What goes wrong if you upload `std::vector<Vec3f>` into a `vec3[]` SSBO?
3. Why isn't GPU output bit-identical to CPU output?
4. Why one thread per Gaussian rather than per pixel for this step?

<details><summary>Answers</summary>

1. `groups × 256` usually exceeds `N`; the extra threads would read and write past the end.
2. std430 gives `vec3` array elements a 16-byte stride, so the shader reads misaligned data after
   element 0.
3. Different `exp`/`sqrt` implementations, fast-math reassociation, FMA contraction, and different
   rounding in `ceil` near integers.
4. The work is per Gaussian and independent; per-pixel work comes later (blending).

</details>

## Going further

- Store `Σ₃D` (6 floats) instead of scale + rotation. Precomputing on load trades memory for ALU. Measure it.
- Try `local_size_x = 64, 128, 512`. What changes?

## My notes

_Fill in after finishing._
