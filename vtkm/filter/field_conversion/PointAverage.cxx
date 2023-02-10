//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/CellSetExtrude.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/UncertainCellSet.h>
#include <vtkm/cont/UnknownCellSet.h>
#include <vtkm/filter/field_conversion/PointAverage.h>
#include <vtkm/filter/field_conversion/worklet/PointAverage.h>

namespace vtkm
{
namespace filter
{
namespace field_conversion
{
vtkm::cont::DataSet PointAverage::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& field = GetFieldFromDataSet(input);
  if (!field.IsCellField())
  {
    throw vtkm::cont::ErrorFilterExecution("Cell field expected.");
  }

  vtkm::cont::UnknownCellSet cellSet = input.GetCellSet();
  vtkm::cont::UnknownArrayHandle inArray = field.GetData();
  vtkm::cont::UnknownArrayHandle outArray = inArray.NewInstanceBasic();

  auto resolveType = [&](const auto& concrete) {
    using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
    auto result = outArray.ExtractArrayFromComponents<T>();
    using SupportedCellSets =
      vtkm::ListAppend<vtkm::List<vtkm::cont::CellSetExtrude>, VTKM_DEFAULT_CELL_SET_LIST>;

    this->Invoke(vtkm::worklet::PointAverage{},
                 cellSet.ResetCellSetList<SupportedCellSets>(),
                 concrete,
                 result);
  };
  // TODO: Do we need to deal with XCG storage type (vtkm::cont::ArrayHandleXGCCoordinates)
  // explicitly? Extracting from that is slow.
  inArray.CastAndCallWithExtractedArray(resolveType);

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = field.GetName();
  }
  return this->CreateResultFieldPoint(input, outputName, outArray);
}
}
}
}
