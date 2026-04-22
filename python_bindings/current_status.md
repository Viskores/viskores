## Python Bindings Current Status

This file is a handoff note for continuing the current Python bindings work.
It records the current implementation state, the major design decisions, and
the deferred items that still need follow-up.

### Current implementation state

- The `_viskores` extension is now implemented as a `nanobind` module.
- The active runtime path is pure `nanobind`; there is no longer a separate
  CPython module-construction path.
- Array conversion in the active binding path now uses `nanobind` ndarrays
  rather than the direct NumPy C API.
- The module no longer imports NumPy through `_import_array()` at startup.
- The extension is now built in `nanobind` stable-ABI (`abi3`) mode.
- The `abi3` build/import path has been validated on Python `3.12`, `3.13`,
  and `3.14`.
- The implementation is physically split into multiple translation units under
  `python_bindings/src/`, rather than one monolithic binding file.
- The Python package layout mirrors Viskores namespaces, for example:
  - `viskores.cont`
  - `viskores.source`
  - `viskores.filter.contour`
  - `viskores.filter.scalar_topology`
  - `viskores.rendering`
  - `viskores.interop`
- Example-specific contour-tree convenience workflows now live only in the
  example scripts, not in the public binding API.

### Public API decisions

- Prefer real Viskores classes and C++-style names in the public API.
- Do not add Pythonic alias names for bound Viskores classes unless there is an
  explicit reason and agreement to do so.
- Python-only convenience surfaces are allowed, but they should be clearly
  separated from the main API.
- The direct class API is preferred for examples whenever the necessary
  bindings exist.
- `viskores.cont.Initialize(...)` is exposed and should be used in Python
  examples whenever the corresponding C++ example uses
  `viskores::cont::Initialize(...)`.

### Testing state

- The Python binding tests now live under `python_bindings/testing/`.
- When both `Viskores_ENABLE_PYTHON_BINDINGS=ON` and
  `Viskores_ENABLE_TESTING=ON` are enabled, the Python wrapper tests are
  registered with CTest automatically from `python_bindings/CMakeLists.txt`.
- Current snapshot after the latest additions:
  - `105/105` Python binding tests passed

### Porting and coverage status

- The major ordinary filter families covered so far include:
  - contour
  - vector analysis
  - field conversion
  - field transform
  - scalar topology
  - entity extraction
  - clean grid
  - mesh info
  - connected components
  - multi-block basics
  - resampling
  - density estimate
  - image processing
  - geometry refinement
- The current remaining-test gap analysis is in:
  - `python_bindings/remaining_test_porting_plan.md`

### Deferred decisions and reminders

- `BufferState` currently uses a nanobind-bound holder implementation because
  the underlying Viskores type is reference-based and non-copyable.
- The current build uses `nanobind` stable-ABI (`abi3`) mode. Keep that as the
  default unless a future binding feature runs into meaningful limited-API
  constraints; if that happens, explicitly re-evaluate whether staying on
  `abi3` is still the right tradeoff.
- `UnitTestMultiMapper.cxx` is still deferred pending a decision on whether to
  expose low-level mapper `RenderCells(...)` in Python.
- MPI test coverage is still deferred.
- Worklet-specific tests are generally skipped unless they become relevant to a
  real public Python API surface.

### Files to check first when continuing

- `python_bindings/README.md`
- `python_bindings/remaining_test_porting_plan.md`
- `REMINDERS.md`
- `python_bindings/commit_update_notes.md`
- `python_bindings/pr_update_notes.md`
