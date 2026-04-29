//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_python_bindings_shared_data_helpers_h
#define viskores_python_bindings_shared_data_helpers_h

#include "pyviskores_common.h"
#include "pyviskores_bindings.h"

#include <functional>
#include <string>
#include <vector>

namespace viskores::python::bindings
{

inline nb::object NumPyObjectFromUnknownArray(const viskores::cont::UnknownArrayHandle& array)
{
  return UnknownArrayToNumPyArray(array);
}

inline nb::object NumPyObjectFromField(const viskores::cont::Field& field)
{
  return FieldToNumPyArray(field);
}

bool TryPythonObjectToRegisteredArray(nb::handle object,
                                      viskores::cont::UnknownArrayHandle& array);
viskores::cont::UnknownArrayHandle PythonObjectToUnknownArray(nb::handle object);

std::vector<viskores::Vec3f> ParseVec3Sequence(nb::handle object);
std::vector<viskores::UInt8> ParseUInt8Sequence(nb::handle object);
std::vector<viskores::IdComponent> ParseIdComponentSequence(nb::handle object);
std::vector<viskores::Id> ParseIdSequence(nb::handle object);
viskores::cont::ArrayHandle<viskores::FloatDefault> ParseRectilinearAxis(nb::handle object,
                                                                         const char* name);

void RegisterNanobindArrayClasses(nb::module_& m,
                                  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindCellSetClass(nb::module_& m,
                                  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindFieldClasses(nb::module_& m,
                                  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindDataSetClasses(nb::module_& m,
                                    const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindDataSetBuilderClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindPartitionedDataSetClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindFieldFactoryFunctions(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);

} // namespace viskores::python::bindings

#endif
