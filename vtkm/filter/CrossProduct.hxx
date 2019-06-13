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
#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace filter
{

namespace detail
{

template <typename T>
struct CrossProductFunctor
{
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> OutArray;

  template <typename PrimaryFieldType, typename SecondaryFieldType>
  void operator()(const SecondaryFieldType& secondaryField, const PrimaryFieldType& primaryField)
  {
    vtkm::worklet::DispatcherMapField<vtkm::worklet::CrossProduct> dispatcher;
    dispatcher.Invoke(primaryField,
                      vtkm::cont::make_ArrayHandleCast<vtkm::Vec<T, 3>>(secondaryField),
                      this->OutArray);
  }
};

} // namespace detail

//-----------------------------------------------------------------------------
inline VTKM_CONT CrossProduct::CrossProduct()
  : vtkm::filter::FilterField<CrossProduct>()
  , SecondaryFieldName()
  , SecondaryFieldAssociation(vtkm::cont::Field::Association::ANY)
  , UseCoordinateSystemAsSecondaryField(false)
  , SecondaryCoordinateSystemIndex(0)
{
  this->SetOutputFieldName("crossproduct");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet CrossProduct::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  detail::CrossProductFunctor<T> functor;
  try
  {
    if (this->UseCoordinateSystemAsSecondaryField)
    {
      vtkm::cont::CastAndCall(
        inDataSet.GetCoordinateSystem(this->GetSecondaryCoordinateSystemIndex()), functor, field);
    }
    else
    {
      using Traits = vtkm::filter::FilterTraits<CrossProduct>;
      using TypeList = vtkm::ListTagBase<vtkm::Vec<T, 3>>;
      vtkm::filter::ApplyPolicy(
        inDataSet.GetField(this->SecondaryFieldName, this->SecondaryFieldAssociation),
        policy,
        Traits())
        .ResetTypes(TypeList())
        .CastAndCall(functor, field);
    }
  }
  catch (const vtkm::cont::Error&)
  {
    throw vtkm::cont::ErrorExecution("failed to execute.");
  }


  return internal::CreateResult(inDataSet,
                                functor.OutArray,
                                this->GetOutputFieldName(),
                                fieldMetadata.GetAssociation(),
                                fieldMetadata.GetCellSetName());
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool CrossProduct::DoMapField(vtkm::cont::DataSet& result,
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
