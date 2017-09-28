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

template <typename Derived, typename DerivedPolicy, typename ResultType>
struct ResolveFieldTypeAndExecute
{
  typedef ResolveFieldTypeAndExecute<Derived, DerivedPolicy, ResultType> Self;

  Derived* DerivedClass;
  const vtkm::cont::DataSet& InputData;
  const vtkm::filter::FieldMetadata& Metadata;
  const vtkm::filter::PolicyBase<DerivedPolicy>& Policy;
  vtkm::cont::RuntimeDeviceTracker Tracker;
  ResultType& Result;

  ResolveFieldTypeAndExecute(Derived* derivedClass,
                             const vtkm::cont::DataSet& inputData,
                             const vtkm::filter::FieldMetadata& fieldMeta,
                             const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                             const vtkm::cont::RuntimeDeviceTracker& tracker,
                             ResultType& result)
    : DerivedClass(derivedClass)
    , InputData(inputData)
    , Metadata(fieldMeta)
    , Policy(policy)
    , Tracker(tracker)
    , Result(result)
  {
  }

private:
  template <typename T, typename StorageTag>
  struct ResolveFieldTypeAndExecuteForDevice
  {
    typedef vtkm::cont::ArrayHandle<T, StorageTag> FieldArrayHandle;
    ResolveFieldTypeAndExecuteForDevice(const Self& instance, const FieldArrayHandle& field)
      : Instance(instance)
      , Field(field)
    {
    }

    const Self& Instance;
    const vtkm::cont::ArrayHandle<T, StorageTag>& Field;

    template <typename DeviceAdapterTag>
    bool operator()(DeviceAdapterTag tag) const
    {
      this->Instance.Result = this->Instance.DerivedClass->DoExecute(
        this->Instance.InputData, this->Field, this->Instance.Metadata, this->Instance.Policy, tag);
      return this->Instance.Result.IsValid();
    }

  private:
    void operator=(const ResolveFieldTypeAndExecuteForDevice<T, StorageTag>&) = delete;
  };

public:
  template <typename T, typename StorageTag>
  void operator()(const vtkm::cont::ArrayHandle<T, StorageTag>& field) const
  {
    ResolveFieldTypeAndExecuteForDevice<T, StorageTag> doResolve(*this, field);
    vtkm::cont::TryExecute(doResolve, this->Tracker, typename DerivedPolicy::DeviceAdapterList());
  }

private:
  void operator=(const ResolveFieldTypeAndExecute<Derived, DerivedPolicy, ResultType>&) = delete;
};
}
}
} // namespace vtkm::filter::internal

#endif //vtk_m_filter_internal_ResolveFieldTypeAndExecute_h
