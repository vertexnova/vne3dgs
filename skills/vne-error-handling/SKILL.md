---
name: vne-error-handling
description: >
  Decide how to report failure on the vne3dgs render path. Use this skill when
  adding or editing a fallible API, or whenever you are about to write `try`,
  `catch`, or a `noexcept` function that allocates.
---

# vne3dgs Error Handling

The CPU render path does not throw. Operations that can fail return
`std::optional`, `bool`, or a sentinel (for example radius `0` or alpha `0`)
and skip the work. Nothing may escape the public `VNE_GS_API` boundary on the
hot path.

The rule:

> Prefer a value the caller can ignore or skip. Reach for `try` only when a
> third-party API offers no non-throwing form, or when you are at a
> `noexcept` / destructor boundary that allocates.

## 1. Patterns already in the library

| Situation | What to return |
|-----------|----------------|
| Covariance is not positive definite | `computeConic` -> `std::nullopt`; `computeRadius` -> `0` |
| Splat too faint or numerically invalid | `computeAlpha` -> `0` (skip) |
| Pixel is saturated | `FrontToBackBlender::composite` -> `false` (splat not composited) |
| Example I/O (PNG) | `bool` from `saveImage`; log and return `1` from `main` |

Do not turn those into exceptions. Callers already branch on empty / zero / false.

## 2. When a catch is actually correct

Only two cases.

**A third-party API that throws by design.** Catch the narrowest type that works,
and say why. An empty catch must carry `@expected` (what `.clang-tidy`
`bugprone-empty-catch` looks for) and a reason.

**A `noexcept` or destructor boundary.** A `noexcept` function that allocates will
terminate, not report. Guard the allocating statement, or hoist it out.

Prefer non-throwing standard overloads where they exist:
`std::filesystem::create_directories(p, error_code)` rather than the throwing form.

## 3. Logging vs return codes

Logging tells a developer. A return value tells the caller. On the render path,
skip and continue; in examples and tools, log and exit non-zero.

## 4. Checklist

- [ ] No `throw` on the per-pixel / per-splat path.
- [ ] Invalid input is skipped (`nullopt`, `0`, `false`), not asserted in release.
- [ ] Every empty catch carries `@expected` and a reason.
- [ ] Every allocation in a `noexcept` function is guarded or hoisted out.
- [ ] Verified with [vne-build-verify](../vne-build-verify/SKILL.md).

Source of truth: [CODING_GUIDELINES.md](../../CODING_GUIDELINES.md) (Error Handling),
[conic.h](../../include/vertexnova/gs/core/conic.h),
[gaussian2d.h](../../include/vertexnova/gs/core/gaussian2d.h),
[front_to_back_blender.h](../../include/vertexnova/gs/core/front_to_back_blender.h).
