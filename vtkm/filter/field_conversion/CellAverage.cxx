//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/UnknownCellSet.h>
#include <vtkm/filter/field_conversion/CellAverage.h>
#include <vtkm/filter/field_conversion/worklet/CellAverage.h>

namespace vtkm
{
namespace filter
{
namespace field_conversion
{
//-----------------------------------------------------------------------------
vtkm::cont::DataSet CellAverage::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& field = GetFieldFromDataSet(input);
  if (!field.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  vtkm::cont::UnknownCellSet inputCellSet = input.GetCellSet();
  vtkm::cont::UnknownArrayHandle outArray;

  auto resolveType = [&](const auto& concrete) {
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    vtkm::cont::ArrayHandle<T> result;
    this->Invoke(vtkm::worklet::CellAverage{}, inputCellSet, concrete, result);
    outArray = result;
  };
  field.GetData()
    .CastAndCallForTypesWithFloatFallback<vtkm::TypeListField, VTKM_DEFAULT_STORAGE_LIST>(
      resolveType);

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = field.GetName();
  }
  return this->CreateResultFieldCell(input, outputName, outArray);
}
} // namespace field_conversion
} // namespace filter
} // namespace vtkm
