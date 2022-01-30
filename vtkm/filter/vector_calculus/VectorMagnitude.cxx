//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/vector_calculus/VectorMagnitude.h>
#include <vtkm/filter/vector_calculus/worklet/Magnitude.h>

namespace vtkm
{
namespace filter
{
namespace vector_calculus
{
VectorMagnitude::VectorMagnitude()

{
  this->SetOutputFieldName("magnitude");
}

VTKM_CONT vtkm::cont::DataSet VectorMagnitude::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  auto field = this->GetFieldFromDataSet(inDataSet);
  vtkm::cont::UnknownArrayHandle outArray;

  auto resolveType = [&, this](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    using ReturnType = typename ::vtkm::detail::FloatingPointReturnType<T>::Type;
    vtkm::cont::ArrayHandle<ReturnType> result;

    this->Invoke(vtkm::worklet::Magnitude{}, concrete, result);
    outArray = result;
  };
  field.GetData().CastAndCallForTypesWithFloatFallback<SupportedTypes, VTKM_DEFAULT_STORAGE_LIST>(
    resolveType);

  return this->CreateResultField(
    inDataSet, this->GetOutputFieldName(), field.GetAssociation(), outArray);
}
} // namespace vector_calculus
} // namespace filter
} // namespace vtkm
