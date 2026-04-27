# Python Bindings

This directory contains the Viskores Python package and its CMake build logic.
The current binding implementation uses `nanobind`, including the active
ndarray conversion layer. By default, the extension is built as a normal
Python-minor-version-specific extension so the repository can keep its CMake
3.15 baseline.

Current Python surface includes:
- core data objects such as `viskores.cont.DataSet`
- sources such as `viskores.source.Tangle`
- filters in namespace-style submodules such as `viskores.filter.contour`
- rendering classes in `viskores.rendering`
- OpenGL interop entry points in `viskores.interop`

## Recommended Workflow

Build the Python bindings against the same Python interpreter that you will use
to run them. A virtual environment is the simplest way to keep that consistent.

### 1. Create a Virtual Environment

With a Homebrew Python on macOS:

```bash
python3 -m venv viskores-build/.venv
viskores-build/.venv/bin/python -m ensurepip --upgrade
viskores-build/.venv/bin/python -m pip install numpy pyglet PyOpenGL PyOpenGL_accelerate
```

Run these commands from the repository root so the relative
`viskores-build/.venv` path resolves inside the build tree.

If you want to run the current `GameOfLife.py` OpenGL window on macOS, the
virtual-environment commands above are sufficient on the Python side. `pyglet`
provides the native window and event loop, while `PyOpenGL` handles the raw GL
calls used by the demo. On Linux, you may still need the usual system OpenGL
development/runtime packages from your distribution.

### 2. Configure Viskores with Python Bindings

Configure Viskores to use the Python from the virtual environment:

```bash
cmake -S . -B viskores-build \
  -DPython_EXECUTABLE=$PWD/viskores-build/.venv/bin/python \
  -DViskores_ENABLE_PYTHON_BINDINGS=ON \
  -DViskores_ENABLE_RENDERING=ON \
  -DViskores_ENABLE_TESTING=ON
```

The optional stable-ABI (`abi3`) build path is available by adding:

```bash
-DVISKORES_PYTHON_STABLE_ABI=ON
```

That mode currently requires Python `3.12` or newer and CMake FindPython
support for `Python::SABIModule`, which is available in CMake `3.26` or newer.
Leave `VISKORES_PYTHON_STABLE_ABI` off when building with the repository's
CMake `3.15` baseline.

If your macOS build needs Homebrew OpenMP, add:

```bash
-DOpenMP_ROOT=/opt/homebrew/opt/libomp
```

Then build the bindings:

```bash
cmake --build viskores-build --target viskores_python_bindings -j 4
```

The built package is staged in:

```bash
viskores-build/python_bindings
```

### 3. Sanity Check the Package

Use the same virtual environment to import the package directly from the build
tree:

```bash
PYTHONPATH=$PWD/viskores-build/python_bindings \
$PWD/viskores-build/.venv/bin/python -c "import viskores, viskores.rendering, viskores.interop; print('ok')"
```

If you enabled testing, you can also run the Python binding tests:

```bash
ctest --test-dir viskores-build -R '^(PythonBindingsSmokeTest|(UnitTestPythonWrapper|PythonWrapper).*)$' --output-on-failure
```

The Python binding tests live under `python_bindings/testing/` and are
registered with CTest automatically when both
`Viskores_ENABLE_PYTHON_BINDINGS=ON` and `Viskores_ENABLE_TESTING=ON` are
enabled.

## Array Interoperability

The Python package accepts NumPy arrays for fields and coordinates. The binding
converts C-contiguous CPU arrays with these NumPy dtypes:

- `int8`, `uint8`, `int16`, `uint16`, `int32`, `uint32`, `int64`, `uint64`
- `float32`, `float64`

One-dimensional arrays become scalar Viskores arrays. Two-dimensional arrays
with shape `N x 1`, `N x 2`, `N x 3`, or `N x 4` become scalar or fixed-size
vector arrays. For example:

```python
import numpy as np
from viskores.cont import Association, Field

values = np.arange(12, dtype=np.float32).reshape(4, 3)
field = Field("velocity", Association.POINTS, values)
```

Use `viskores.cont.array_from_numpy` when you want an explicit
`UnknownArrayHandle`:

```python
from viskores.cont import array_from_numpy, asnumpy

array = array_from_numpy(values)
round_trip = asnumpy(array)
```

