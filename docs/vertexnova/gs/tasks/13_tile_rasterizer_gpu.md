# Task 13 — Tile-Based Compute Rasterizer

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 4 — Real-time GPU viewer | [07](07_tile_renderer.md), [12](12_splat_quads.md) | [18](18_cpp_backward.md) | [ ] |

> **Goal:** port Task 07's tile pipeline to compute shaders (the paper's rasterizer), match the CPU image,
> and compare speed with Task 12.

## Why this task

This is the renderer from the paper. It's more work than quads, but it has **per-pixel early termination**
and **no overdraw blending**, and its data layout (per-tile sorted lists, per-pixel final `T`) is exactly
what a backward pass needs. Task 18 builds on it.

## Learn

### 1. The pipeline on the GPU

Same six stages as [Task 07](07_tile_renderer.md#2-the-pipeline), each a compute dispatch:

| # | Kernel | Threads | Output |
|---|--------|---------|--------|
| 1 | preprocess (Task 10 + tile rect) | one per Gaussian | projected data, `tiles_touched[g]` |
| 2 | scan (Task 11) | — | `offsets[g]`, total `M` |
| 3 | duplicate keys | one per Gaussian | `M` × (`uvec2` key = (tile, depth bits), value = g) |
| 4 | radix sort (Task 11) | — | sorted pairs (sort only `32 + bits(tile count)` bits) |
| 5 | tile ranges | one per key | `ranges[tile] = [start, end)` |
| 6 | rasterize | **one workgroup per tile, one thread per pixel** | image |

### 2. The rasterize kernel (the heart)

```
workgroup = one 16×16 tile (256 threads); thread = one pixel
shared: ids[256], mean[256], conic_opacity[256], color[256]; done_count

range = ranges[tile]; T = 1; C = 0; done = false
for batch in range, 256 at a time:
    barrier
    if all 256 pixels are done (done_count == 256): break
    each thread loads ONE splat of the batch into shared memory
    barrier
    if not done:
        for each splat j in the batch (front to back):
            (Task 06 rules: power, α, 1/255 skip, test_T, early stop → done = true, atomicAdd(done_count, 1))
write C + T · background to the output image (a storage image or buffer)
store T and the number of splats walked per pixel   ← the backward pass needs these (Task 18)
```

- **Shared-memory batching:** each splat is read from global memory once per tile, not once per pixel,
  a 256× reduction in reads for that data.
- **Collective early exit:** the workgroup stops only when *every* pixel is saturated. CUDA's
  `__syncthreads_count(done)` becomes a shared counter + `barrier()` in GLSL.

### 3. Buffer sizing: `M` is only known on the GPU

`M` (the total key count) comes out of the scan in stage 2. Options:

1. **Read back `M`** and resize buffers: simple, but a CPU/GPU sync every frame.
2. **Allocate for a worst case** (e.g. `k · N`) and detect overflow (write `M` to a buffer; grow next frame
   if exceeded): no sync.
3. **Indirect dispatch:** a tiny kernel writes the dispatch arguments for stages 3–5 from `M` on the GPU;
   vnerhi supports `dispatchIndirect`.

Start with 1, move to 2 + 3. The reference CUDA code re-allocates each frame, which is fine in CUDA and
costly elsewhere.

### 4. No 64-bit integers

Keys are `uvec2(depth_bits, tile_id)` (Task 11 §1). Sorting LSD over the depth word first, then the tile
word, gives exactly the `(tile << 32) | depth` order, with no `uint64` needed anywhere, so it works on WGSL.

### 5. Why the result should match Task 07 closely

Same algorithm, same traversal order per pixel, same thresholds. Only float differences remain (`exp`,
fast math). Tile coverage is identical to Task 07's CPU renderer. This is your strongest correctness
check of the whole GPU pipeline.

## Read

- **Code**, `diff-gaussian-rasterization/cuda_rasterizer/forward.cu`: `renderCUDA` (the shared-memory
  batch loop, `__syncthreads_count`, the stored `final_T` and `n_contrib`).
- **Code**, `rasterizer_impl.cu`: the forward function end to end (allocation, scan, duplicate, sort,
  ranges, render).
- **Paper** §6 again: now every sentence should map to a kernel.

## Build

- [ ] `shaders/duplicate_keys.comp.glsl`, `shaders/tile_ranges.comp.glsl`, `shaders/rasterize_tiles.comp.glsl`
  (+ reuse the Task 10 preprocess with tile counting, and the Task 11 scan/sort).
- [ ] `include/vertexnova/gs/render/gpu/tile_rasterizer.h`: owns buffers and dispatches; outputs color,
  final `T` and `n_contrib` per pixel; per-stage timings.
- [ ] Viewer: renderer switch "Quads (T12) / Tiles (T13)", per-stage timing for both, and `M` in the panel.
- [ ] `tests/gpu/tile_rasterizer_gpu_test.cpp`

## Test

| Case | Expected |
|------|----------|
| Tile ranges for a synthetic scene | equal to CPU `computeTileRanges` (Task 07) |
| Synthetic 1,000 Gaussians, opacities ≤ 0.35 | image vs `renderTiled`: max abs diff ≤ 1/255 |
| Same scene, per-pixel `n_contrib` and final `T` | match the CPU renderer's values (instrument it) |
| Garden, golden camera | PSNR vs golden > 45 dB |
| Camera inside the scene (huge `M`) | no crash; overflow handled (grow or clamp + warning) |

## Done when

- [ ] Tests pass on Metal and Vulkan.
- [ ] A timing table in *My notes*: Quads vs Tiles, per stage, for 2–3 viewpoints (overview, close-up,
  inside the scene). Explain which wins where and why.

## Check yourself

1. Why is shared-memory batching such a big win?
2. Why does the workgroup need a collective early exit instead of each thread returning?
3. What do `final_T` and `n_contrib` have to do with training?
4. Name the three ways to size buffers when `M` is only known on the GPU, and their trade-offs.

<details><summary>Answers</summary>

1. The 256 pixels of a tile read the same splats; loading each splat once into shared memory replaces
   256 global reads with one.
2. Threads must keep hitting `barrier()` together while the batch loads continue. A thread that returns
   early would break the barrier. The loop exits only when all pixels are done.
3. The backward pass walks each pixel's list back to front and reconstructs each earlier transmittance
   from the final one (`T_i = T_{i+1} / (1 − α_i)`); `n_contrib` says where to start.
4. Readback (simple, sync), worst-case allocation with overflow detection (no sync, wastes memory),
   indirect dispatch (no sync, more kernels).

</details>

## Going further

- Try 8×8 and 32×8 tiles (thread counts 64, 256) and see the effect on list lengths and occupancy.
- Load-balance: split tiles with very long lists across several workgroups.

## My notes

_Fill in after finishing._
