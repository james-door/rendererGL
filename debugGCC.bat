@echo on
:: Set relative paths
set build_directory=Build
set executable_name=mpmRenderer.exe

if not exist %build_directory% (
    mkdir %build_directory%
)


set includeFlags=-I"..\src" -I"..\src\external"
set compiler_flags=%includeFlags% -std=c++20 -frtti -fasynchronous-unwind-tables -fexceptions -g -Wall -D_DEBUG
set linkerFlags=-L"..\src\external" -lglfw3 -luser32 -lgdi32 -lshell32

pushd %build_directory%
g++ %compiler_flags% "..\src\main.cpp" "..\src\external\glad.c" %linkerFlags% -o "%executable_name%"
popd
