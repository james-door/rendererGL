#!/bin/bash
set -e  # Exit on error

# Set relative paths
executable_name="mpmRenderer"

includeFlags="-I../../src/external -I../../src"
compiler_flags="$includeFlags -std=c++20 -frtti -fasynchronous-unwind-tables -fexceptions -g -Wall -D_DEBUG"

if [[ $1 == "X11" ]]; then
    linkerFlags="-lX11 -lGL"
    files_to_compile="../../src/main.cpp ../../src/external/glad.c ../../src/external/glad_glx.c"
    build_directory="Build/X11"
elif [[ $1 == "EGL" ]]; then
    linkerFlags="-lGL -lEGL"
    files_to_compile="../../src/mainEGL.cpp ../../src/external/glad.c"
    build_directory="Build/EGL"
else
    echo "Expected \"X11\" or \"EGL\""
    exit 1
fi

mkdir -p "$build_directory"
pushd "$build_directory"
g++ $compiler_flags $files_to_compile -o "$executable_name" $linkerFlags
popd
