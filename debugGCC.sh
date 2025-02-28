#!/bin/bash
set -e  # Exit on error

# Set relative paths
build_directory="Build"
executable_name="mpmRenderer"

mkdir -p "$build_directory"

linkerFlags="-lX11 -lGL"
includeFlags="-I../src/external -I../src"
compiler_flags="$includeFlags -std=c++20 -frtti -fasynchronous-unwind-tables -fexceptions -g -Wall -D_DEBUG"

pushd "$build_directory"
g++ $compiler_flags ../src/main.cpp ../src/external/glad.c  -o "$executable_name" $linkerFlags
popd
