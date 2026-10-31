#!/bin/sh
# host tests: needs gcc. The game-facing code (src/SellCar.c) is syntax-checked when clang is available.
set -e
cd "$(dirname "$0")/.."
gcc -O1 -Wall -Wextra -std=c11 -o "${TMPDIR:-/tmp}"/test_sc tests/test_sc.c
"${TMPDIR:-/tmp}"/test_sc
if command -v clang >/dev/null 2>&1; then
  clang -fsyntax-only -fms-extensions -Wall -Wextra -Wno-ignored-attributes -std=c11 -Iinclude -Isrc -Itests/win_stub src/SellCar.c && echo "syntax OK"
fi
