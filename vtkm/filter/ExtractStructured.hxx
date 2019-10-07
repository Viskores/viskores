//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ExtractStructured_hxx
#define vtk_m_filter_ExtractStructured_hxx

namespace vtkm
{
namespace filter
{
//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ExtractStructured::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& coordinates = input.GetCoordinateSystem();

  auto cellset = this->Worklet.Run(vtkm::filter::ApplyPolicyCellSetStructured(cells, policy),
                                   this->VOI,
                                   this->SampleRate,
                                   this->IncludeBoundary,
                                   this->IncludeOffset);

  auto coords = this->Worklet.MapCoordinates(coordinates);
  vtkm::cont::CoordinateSystem outputCoordinates(coordinates.GetName(), coords);

  vtkm::cont::DataSet output;
  output.SetCellSet(vtkm::cont::DynamicCellSet(cellset));
  output.AddCoordinateSystem(outputCoordinates);
  return output;
}
}
}

#endif
