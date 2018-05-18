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

#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/FilterTraits.h>
#include <vtkm/filter/PolicyDefault.h>

#include <vtkm/filter/internal/ResolveFieldTypeAndExecute.h>
#include <vtkm/filter/internal/ResolveFieldTypeAndMap.h>

#include <vtkm/cont/Error.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorExecution.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

namespace vtkm
{
namespace filter
{

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT FilterDataSetWithField<Derived>::FilterDataSetWithField()
  : OutputFieldName()
  , CellSetIndex(0)
  , CoordinateSystemIndex(0)
  , ActiveFieldName()
  , ActiveFieldAssociation(vtkm::cont::Field::Association::ANY)
  , UseCoordinateSystemAsField(false)
{
}

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT FilterDataSetWithField<Derived>::~FilterDataSetWithField()
{
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet FilterDataSetWithField<Derived>::PrepareForExecution(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (this->UseCoordinateSystemAsField)
  {
    // we need to state that the field is actually a coordinate system, so that
    // the filter uses the proper policy to convert the types.
    return this->PrepareForExecution(
      input, input.GetCoordinateSystem(this->GetActiveCellSetIndex()), policy);
  }
  else
  {
    return this->PrepareForExecution(
      input, input.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation()), policy);
  }
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet FilterDataSetWithField<Derived>::PrepareForExecution(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::Field& field,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  vtkm::filter::FieldMetadata metaData(field);
  vtkm::cont::DataSet result;

  using FunctorType =
    internal::ResolveFieldTypeAndExecute<Derived, DerivedPolicy, vtkm::cont::DataSet>;
  FunctorType functor(static_cast<Derived*>(this), input, metaData, policy, result);
  using Traits = vtkm::filter::FilterTraits<Derived>;
  vtkm::cont::CastAndCall(
    vtkm::filter::ApplyPolicy(field, policy, Traits()), functor, this->GetRuntimeDeviceTracker());
  return result;
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet FilterDataSetWithField<Derived>::PrepareForExecution(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::CoordinateSystem& field,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  //We have a special signature just for CoordinateSystem, so that we can ask
  //the policy for the storage types and value types just for coordinate systems
  vtkm::filter::FieldMetadata metaData(field);

  //determine the field type first
  vtkm::cont::DataSet result;
  using FunctorType =
    internal::ResolveFieldTypeAndExecute<Derived, DerivedPolicy, vtkm::cont::DataSet>;
  FunctorType functor(static_cast<Derived*>(this), input, metaData, policy, result);

  using Traits = vtkm::filter::FilterTraits<Derived>;
  constexpr bool supportsVec3 = vtkm::ListContains<typename Traits::InputFieldTypeList,
                                                   vtkm::Vec<vtkm::FloatDefault, 3>>::value;
  using supportsCoordinateSystem = std::integral_constant<bool, supportsVec3>;
  vtkm::cont::ConditionalCastAndCall(
    supportsCoordinateSystem(), field, functor, this->GetRuntimeDeviceTracker());

  return result;
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT bool FilterDataSetWithField<Derived>::MapFieldOntoOutput(
  vtkm::cont::DataSet& result,
  const vtkm::cont::Field& field,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  bool valid = false;

  vtkm::filter::FieldMetadata metaData(field);
  using FunctorType = internal::ResolveFieldTypeAndMap<Derived, DerivedPolicy>;
  FunctorType functor(static_cast<Derived*>(this), result, metaData, policy, valid);

  vtkm::cont::CastAndCall(
    vtkm::filter::ApplyPolicy(field, policy), functor, this->GetRuntimeDeviceTracker());

  //the bool valid will be modified by the map algorithm to hold if the
  //mapping occurred or not. If the mapping was good a new field has been
  //added to the result that was passed in.
  return valid;
}
}
}
