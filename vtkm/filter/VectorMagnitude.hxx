//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/internal/CreateResult.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/Math.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT VectorMagnitude::VectorMagnitude()
  : vtkm::filter::FilterField<VectorMagnitude>()
  , Worklet()
{
  this->SetOutputFieldName("magnitude");
}

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

  vtkm::worklet::DispatcherMapField<vtkm::worklet::Magnitude> dispatcher(this->Worklet);

  dispatcher.Invoke(field, outArray);

  return internal::CreateResult(inDataSet,
                                outArray,
                                this->GetOutputFieldName(),
                                fieldMetadata.GetAssociation(),
                                fieldMetadata.GetCellSetName());
}
}
} // namespace vtkm::filter
