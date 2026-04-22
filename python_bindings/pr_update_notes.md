## Python Bindings PR Notice

### What we tried

#### Binder

- Binder was explored early because one of its main attractions was the
  possibility of tracking changes in the underlying Viskores classes with
  relatively little manual rebinding work.
- The hope was that as the raw C++ class surface evolved, the generated
  bindings could adapt with less maintenance than a fully manual binding layer.
- In practice, though, it looked like Binder would still require a curated
  adapter layer between the raw Viskores API and the desired Python API.
- The purpose of such a layer would have been to smooth over templates,
  awkward C++ types, ownership and lifetime issues, and other interfaces that
  do not map cleanly into Python.
- Once such a layer becomes necessary, a significant part of the
  “automatically follow the raw class surface” advantage is lost, because the
  adapter layer itself must still be maintained and updated as the C++ API
  changes.
- For this branch, that weakened the main reason to prefer Binder, while also
  moving the design away from the goal of exposing the real Viskores classes,
  namespaces, and calling patterns as directly as practical.

#### cppyy

- `cppyy` remained interesting in principle because of its low-maintenance,
  header-driven model.
- In practice, the Apple Silicon/macOS Cling/rootcling toolchain problems were
  severe enough that it never reached a fair Viskores-specific evaluation.
- The packaged `cppyy` stack was broken before Viskores-specific testing.
- A source-built `cppyy-cling` stack got farther, but still failed in
  `rootcling_stage1` dictionary generation on a non-Viskores test case.
- That made `cppyy` a poor choice for the current bindings effort, even though
  it may still be interesting later if the platform and toolchain situation
  improves.

### Why nanobind

- `nanobind` gives direct control over the Python surface while still keeping
  the implementation relatively compact.
- It supports a public API that tracks the Viskores C++ classes and namespaces
  closely.
- It avoids the adapter-heavy Binder direction and the toolchain blockers seen
  in the `cppyy` exploration.
- It was the most practical path to a real, reviewable binding surface for the
  current branch.

### Current design decisions

- The public Python API should prefer real Viskores classes and C++-style
  names.
- Python-only convenience surfaces should not replace the main Viskores API.
- Example-specific workflows should stay in examples, not in the public
  bindings.
- The active binding/runtime path is now pure `nanobind`; the earlier separate
  CPython module-construction path is gone.
- The active array-conversion path now uses `nanobind` ndarrays rather than
  the direct NumPy C API.
- The extension now builds in `nanobind` stable-ABI (`abi3`) mode rather than
  as a Python-minor-version-specific extension.

### What changed relative to the earlier branch state

- The initial flat and partly helper-oriented Python surface has been refactored
  toward the actual Viskores namespace and class structure.
- The implementation is now split into multiple source files under
  `python_bindings/src/` rather than one large binding source.
- The Python wrapper tests now live under `python_bindings/testing/` and are
  registered with CTest when Python bindings and testing are enabled.
- Example-specific scalar-topology helper functions were removed from the
  public bindings. The example scripts keep any needed local convenience logic
  themselves.
- The extension and wheel now build in `abi3` mode, and the wheel/import path
  has been exercised successfully on Python `3.12`, `3.13`, and `3.14`.

### Current validation status

- Current snapshot: `105/105` Python wrapper tests passed.
- Coverage and remaining gaps are tracked in:
  - `python_bindings/remaining_test_porting_plan.md`

### Deferred items to mention explicitly

- `BufferState` still uses an interim holder implementation in the bindings and
  should receive a deeper design review later.
- `UnitTestMultiMapper.cxx` remains deferred pending a decision on exposing
  low-level mapper `RenderCells(...)`.
- The bindings currently build in `abi3` mode. That is the preferred default,
  but if a future feature needs CPython or limited-API-incompatible behavior,
  the project should explicitly re-evaluate whether to stay on `abi3`.
