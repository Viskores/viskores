//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/filter/internal/CreateResult.h>

namespace vtkm
{
namespace filter
{

namespace detail
{

template <typename T, typename DeviceAdapter>
struct DotProductFunctor
{
  vtkm::cont::ArrayHandle<T> OutArray;

  template <typename PrimaryFieldType, typename SecondaryFieldType>
  void operator()(const SecondaryFieldType& secondaryField, const PrimaryFieldType& primaryField)
  {
    vtkm::worklet::DispatcherMapField<vtkm::worklet::DotProduct, DeviceAdapter> dispatcher;
    dispatcher.Invoke(primaryField,
                      vtkm::cont::make_ArrayHandleCast<vtkm::Vec<T, 3>>(secondaryField),
                      this->OutArray);
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
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet DotProduct::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter&)
{
  detail::DotProductFunctor<T, DeviceAdapter> functor;
  try
  {
    if (this->UseCoordinateSystemAsSecondaryField)
    {
      vtkm::cont::CastAndCall(
        inDataSet.GetCoordinateSystem(this->GetSecondaryCoordinateSystemIndex()), functor, field);
    }
    else
    {
      using Traits = vtkm::filter::FilterTraits<DotProduct>;
      using TypeList = vtkm::ListTagBase<vtkm::Vec<T, 3>>;
      vtkm::filter::ApplyPolicy(
        inDataSet.GetField(this->SecondaryFieldName, this->SecondaryFieldAssociation),
        policy,
        Traits())
        .ResetTypeList(TypeList())
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
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool DotProduct::DoMapField(vtkm::cont::DataSet& result,
                                             const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                             const vtkm::filter::FieldMetadata& fieldMeta,
                                             const vtkm::filter::PolicyBase<DerivedPolicy>&,
                                             DeviceAdapter)
{
  //we copy the input handle to the result dataset, reusing the metadata
  result.AddField(fieldMeta.AsField(input));
  return true;
}
}
} // namespace vtkm::filter
