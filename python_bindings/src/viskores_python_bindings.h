//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_python_bindings_h
#define viskores_python_bindings_h

#include <nanobind/nanobind.h>

namespace nb = nanobind;

namespace viskores::python::bindings
{

void BindCont(nb::module_& m);

} // namespace viskores::python::bindings

#endif
