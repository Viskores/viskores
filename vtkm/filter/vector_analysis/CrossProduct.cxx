//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/vector_analysis/CrossProduct.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/ArrayCopy.h>

#include <vtkm/VectorAnalysis.h>

namespace
{

class CrossProductWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);

  template <typename T>
  VTKM_EXEC void operator()(const vtkm::Vec<T, 3>& vec1,
                            const vtkm::Vec<T, 3>& vec2,
                            vtkm::Vec<T, 3>& outVec) const
  {
    outVec = vtkm::Cross(vec1, vec2);
  }
};

} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace vector_analysis
{

//-----------------------------------------------------------------------------
VTKM_CONT CrossProduct::CrossProduct()
{
  this->SetOutputFieldName("crossproduct");
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet CrossProduct::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::Field primaryField = this->GetFieldFromDataSet(0, inDataSet);
  vtkm::cont::UnknownArrayHandle primaryArray = primaryField.GetData();

  vtkm::cont::UnknownArrayHandle outArray;

  // We are using a C++14 auto lambda here. The advantage over a Functor is obvious, we don't
  // need to explicitly pass filter, input/output DataSets etc. thus reduce the impact to
  // the legacy code. The lambda can also access the private part of the filter thus reducing
  // filter's public interface profile. CastAndCall tries to cast primaryArray of unknown value
  // type and storage to a concrete ArrayHandle<T, S> with T from the `TypeList` and S from
  // `StorageList`. It then passes the concrete array to the lambda as the first argument.
  // We can later recover the concrete ValueType, T, from the concrete array.
  auto resolveType = [&](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    const auto& secondaryField = this->GetFieldFromDataSet(1, inDataSet);
    vtkm::cont::ArrayHandle<T> secondaryArray;
    vtkm::cont::ArrayCopyShallowIfPossible(secondaryField.GetData(), secondaryArray);

    vtkm::cont::ArrayHandle<T> result;
    this->Invoke(CrossProductWorklet{}, concrete, secondaryArray, result);
    outArray = result;
  };

  this->CastAndCallVecField<3>(primaryArray, resolveType);

  return this->CreateResultField(
    inDataSet, this->GetOutputFieldName(), primaryField.GetAssociation(), outArray);
}

}
}
} // namespace vtkm::filter::vector_analysis
