

if [[ $1 == "X11" ]]; then
    build_dir=build/X11
elif [[ $1 == "EGL" ]]; then
    build_dir=build/EGL
else
    echo "Expected \"X11\" or \"EGL\""
    exit 1
fi

if [-z "$2"]; then
    echo "Expected that path to the python executable to use for compiling."
    exit 1
else
    python_dir=$2
fi

mkdir -p $build_dir

pushd $build_dir
ln -sf ../../src/shaders/vertexShader.glsl .
ln -sf ../../src/shaders/fragmentShader.glsl .

objcopy --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 vertexShader.glsl vertex_shader_data.o
objcopy --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 fragmentShader.glsl fragment_shader_data.o
popd


cmake -S . -B $build_dir -DPython_EXECUTABLE=$python_dir -DDISPLAY_TYPE:STRING=$1

pushd $build_dir
make
popd
