cmake_minimum_required(VERSION 3.15...3.27)
project(glrendererX11) 
set(CMAKE_CXX_STANDARD 20)

if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

find_package(Python 3.8 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/external/nanobind)
SET(DISPLAY_TYPE "X11" CACHE STRING :"X11")
add_definitions(-DPYTHON_BINDING=1)

if(DISPLAY_TYPE STREQUAL "X11")
nanobind_add_module(glrendererX11 src/glrendererX11.cpp src/external/glad_glx.c src/external/glad.c)
target_include_directories(glrendererX11 PRIVATE src/external)
find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)
target_link_libraries(glrendererX11 PRIVATE X11 OpenGL)

elseif(DISPLAY_TYPE STREQUAL "EGL")



nanobind_add_module(glrendererEGL src/glrendererEGL.cpp  src/external/glad.c vertex_shader_data.o fragment_shader_data.o)

target_include_directories(glrendererEGL PRIVATE src/external)
find_package(OpenGL REQUIRED EGL OpenGL)
target_link_libraries(glrendererEGL PRIVATE OpenGL::OpenGL OpenGL::EGL)

else()

message(FATAL_ERROR "Expected \"X11\" or \"EGL\"")
endif()