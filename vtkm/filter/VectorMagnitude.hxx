//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_VectorMagnitude_hxx
#define vtk_m_filter_VectorMagnitude_hxx

#include <vtkm/Math.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet VectorMagnitude::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  using ReturnType = typename ::vtkm::detail::FloatingPointReturnType<T>::Type;
  vtkm::cont::ArrayHandle<ReturnType> outArray;

  this->Invoke(this->Worklet, field, outArray);

  return CreateResult(inDataSet, outArray, this->GetOutputFieldName(), fieldMetadata);
}
}
} // namespace vtkm::filter
#endif