By default, NumPy input is copied into Viskores-owned memory. Passing
`copy=False` creates a Viskores array that aliases the NumPy buffer, so the
NumPy array must outlive the Viskores array:

```python
array = array_from_numpy(values, copy=False)
view = array.AsNumPy(copy=False)
```

The `copy=False` NumPy export path is available for basic scalar and fixed-size
vector arrays and returns a read-only NumPy view. Other storage layouts export
through a copy. Low-level `ArrayHandleSOA*` and `ArrayHandleRecombineVec*`
classes are also exposed for workflows that need to preserve or test specific
Viskores storage layouts, while `array_from_numpy` and `asnumpy` are the
recommended high-level entry points.

### 4. Build a Wheel

From the repository root:

```bash
cd python_bindings
Viskores_DIR=$PWD/../viskores-build/lib/cmake/viskores-1.1 \
../viskores-build/.venv/bin/python setup.py bdist_wheel
```

To build an `abi3` wheel, use a CMake `3.26` or newer environment and enable
the matching setup/CMake option:

```bash
VISKORES_PYTHON_STABLE_ABI=ON \
Viskores_DIR=$PWD/../viskores-build/lib/cmake/viskores-1.1 \
../viskores-build/.venv/bin/python setup.py bdist_wheel
```

This writes the wheel to:

```bash
python_bindings/dist/
```

For example, a Python `3.14` build produces a wheel tagged for that Python
minor version:

```bash
python_bindings/dist/viskores-1.1.9999-cp314-cp314-macosx_26_0_arm64.whl
```

With `VISKORES_PYTHON_STABLE_ABI=ON`, the wheel is tagged as an `abi3` wheel
rather than a Python-minor-version-specific wheel.

## Running the Demos

For applications that want Viskores runtime option parsing and backend
selection, call `viskores.cont.Initialize(...)` explicitly near startup, just as
the C++ examples do. In Python, pass a mutable `sys.argv`-style list including
the program name:

```python
import sys
import viskores.cont

viskores.cont.Initialize(sys.argv)
```

This strips Viskores-specific arguments from the list in place and returns a
`viskores.cont.InitializeResult`.

### Demo.py

`Demo.py` is the Python counterpart to `examples/demo/Demo.cxx`.

Run it from a directory where you want the output images written:

```bash
cd /tmp
PYTHONPATH=/path/to/viskores-build/python_bindings \
/path/to/viskores-build/.venv/bin/python \
  /path/to/viskores/python_bindings/examples/demo/Demo.py
```

For a quicker test:

```bash
cd /tmp
PYTHONPATH=/path/to/viskores-build/python_bindings \
/path/to/viskores-build/.venv/bin/python \
  /path/to/viskores/python_bindings/examples/demo/Demo.py --dims 8 8 8 --iso-value 3.0
```

This writes:
- `volume.png`
- `isosurface_wireframer.png`
- `isosurface_raytracer.png`

### GameOfLife.py

`GameOfLife.py` is the Python port of `examples/game_of_life/GameOfLife.cxx`.

Run it with a native window:

```bash
PYTHONPATH=$PWD/viskores-build/python_bindings \
$PWD/viskores-build/.venv/bin/python python_bindings/examples/game_of_life/GameOfLife.py
```

As in the C++ demo, you can optionally pass the initial live-cell rate:

```bash
PYTHONPATH=$PWD/viskores-build/python_bindings \
$PWD/viskores-build/.venv/bin/python python_bindings/examples/game_of_life/GameOfLife.py 0.3
```

The current Python demo now uses a native `pyglet` window plus Viskores OpenGL
upload through `viskores.interop`.

## Python OpenGL Interop Surface

The binding layer now exposes:

```python
from viskores.interop import (
    BufferState,
    TransferToOpenGL,
)
```

This is the Viskores side of the C++ OpenGL upload path. `GameOfLife.py` now
uses that surface together with a `pyglet` window and PyOpenGL. Use the OpenGL
constants from `OpenGL.GL`, not from `viskores.interop`.

## Notes

- Build the bindings with the same Python you intend to use at runtime.
- `numpy` is required both to build and to use the bindings.
- `pyglet`, `PyOpenGL`, and `PyOpenGL_accelerate` are required for the current
  windowed `GameOfLife.py` demo.
