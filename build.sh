#!/usr/bin/env bash
# Configure and compile mini-pd.
# On /mnt/c (WSL 9p) CMake's configure_file fails with "Operation not permitted",
# so the build tree lives on the Linux disk and ./build is a symlink to it.
set -euo pipefail
cd "$(dirname "$0")"

if [[ "$(pwd -P)" == /mnt/* ]]; then
  real="/tmp/mini-pd-build"
  mkdir -p "$real"
  if [[ -e build && ! -L build ]]; then
    echo "error: ./build exists and is not a symlink." >&2
    echo "       CMake cannot configure a real directory on /mnt/c." >&2
    echo "       Remove it and re-run:  rm -rf build && ./build.sh" >&2
    exit 1
  fi
  ln -sfn "$real" build
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-Release}"
cmake --build build -j"$(nproc)"
echo "binaries: ./build/mini-pd  ./build/test_hpwl  ./build/test_overflow  ./build/test_mst"
