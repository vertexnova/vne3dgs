---
name: vne-testing
description: >
  Conventions for writing and organizing GoogleTest tests in vne3dgs. Use this
  skill when adding or changing tests, or when a behavior change needs test
  coverage.
---

# Testing Conventions

vne3dgs uses GoogleTest. A feature or bug fix ships with a test unless the
behavior can only be observed by looking at a PNG. Tests must be deterministic
and fast.

## 1. Layout

- `tests/` - the main suite, one file per unit, built into a single `vne3dgs_tests`
  binary.
- Register new files in [tests/CMakeLists.txt](../../tests/CMakeLists.txt)
  (`TEST_SOURCES`).

## 2. File and case naming

- One test file per unit: `<subject>_test.cpp`, matching the header under test
  (`gaussian2d.h` -> `gaussian2d_test.cpp`, `conic.h` -> `conic_test.cpp`).
- Follow project naming in test code too (PascalCase types, camelCase methods,
  snake_case locals); see [vne-coding-style](../vne-coding-style/SKILL.md).

## 3. Structure

- Arrange, Act, Assert, with a blank line between phases.
- One behavior per `TEST` / `TEST_F`; give the case a name that states the behavior.
- Keep mocks minimal and assert on observable behavior, not implementation details.
- Learning-task specs under `docs/vertexnova/gs/tasks/` list the cases and
  expected values; write those tests first.

## 4. Determinism

- No `sleep`, no timeouts, no wall-clock or ordering assumptions.
- Fix random seeds; avoid data races in threaded tests.
- Use `EXPECT_NEAR` with the tolerance from the task (often `1e-4`) for float math.

## 5. What is worth a test here

Core math is testable without I/O and is where most silent breakage lives: conic
inversion, eigenvalues, radius, power, alpha skip/cap, and front-to-back
transmittance. Prefer a unit test over a visual check when the behavior can be
observed through a number.

PNG examples (`examples/NN_*`) are smoke, not the golden test. They confirm
orientations and overlap; they do not replace `vne3dgs_tests`.

## 6. Running

```bash
ctest --test-dir <build-dir> --output-on-failure
<build-dir>/bin/vne3dgs_tests --gtest_filter=Gaussian2D.AxisAlignedWorkedExample
<build-dir>/bin/vne3dgs_tests --gtest_filter=Conic.PowerAtMeanIsZero
```

Build the test target with `VNE_GS_TESTS=ON` (default under `VNE_GS_DEV`); see
[vne-build-verify](../vne-build-verify/SKILL.md).

Source of truth: [tests/CMakeLists.txt](../../tests/CMakeLists.txt),
[CONTRIBUTING.md](../../CONTRIBUTING.md).
