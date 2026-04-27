//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "src/pyviskores_common.h"
#include "src/pyviskores_bindings.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

using namespace viskores::python::bindings;

NB_MODULE(_viskores, m)
{
  RegisterNanobindModuleConstants(m);
  RegisterNanobindModule(m);
}
