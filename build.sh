#!/bin/bash
set -euo pipefail

REBUILD="${REBUILD:-0}"

if [[ "$REBUILD" -eq 1 ]]; then
      echo "Cleaning build directory!"
      rm -rf build
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cmake -S . -B ${SCRIPT_DIR}/build -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build ${SCRIPT_DIR}/build --parallel
