#!/usr/bin/env bash
# Thin wrapper: same behavior as vneio-style pre-commit (python clang_formatter).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
if [[ "${1:-}" == "-check" || "${1:-}" == "--check" ]]; then
  exec python3 scripts/clang_formatter.py all --dry-run
fi
if [[ $# -eq 0 ]]; then
  exec python3 scripts/clang_formatter.py all
fi
exec python3 scripts/clang_formatter.py "$@"
