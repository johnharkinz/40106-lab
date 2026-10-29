#!/bin/sh
# Builds the project (if needed) and renders one inverter to out/inverter.wav
# and out/inverter.csv. Any arguments are passed to render_wav (see --help).
set -e
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build build --target render_wav > /dev/null
build/render_wav "$@"
