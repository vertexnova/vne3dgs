# Test fixtures

Fixtures used by unit tests and examples. Keep binary files small and checked in
so CI does not regenerate them.

## `three_gaussians.ply`

- **Size:** 2270 bytes
- **Format:** binary little-endian PLY, 3 vertices, SH degree 3 (`f_rest_0` … `f_rest_44`)
- **Provenance:** hand-authored fixture for Task 03; packed with Python `struct.pack('<f', …)`
  using the same property order as `writeGaussianPly`

Property order:

```text
x y z nx ny nz f_dc_0..2 f_rest_0..44 opacity scale_0..2 rot_0..3
```

Chosen stored (pre-activation) values:

- **Gaussian 0:** position `(1, 2, 3)`; `scale_0 = log(2)`; opacity logit `0` → `0.5`;
  `rot = (2, 0, 0, 0)` file order → identity; `f_dc = (0.1, -0.2, 0.3)`; `f_rest_k = k`
- **Gaussian 1:** `rot = (0, 1, 0, 0)` file order → `Quatf(1, 0, 0, 0)`; unit scales; zero DC / rest
- **Gaussian 2:** position `(-1, 0.5, 2)`; scales `(3, 2, 1)` after activation; opacity logit `2`;
  identity rotation; `f_rest_k = k`

### Regenerating

Prefer writing an equivalent cloud through `writeGaussianPly` and copying the output into
`testdata/`. A minimal Python packer that matches the writer’s layout:

```python
import math, struct
from pathlib import Path

def f(*vals):
    return b"".join(struct.pack("<f", float(v)) for v in vals)

props = (
    ["x", "y", "z", "nx", "ny", "nz", "f_dc_0", "f_dc_1", "f_dc_2"]
    + [f"f_rest_{i}" for i in range(45)]
    + ["opacity", "scale_0", "scale_1", "scale_2", "rot_0", "rot_1", "rot_2", "rot_3"]
)
header = "\n".join(
    ["ply", "format binary_little_endian 1.0", "element vertex 3"]
    + [f"property float {p}" for p in props]
    + ["end_header", ""]
).encode()

g0 = f(1, 2, 3, 0, 0, 0, 0.1, -0.2, 0.3, *range(45), 0, math.log(2), 0, 0, 2, 0, 0, 0)
g1 = f(0, 0, 0, 0, 0, 0, 0, 0, 0, *([0] * 45), 0, 0, 0, 0, 0, 1, 0, 0)
g2 = f(-1, 0.5, 2, 0, 0, 0, 0, 0, 0, *range(45), 2, math.log(3), math.log(2), 0, 1, 0, 0, 0)
Path("testdata/three_gaussians.ply").write_bytes(header + g0 + g1 + g2)
```

Commit the regenerated file when its contents or property layout change; do not regenerate on every CI run.
