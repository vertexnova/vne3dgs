---
name: plain-ascii-authoring
description: >
  Keep all authored code, comments, docs, commit messages, and PR text free of
  AI-generated symbols and stock AI phrasing. Use this skill whenever you write
  or edit any text or source in this repository.
---

# Plain ASCII Authoring

VertexNova source and prose must read as human-written: plain ASCII punctuation,
no telltale AI glyphs, no filler phrasing. This file is itself ASCII-only and
describes the banned characters by name and code point so it passes its own check.

## 1. Banned glyphs, and the ASCII to use instead

Do not emit these non-ASCII characters. Replace each with the ASCII form:

| Banned character | Code point | Use instead |
|------------------|-----------|-------------|
| em dash | U+2014 | ` - ` (spaced hyphen) or ` -- ` |
| en dash | U+2013 | `-` or `--`, or `to` in ranges (`1 to 9`) |
| left/right single quote | U+2018 / U+2019 | straight `'` |
| left/right double quote | U+201C / U+201D | straight `"` |
| ellipsis | U+2026 | `...` |
| right/left arrow | U+2192 / U+2190 | `->` / `<-` |
| double arrow | U+21D2 | `=>` |
| multiplication sign | U+00D7 | `x` |
| bullet | U+2022 | `-` or `*` in markdown lists |
| box drawing (any) | U+2500..U+257F | hyphen, vertical bar, or plus in comment banners |
| non-breaking space | U+00A0 | a normal space |
| checkmark, cross marks | U+2713 / U+2717 / U+2705 | words: `pass` / `fail`, or `[x]` / `[ ]` |

No emoji anywhere in source, comments, commit messages, or PR text.

Section banners use ASCII hyphens only, for example:

```cpp
// ---- geometry helpers -------------------------------------------------
```

## 2. Banned phrasing

Avoid stock AI tells in comments and docs. Do not write:

- "delve", "it is worth noting", "in today's fast-paced", "at the end of the day".
- "Certainly!", "Great question!", "I hope this helps", "Let's dive in".
- Needless hedging ("it seems", "arguably", "essentially") and empty summaries.

Comments state intent and non-obvious reasoning only. Match the density and tone of
the surrounding code; do not narrate what the code plainly says.

## 3. Detection

Scan any path for non-ASCII bytes before committing:

```bash
rg -nP "[^\x00-\x7F]" <path>
```

Repo-wide, excluding generated and vendored trees:

```bash
rg -nP "[^\x00-\x7F]" src include examples tests scripts skills docs cmake
```

A clean run prints nothing. Investigate every hit.

## 4. Exceptions

Legitimate non-ASCII is allowed only in genuine data: test fixtures and reference
assets that deliberately exercise Unicode under `tests/` and `testdata/`, and
third-party code under `deps/`. Learning-task docs under `docs/vertexnova/gs/`
may keep math symbols already in those files; do not add new ones. Keep
exceptions out of hand-authored source, comments, and messages.

## 5. Commits and PRs

Conventional Commits (`feat:`, `fix:`, `chore:`) with ASCII-only bodies. No emoji,
no smart punctuation, no AI filler in commit or PR descriptions.
