#!/usr/bin/env bash
# Zero-dependency build: needs only g++/gcc. No GPU, no -dev packages, no apt.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
echo "[1/2] miniz (C)"
gcc -std=c11 -O2 -c third_party/miniz.c -o build/miniz.o
echo "[2/2] cod2-iwi-gallery (C++)"
g++ -std=c++17 -O2 -Isrc src/*.cpp build/miniz.o -o build/cod2-iwi-gallery
echo "-> build/cod2-iwi-gallery"
