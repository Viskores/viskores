//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "shared_data_helpers.h"

namespace viskores::python::bindings
{

void RegisterNanobindSharedDataClasses(nb::module_& m,
                                       const std::function<void(const char*)>& erase_existing_name)
{
  RegisterNanobindArrayClasses(m, erase_existing_name);
  RegisterNanobindCellSetClass(m, erase_existing_name);
  RegisterNanobindGeneratedEarlyClasses(m, erase_existing_name);
  RegisterNanobindDataSetClasses(m, erase_existing_name);
  RegisterNanobindDataSetBuilderClasses(m, erase_existing_name);
  RegisterNanobindFieldFactoryFunctions(m, erase_existing_name);
}

} // namespace viskores::python::bindings
