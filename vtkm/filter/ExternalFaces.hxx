//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ExternalFaces_hxx
#define vtk_m_filter_ExternalFaces_hxx

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ExternalFaces::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //1. extract the cell set
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  //2. using the policy convert the dynamic cell set, and run the
  // external faces worklet
  vtkm::cont::CellSetExplicit<> outCellSet;

  if (cells.IsSameType(vtkm::cont::CellSetStructured<3>()))
  {
    this->Worklet.Run(cells.Cast<vtkm::cont::CellSetStructured<3>>(),
                      input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
                      outCellSet);
  }
  else
  {
    this->Worklet.Run(vtkm::filter::ApplyPolicyCellSetUnstructured(cells, policy), outCellSet);
  }

  return this->GenerateOutput(input, outCellSet);
}
}
}

#endif
