##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

set(headers
  ArrayHandleToNumPy.h
  NumPyToArrayHandle.h
)

viskores_declare_headers(${headers})

# Header-only: every consumer of this library is itself a nanobind extension
# module, since the entry points take and return nb::object/nb::handle.
# nanobind types are only interoperable across extension modules that share
# an identical ABI tag (nanobind version, stable-ABI setting, compiler), so a
# compiled (static/shared/object) library here would tie every consumer to
# the exact nanobind build used for Viskores itself. Building header-only
# lets each consumer compile these entry points against its own nanobind and
# Python headers, which it already provides via nanobind_add_module().
add_library(viskores_interop_python INTERFACE)
target_link_libraries(viskores_interop_python INTERFACE viskores_cont)
viskores_install_targets(TARGETS viskores_interop_python)
