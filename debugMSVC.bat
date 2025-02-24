@echo off
:: Set relative paths

set build_directory=Build
set executable_name=mpmRenderer.exe


if not exist %build_directory% (
    mkdir %build_directory%
)

set includeFlags= -I..\src\external -I..\src\external\glad -I..\src\external\glad\KHR
set compiler_flags=   /std:c++20  /GR /FC /EHsc /Zi /W3 /D_DEBUG /MDd  %includeFlags% /Fe%cd%\%build_directory%\%executable_name%


set linkerFlags=/link /LIBPATH:"%cd%\src\external" glfw3.lib user32.lib gdi32.lib shell32.lib


pushd %build_directory%
cl.exe %compiler_flags% ..\src\main.cpp ..\src\external\glad.c %linkerFlags%
popd
