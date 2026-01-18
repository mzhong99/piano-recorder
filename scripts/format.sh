#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# Only format files tracked by this repo (submodules won't be included)
mapfile -t FILES < <(
  git ls-files '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx'
)

if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "No tracked source files found."
  exit 0
fi

echo "Formatting ${#FILES[@]} files..."
clang-format -i "${FILES[@]}"

