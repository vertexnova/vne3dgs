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

Executables are placed in `build/shared/bin/examples/` (or `build/static/bin/examples/`).

## Available Examples

| Example | Task | What it shows |
|---------|------|---------------|
| `00_hello_gs` | [00](../docs/vertexnova/gs/tasks/00_scaffold.md) | Library links and runs; prints `vne::gs::getVersion()` |

**Run:** `./build/<lib_type>/bin/examples/example_00_hello_gs`
