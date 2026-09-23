# Vne3dgs Scripts

Build scripts for macOS, Linux, and Windows. Formatting matches vnerhi
(`scripts/clang_formatter.py`, `scripts/format.sh`).

## Format

C/C++ scope matches CI: `src`, `include`, `examples`, `tests`.

```bash
python3 scripts/clang_formatter.py all              # format in place
python3 scripts/clang_formatter.py all --dry-run    # CI-style check
python3 scripts/clang_formatter.py --file path/to/file.cpp
./scripts/format.sh                                 # same as `all`
./scripts/format.sh -check                          # same as `all --dry-run`
```

CI pins clang-format 17. `.clang-format` is identical to vnerhi.

## Build Scripts

### Linux (`build_linux.sh`)

```bash
./scripts/build_linux.sh -t Debug -a configure_and_build
./scripts/build_linux.sh -c clang -j 20 -t Release
./scripts/build_linux.sh -interactive
./scripts/build_linux.sh -clean -t Debug
```

**Options:** `-t` build type, `-c` compiler (gcc|clang), `-a` action (configure|build|configure_and_build|test), `-j` jobs, `-clean`, `-interactive`, `-h`

### macOS (`build_macos.sh`)

```bash
./scripts/build_macos.sh -t Debug -a configure_and_build
./scripts/build_macos.sh -xcode -t Release
./scripts/build_macos.sh -xcode-only -t Release
./scripts/build_macos.sh -interactive
```

**Options:** `-t` build type, `-a` action, `-xcode` (also generate Xcode project), `-xcode-only`, `-j` jobs, `-clean`, `-interactive`, `-h`

### Windows

**Bash** (Git Bash / WSL): `build_windows.sh`

```bash
./scripts/build_windows.sh -t Debug -a configure_and_build
./scripts/build_windows.sh -j 8 -t Release
```

**Python** (recommended on Windows): `build_windows.py`

Run from a **Visual Studio Developer Command Prompt**:

```bash
python scripts/build_windows.py -t Debug -a configure_and_build
python scripts/build_windows.py --build-type Release --jobs 8
python scripts/build_windows.py --interactive
python scripts/build_windows.py --build-type Release --clean
```

**Options:** `-t` / `--build-type`, `-a` / `--action`, `-j` / `--jobs`, `--clean`, `--interactive`, `-h`
