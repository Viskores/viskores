## Python Bindings Commit Message Notes

This note is meant to help draft the commit message for the current update to
the existing `python-bindings` branch. Unlike the PR notice, it should focus on
what changed in the code and documentation, not on the broader evaluation
history.

### Suggested commit message shape

Subject:

`Refactor Python bindings to nanobind and expand wrapper coverage`

Body:

- switch the Python bindings implementation to a pure `nanobind` runtime path
- remove the old mixed CPython binding machinery from the active module path
- move array conversion to `nanobind` ndarrays instead of the direct NumPy C API
- split the binding implementation into module-oriented source files under
  `python_bindings/src/`
- align the public Python API more closely with the Viskores C++ namespaces,
  classes, and method names
- build the extension and wheel in `nanobind` stable-ABI (`abi3`) mode
- register the Python wrapper tests with CTest and expand coverage to the
  currently implemented filters, IO, rendering, scalar-topology, and example
  slices
- clean out example-specific binding helpers so convenience workflows live in
  the example scripts rather than the public API
- update the Python binding documentation, status notes, and PR support notes

### Things to emphasize in the commit, if desired

- This update is much broader than a small incremental tweak to the original
  `Add initial Python bindings` commit.
- It includes the transition to `nanobind`, the API cleanup toward the C++
  surface, the `abi3` work, and the expanded wrapper coverage.
- The commit message should describe those code changes directly, whereas the
  PR notice should explain the Binder/`cppyy` exploration and the design
  rationale.
