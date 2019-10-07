//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ImageConnectivity_hxx
#define vtk_m_filter_ImageConnectivity_hxx

namespace vtkm
{
namespace filter
{

VTKM_CONT ImageConnectivity::ImageConnectivity()
{
  this->SetOutputFieldName("component");
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ImageConnectivity::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (!fieldMetadata.IsPointField())
  {
    throw vtkm::cont::ErrorBadValue("Active field for ImageConnectivity must be a point field.");
  }

  vtkm::cont::ArrayHandle<vtkm::Id> component;

  vtkm::worklet::connectivity::ImageConnectivity().Run(
    vtkm::filter::ApplyPolicyCellSet(input.GetCellSet(), policy), field, component);

  auto result = CreateResult(input, component, this->GetOutputFieldName(), fieldMetadata);
  return result;
}
}
}

#endif
