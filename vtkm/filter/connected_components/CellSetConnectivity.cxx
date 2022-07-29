//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/connected_components/CellSetConnectivity.h>
#include <vtkm/filter/connected_components/worklet/CellSetConnectivity.h>

namespace vtkm
{
namespace filter
{
namespace connected_components
{
VTKM_CONT vtkm::cont::DataSet CellSetConnectivity::DoExecute(const vtkm::cont::DataSet& input)
{
  vtkm::cont::ArrayHandle<vtkm::Id> component;

  vtkm::worklet::connectivity::CellSetConnectivity().Run(input.GetCellSet(), component);

  return this->CreateResultFieldCell(input, this->GetOutputFieldName(), component);
}
} // namespace connected_components
} // namespace filter
} // namespace vtkm
