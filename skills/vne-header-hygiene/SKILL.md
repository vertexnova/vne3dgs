---
name: vne-header-hygiene
description: >
  Enforce include ordering, header self-containment, and forward-declaration
  rules. Use this skill when adding or editing #include directives or creating
  C++ headers in vne3dgs.
---

# Header Hygiene

Rules for includes and headers in VertexNova. Full context is in the Header Files
section of [CODING_GUIDELINES.md](../../CODING_GUIDELINES.md).

## 1. Include group order

Group includes, one blank line between groups, in this order:

1. The corresponding header (in a `.cpp`): `#include "vertexnova/gs/core/conic.h"`.
2. Local project headers, relative to the target's include root:
   `#include "vertexnova/gs/export.h"`.
3. Other VertexNova public headers: `#include "vertexnova/math/core/vec.h"`.
4. System headers: `#include <vector>`, `<string>`, `<optional>`.
5. Third-party headers: `#include <gtest/gtest.h>`.

`.clang-format` sets `SortIncludes: false` and `IncludeBlocks: Preserve`, so the
formatter will neither sort nor regroup includes. Order them by hand, alphabetically
within each group, and keep the blank-line separators.

## 2. Header self-containment

Every `.h` must compile on its own. Include what you use directly in the header;
never rely on a type being pulled in transitively by another include or by the
`.cpp` that includes it. If a header names `math::Mat2f` or `Conic`, that header
includes or forward-declares it, not its callers.

## 3. Forward declarations

Prefer a forward declaration over an include when only a pointer or reference is
used in the header. Include the full definition only when the layout is needed
(by value, base class, container element, or member access).

## 4. Header structure

- `#pragma once` first, then the copyright banner block.
- Then: includes (grouped as above), forward declarations, `namespace`, then the
  class / struct / enum. Close with `}  // namespace vne::gs`.

## 5. After editing includes

Run `python3 scripts/clang_formatter.py all` (or add `--dry-run` to check), then
build. Because sorting is off, a build is the only check that an include is
actually present: a header that compiles only because a sibling included it first
will break the moment include order changes. See
[vne-build-verify](../vne-build-verify/SKILL.md).

Source of truth: [CODING_GUIDELINES.md](../../CODING_GUIDELINES.md) (Header Files).
