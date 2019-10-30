//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ClipWithImplicitFunction_hxx
#define vtk_m_filter_ClipWithImplicitFunction_hxx

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DynamicCellSet.h>

namespace vtkm
{
namespace filter
{
//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline vtkm::cont::DataSet ClipWithImplicitFunction::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  const vtkm::cont::CoordinateSystem& inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::CellSetExplicit<> outputCellSet = this->Worklet.Run(
    vtkm::filter::ApplyPolicyCellSet(cells, policy), this->Function, inputCoords, this->Invert);

  // compute output coordinates
  auto outputCoordsArray = this->Worklet.ProcessPointField(inputCoords.GetData());
  vtkm::cont::CoordinateSystem outputCoords(inputCoords.GetName(), outputCoordsArray);

  //create the output data
  vtkm::cont::DataSet output;
  output.SetCellSet(outputCellSet);
  output.AddCoordinateSystem(outputCoords);

  return output;
}
}
} // end namespace vtkm::filter

#endif
