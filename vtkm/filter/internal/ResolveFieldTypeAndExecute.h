//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_filter_internal_ResolveFieldTypeAndExecute_h
#define vtk_m_filter_internal_ResolveFieldTypeAndExecute_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{
namespace internal
{

struct ResolveFieldTypeAndExecuteForDevice
{
  template <typename DeviceAdapterTag, typename InstanceType, typename FieldType>
  bool operator()(DeviceAdapterTag tag, InstanceType&& instance, FieldType&& field) const
  {
    instance.Result = instance.DerivedClass->DoExecute(
      instance.InputData, field, instance.Metadata, instance.Policy, tag);
    // `DoExecute` is expected to throw an exception on any failure. If it
    // returned anything, it's taken as a success and we won't try executing on
    // other available devices.
    return true;
  }
};


template <typename Derived, typename DerivedPolicy, typename ResultType>
struct ResolveFieldTypeAndExecute
{
  using Self = ResolveFieldTypeAndExecute<Derived, DerivedPolicy, ResultType>;

  Derived* DerivedClass;
  const vtkm::cont::DataSet& InputData;
  const vtkm::filter::FieldMetadata& Metadata;
  const vtkm::filter::PolicyBase<DerivedPolicy>& Policy;
  ResultType& Result;

  ResolveFieldTypeAndExecute(Derived* derivedClass,
                             const vtkm::cont::DataSet& inputData,
                             const vtkm::filter::FieldMetadata& fieldMeta,
                             const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                             ResultType& result)
    : DerivedClass(derivedClass)
    , InputData(inputData)
    , Metadata(fieldMeta)
    , Policy(policy)
    , Result(result)
  {
  }

  template <typename T, typename StorageTag>
  void operator()(const vtkm::cont::ArrayHandle<T, StorageTag>& field,
                  vtkm::cont::RuntimeDeviceTracker& tracker) const
  {
    ResolveFieldTypeAndExecuteForDevice doResolve;
    vtkm::cont::TryExecute(
      doResolve, tracker, typename DerivedPolicy::DeviceAdapterList(), *this, field);
  }

private:
  void operator=(const ResolveFieldTypeAndExecute<Derived, DerivedPolicy, ResultType>&) = delete;
};
}
}
} // namespace vtkm::filter::internal

#endif //vtk_m_filter_internal_ResolveFieldTypeAndExecute_h
