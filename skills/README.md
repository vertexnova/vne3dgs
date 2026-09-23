# vne3dgs Skills

Task playbooks for working in vne3dgs. Each folder holds a self-contained
`SKILL.md` (YAML frontmatter plus a short body) that any AI coding tool or human
can read. They are generic and tool-agnostic; they capture the conventions
already documented in `CODING_GUIDELINES.md`, `CONTRIBUTING.md`, and
`scripts/README.md`, distilled into a checklist you apply while working.

Format and structure follow the vnerhi / Filament skills pattern (one `SKILL.md`
per skill, cross-linked with relative paths), rewritten against vne3dgs scripts,
CMake options (`VNE_GS_*`), and layout (`examples/` not `samples/`).

## Skills

| Skill | Enforces | Fires when |
|-------|----------|------------|
| [vne-coding-style](vne-coding-style/SKILL.md) | Naming, formatting, initialization, modern C++ | Writing or editing C++ |
| [plain-ascii-authoring](plain-ascii-authoring/SKILL.md) | ASCII-only in new and edited text | Producing any code, comment, doc, or message |
| [vne-header-hygiene](vne-header-hygiene/SKILL.md) | Include order, header self-containment, forward decls | Editing includes or creating headers |
| [vne-build-verify](vne-build-verify/SKILL.md) | Format, build, run, and test pipeline via scripts | Before finishing any source change |
| [vne-testing](vne-testing/SKILL.md) | GoogleTest layout and determinism | Adding or changing tests |
| [vne-error-handling](vne-error-handling/SKILL.md) | optional / skip / log, when a catch is correct | Calling a fallible API, or writing `try` / `catch` / `noexcept` |

## Using these skills

- Claude Code: symlink this directory into `.claude/skills/` so the skills are
  auto-discovered and invocable by name:

  ```bash
  mkdir -p .claude/skills
  for s in skills/*/; do ln -sfn "../../$s" ".claude/skills/$(basename "$s")"; done
  ```

- Cursor and others: read the matching `SKILL.md` before build, test, or style work.

## Self-check

Keep this directory ASCII-clean per
[plain-ascii-authoring](plain-ascii-authoring/SKILL.md):

```bash
rg -nP "[^\x00-\x7F]" skills
```

A clean run prints nothing.
