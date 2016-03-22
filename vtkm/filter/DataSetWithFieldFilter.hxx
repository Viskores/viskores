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
#include <vtkm/filter/internal/ResolveFieldTypeAndMap.h>

#include <vtkm/cont/Error.h>
#include <vtkm/cont/ErrorControlBadAllocation.h>
#include <vtkm/cont/ErrorExecution.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

namespace vtkm {
namespace filter {

//----------------------------------------------------------------------------
template<class Derived>
DataSetWithFieldFilter<Derived>::DataSetWithFieldFilter():
  OutputFieldName(),
  CellSetIndex(0),
  CoordinateSystemIndex(0),
  Tracker()
{

}

//-----------------------------------------------------------------------------
template<typename Derived>
DataSetResult DataSetWithFieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
                                             const std::string &inFieldName)
{
  return this->Execute(input,
                       input.GetField(inFieldName),
                       vtkm::filter::DefaultPolicy());
}

//-----------------------------------------------------------------------------
template<typename Derived>
DataSetResult DataSetWithFieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
                                             const vtkm::cont::Field &field)
{
  return this->Execute(input,
                       field,
                       vtkm::filter::DefaultPolicy());
}

//-----------------------------------------------------------------------------
template<typename Derived>
DataSetResult DataSetWithFieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
                                             const vtkm::cont::CoordinateSystem &field)
{
  return this->Execute(input,
                       field,
                       vtkm::filter::DefaultPolicy());
}

//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
DataSetResult DataSetWithFieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
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
DataSetResult DataSetWithFieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
                                             const vtkm::cont::Field &field,
                                             const vtkm::filter::PolicyBase<DerivedPolicy>& policy )
{
  return this->PrepareForExecution(input, field, policy);
}

//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
DataSetResult DataSetWithFieldFilter<Derived>::Execute(const vtkm::cont::DataSet &input,
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
DataSetResult DataSetWithFieldFilter<Derived>::PrepareForExecution(const vtkm::cont::DataSet &input,
                                                         const vtkm::cont::Field &field,
                                                         const vtkm::filter::PolicyBase<DerivedPolicy>& policy )
{
  vtkm::filter::FieldMetadata metaData(field);
  DataSetResult result;

  typedef internal::ResolveFieldTypeAndExecute< Derived,  DerivedPolicy,
                                                DataSetResult > FunctorType;
  FunctorType functor(static_cast<Derived*>(this),
                      input,
                      metaData,
                      policy,
                      this->Tracker,
                      result);

  typedef vtkm::filter::FilterTraits< Derived > Traits;
  vtkm::filter::ApplyPolicy(field, policy, Traits()).CastAndCall(functor);

  return result;
}

//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
DataSetResult DataSetWithFieldFilter<Derived>::PrepareForExecution(const vtkm::cont::DataSet &input,
                                                         const vtkm::cont::CoordinateSystem &field,
                                                         const vtkm::filter::PolicyBase<DerivedPolicy>& policy )
{
  //We have a special signature just for CoordinateSystem, so that we can ask
  //the policy for the storage types and value types just for coordinate systems
  vtkm::filter::FieldMetadata metaData(field);

  //determine the field type first
  DataSetResult result;
  typedef internal::ResolveFieldTypeAndExecute< Derived, DerivedPolicy,
                                                DataSetResult > FunctorType;
  FunctorType functor(static_cast<Derived*>(this),
                      input,
                      metaData,
                      policy,
                      this->Tracker,
                      result);

  typedef vtkm::filter::FilterTraits< Derived > Traits;
  vtkm::filter::ApplyPolicy(field, policy, Traits()).CastAndCall(functor);

  return result;
}

//-----------------------------------------------------------------------------
template<typename Derived>
bool DataSetWithFieldFilter<Derived>::MapFieldOntoOutput(DataSetResult& result,
                                                const vtkm::cont::Field& field)
{
  return this->MapFieldOntoOutput(result, field, vtkm::filter::DefaultPolicy());
}

//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
bool DataSetWithFieldFilter<Derived>::MapFieldOntoOutput(DataSetResult& result,
                                                const vtkm::cont::Field& field,
                                                const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  bool valid = false;
  if(result.IsValid())
    {
    vtkm::filter::FieldMetadata metaData(field);
    typedef internal::ResolveFieldTypeAndMap< Derived,
                                              DerivedPolicy > FunctorType;
    FunctorType functor(static_cast<Derived*>(this),
                        result,
                        metaData,
                        policy,
                        this->Tracker,
                        valid);

    typedef vtkm::filter::FilterTraits< Derived > Traits;
    vtkm::filter::ApplyPolicy(field, policy, Traits()).CastAndCall(functor);
    }

  //the bool valid will be modified by the map algorithm to hold if the
  //mapping occurred or not. If the mapping was good a new field has been
  //added to the DataSetResult that was passed in.
  return valid;

}


}
}
