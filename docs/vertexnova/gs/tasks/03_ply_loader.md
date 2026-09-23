# Task 03 — Read a Trained Scene (PLY)

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 2 — Real data | [02](02_gaussian_3d.md) | [04](04_camera_and_points.md) | [x] |

> **Goal:** load a real trained 3DGS scene (millions of Gaussians) into a GPU-friendly struct-of-arrays,
> applying each parameter's activation correctly.

## Why this task

From here on, every task runs on real data. The file format looks trivial, but it hides four decisions
that silently break rendering if you get them wrong: activations, quaternion order, SH coefficient layout
and SH degree. You'll write your own reader: vneio's PLY path goes through Assimp, which is built for
meshes and not for 3DGS's custom per-vertex properties.

## Learn

### 1. What's in the file

A PLY file is an ASCII header followed by a binary body. A trained 3DGS scene is a single `vertex`
element whose properties are the Gaussian parameters:

```
ply
format binary_little_endian 1.0
element vertex 5834784
property float x                ┐ position (world space)
property float y                │
property float z                ┘
property float nx               ┐ normals: always 0, unused; still present in the file
property float ny               │
property float nz               ┘
property float f_dc_0           ┐ SH degree-0 coefficient, one per RGB channel
property float f_dc_1           │
property float f_dc_2           ┘
property float f_rest_0         ┐ SH degrees 1..3: 15 coefficients × 3 channels = 45
...                             │
property float f_rest_44        ┘
property float opacity            logit
property float scale_0          ┐ log scale
property float scale_1          │
property float scale_2          ┘
property float rot_0            ┐ quaternion (w, x, y, z), NOT normalized
property float rot_1            │
property float rot_2            │
property float rot_3            ┘
end_header
<binary: vertex_count × 62 floats, little-endian, one vertex after another>
```

62 floats = 248 bytes per Gaussian, so the 5.8M-Gaussian garden is about 1.4 GB.

### 2. Activations (stored value → usable value)

| Field | Stored as | Activation | Check value |
|-------|-----------|-----------|-------------|
| position | world coordinates | none | — |
| scale | `log(s)` | `s = exp(v)` | `log(2) → 2` |
| opacity | `logit(o)` | `o = 1 / (1 + exp(−v))` | `0 → 0.5` |
| rotation | `(w, x, y, z)`, any length | normalize | `(2, 0, 0, 0) → identity` |
| color (DC only) | SH coefficient `f_dc` | `0.5 + C0 · f_dc`, `C0 = 0.28209479177387814` | `0 → 0.5` (gray) |

Why store them this way? The optimizer works on the stored values, which can be any real number; the
activations map them into valid ranges (Task 02 §2). Full SH evaluation comes in Task 08; DC color is
enough for statistics and for Tasks 04–07.

### 3. The quaternion-order trap

The file stores `rot_0 = w`, `rot_1 = x`, `rot_2 = y`, `rot_3 = z`. `vne::math::Quat` is constructed as
`Quat(x, y, z, w)`. So:

```cpp
math::Quatf q(rot_1, rot_2, rot_3, rot_0);   // then normalize
```

### 4. The SH layout trap

`f_rest` is **channel-major**: `f_rest_0..14` are the 15 red coefficients, `15..29` green and `30..44` blue.
(The reference code stores `[N, 15, 3]`, transposes it to `[N, 3, 15]` and flattens it when saving.)

Renderers want **coefficient-major** RGB triplets: for Gaussian *n*, `sh[n][k] = (r, g, b)` for
`k = 0..15`, with the DC term at `k = 0`. The loader transposes:

```
R = number of f_rest properties / 3          (15 for degree 3)
sh[n][0]     = (f_dc_0,        f_dc_1,          f_dc_2)
sh[n][k≥1]   = (f_rest[k−1],   f_rest[R+k−1],   f_rest[2R+k−1])
```

If you get this wrong, **nothing looks broken until Task 08**, when highlights come out in the wrong
colors. Test it now.

### 5. SH degree from the header

Count the `f_rest_*` properties: `rest = 3 · ((d + 1)² − 1)`.

| `f_rest` count | coefficients per channel (incl. DC) | degree |
|----------------|-------------------------------------|--------|
| 0 | 1 | 0 |
| 9 | 4 | 1 |
| 24 | 9 | 2 |
| 45 | 16 | 3 |

Anything else is an error. Find properties **by name**; don't hard-code byte offsets (other tools reorder
or add properties).

### 6. Memory layout: struct-of-arrays

Store each attribute in its own contiguous array (`positions[N]`, `scales[N]`, ...), not an array of
`Gaussian` structs. It's what the GPU wants (Task 10), what SIMD wants, and what lets you drop the
45 `f_rest` floats when you only need DC color.

Read speed: read the whole body in one go (or in large chunks) and de-interleave in a loop. Parsing
floats one `fread` at a time is 10–100× slower.

### 7. What real data looks like

Print statistics once you can load it. Expect:

- **Bounds:** the min/max box is huge. 3DGS puts large, faint Gaussians far away for the sky and
  background. Use the 1st/99th percentile, or the median, to find the "interesting" center.
- **Opacity:** strongly bimodal. Many near 0 (about to be pruned) and many near 1.
- **Scale:** spans orders of magnitude; `log(scale)` is roughly bell-shaped. Many Gaussians are very flat
  (one scale ≪ the other two): little surface discs.

