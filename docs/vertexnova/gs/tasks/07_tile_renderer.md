# Task 07 — Tiles: Rehearsing the GPU Algorithm

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 3 — CPU reference renderer | [06](06_sort_and_blend.md) | [13](13_tile_rasterizer_gpu.md) | [ ] |

> **Goal:** re-implement the renderer with the paper's tile-based structure (duplicate keys per tile, one
> sort, per-tile ranges) on the CPU and multithreaded, matching Task 06.

## Why this task

The GPU rasterizer (Task 13) is exactly this algorithm with one GPU workgroup per tile. Building it on the
CPU first, where you can print arrays and set breakpoints, means the GPU version is "just" a port. It's
also much faster than Task 06, which you'll welcome.

## Learn

### 1. Why tiles?

Split the image into **16×16-pixel tiles**. For each tile, build the list of splats that touch it, sorted
front to back. Then every pixel in a tile walks the **same short list**.

- On a GPU: one workgroup (256 threads) per tile, pixels in lockstep, and splats loaded once into shared
  memory for all 256 pixels.
- On a CPU: tiles are independent, so they parallelize with no locks.

### 2. The pipeline

```
1. PREPROCESS   for each Gaussian: projectGaussian (Task 05) → mean, conic, radius, depth
                tile rect: rect_min = clamp(floor((mean − r) / 16), 0, grid)
                           rect_max = clamp(floor((mean + r + 15) / 16), 0, grid)   (exclusive)
                tiles_touched = (max.x − min.x) · (max.y − min.y)

2. SCAN         offsets = exclusive prefix sum of tiles_touched;  M = total (the number of keys)

3. DUPLICATE    for Gaussian g, for each tile (tx, ty) in its rect, at position offsets[g] + k:
                    key   = (tile_id << 32) | float_bits(depth)      tile_id = ty · grid_x + tx
                    value = g

4. SORT         sort the M (key, value) pairs by key

5. RANGES       scan the sorted keys; where tile_id changes, record [start, end) for that tile

6. BLEND        for each tile, for each pixel in the tile:
                    walk values[start..end) front to back with the Task 06 per-pixel rules
```

### 3. The depth-as-integer trick

For **positive** IEEE-754 floats, the bit pattern read as an unsigned integer is ordered the same way as
the float value. Every visible depth is > 0.2, so `float_bits(depth)` is a valid 32-bit sort key, and one
64-bit key sorts by tile first, then by depth. (Negative floats would break this. That's another reason
for the near cull.)

**Ties:** two splats with bit-identical depth in the same tile need a deterministic order, or CPU and GPU
images won't match exactly. Use a stable sort over keys generated in Gaussian-index order, and document it.

### 4. How many keys?

`M` can be many times the number of visible Gaussians: a large splat near the camera can touch hundreds
of tiles. Print `M` and `M / visible` for the garden; this is the number that sizes GPU buffers in
Task 13.

### 5. Relationship to Task 06: when must the images be identical?

Task 06 lets a splat touch only pixels inside its `[μ − r, μ + r]` box. A tile renderer lets it touch
**every pixel of every tile** its rect overlaps, a superset. The extra pixels are all farther than `r`
(≥ 3σ along the major axis), where `G ≤ e^(−4.5) ≈ 0.0111`.

- If every opacity is ≤ 0.35, then `α ≤ 0.35 · 0.0111 = 0.00389 < 1/255`, so those extra pixels are
  skipped anyway, and **the two renderers must agree bit for bit** (same per-pixel order, same float ops).
- With real scenes (opacity up to ~1) a faint fringe between 3σ and ~3.3σ differs. Expect a tiny max
  difference, only at splat edges.

This is exactly the kind of reasoning you need when comparing implementations. Write the numbers in your
notes.

### 6. Threading

