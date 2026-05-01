//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "shared_data_helpers.h"

#include <nanobind/stl/string.h>

namespace viskores::python::bindings
{

namespace
{

std::string DataSetCellSetTypeName(const viskores::cont::DataSet& dataSet)
{
  const auto& cellSet = dataSet.GetCellSet();
  if (cellSet.IsType<viskores::cont::CellSetExplicit<>>())
  {
    return "explicit";
  }
  if (cellSet.CanConvert<viskores::cont::CellSetStructured<1>>())
  {
    return "structured1d";
  }
  if (cellSet.CanConvert<viskores::cont::CellSetStructured<2>>())
  {
    return "structured2d";
  }
  if (cellSet.CanConvert<viskores::cont::CellSetStructured<3>>())
  {
    return "structured3d";
  }
  return "other";
}

nb::list DataSetFieldNames(const viskores::cont::DataSet& dataSet)
{
  nb::list names;
  const viskores::IdComponent numberOfFields = dataSet.GetNumberOfFields();
  for (viskores::IdComponent index = 0; index < numberOfFields; ++index)
  {
    names.append(nb::str(dataSet.GetField(index).GetName().c_str()));
  }
  return names;
}

} // namespace

void RegisterNanobindDataSetClasses(nb::module_& m,
                                    const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("_dataset_cell_set_type_name");
  m.attr("_dataset_cell_set_type_name") =
    nb::cpp_function([](const viskores::cont::DataSet& dataSet)
                     { return DataSetCellSetTypeName(dataSet); });

  erase_existing_name("_dataset_field_names");
  m.attr("_dataset_field_names") = nb::cpp_function([](const viskores::cont::DataSet& dataSet)
                                                    { return DataSetFieldNames(dataSet); });
}

} // namespace viskores::python::bindings
