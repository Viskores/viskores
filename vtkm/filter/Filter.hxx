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
#include <vtkm/filter/Result.h>

#include <vtkm/filter/internal/ResolveFieldTypeAndExecute.h>
#include <vtkm/filter/internal/ResolveFieldTypeAndMap.h>

#include <vtkm/cont/Error.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/Field.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

namespace vtkm
{
namespace filter
{

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT Filter<Derived>::Filter()
  : Tracker(vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
}

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT Filter<Derived>::~Filter()
{
}

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT vtkm::cont::DataSet Filter<Derived>::Execute(const vtkm::cont::DataSet& input,
                                                              const FieldSelection& fieldSelection)
{
  return this->Execute(input, vtkm::filter::PolicyDefault(), fieldSelection);
}

//----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet Filter<Derived>::Execute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const FieldSelection& fieldSelection)
{
  Derived* self = static_cast<Derived*>(this);
  //self->DoPreExecute(input, policy, fieldSelection);
  Result result = self->PrepareForExecution(input, policy);
  //self->DoPostExecute(result.GetDataSet(), input, policy);
  if (!result.IsValid())
  {
    throw vtkm::cont::ErrorExecution("Failed to execute filter.");
  }

  for (vtkm::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
  {
    auto field = input.GetField(cc);
    if (fieldSelection.IsFieldSelected(field))
    {
      self->MapFieldOntoOutput(result, field, policy);
    }
  }

  return result.GetDataSet();
}

//----------------------------------------------------------------------------
template <typename Derived>
inline VTKM_CONT vtkm::cont::MultiBlock Filter<Derived>::Execute(
  const vtkm::cont::MultiBlock& input,
  const FieldSelection& fieldSelection)
{
  return this->Execute(input, vtkm::filter::PolicyDefault(), fieldSelection);
}

//----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::MultiBlock Filter<Derived>::Execute(
  const vtkm::cont::MultiBlock& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const FieldSelection& fieldSelection)
{
  // Derived* self = static_cast<Derived*>(this);
  //self->DoPreExecute(input, policy, fieldSelection);

  vtkm::cont::MultiBlock output;
  for (auto& inDataSet : input)
  {
    vtkm::cont::DataSet outDataSet = this->Execute(inDataSet, policy, fieldSelection);
    output.AddBlock(outDataSet);
  }
  //self->DoPreExecute(output, input, policy);
  return output;
}
}
}
