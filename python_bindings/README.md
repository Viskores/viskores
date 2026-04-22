# Python Bindings

This directory contains an initial Python binding layer for Viskores.

Current scope:
- Create uniform datasets from Python.
- Create Tangle source datasets from Python.
- Add point and cell fields from NumPy `float32` or `float64` arrays.
- Split uniform datasets into block-partitioned datasets.
- Run a small set of filters end to end:
  - `cell_average`
  - `point_average`
  - `vector_magnitude`
  - `gradient`
  - `contour`
- Run contour-tree workflows needed by the example ports:
  - `contour_tree_augmented`
  - `contour_tree_distributed`
  - `distributed_branch_decomposition`
  - `select_top_volume_branches`
  - `extract_top_volume_contours`
- Use Python-side wrappers for the `demo` example source, contour, and rendering flow.
- Read result fields back as NumPy arrays.

Build example:

```bash
cmake -S . -B build-python \
  -DViskores_ENABLE_PYTHON_BINDINGS=ON \
  -DViskores_ENABLE_TESTING=OFF \
  -DViskores_ENABLE_EXAMPLES=OFF \
  -DViskores_ENABLE_TUTORIALS=OFF
cmake --build build-python --target _viskores
PYTHONPATH=build-python/python_bindings python3 python_bindings/examples/basic_pipeline.py
PYTHONPATH=build-python/python_bindings python3 python_bindings/examples/demo/Demo.py
```

The Python package is staged into `build-python/python_bindings/viskores`.

## Running `Demo.py` from a Viskores build

If you want to run the Python demo directly from a Viskores build tree with
Python bindings enabled:

```bash
cmake -S . -B build-python \
  -DViskores_ENABLE_PYTHON_BINDINGS=ON \
  -DViskores_ENABLE_TESTING=OFF \
  -DViskores_ENABLE_EXAMPLES=OFF \
  -DViskores_ENABLE_TUTORIALS=OFF
cmake --build build-python --target _viskores

PYTHONPATH=$PWD/build-python/python_bindings \
python3 python_bindings/examples/demo/Demo.py
```

For a smaller test run:

```bash
PYTHONPATH=$PWD/build-python/python_bindings \
python3 python_bindings/examples/demo/Demo.py --dims 8 8 8 --iso-value 3.0
```

This writes `volume.png`, `isosurface_wireframer.png`, and
`isosurface_raytracer.png` in the current working directory.

## Building a wheel

You can also build a wheel that contains only the Python bindings and expects
Viskores to already be installed on the target system.

Point CMake at an installed Viskores package configuration with either
`Viskores_DIR` or `CMAKE_PREFIX_PATH`, then run:

```bash
cd python_bindings
python3 -m pip wheel . -w dist --no-build-isolation --no-deps
```

For example, when building against an existing Viskores build tree:

```bash
CMAKE_PREFIX_PATH=$PWD/build-python \
python3 -m pip wheel ./python_bindings -w python_bindings/dist --no-build-isolation --no-deps
```

If the referenced Viskores installation was built with OpenMP, you may also
need to help CMake find the OpenMP runtime. On macOS with Homebrew `libomp`:

```bash
OpenMP_ROOT=/opt/homebrew/opt/libomp \
CMAKE_PREFIX_PATH=$PWD/install \
python3 -m pip wheel ./python_bindings -w python_bindings/dist --no-build-isolation --no-deps
```

On Linux this is often unnecessary if OpenMP is installed in the default
compiler search paths. If it is not, point `OpenMP_ROOT` or your compiler
environment at the OpenMP installation used for the Viskores build.

This creates a wheel in `python_bindings/dist/` containing `viskores/__init__.py` and the
compiled `_viskores` extension module. The wheel does not bundle the Viskores
libraries themselves.

## Running `Demo.py` from an installed wheel in a virtual environment

Build the wheel against an installed Viskores tree:

```bash
OpenMP_ROOT=/opt/homebrew/opt/libomp \
CMAKE_PREFIX_PATH=$PWD/install \
python3 -m pip wheel ./python_bindings -w python_bindings/dist --no-build-isolation --no-deps
```

On Linux, if no extra OpenMP hint is needed, the same command is usually just:

```bash
CMAKE_PREFIX_PATH=$PWD/install \
python3 -m pip wheel ./python_bindings -w python_bindings/dist --no-build-isolation --no-deps
```

Create a virtual environment and install the wheel with `uv`:

```bash
uv venv .venv --system-site-packages
uv pip install --python .venv/bin/python --no-deps \
  python_bindings/dist/viskores-*.whl
```

Then run the demo from that environment:

```bash
.venv/bin/python python_bindings/examples/demo/Demo.py
```

For example:

```bash
.venv/bin/python python_bindings/examples/demo/Demo.py --dims 8 8 8 --iso-value 3.0
```

If Viskores remains installed at the same location used when the wheel was
built, macOS and Linux should resolve the Viskores shared libraries through the
embedded rpath and no extra environment variables should be needed.

If the Viskores install tree is moved after building the wheel, you may need to
rebuild the wheel against the new install location or set a runtime library
path variable as a fallback:

- macOS: `DYLD_LIBRARY_PATH`
- Linux: `LD_LIBRARY_PATH`
