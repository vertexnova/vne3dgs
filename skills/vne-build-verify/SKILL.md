---
name: vne-build-verify
description: >
  Standard format, build, run, and test pipeline to run after modifying C++ in
  vne3dgs. Use this skill to verify a change before finishing.
---

# Build and Verify

Run this pipeline before completing any change to source. Build and run go
through the `./scripts/build_*.sh` scripts, not Qt-Creator or IDE binaries.

## 1. Format

Run the clang formatter directly (C/C++ scope matches CI: src, include, examples,
tests):

```bash
python3 scripts/clang_formatter.py all              # format everything
python3 scripts/clang_formatter.py all --dry-run    # CI-style check (no writes)
python3 scripts/clang_formatter.py src              # limit to a scope
python3 scripts/clang_formatter.py --file <path>    # a single file
./scripts/format.sh                                 # same as `all`
./scripts/format.sh -check                          # same as `all --dry-run`
```

CI enforces clang-format-17. `.clang-format` and `.clang-tidy` run in separate CI
actions, so format locally rather than hand-adjusting whitespace.

Note: `.clang-format` sets `SortIncludes: false`, so the formatter will not fix
include order for you. See [vne-header-hygiene](../vne-header-hygiene/SKILL.md).

## 2. Dependencies (first checkout or when deps change)

```bash
git submodule update --init --recursive
```

PNG examples need vneio's nrrdio (image component). If `deps/internal/vneio` is
present:

```bash
git -C deps/internal/vneio submodule update --init deps/external/nrrdio
```

## 3. Build (macOS)

```bash
./scripts/build_macos.sh -t Debug -a configure_and_build
```

Or drive CMake directly:

```bash
cmake -S . -B build/shared/Debug -DCMAKE_BUILD_TYPE=Debug -DVNE_GS_LIB_TYPE=shared
cmake --build build/shared/Debug -j10
```

Top-level `VNE_GS_DEV` (default ON) turns on tests and examples. CI uses
`-DVNE_GS_CI=ON` (tests on, examples off).

The script's build directory is
`build/<lib_type>/<build_type>/build-macos-<compiler>-<version>`, for example
`build/shared/Debug/build-macos-clang-17.0.0`. Later commands refer to it as
`<build-dir>`.

## 4. Run an example

Example binaries land in `<build-dir>/bin/examples/`:

```bash
<build-dir>/bin/examples/example_00_hello_gs
<build-dir>/bin/examples/example_01_gaussians_2d [output_dir]
```

## 5. Test

```bash
ctest --test-dir <build-dir> --output-on-failure
<build-dir>/bin/vne3dgs_tests --gtest_filter=Suite.TestName
```

Or via the script: `./scripts/build_macos.sh -t Debug -a test`. See
[vne-testing](../vne-testing/SKILL.md) for test conventions.

## 6. Other platforms

Same flag set; pick the matching script (all documented in
[scripts/README.md](../../scripts/README.md)):

| Platform | Script |
|----------|--------|
| Linux | `./scripts/build_linux.sh -t Debug -a configure_and_build` |
| Windows | `python scripts/build_windows.py -t Debug -a configure_and_build` |

Source of truth: [CONTRIBUTING.md](../../CONTRIBUTING.md),
[scripts/README.md](../../scripts/README.md).
