#!/bin/bash
set -e
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build/

pushd build
cmake --build . -j$(nproc)
# cpack -G DEB
# cpack -G RPM
popd

