# OpenGL Renderer

This is a basic renderer designed to visualise a continuum as a set of points displayed as spheres. For performance reasons, it does not render full 3D spheres. Instead, it uses imposter spheres (textured quads shaded to appear spherical). Transparency is supported.

![til](./example/output.gif)

## Build Instructions

Clone the submodules and bind the EGL interface:

```
git submodule update --init --recursive
bash bind.sh EGL <path_to_python>
```

Replace `<path_to_python>` with the path to your Python interpreter.

## Usage

This renderer uses EGL to create an offscreen OpenGL context. EGL enables headless rendering, which is useful in environments without a display server. The context is created on a selected GPU to allow access to the fixed-function rendering pipeline.

The project is intended to be used with [Taichi](https://www.taichi-lang.org/). It uses [nanobind](https://nanobind.readthedocs.io/en/latest/) to build a Python extension module from C++ code.

Run the following command to test:

```
python test.py
```