## viskores_interop_python is now header-only

The `viskores::interop_python` library, which provides `ArrayHandleToNumPy`
and `NumPyToArrayHandle` conversion functions between
`viskores::cont::UnknownArrayHandle` and NumPy, is now a header-only
interface library instead of a compiled static library.

Every consumer of these functions is a nanobind extension module, and
nanobind types are only interoperable across extension modules that share
an identical ABI tag (nanobind version, stable-ABI setting, compiler). A
compiled library would tie every consumer to the exact nanobind build used
for Viskores itself. Building header-only lets each consumer compile these
entry points against its own nanobind and Python headers, and also makes
the library usable from outside the Viskores build via
`find_package(Viskores)` and `target_link_libraries(... viskores::interop_python)`.
