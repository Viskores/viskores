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

#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/FilterTraits.h>
#include <vtkm/filter/PolicyDefault.h>

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
FilterDataSet<Derived>::FilterDataSet():
  OutputFieldName(),
  CellSetIndex(0),
  CoordinateSystemIndex(0),
  Tracker()
{

}

//-----------------------------------------------------------------------------
template<typename Derived>
ResultDataSet FilterDataSet<Derived>::Execute(const vtkm::cont::DataSet &input)
{
  return this->Execute(input, vtkm::filter::DefaultPolicy());
}

//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
ResultDataSet FilterDataSet<Derived>::Execute(const vtkm::cont::DataSet &input,
                                              const vtkm::filter::PolicyBase<DerivedPolicy>& policy )
{
  return this->PrepareForExecution(input, policy);
}


//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
ResultDataSet FilterDataSet<Derived>::PrepareForExecution(const vtkm::cont::DataSet &input,
                                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy )
{
  typedef vtkm::cont::DeviceAdapterTagCuda CudaTag;
  typedef vtkm::cont::DeviceAdapterTagTBB TBBTag;
  typedef vtkm::cont::DeviceAdapterTagSerial SerialTag;

  ResultDataSet result = run_if_valid<ResultDataSet>( static_cast<Derived*>(this),
                                       input,
                                       policy,
                                       this->Tracker,
                                       CudaTag() );
  if( !result.IsValid() )
  {
    result = run_if_valid<ResultDataSet>( static_cast<Derived*>(this),
                                       input,
                                       policy,
                                       this->Tracker,
                                       TBBTag() );
  }
  if( !result.IsValid() )
  {
    result = run_if_valid<ResultDataSet>( static_cast<Derived*>(this),
                                       input,
                                       policy,
                                       this->Tracker,
                                       SerialTag() );
  }

  return result;
}

//-----------------------------------------------------------------------------
template<typename Derived>
bool FilterDataSet<Derived>::MapFieldOntoOutput(ResultDataSet& result,
                                                const vtkm::cont::Field& field)
{
  return this->MapFieldOntoOutput(result, field, vtkm::filter::DefaultPolicy());
}

//-----------------------------------------------------------------------------
template<typename Derived>
template<typename DerivedPolicy>
bool FilterDataSet<Derived>::MapFieldOntoOutput(ResultDataSet& result,
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
    vtkm::cont::CastAndCall( vtkm::filter::ApplyPolicy(field, policy, Traits()),
                             functor );
    }

  //the bool valid will be modified by the map algorithm to hold if the
  //mapping occurred or not. If the mapping was good a new field has been
  //added to the ResultDataSet that was passed in.
  return valid;

}


}
}
