//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ClipWithField_hxx
#define vtk_m_filter_ClipWithField_hxx

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

namespace vtkm
{
namespace filter
{
//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ClipWithField::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  if (fieldMeta.IsPointField() == false)
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  const vtkm::cont::CoordinateSystem& inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::CellSetExplicit<> outputCellSet = this->Worklet.Run(
    vtkm::filter::ApplyPolicyCellSet(cells, policy), field, this->ClipValue, this->Invert);

  //create the output data
  vtkm::cont::DataSet output;
  output.SetCellSet(outputCellSet);

  // Compute the new boundary points and add them to the output:
  auto outputCoordsArray = this->Worklet.ProcessPointField(inputCoords.GetData());
  vtkm::cont::CoordinateSystem outputCoords(inputCoords.GetName(), outputCoordsArray);
  output.AddCoordinateSystem(outputCoords);
  return output;
}
}
} // end namespace vtkm::filter

#endif
