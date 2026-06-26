//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "viskores_python_bindings.h"

NB_MODULE(_viskores, m)
{
  m.doc() = "Minimal manual Viskores Python bindings.";

  nb::module_ cont = m.def_submodule("cont");
  viskores::python::bindings::BindCont(cont);
}