Tiles are independent: a pool of `std::thread::hardware_concurrency()` workers pulls tile indices from an
`std::atomic<uint32_t>` counter. (Apple's libc++ has limited support for C++17 parallel algorithms, so
don't rely on `std::execution::par`.) Steps 1 and 3 parallelize over Gaussians; steps 2 and 4 can stay
single-threaded at first.

## Read

- **Paper** §6: the paragraph on tiles, key duplication and sorting (look for "radix sort").
- **Code**, `diff-gaussian-rasterization/cuda_rasterizer/rasterizer_impl.cu`: `duplicateWithKeys`,
  `identifyTileRanges`, `getHigherMsb`, and the forward function calling `cub::DeviceScan::InclusiveSum`
  and `cub::DeviceRadixSort::SortPairs` (note it sorts only `32 + bit` bits).
- **Code**, `auxiliary.h`: `getRect`, `BLOCK_X`, `BLOCK_Y`.

## Build

- [ ] `include/vertexnova/gs/render/tiling.h` + `.cpp`: shared with the GPU tasks, so keep it plain.

  ```cpp
  namespace vne::gs {
  inline constexpr std::uint32_t kTileSize = 16;
  struct TileGrid { std::uint32_t tiles_x, tiles_y; static TileGrid forImage(std::uint32_t w, std::uint32_t h); };
  struct TileRect { std::uint32_t min_x, min_y, max_x, max_y; };           // max exclusive
  struct TileRange { std::uint32_t start, end; };

  [[nodiscard]] TileRect computeTileRect(const math::Vec2f& mean, int radius, const TileGrid& grid);
  [[nodiscard]] std::uint64_t makeTileDepthKey(std::uint32_t tile_id, float depth);
  [[nodiscard]] std::vector<TileRange> computeTileRanges(const std::vector<std::uint64_t>& sorted_keys,
                                                         const TileGrid& grid);
  }  // namespace vne::gs
  ```

- [ ] `include/vertexnova/gs/render/cpu/tile_renderer.h` + `.cpp`: `ImageRGBf renderTiled(cloud, cam,
  settings, TileStats* stats)`, with `TileStats { visible, num_keys, max_list_length, milliseconds per stage }`.
- [ ] `tests/tiling_test.cpp`, `tests/tile_renderer_test.cpp`
- [ ] `examples/07_tile_render/`: renders with both renderers, prints timings, writes the image, the
  absolute-difference image (×50) and a **heat map of list length per tile**.

## Test

| Case | Expected |
|------|----------|
| `computeTileRect` for a splat at (8, 8), r = 3 | tile (0, 0) only |
| Splat at (16, 16), r = 1 (on a corner) | tiles (0,0), (1,0), (0,1), (1,1) |
| Splat entirely off-screen | empty rect (`tiles_touched = 0`) |
| `makeTileDepthKey`: random positive `a < b`, same tile | `key(a) < key(b)` |
| Different tiles | tile order dominates depth |
| `computeTileRanges` on a hand-made sorted list | expected [start, end) per tile, empty tiles `{0, 0}` |
| Synthetic random scene, 1,000 Gaussians, all opacities ≤ 0.35 | `renderTiled == renderNaive` **exactly** |
| Same scene, opacities up to 1 | max abs diff small (e.g. < 2/255), located only at splat fringes |

## Done when

- [ ] Tests pass, including the exact-equality test.
- [ ] The garden golden view renders identically in structure, many times faster than Task 06; timings
  and `M` are written in *My notes*.

## Check yourself

1. Why can positive floats be sorted as unsigned integers?
2. What's the worst case for the number of keys, and what scene causes it?
3. Where exactly can the tiled and naive images differ, and why?
4. Why does the reference sort only `32 + bit` bits of the 64-bit key?
5. What does a tile's list length tell you about GPU cost?

<details><summary>Answers</summary>

1. IEEE-754 stores sign, exponent, mantissa from high bits to low; for non-negative values a larger float
   has a larger bit pattern.
2. Every splat touching every tile (`M = visible × tiles`): huge Gaussians close to the camera, or a
   camera inside the scene looking at big foggy blobs.
3. Only pixels outside a splat's `r`-box but inside its tile rect, where α ≥ 1/255 is still possible for
   opacity > ~0.35. That's the 3σ–3.3σ fringe.
4. The high bits are the tile id, which needs only `ceil(log2(num_tiles))` bits; sorting fewer bits means
   fewer radix passes.
5. The workgroup for that tile must loop over the whole list (minus early termination); long lists mean
   slow tiles, and the slowest tile bounds the frame.

</details>

## Going further

- Implement your own radix sort on the CPU (8-bit digits, LSD) instead of `std::stable_sort`: a
  rehearsal for Task 11.
- Try 8×8 and 32×32 tiles. How do `M`, list lengths and time change?

## My notes

_Fill in after finishing._
