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
#include <vtkm/filter/mesh_info/CellMeasures.h>
#include <vtkm/filter/mesh_info/worklet/CellMeasure.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{
//-----------------------------------------------------------------------------
VTKM_CONT CellMeasures::CellMeasures(IntegrationType m)
  : measure(m)
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetCellMeasureName("measure");
}

//-----------------------------------------------------------------------------

VTKM_CONT vtkm::cont::DataSet CellMeasures::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsFieldPoint())
  {
    throw vtkm::cont::ErrorFilterExecution("CellMeasures expects point field input.");
  }

  const auto& cellset = input.GetCellSet();
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outArray;

  auto resolveType = [&](const auto& concrete) {
    this->Invoke(vtkm::worklet::CellMeasure{ this->measure }, cellset, concrete, outArray);
  };
  this->CastAndCallVecField<3>(field, resolveType);

  std::string outputName = this->GetCellMeasureName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = "measure";
  }
  return this->CreateResultFieldCell(input, outputName, outArray);
}
} // namespace mesh_info
} // namespace filter
} // namespace vtkm
