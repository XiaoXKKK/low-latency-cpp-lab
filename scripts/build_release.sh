#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
cmake_bin=${CMAKE:-cmake}
if ! command -v "$cmake_bin" >/dev/null && [[ -x "$root/.tools/bin/cmake" ]]; then cmake_bin="$root/.tools/bin/cmake"; fi
"$cmake_bin" -S "$root" -B "$root/build/release" -DCMAKE_BUILD_TYPE=Release -DLAB_SANITIZER=none "$@"
"$cmake_bin" --build "$root/build/release" -j "${JOBS:-4}"
"$(dirname "$(command -v "$cmake_bin")")/ctest" --test-dir "$root/build/release" --output-on-failure
