#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

CTAGS_FLAGS="
--languages=C,C++
--c++-kinds=+p
--fields=+iaS
--extras=+q
--sort=yes
--exclude=build
--exclude=.git
"
ctags -R $CTAGS_FLAGS \
  src \
  third_party \
  /usr/include/
