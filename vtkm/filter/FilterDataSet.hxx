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
template <class Derived>
inline VTKM_CONT FilterDataSet<Derived>::FilterDataSet()
  : OutputFieldName()
  , CellSetIndex(0)
  , CoordinateSystemIndex(0)
  , Tracker(vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
}

//----------------------------------------------------------------------------
template <class Derived>
inline VTKM_CONT FilterDataSet<Derived>::~FilterDataSet()
{
}

//-----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT Result FilterDataSet<Derived>::Execute(const vtkm::cont::DataSet& input)
{
  return this->Execute(input, vtkm::filter::PolicyDefault());
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT Result
FilterDataSet<Derived>::Execute(const vtkm::cont::DataSet& input,
                                const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  return this->PrepareForExecution(input, policy);
}

//-----------------------------------------------------------------------------
namespace detail
{
template <typename Derived, typename DerivedPolicy>
struct FilterDataSetPrepareForExecutionFunctor
{
  vtkm::filter::Result Result;
  Derived* Self;
  const vtkm::cont::DataSet& Input;
  const vtkm::filter::PolicyBase<DerivedPolicy>& Policy;

  VTKM_CONT
  FilterDataSetPrepareForExecutionFunctor(Derived* self,
                                          const vtkm::cont::DataSet& input,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
    : Self(self)
    , Input(input)
    , Policy(policy)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    this->Result = this->Self->DoExecute(this->Input, this->Policy, Device());
    return this->Result.IsValid();
  }

private:
  void operator=(FilterDataSetPrepareForExecutionFunctor<Derived, DerivedPolicy>&) = delete;
};
} // namespace detail

template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT Result
FilterDataSet<Derived>::PrepareForExecution(const vtkm::cont::DataSet& input,
                                            const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  // When we move to C++11, this could probably be an anonymous class
  detail::FilterDataSetPrepareForExecutionFunctor<Derived, DerivedPolicy> functor(
    static_cast<Derived*>(this), input, policy);

  vtkm::cont::TryExecute(functor, this->Tracker, typename DerivedPolicy::DeviceAdapterList());

  return functor.Result;
}

//-----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT bool FilterDataSet<Derived>::MapFieldOntoOutput(Result& result,
                                                                 const vtkm::cont::Field& field)
{
  return this->MapFieldOntoOutput(result, field, vtkm::filter::PolicyDefault());
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT bool FilterDataSet<Derived>::MapFieldOntoOutput(
  Result& result,
  const vtkm::cont::Field& field,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  bool valid = false;
  if (result.IsValid())
  {
    vtkm::filter::FieldMetadata metaData(field);
    typedef internal::ResolveFieldTypeAndMap<Derived, DerivedPolicy> FunctorType;
    FunctorType functor(
      static_cast<Derived*>(this), result, metaData, policy, this->Tracker, valid);

    typedef vtkm::filter::FilterTraits<Derived> Traits;
    vtkm::cont::CastAndCall(vtkm::filter::ApplyPolicy(field, policy, Traits()), functor);
  }

  //the bool valid will be modified by the map algorithm to hold if the
  //mapping occurred or not. If the mapping was good a new field has been
  //added to the Result that was passed in.
  return valid;
}
}
}
