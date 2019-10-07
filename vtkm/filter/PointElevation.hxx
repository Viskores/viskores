//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_PointElevation_hxx
#define vtk_m_filter_PointElevation_hxx

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT PointElevation::PointElevation()
  : Worklet()
{
  this->SetOutputFieldName("elevation");
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointElevation::SetLowPoint(vtkm::Float64 x, vtkm::Float64 y, vtkm::Float64 z)
{
  this->Worklet.SetLowPoint(vtkm::make_Vec(x, y, z));
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointElevation::SetHighPoint(vtkm::Float64 x,
                                                   vtkm::Float64 y,
                                                   vtkm::Float64 z)
{
  this->Worklet.SetHighPoint(vtkm::make_Vec(x, y, z));
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointElevation::SetRange(vtkm::Float64 low, vtkm::Float64 high)
{
  this->Worklet.SetRange(low, high);
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet PointElevation::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  vtkm::cont::ArrayHandle<vtkm::Float64> outArray;

  //todo, we need to use the policy to determine the valid conversions
  //that the dispatcher should do
  this->Invoke(this->Worklet, field, outArray);

  return CreateResult(inDataSet, outArray, this->GetOutputFieldName(), fieldMetadata);
}
}
} // namespace vtkm::filter
#endif
