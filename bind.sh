python_dir=/home/woo487/anaconda3/bin/python


if [[ $1 == "X11" ]]; then
    build_dir=build/X11
elif [[ $1 == "EGL" ]]; then
    build_dir=build/EGL
else
    echo "Expected \"X11\" or \"EGL\""
    exit 1
fi


cmake -S . -B $build_dir -DPython_EXECUTABLE=$python_dir -DDISPLAY_TYPE:STRING=$1

pushd $build_dir
make
popd
