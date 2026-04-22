## Python Bindings Reminders

- `BufferState` currently uses a nanobind-bound holder implementation to preserve the public
  `viskores.interop.BufferState` API while the underlying Viskores type is reference-based and
  non-copyable.
- The bindings currently build in `nanobind` stable-ABI (`abi3`) mode. Keep that unless a future
  binding feature runs into real limited-API constraints, in which case explicitly re-evaluate
  whether the project should stay on `abi3` or switch back to the full ABI.
- Revisit that `BufferState` implementation once the broader nanobind port is otherwise complete.
- When updating the pull request description, explicitly mention that `BufferState` still needs a
  deeper design review and that the current holder approach is an interim implementation choice.
- `UnitTestMultiMapper.cxx` / low-level mapper `RenderCells(...)` exposure is still deferred.
- Revisit whether to expose `RenderCells(...)` after the safer example and test ports are done.
- When updating the pull request description, explicitly mention that the multi-mapper /
  `RenderCells(...)` design decision is still outstanding.
