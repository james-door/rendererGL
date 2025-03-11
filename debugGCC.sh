#!/bin/bash
set -e  # Exit on error

# Set relative paths

includeFlags="-I../src/external -I../src"
compiler_flags="$includeFlags -std=c++20 -frtti -fasynchronous-unwind-tables -fexceptions -g -Wall -D_DEBUG -DPYTHON_BINDING=0"
build_directory="Build"

if [[ $1 == "X11" ]]; then
    linkerFlags="-lX11 -lGL"
    files_to_compile="../src/glrendererX11.cpp ../src/external/glad.c ../src/external/glad_glx.c"
    executable_name="rendererX11"
elif [[ $1 == "EGL" ]]; then
    linkerFlags="-lGL -lEGL"
    files_to_compile="../src/glrendererEGL.cpp ../src/external/glad.c"
    executable_name="rendererEGL"
else
    echo "Expected \"X11\" or \"EGL\""
    exit 1
fi

mkdir -p "$build_directory"
pushd "$build_directory"
g++ $compiler_flags $files_to_compile -o "$executable_name" $linkerFlags
popd