## Read

- **Code**, `gaussian-splatting/scene/gaussian_model.py`: `construct_list_of_attributes`, `save_ply`,
  `load_ply`. Find the `transpose(1, 2)` that makes `f_rest` channel-major.
- **Format**: Paul Bourke's PLY description ([references](../learn/references.md#formats)), header grammar only.

## Build

- [x] `include/vertexnova/gs/core/gaussian_cloud.h`

  ```cpp
  namespace vne::gs {
  [[nodiscard]] constexpr int shCoeffCount(int degree) { return (degree + 1) * (degree + 1); }

  struct GaussianCloud {                       // all values activated
      std::vector<math::Vec3f> positions;
      std::vector<math::Vec3f> scales;
      std::vector<math::Quatf> rotations;
      std::vector<float> opacities;
      std::vector<float> sh;                   // size() * shCoeffCount(sh_degree) * 3, coefficient-major RGB
      int sh_degree = 0;

      [[nodiscard]] std::size_t size() const { return positions.size(); }
      [[nodiscard]] math::Vec3f dcColor(std::size_t i) const;   // 0.5 + C0 * sh[i][0], clamped >= 0
  };
  }  // namespace vne::gs
  ```

- [x] `include/vertexnova/gs/io/ply_reader.h` + `src/vertexnova/gs/io/ply_reader.cpp`

  ```cpp
  [[nodiscard]] bool readGaussianPly(const std::string& path, GaussianCloud& out, std::string* error = nullptr);
  [[nodiscard]] bool writeGaussianPly(const std::string& path, const GaussianCloud& cloud, std::string* error = nullptr);
  ```

  The writer undoes the activations (`log`, `logit`, w-first quaternion, channel-major `f_rest`). It makes
  round-trip tests and fixtures trivial, and Task 16's Python trainer writes the same layout.

- [x] `testdata/three_gaussians.ply`: 3 Gaussians with chosen values, written once by a small test helper
  or by hand. Record how it was made in `testdata/README.md`.
- [x] `tests/ply_reader_test.cpp`
- [x] `examples/03_ply_stats/`: `example_03_ply_stats <file.ply>` prints the count, SH degree, load time,
  min/max and 1st/99th-percentile bounds, the median position, a 10-bin opacity histogram and log-scale
  statistics.

## Test

| Case | Expected |
|------|----------|
| Header of `three_gaussians.ply` | 3 vertices, degree 3, property order parsed |
| `scale_0 = log(2)` | `scales[i].x == 2` |
| `opacity = 0` | `0.5` |
| `rot = (2, 0, 0, 0)` | unit identity quaternion |
| `rot = (0, 1, 0, 0)` (file order) | quaternion with x = 1, i.e. `Quatf(1, 0, 0, 0)` in vnemath order |
| `f_rest_k = k` (pattern) | `sh[i][1] = (0, 15, 30)`, `sh[i][15] = (14, 29, 44)` |
| Headers with 0 / 9 / 24 / 45 `f_rest` | degree 0 / 1 / 2 / 3 |
| 10 `f_rest` properties | error, with a message naming the problem |
| `format ascii 1.0` or big-endian | error (not supported) |
| Missing `opacity` | error |
| Round trip: write → read | values equal to 1e-6 (relative) |

## Done when

- [x] Tests pass.
- [ ] `example_03_ply_stats` loads the garden scene (`garden/point_cloud/iteration_30000/point_cloud.ply`
  from the Inria pre-trained models; the Python prototype reported 5,834,784 Gaussians) in a few seconds,
  and the statistics match what §7 predicts. (Run when the file is available locally.)

## Check yourself

1. Why is opacity stored as a logit?
2. How many `f_rest` values does a degree-2 scene have?
3. You forgot to transpose `f_rest`. When, and how, would you find out?
4. Why do the median center and the bounding-box center differ so much?
5. Why struct-of-arrays?

<details><summary>Answers</summary>

1. So the optimizer can move it freely over all real numbers while the activated opacity stays in (0, 1).
2. 24 (8 per channel).
3. Only in Task 08, when view-dependent color is evaluated: highlights get the wrong hue. DC color, and so
   Tasks 04–07, look fine. Hence the pattern test now.
4. Background and sky Gaussians sit very far away and stretch the box; the median ignores them.
5. Contiguous per-attribute arrays upload straight to GPU buffers, vectorize well, and let you skip
   attributes you don't need.

</details>

## Going further

- Memory-map the file instead of reading it (`mmap`), and compare load times.
- Filter the cloud: drop Gaussians with opacity < 0.05 or outside the 99th-percentile box. How many are
  left? You'll want a small cloud for the CPU tasks.
- Look at the `.splat` format used by web viewers (32 bytes per Gaussian). What does it throw away?

## My notes

`GaussianCloud` is SoA (not `vector<Gaussian3D>`). The reader finds properties by name, activates
`exp` / sigmoid / normalize, and remaps PLY `(w,x,y,z)` into `Quatf(x,y,z,w)`. `f_rest` is
channel-major in the file and coefficient-major RGB in memory; the pattern test catches a missed
transpose before Task 08. `writeGaussianPly` undoes activations for round-trips.

Garden timing was not run here: the pretrained PLY is outside the repo. Use
`example_03_ply_stats <path-to-point_cloud.ply>` when it is available.
