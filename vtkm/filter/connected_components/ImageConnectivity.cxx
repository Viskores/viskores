//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/connected_components/ImageConnectivity.h>
#include <vtkm/filter/connected_components/worklet/ImageConnectivity.h>

namespace vtkm
{
namespace filter
{
namespace connected_components
{
VTKM_CONT vtkm::cont::DataSet ImageConnectivity::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);

  if (!field.IsPointField())
  {
    throw vtkm::cont::ErrorBadValue("Active field for ImageConnectivity must be a point field.");
  }

  vtkm::cont::ArrayHandle<vtkm::Id> component;

  auto resolveType = [&](const auto& concrete) {
    vtkm::worklet::connectivity::ImageConnectivity().Run(input.GetCellSet(), concrete, component);
  };
  this->CastAndCallScalarField(field, resolveType);

  return this->CreateResultFieldPoint(input, this->GetOutputFieldName(), component);
}
} // namespace connected_components
} // namespace filter
} // namespace vtkm
