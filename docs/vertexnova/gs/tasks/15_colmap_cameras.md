# Task 15 — Cameras from Photos (COLMAP)

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 5 — Training | [04](04_camera_and_points.md), [09](09_viewer_shell.md) | [16](16_pytorch_trainer.md) | [ ] |

> **Goal:** read a COLMAP reconstruction (camera intrinsics, photo poses, sparse points), show it in the
> viewer, and render the pre-trained scene **from a real training camera** to compare with the photo.

## Why this task

Training starts from COLMAP output: it provides every photo's camera and the initial point cloud. It
also gives you the best end-to-end correctness check so far: render the pre-trained garden from a
training camera and compare with the actual photo. If Tasks 04–13 got every convention right, they match
closely.

## Learn

### 1. Structure from motion in one paragraph

COLMAP detects features (SIFT) in every photo, matches them across photos, then incrementally solves for
camera poses and 3D points that best explain the matches, refining everything with **bundle adjustment**
(minimizing reprojection error). Output: per-camera intrinsics, per-photo pose, and a sparse point cloud
(each point seen in ≥ 2 photos, with a color).

### 2. Output files (`sparse/0/`)

All binary files are little-endian.

**`cameras.bin`**: `uint64 num_cameras`, then per camera:

```
int32  camera_id
int32  model_id           0 SIMPLE_PINHOLE (f, cx, cy)   1 PINHOLE (fx, fy, cx, cy)   2 SIMPLE_RADIAL, 4 OPENCV, ...
uint64 width
uint64 height
double params[n]          n depends on the model
```

3DGS needs **undistorted** pinhole cameras (`SIMPLE_PINHOLE` or `PINHOLE`); reject other models with a
clear error.

**`images.bin`**: `uint64 num_images`, then per image:

```
int32  image_id
double qvec[4]            (w, x, y, z)  rotation world → camera
double tvec[3]            translation world → camera
int32  camera_id
char   name[]             null-terminated
uint64 num_points2D
{ double x, double y, int64 point3D_id } × num_points2D      (skip these; you don't need them)
```

**`points3D.bin`**: `uint64 num_points`, then per point:

```
uint64 point3D_id
double xyz[3]
uint8  rgb[3]
double error
uint64 track_length
{ int32 image_id, int32 point2D_idx } × track_length          (skip)
```

### 3. Pose convention

COLMAP uses the **OpenCV camera convention** (+X right, +Y down, +Z forward), and `(qvec, tvec)` map world
→ camera: `X_cam = R(q) · X_world + t`. That's exactly `gs::Camera` from Task 04: `rotation = R(q)`,
`translation = t`. Camera center: `C = −Rᵀ t`. Note `qvec` is w-first (like PLY; unlike
`vne::math::Quat`).

### 4. Resolution and the dataset layout

Mip-NeRF 360 scenes look like:

```
garden/
├── images/          full resolution (~5K wide for garden)
├── images_2/ images_4/ images_8/   downsampled copies
└── sparse/0/        cameras.bin, images.bin, points3D.bin (intrinsics are for full resolution)
```

The reference trainer uses `images_4` for outdoor scenes and `images_2` for indoor ones. When you use a
downsampled folder, **scale `fx, fy, cx, cy` by the same factor**.

Shortcut without the dataset: each Inria pre-trained model folder has a **`cameras.json`** with the
training cameras (position, rotation, fx, fy, width, height). Worth reading too; compare it with your
COLMAP-derived cameras.

### 5. Finding "up" properly

Average the cameras' **up vectors** (`−(second row of R)`, since camera +Y is down) and use that as the
viewer's up. For a handheld capture walking around an object this is reliable. This retires the up-axis
combo box from Task 09.

### 6. Scene extent

The trainer scales position learning rates by the **scene extent**:
`1.1 × max distance from the mean camera center to any camera center`. Compute it now; Task 16 needs it.

## Read

