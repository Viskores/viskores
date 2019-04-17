//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm
{
namespace filter
{
//-----------------------------------------------------------------------------

ClipWithImplicitFunction::ClipWithImplicitFunction()
  : Invert(false)
{
}

template <typename DerivedPolicy>
inline vtkm::cont::DataSet ClipWithImplicitFunction::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());

  const vtkm::cont::CoordinateSystem& inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::CellSetExplicit<> outputCellSet = this->Worklet.Run(
    vtkm::filter::ApplyPolicy(cells, policy), this->Function, inputCoords, this->Invert);

  // compute output coordinates
  auto outputCoordsArray = this->Worklet.ProcessPointField(inputCoords.GetData());
  vtkm::cont::CoordinateSystem outputCoords(inputCoords.GetName(), outputCoordsArray);

  //create the output data
  vtkm::cont::DataSet output;
  output.AddCellSet(outputCellSet);
  output.AddCoordinateSystem(outputCoords);

  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline bool ClipWithImplicitFunction::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  vtkm::cont::ArrayHandle<T> output;

  if (fieldMeta.IsPointField())
  {
    output = this->Worklet.ProcessPointField(input);
  }
  else if (fieldMeta.IsCellField())
  {
    output = this->Worklet.ProcessCellField(input);
  }
  else
  {
    return false;
  }

  //use the same meta data as the input so we get the same field name, etc.
  result.AddField(fieldMeta.AsField(output));

  return true;
}
}
} // end namespace vtkm::filter
