#!/usr/bin/env bash
# ----------------------------------------------------------------------
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   September 2026
#
# Description: Format (or check) C++ sources with the project .clang-format.
#              Same file set and flags as the CI "clang-format" job.
# Usage:       ./scripts/format.sh          # format in place
#              ./scripts/format.sh -check   # check only, non-zero exit on diffs
# ----------------------------------------------------------------------

set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format not found (CI pins clang-format 17)" >&2
    exit 1
fi

MODE_ARGS=(-i)
if [[ "${1:-}" == "-check" ]]; then
    MODE_ARGS=(--dry-run --Werror)
fi

find src include examples tests \
    -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.c' \) \
    -print0 \
| xargs -0 clang-format "${MODE_ARGS[@]}" --style=file --fallback-style=Google
