#!/bin/bash
set -e  # Exit on error

# Set relative paths
includeFlags="-I../../src/external -I../../src"

if [[ $2 == "release" ]]; then
    compiler_flags="$includeFlags -std=c++20 -O3 -Wall -DPYTHON_BINDING=0"
    build_directory="Build/Release"
elif [[ $2 == "debug" ]]; then
    compiler_flags="$includeFlags -std=c++20 -frtti -fasynchronous-unwind-tables -fexceptions -g -Wall -D_DEBUG -DPYTHON_BINDING=0"
    build_directory="Build/Debug"
else
    echo "Expected second parameter to be 'debug' or 'release'"
    exit 1
fi

if [[ $1 == "X11" ]]; then
    linkerFlags="-lX11 -lGL"
    files_to_compile="../../src/glrendererX11.cpp ../../src/external/glad.c ../../src/external/glad_glx.c"
    executable_name="rendererX11"
elif [[ $1 == "EGL" ]]; then
    linkerFlags="-lGL -lEGL -ldl"
    files_to_compile="../../src/glrendererEGL.cpp ../../src/external/glad.c ../../src/external/glad_egl.c"
    executable_name="rendererEGL"
else
    echo "Expected first parameter to be 'X11' or 'EGL'"
    exit 1
fi

mkdir -p "$build_directory"
pushd "$build_directory"

ln -sf ../../src/shaders/vertexShader.glsl .
ln -sf ../../src/shaders/fragmentShader.glsl .

objcopy --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 vertexShader.glsl vertex_shader_data.o
objcopy --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 fragmentShader.glsl fragment_shader_data.o

g++ $compiler_flags $files_to_compile vertex_shader_data.o fragment_shader_data.o -o "$executable_name" $linkerFlags

popd
