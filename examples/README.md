# Vne3dgs Examples

One example per learning task. Each task file in [docs/vertexnova/gs/tasks/](../docs/vertexnova/gs/tasks/)
says which example it adds.

## Building Examples

From the project root (use `build/shared` or `build/static`):

```bash
# Shared library build
cmake -B build/shared -DVNE_GS_EXAMPLES=ON -DVNE_GS_LIB_TYPE=shared
cmake --build build/shared

# Static library build
cmake -B build/static -DVNE_GS_EXAMPLES=ON -DVNE_GS_LIB_TYPE=static
cmake --build build/static
```

Alternatively, `-DVNE_GS_DEV=ON` enables both tests and examples (the default when vne3dgs is the top-level project).

PNG examples need **vneio** (image component only). It is an examples-only dependency:
`deps/internal/vneio`, with fallbacks to `deps/external/vneio` and a sibling `../vneio`.
Initialize nrrdio after cloning: `git -C deps/internal/vneio submodule update --init deps/external/nrrdio`.
The core `vne3dgs` library does not link vneio.

Executables are placed in `build/shared/bin/examples/` (or `build/static/bin/examples/`).

## Available Examples

| Example | Task | What it shows |
|---------|------|---------------|
| `00_hello_gs` | [00](../docs/vertexnova/gs/tasks/00_scaffold.md) | Library links and runs; prints `vne::gs::getVersion()` |
| `01_gaussians_2d` | [01](../docs/vertexnova/gs/tasks/01_gaussians_2d.md) | Five 2D Gaussians alpha-blended to `gaussians_2d.png` (and reversed order) |

**Run:** `./build/<lib_type>/bin/examples/example_00_hello_gs`

```bash
./build/<lib_type>/bin/examples/example_01_gaussians_2d [output_dir]
```
