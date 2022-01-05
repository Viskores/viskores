//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/vector_calculus/DotProduct.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace // anonymous namespace making worklet::DotProduct internal to this .cxx
{
namespace worklet
{
class DotProduct : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);

  template <typename T, vtkm::IdComponent Size>
  VTKM_EXEC void operator()(const vtkm::Vec<T, Size>& v1,
                            const vtkm::Vec<T, Size>& v2,
                            T& outValue) const
  {
    outValue = static_cast<T>(vtkm::Dot(v1, v2));
  }

  template <typename T>
  VTKM_EXEC void operator()(T s1, T s2, T& outValue) const
  {
    outValue = static_cast<T>(s1 * s2);
  }
};
} // namespace worklet
} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace vector_calculus
{

VTKM_CONT DotProduct::DotProduct()
{
  this->SetOutputFieldName("dotproduct");
}

VTKM_CONT vtkm::cont::DataSet DotProduct::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  const auto& primaryArray = this->GetFieldFromDataSet(inDataSet).GetData();

  vtkm::cont::UnknownArrayHandle outArray;

  // We are using a C++14 auto lambda here. The advantage over a Functor is obvious, we don't
  // need to explicitly pass filter, input/output DataSets etc. thus reduce the impact to
  // the legacy code. The lambda can also access the private part of the filter thus reducing
  // filter's public interface profile. CastAndCall tries to cast primaryArray of unknown value
  // type and storage to a concrete ArrayHandle<T, S> with T from the `TypeList` and S from
  // `StorageList`. It then passes the concrete array to the lambda as the first argument.
  // We can later recover the concrete ValueType, T, from the concrete array.
  auto ResolveType = [&, this](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    const auto& secondaryField = this->GetFieldFromDataSet(1, inDataSet);
    vtkm::cont::UnknownArrayHandle secondary = vtkm::cont::ArrayHandle<T>{};
    secondary.CopyShallowIfPossible(secondaryField.GetData());

    vtkm::cont::ArrayHandle<typename vtkm::VecTraits<T>::ComponentType> result;
    this->Invoke(::worklet::DotProduct{},
                 concrete,
                 secondary.template AsArrayHandle<vtkm::cont::ArrayHandle<T>>(),
                 result);
    outArray = result;
  };

  primaryArray
    .CastAndCallForTypesWithFloatFallback<VTKM_DEFAULT_TYPE_LIST, VTKM_DEFAULT_STORAGE_LIST>(
      ResolveType);

  vtkm::cont::DataSet outDataSet = inDataSet; // copy
  outDataSet.AddField({ this->GetOutputFieldName(),
                        this->GetFieldFromDataSet(inDataSet).GetAssociation(),
                        outArray });

  this->MapFieldsOntoOutput(inDataSet, outDataSet);

  return outDataSet;
}

} // namespace vector_calculus
} // namespace filter
} // namespace vtkm
