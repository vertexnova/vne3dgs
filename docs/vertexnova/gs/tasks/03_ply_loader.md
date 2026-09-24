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
  [[nodiscard]] int shCoeffCount(int degree) noexcept;  // (degree + 1)^2, degree clamped to [0, 3]

  class GaussianCloud {                        // all values activated
     public:
      static constexpr int kMaxShDegree = 3;
      static constexpr std::size_t kChannels = 3;

      [[nodiscard]] std::size_t size() const noexcept;
      [[nodiscard]] bool isEmpty() const noexcept;

      void resize(std::size_t count);   // every column together; SH keeps its prefix
      void reserve(std::size_t count);
      void clear() noexcept;

      [[nodiscard]] int shDegree() const noexcept;
      void setShDegree(int degree);     // clamps, and reshapes (and zeroes) the SH array
      [[nodiscard]] int shCoeffCount() const noexcept;

      // Mutable access is a span: writes in place, but cannot resize one column
      // out from under the others.
      [[nodiscard]] std::span<const math::Vec3f> positions() const noexcept;
      [[nodiscard]] std::span<math::Vec3f> positions() noexcept;
      // ... scales(), rotations(), opacities(), sh() likewise

      [[nodiscard]] math::Vec3f shCoefficient(std::size_t i, int k) const noexcept;
      void setShCoefficient(std::size_t i, int k, const math::Vec3f& rgb) noexcept;

      [[nodiscard]] math::Vec3f dcColor(std::size_t i) const noexcept;  // max(0, 0.5 + C0 * f_dc)
      [[nodiscard]] Gaussian3D gaussian(std::size_t i) const noexcept;  // gather one splat
      [[nodiscard]] bool isConsistent() const noexcept;                 // for asserts across an ABI
  };
  }  // namespace vne::gs
  ```

  **The invariant is the point.** `size()` is meaningless unless every column has that length and
  `sh()` has exactly `size() * shCoeffCount() * 3` entries. Handing out `std::vector&` would let a
  caller resize one column alone and leave `scales()[i]` undefined behaviour, so the class hands out
  spans and owns `resize`/`setShDegree` itself. Index access (`shCoefficient`, `dcColor`,
  `gaussian`) is bounds-checked and returns a zero value rather than reading past the end.

- [x] `io/ply_reader.h` / `PlyReader` and `io/ply_writer.h` / `PlyWriter`, over a private
      `src/.../io/ply_format.h` that models the whole PLY header

  ```cpp
  struct PlyReadOptions {
      bool skip_non_finite = true;      // drop NaN splats instead of failing the load
      std::size_t chunk_bytes = 4 << 20;  // body is streamed, never buffered whole
  };
  struct PlyReadStats {
      std::size_t declared_vertex_count, loaded_vertex_count, skipped_non_finite;
      int sh_degree;
  };

  class PlyReader {
   public:
      explicit PlyReader(const PlyReadOptions& options);
      [[nodiscard]] bool read(const std::string& path, GaussianCloud& out_cloud);
      [[nodiscard]] const PlyReadStats& stats() const noexcept;
      [[nodiscard]] const std::string& error() const noexcept;
  };

  class PlyWriter {                     // PlyWriteOptions { chunk_bytes, write_normals }
   public:
      [[nodiscard]] bool write(const std::string& path, const GaussianCloud& cloud);
      [[nodiscard]] const std::string& error() const noexcept;
  };

  // Convenience wrappers with default options:
  [[nodiscard]] bool readGaussianPly(const std::string& path, GaussianCloud& out, std::string* error = nullptr);
  [[nodiscard]] bool writeGaussianPly(const std::string& path, const GaussianCloud& cloud, std::string* error = nullptr);
  ```

  The writer undoes the activations (`log`, `logit`, w-first quaternion, channel-major `f_rest`). It makes
  round-trip tests and fixtures trivial, and Task 16's Python trainer writes the same layout.

  **Four things the reader has to get right, none of them obvious from the format:**

  1. **Element order.** The `vertex` body does not start at `end_header`: every element declared
     before it sits in between. Parse *all* elements and every scalar type, sum the preceding
     elements' `count × stride`, and seek past it. A reader that assumes `vertex` comes first will
     happily return another element's bytes as Gaussians, with no error at all.
  2. **The declared count sizes an allocation.** Check it against the real file length before
     resizing, or a corrupt header can ask for gigabytes the file cannot back.
  3. **Non-finite values are a policy, not an error.** Third-party scenes contain the occasional NaN
     splat. Dropping those and reporting the count beats rejecting the whole file.
  4. **Failure must not touch the destination.** Assemble into a local cloud and move it into place
     only once the whole body is consumed; otherwise a mid-file failure leaves the caller holding a
     cloud that reports the full `size()` with a garbage tail.

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
| An `element camera 1` declared **before** `element vertex` | vertex values still correct (the preceding element's bytes are skipped, not read as Gaussians) |
| A list-property element before `vertex` | error: its length is unknowable, so guessing is worse than refusing |
| Extra `uchar` / `double` / `int` properties interleaved in `vertex` | ignored; the Gaussian values are still correct |
| Header declaring 100M vertices in a 1-vertex file | error before any large allocation |
| One NaN Gaussian among three | loads 2, `stats().skipped_non_finite == 1`, survivors keep their order |
| Same file with `skip_non_finite = false` | error naming the vertex |
| Failed load into an already-populated cloud | destination unchanged and still consistent |
| `chunk_bytes = 1` | identical result to a single-chunk read |

## Done when

- [x] Tests pass.
- [x] Garden-scale load measured on a synthetic 5,834,784-Gaussian degree-3 file (1,447 MB), Release,
  macOS/clang: **709 ms, 1.38 GB peak RSS**. Streaming the body in chunks is what keeps peak memory at
  roughly the size of the cloud; buffering the whole file first cost 2.83 GB and 2,407 ms.
- [ ] Confirm against the real garden scene (`garden/point_cloud/iteration_30000/point_cloud.ply` from
  the Inria pre-trained models) and check the statistics against what §7 predicts. (Run when the file
  is available locally.)

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

`GaussianCloud` is SoA (not `vector<Gaussian3D>`), and it enforces that its columns stay the same
length: mutable access is `std::span`, so only `resize()` and `setShDegree()` can change shape. The
reader finds properties by **byte offset resolved from the name**, activates `exp` / sigmoid /
normalize, and remaps PLY `(w,x,y,z)` into `Quatf(x,y,z,w)`. `f_rest` is channel-major in the file and
coefficient-major RGB in memory; the pattern test catches a missed transpose before Task 08.
`PlyWriter` undoes activations for round-trips and batches the body into chunk-sized writes (one
`write` per few thousand vertices rather than one per float).

Resolving offsets once per file and hoisting the cloud's spans out of the per-vertex loop matters more
than it looks: the accessors are exported, so calling `cloud.sh()` inside the loop is a cross-library
call that cannot inline. Together with chunked reads it took garden scale from 2,407 ms to 709 ms.

Garden timing was not run here: the pretrained PLY is outside the repo. Use
`example_03_ply_stats <path-to-point_cloud.ply>` when it is available.