- **COLMAP docs**, "Output format" ([references](../learn/references.md#formats)): the binary layouts
  and the pose convention.
- **COLMAP source**, `scripts/python/read_write_model.py`: the reference reader; your C++ mirrors it.
- **Code**, `gaussian-splatting/scene/colmap_loader.py`, `scene/dataset_readers.py`
  (`readColmapSceneInfo`, `getNerfppNorm`), `utils/camera_utils.py` (resolution scaling).

## Build

- [ ] `include/vertexnova/gs/io/colmap_reader.h` + `.cpp`

  ```cpp
  namespace vne::gs {
  struct ColmapCamera { int id; int model; std::uint64_t width, height; std::vector<double> params; };
  struct ColmapImage  { int id; math::Quatf rotation; math::Vec3f translation; int camera_id; std::string name; };
  struct ColmapPoint  { math::Vec3f xyz; math::Vec3f rgb; float error; };
  struct ColmapReconstruction {
      std::vector<ColmapCamera> cameras;
      std::vector<ColmapImage> images;
      std::vector<ColmapPoint> points;
  };
  [[nodiscard]] bool readColmapBinary(const std::string& sparse_dir, ColmapReconstruction& out, std::string* error = nullptr);
  [[nodiscard]] Camera toCamera(const ColmapImage& image, const ColmapCamera& camera, float resolution_scale = 1.0f);
  [[nodiscard]] math::Vec3f estimateUp(const ColmapReconstruction& rec);
  [[nodiscard]] float sceneExtent(const ColmapReconstruction& rec);
  }  // namespace vne::gs
  ```

- [ ] `tests/colmap_reader_test.cpp`: tiny binary files written by a test helper.
- [ ] Viewer: draw camera frustums (lines) and sparse points; a list of training images; "snap to
  training camera N"; a side-by-side or split view of **render vs photo** (load the photo with vneio)
  with the PSNR shown.

## Test

| Case | Expected |
|------|----------|
| Hand-written `cameras.bin` with one PINHOLE camera | fields parsed; `toCamera` gives those `fx, fy, cx, cy` |
| SIMPLE_PINHOLE | `fx = fy = f` |
| OPENCV model | error mentioning undistortion |
| Image with `qvec = (1,0,0,0)`, `tvec = (0,0,−5)` | camera center `(0, 0, 5)` |
| `qvec` order | w-first read, converted to `Quatf(x, y, z, w)` |
| `resolution_scale = 0.25` | intrinsics and size scaled by 0.25 |
| 4 cameras on a circle, all with up = +Y | `estimateUp ≈ (0, 1, 0)` |

## Done when

- [ ] Tests pass.
- [ ] In the viewer the garden's training cameras sit in a ring around the table, looking inward.
- [ ] Rendering the **pre-trained** garden from several training cameras (at `images_4` resolution) gives
  **PSNR ≈ 25–30 dB** against the real photos. A much lower value points to a convention bug somewhere
  in Tasks 04–13; find it.

## Check yourself

1. Why must 3DGS training use undistorted images?
2. `qvec = (0.7071, 0, 0.7071, 0)`, `tvec = (0, 0, 0)`: where does this camera look?
3. Why scale intrinsics when using `images_4`?
4. Why is render-vs-photo PSNR on a *training* view a strong test of your pipeline?

<details><summary>Answers</summary>

1. The rasterizer assumes an ideal pinhole projection; lens distortion would bend straight lines that the
   linear projection can't represent.
2. It's a 90° rotation about +Y for world→camera, so camera +Z (forward) is `Rᵀ·(0,0,1)` in world
   coordinates. Work it through: the camera looks along world −X.
3. Intrinsics are in pixels of the full-resolution image; a 4× smaller image has 4× smaller focal lengths
   and principal point.
4. The model was optimized to reproduce exactly those photos from exactly those cameras. Any convention
   error (axes, pixel centers, SH direction, quaternion order) drops the PSNR sharply.

</details>

## Going further

- Parse the text format (`cameras.txt` etc.) too; it's handy for hand-editing tests.
- Run COLMAP yourself on 50 photos of an object on your desk. That's your Task 17 dataset.

## My notes

_Fill in after finishing._
