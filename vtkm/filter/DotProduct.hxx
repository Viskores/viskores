//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/filter/internal/CreateResult.h>

namespace vtkm
{
namespace filter
{

namespace detail
{

struct DotProductFunctor
{
  vtkm::cont::Invoker& Invoke;
  DotProductFunctor(vtkm::cont::Invoker& invoke)
    : Invoke(invoke)
  {
  }

  template <typename SecondaryFieldType, typename StorageType, typename T>
  void operator()(const SecondaryFieldType& secondaryField,
                  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& primaryField,
                  vtkm::cont::ArrayHandle<T>& output) const
  {
    this->Invoke(vtkm::worklet::DotProduct{},
                 primaryField,
                 vtkm::cont::make_ArrayHandleCast<vtkm::Vec<T, 3>>(secondaryField),
                 output);
  }
};

} // namespace detail

//-----------------------------------------------------------------------------
inline VTKM_CONT DotProduct::DotProduct()
  : vtkm::filter::FilterField<DotProduct>()
  , SecondaryFieldName()
  , SecondaryFieldAssociation(vtkm::cont::Field::Association::ANY)
  , UseCoordinateSystemAsSecondaryField(false)
  , SecondaryCoordinateSystemIndex(0)
{
  this->SetOutputFieldName("dotproduct");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet DotProduct::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  detail::DotProductFunctor functor(this->Invoke);
  vtkm::cont::ArrayHandle<T> output;
  try
  {
    if (this->UseCoordinateSystemAsSecondaryField)
    {
      vtkm::cont::CastAndCall(
        inDataSet.GetCoordinateSystem(this->GetSecondaryCoordinateSystemIndex()),
        functor,
        field,
        output);
    }
    else
    {
      using Traits = vtkm::filter::FilterTraits<DotProduct>;
      using TypeList = vtkm::ListTagBase<vtkm::Vec<T, 3>>;
      vtkm::filter::ApplyPolicy(
        inDataSet.GetField(this->SecondaryFieldName, this->SecondaryFieldAssociation),
        policy,
        Traits())
        .ResetTypes(TypeList())
        .CastAndCall(functor, field, output);
    }
  }
  catch (const vtkm::cont::Error&)
  {
    throw vtkm::cont::ErrorExecution("failed to execute.");
  }

  return internal::CreateResult(inDataSet,
                                output,
                                this->GetOutputFieldName(),
                                fieldMetadata.GetAssociation(),
                                fieldMetadata.GetCellSetName());
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool DotProduct::DoMapField(vtkm::cont::DataSet& result,
                                             const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                             const vtkm::filter::FieldMetadata& fieldMeta,
                                             vtkm::filter::PolicyBase<DerivedPolicy>)
{
  //we copy the input handle to the result dataset, reusing the metadata
  result.AddField(fieldMeta.AsField(input));
  return true;
}
}
} // namespace vtkm::filter
