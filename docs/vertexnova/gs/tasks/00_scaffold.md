# Task 00 — Scaffold the Library

| Phase | Depends on | Unlocks | Status |
|-------|------------|---------|--------|
| 0 — Setup | — | [01](01_gaussians_2d.md) | [x] |

> **Goal:** a vne-standard C++20 library that builds, tests and runs an example before any 3DGS code exists.

## Why this task

Every later task adds a header, a source file, a test and often an example. You want that loop
(edit → build → test → run) to be boring before the math starts.

## Learn

### How the repo is wired

```
vne3dgs/
├── CMakeLists.txt          project, options, internal deps (VneUseDep), install rules
├── cmake/vnecmake/         shared CMake modules (ProjectSetup, ProjectWarnings, VneUseDep, ...)
├── deps/internal/          vnecommon, vnelogging, vnemath (git submodules)
├── deps/external/          googletest v1.17.0 (git submodule)
├── configs/config.h.in     → build/.../config.h (PROJECT_VERSION, paths)
├── include/vertexnova/gs/  public headers (gs.h umbrella, version.h, export.h)
├── src/vertexnova/gs/      implementation
├── tests/                  gtest suite → vne3dgs_tests
├── examples/NN_name/       one per task → bin/examples/example_NN_name
└── docs/vertexnova/gs/     this roadmap, tasks, learning notes
```

- **`vne_use_dep()`** (from `cmake/vnecmake/modules/VneUseDep.cmake`) adds an internal library only if its
  target doesn't already exist. That's how several vne libraries share one copy of vnelogging when nested.
- **vnemath** is required. CMake looks in `deps/internal/vnemath`, then `deps/external/vnemath`, then a
  sibling `../vnemath` checkout, then `find_package(VneMath)`.
- **Library type:** `VNE_GS_LIB_TYPE=shared|static`. vnelogging and vnemath are forced to the same type:
  a static library built without PIC can't be linked into a shared one on Linux, and on iOS mixing in
  shared libraries means Xcode signing for each one.

### Build options

| Option | Default | Meaning |
|--------|---------|---------|
| `VNE_GS_DEV` | `ON` when top-level | Tests + examples on (local development) |
| `VNE_GS_CI` | `OFF` | CI build: tests on, examples off |
| `VNE_GS_TESTS` | `ON` | Build `vne3dgs_tests` |
| `VNE_GS_EXAMPLES` | `OFF` (forced `ON` by `VNE_GS_DEV`) | Build `examples/` |
| `VNE_GS_LIB_TYPE` | `shared` | `shared` or `static` |

### Where each task's code goes

- Headers: `include/vertexnova/gs/<module>/<name>.h`, sources mirror them under `src/`.
- Add both to `src/CMakeLists.txt` (`SOURCE_FILES`, `HEADER_FILES`) and include the header from `gs.h`.
- Tests: `tests/<module>_test.cpp`, added to `TEST_SOURCES` in `tests/CMakeLists.txt`.
- Examples: `examples/NN_<name>/` with its own `CMakeLists.txt`, added in `examples/CMakeLists.txt`.
- Style: [CODING_GUIDELINES.md](../../../../CODING_GUIDELINES.md): camelCase functions, snake_case
  parameters and members, `k` prefix for constants, one concept per header.

## Read

- [CODING_GUIDELINES.md](../../../../CODING_GUIDELINES.md): naming, headers, error handling (skim once).
- `cmake/vnecmake/modules/VneUseDep.cmake`: how internal deps are added once.
- `../vnescene/CMakeLists.txt`, the `_vnescene_configure_vnemath_dep()` function: the pattern vne3dgs copies.

## Build

- [x] Remove the Python prototype (preserved at tag `python-prototype`)
- [x] Copy vnetemplate; rename `template` → `gs` (namespace `vne::gs`, target `vne3dgs`, options `VNE_GS_*`)
- [x] Submodules: vnecmake, googletest (v1.17.0), vnecommon, vnelogging, vnemath
- [x] `vne::gs::getVersion()` in `version.h` / `version.cpp`; `export.h`; umbrella `gs.h`
- [x] `tests/version_test.cpp`
- [x] `examples/00_hello_gs`
- [x] License → Apache 2.0; README; CHANGELOG; `.gitignore` keeps data out but allows `testdata/`
- [x] This roadmap, learning notes and task files

## Test

```bash
./scripts/build_macos.sh -t Debug -a test              # shared
./scripts/build_macos.sh -l static -t Debug -a test    # static
./build/shared/Debug/build-macos-clang-*/bin/examples/example_00_hello_gs
```

## Done when

- [x] Shared and static builds pass `vne3dgs_tests` on macOS.
- [x] `example_00_hello_gs` prints `vne3dgs version: 0.1.0`.
- [ ] CI green on GitHub after the first push (Linux GCC/Clang, macOS, Windows).

## Check yourself

1. Where would the header for the PLY reader (Task 03) go, and which two CMake files change?
2. How do you build a static library with tests?
3. Why does vne3dgs force vnemath's library type to match its own?
4. What does `VNE_GS_DEV` turn on, and why does it default to `OFF` when vne3dgs is used as a submodule?

<details><summary>Answers</summary>

1. `include/vertexnova/gs/io/ply_reader.h` + `src/vertexnova/gs/io/ply_reader.cpp`; add both to
   `src/CMakeLists.txt`, include from `gs.h`, and add `tests/ply_reader_test.cpp` to `tests/CMakeLists.txt`.
2. `./scripts/build_macos.sh -l static -t Debug -a test`, or `cmake -B build/static -DVNE_GS_LIB_TYPE=static`.
3. A static library built without PIC can't be linked into a shared library on Linux. On iOS, shared
   intermediate libraries each need code signing.
4. Tests and examples. A parent project shouldn't build vne3dgs's tests and examples by accident.

</details>

## My notes

_Fill in after finishing._
