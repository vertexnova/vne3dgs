# Internal dependencies

VertexNova internal libraries go here. CMake modules (vnecmake) are in **cmake/vnecmake** at the project root.

- **vnecommon** - Common utilities and build helpers.
  Clone or add as submodule into `deps/internal/vnecommon`.

- **vnelogging** - Logging library (e.g. spdlog-based).
  Clone or add as submodule into `deps/internal/vnelogging`.

- **vnemath** - Math library (vectors, matrices, quaternions; wraps GLM). **Required.**
  Clone or add as submodule into `deps/internal/vnemath`.
- **vneio** - Image I/O (`vne::io::image`). **Examples only** (PNG output). Mesh is off
  (`VNEIO_BUILD_MESH=OFF`). Clone or add as submodule into `deps/internal/vneio`, then
  `git -C deps/internal/vneio submodule update --init deps/external/nrrdio`.
  CMake also looks at `deps/external/vneio` and a sibling `../vneio`.

If vnecommon or vnelogging is missing, the root CMake skips it and vne3dgs builds without linking to vne::common or vne::logging. vnemath is required: CMake falls back to a sibling `../vnemath` checkout, then `find_package(VneMath)`.
vneio is not linked into the core library; examples that write PNGs are skipped if it is absent.

See each subdirectory's README (once the repo is present) for recommended branch/tag and build options.
