//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtkm_m_filter_CellSetConnectivity_hxx
#define vtkm_m_filter_CellSetConnectivity_hxx

#include <vtkm/filter/CreateResult.h>
#include <vtkm/worklet/connectivities/CellSetConnectivity.h>

namespace vtkm
{
namespace filter
{

VTKM_CONT CellSetConnectivity::CellSetConnectivity()
  : OutputFieldName("component")
{
}

template <typename Policy>
inline VTKM_CONT vtkm::cont::DataSet CellSetConnectivity::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<Policy> policy)
{
  vtkm::cont::ArrayHandle<vtkm::Id> component;

  vtkm::worklet::connectivity::CellSetConnectivity().Run(
    vtkm::filter::ApplyPolicyCellSet(input.GetCellSet(), policy), component);

  return CreateResultFieldCell(input, component, this->GetOutputFieldName());
}
}
}

#endif //vtkm_m_filter_CellSetConnectivity_hxx
