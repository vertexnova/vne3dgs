# Task 11 — Sorting on the GPU

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 4 — Real-time GPU viewer | [10](10_gpu_preprocess.md) | [12](12_splat_quads.md), [13](13_tile_rasterizer_gpu.md) | [ ] |

> **Goal:** sort millions of (key, value) pairs on the GPU with a portable radix sort, verified against
> `std::sort`.

## Why this task

Every 3DGS frame needs a sort: of visible Gaussians by depth (Task 12), or of millions of (tile, depth)
keys (Task 13). It's usually the most expensive step, and GPU sorting is a classic parallel-algorithms
topic in its own right, worth learning properly.

## Learn

### 1. What gets sorted

| For | Keys | Values | Count |
|-----|------|--------|-------|
| Task 12 (quads, back to front) | 32-bit depth bits | Gaussian index | visible N (~1–5M) |
| Task 13 (tiles) | (tile id, depth bits) as **two 32-bit words** | Gaussian index | M duplicated keys (often 5–20× N) |

Depth keys: positive float bits sort like unsigned ints (Task 07 §3). For **back-to-front**, either sort
ascending and read the result in reverse, or sort `~bits` (bitwise NOT reverses the order).

Two 32-bit words instead of `uint64`: WGSL has no 64-bit integers and int64 support varies on Vulkan
devices. LSD radix sort handles multi-word keys naturally: sort by the low word's digits first, then the
high word's.

### 2. Step 0: sort on the CPU

Read depths back, `std::sort` the indices, upload. It's slow, but it unblocks Task 12 immediately and
gives you the number to beat. On unified-memory machines (Apple Silicon, GB10), readback is cheaper than
over PCIe, but still a sync point every frame.

### 3. Optional warm-up: bitonic sort

A sorting network: `log₂N · (log₂N + 1)/2` passes of compare-and-swap, each pass fully parallel, with no
atomics or scans. Easy to get right, O(N log² N) and needs a power-of-two size (pad with max keys). Fine
up to ~1M keys and a good first GPU sort.

### 4. LSD radix sort, the real thing

Process the key **8 bits at a time**, least significant digit first: 4 passes for 32-bit keys. Each pass is
a **stable counting sort** on one digit:

```
for each digit position (bits 0–7, 8–15, 16–23, 24–31):
    1. HISTOGRAM  each workgroup counts its keys per digit value (256 bins, shared-memory atomics)
                  → global table hist[digit][workgroup]
    2. SCAN       exclusive prefix sum over the table in digit-major order
                  → offset[digit][workgroup] = where this workgroup's keys with this digit start
    3. SCATTER    each workgroup re-reads its keys; for each key:
                  dst = offset[digit][wg] + (rank of this key among same-digit keys earlier in this workgroup)
                  write key and value to dst in the other buffer (ping-pong)
```

- **Stability** is what makes LSD correct: step 3's rank must preserve input order within a workgroup.
  Compute it with a shared-memory prefix sum per digit (simple) or subgroup ballots (fast, less portable).
- **Scan** (Blelloch): up-sweep then down-sweep in shared memory within a workgroup, recursively for big
  arrays (scan each block, scan the block totals, add them back).
- **Items per thread:** have each thread handle 4–16 keys. Fewer workgroups means a smaller histogram
  table and less scan work.
- Use separate dispatches per step. Vulkan and Metal **don't guarantee forward progress between
  workgroups**, so single-pass designs that spin-wait on other workgroups (Onesweep's decoupled
  look-back) can deadlock on some GPUs. Read about them, but start with the portable 3-dispatch design.

### 5. Sort only what's visible

Compact first: a scan over "visible" flags gives each visible Gaussian its output slot (stream
compaction). This often halves the sort size, and the same scan kernel serves Task 13's tile-count scan.

### 6. Subgroup (wave) operations

`subgroupAdd` or `subgroupBallot` make histograms, scans and ranking much faster. Add them **after** the
portable version works, and only once you've checked that vneshaderc cross-compiles them to MSL and WGSL.
Keep the portable path as a fallback.

## Read

- **Blelloch 1990**, *Prefix Sums and Their Applications*, §1–2 (up-sweep / down-sweep).
- **GPU Gems 3, ch. 39** (Harris et al.): work-efficient scan in shared memory, and bank conflicts.
- **Merrill & Grimshaw 2011**: the upsweep / scan / downsweep radix-sort structure you're building.
- **Optional:** Adinets & Merrill 2022 (Onesweep), to see where the state of the art is and why it needs
  forward-progress guarantees.
- `../vnerhi/samples/18_metaballs/shaders/src/`: compute patterns in your own toolchain.

## Build

- [ ] `include/vertexnova/gs/render/gpu/scan.h` + `shaders/scan_*.comp.glsl`: exclusive scan of `uint`,
  any length.
- [ ] `include/vertexnova/gs/render/gpu/radix_sort.h` + `shaders/radix_histogram.comp.glsl`,
  `radix_scatter.comp.glsl`: sorts `(uint key, uint value)` pairs and `(uvec2 key, uint value)` pairs,
  with a configurable number of key bits.
- [ ] (Optional) `shaders/bitonic_sort.comp.glsl` as a warm-up.
- [ ] Viewer: sort Gaussians by depth each frame (CPU fallback toggle vs GPU), timing in the panel.
- [ ] `tests/gpu/scan_gpu_test.cpp`, `tests/gpu/radix_sort_gpu_test.cpp`

## Test

| Case | Expected |
|------|----------|
| Scan: lengths 1, 255, 256, 257, 1,000,000, 10,000,000 | equals `std::exclusive_scan` |
| Sort: random keys, same lengths | keys equal `std::stable_sort` result; values permuted identically |
| All keys equal | output order == input order (stability) |
| Already sorted / reverse sorted | correct |
| 2-word keys: random tile (13 bits) + depth | order == sorting by `(tile << 32) | depth` on the CPU |
| Real data: garden visible depths | matches the CPU sort |

## Done when

- [ ] GPU tests pass on Metal and Vulkan.
- [ ] Sort time for 1M and 6M keys is recorded in *My notes*, next to the CPU fallback's time.

## Check yourself

1. Why must each counting-sort pass be stable?
2. How many passes for a 45-bit (tile + depth) key with 8-bit digits?
3. Why separate dispatches instead of one kernel?
4. How do you sort back to front with an ascending sort?
5. Why represent 64-bit keys as two 32-bit words?

<details><summary>Answers</summary>

1. LSD sorts the least significant digit first; later passes must keep earlier passes' order among equal
   digits, or the lower digits' ordering is lost.
2. `ceil(45 / 8) = 6`.
3. No inter-workgroup forward-progress guarantee on Vulkan/Metal, and each step needs the previous step's
   complete result (a global barrier).
4. Sort `~depth_bits`, or read the ascending result in reverse.
5. WGSL has no 64-bit integers, and 64-bit integer support varies by device. LSD handles multi-word keys
   naturally.

</details>

## Going further

- Add a subgroup-accelerated histogram/rank path and measure it per backend.
- Sort only the high bits that matter: for depth, quantize to 16 bits (sort quality vs speed). Where does
  popping start to show?

## My notes

_Fill in after finishing._
