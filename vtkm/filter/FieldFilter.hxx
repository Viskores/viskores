//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/filter/DefaultPolicy.h>
#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/FilterTraits.h>

#include <vtkm/filter/internal/ResolveFieldTypeAndExecute.h>

#include <vtkm/cont/Error.h>
#include <vtkm/cont/ErrorControlBadAllocation.h>
#include <vtkm/cont/ErrorExecution.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

namespace vtkm {
namespace filter {


//-----------------------------------------------------------------------------
template<typename T, typename StorageTag>
bool FieldResult::FieldAs(vtkm::cont::ArrayHandle<T, StorageTag>& dest) const
{
  return this->FieldAs(dest, vtkm::filter::DefaultPolicy());
}

//-----------------------------------------------------------------------------
template<typename T, typename StorageTag, typename DerivedPolicy>
bool FieldResult::FieldAs(vtkm::cont::ArrayHandle<T, StorageTag>& dest,
                          const vtkm::filter::PolicyBase<DerivedPolicy>&) const
{
  try
  {
    typedef typename DerivedPolicy::FieldTypeList TypeList;
    typedef typename DerivedPolicy::FieldStorageList StorageList;

    vtkm::cont::DynamicArrayHandle handle = this->Field.GetData();
    handle.ResetTypeAndStorageLists(TypeList(),StorageList()).CopyTo(dest);
    return true;
  }
  catch(vtkm::cont::Error e)
  {
    (void)e;
  }

  return false;
}

//-----------------------------------------------------------------------------
template<typename Derived>
FieldResult FieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
                                          const std::string &inFieldName)
{
  return this->Execute(input,
                       input.GetField(inFieldName),
                       vtkm::filter::DefaultPolicy());
}

//-----------------------------------------------------------------------------
template<typename Derived>
FieldResult FieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
                                          const vtkm::cont::Field &field)
{
  return this->Execute(input,
                       field,
                       vtkm::filter::DefaultPolicy());
}

//-----------------------------------------------------------------------------
template<typename Derived>
FieldResult FieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
                                          const vtkm::cont::CoordinateSystem &field)
{
  return this->Execute(input,
                       field,
                       vtkm::filter::DefaultPolicy());
}

//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
FieldResult FieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
                                          const std::string &inFieldName,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy )
{
  return this->Execute(input,
                       input.GetField(inFieldName),
                       policy);
}

//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
FieldResult FieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
                                          const vtkm::cont::Field &field,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy )
{
  return this->PrepareForExecution(input, field, policy);
}

//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
FieldResult FieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
                                          const vtkm::cont::CoordinateSystem &field,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy )
{
  //we need to state that the field is actually a coordinate system, so that
  //the filter uses the proper policy to convert the types.
  return this->PrepareForExecution(input, field, policy);
}

//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
FieldResult FieldFilter<Derived>::PrepareForExecution(const vtkm::cont::DataSet &input,
                                                      const vtkm::cont::Field &field,
                                                      const vtkm::filter::PolicyBase<DerivedPolicy>& policy )
{
  //determine the field type first
  FieldResult result;
  typedef internal::ResolveFieldTypeAndExecute< Derived,  DerivedPolicy,
                                                FieldResult > FunctorType;
  FunctorType functor(static_cast<Derived*>(this),
                      input,
                      vtkm::filter::FieldMetadata(field),
                      policy,
                      this->Tracker,
                      result);

  typedef vtkm::filter::FilterTraits< Derived > Traits;
  vtkm::filter::Convert(field, policy, Traits()).CastAndCall(functor);
  return result;
}

//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
FieldResult FieldFilter<Derived>::PrepareForExecution(const vtkm::cont::DataSet &input,
                                                      const vtkm::cont::CoordinateSystem &field,
                                                      const vtkm::filter::PolicyBase<DerivedPolicy>& policy )
{
  //We have a special signature just for CoordinateSystem, so that we can ask
  //the policy for the storage types and value types just for coordinate systems

  //determine the field type first
  FieldResult result;
  typedef internal::ResolveFieldTypeAndExecute< Derived, DerivedPolicy,
                                                FieldResult > FunctorType;
  FunctorType functor(static_cast<Derived*>(this),
                      input,
                      vtkm::filter::FieldMetadata(field),
                      policy,
                      this->Tracker,
                      result);

  typedef vtkm::filter::FilterTraits< Derived > Traits;
  vtkm::filter::Convert(field, policy, Traits()).CastAndCall(functor);
  return result;
}

}
}
