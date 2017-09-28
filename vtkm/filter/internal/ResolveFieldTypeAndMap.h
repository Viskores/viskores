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
#ifndef vtk_m_filter_internal_ResolveFieldTypeAndMap_h
#define vtk_m_filter_internal_ResolveFieldTypeAndMap_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/PolicyBase.h>

//forward declarations needed
namespace vtkm
{
namespace filter
{
class Result;
}
}

namespace vtkm
{
namespace filter
{
namespace internal
{

template <typename Derived, typename DerivedPolicy>
struct ResolveFieldTypeAndMap
{
  typedef ResolveFieldTypeAndMap<Derived, DerivedPolicy> Self;

  Derived* DerivedClass;
  vtkm::filter::Result& InputResult;
  const vtkm::filter::FieldMetadata& Metadata;
  const vtkm::filter::PolicyBase<DerivedPolicy>& Policy;
  vtkm::cont::RuntimeDeviceTracker Tracker;
  bool& RanProperly;

  ResolveFieldTypeAndMap(Derived* derivedClass,
                         vtkm::filter::Result& inResult,
                         const vtkm::filter::FieldMetadata& fieldMeta,
                         const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                         const vtkm::cont::RuntimeDeviceTracker& tracker,
                         bool& ran)
    : DerivedClass(derivedClass)
    , InputResult(inResult)
    , Metadata(fieldMeta)
    , Policy(policy)
    , Tracker(tracker)
    , RanProperly(ran)
  {
  }

private:
  template <typename T, typename StorageTag>
  struct ResolveFieldTypeAndMapForDevice
  {
    typedef vtkm::cont::ArrayHandle<T, StorageTag> FieldArrayHandle;
    ResolveFieldTypeAndMapForDevice(const Self& instance, const FieldArrayHandle& field)
      : Instance(instance)
      , Field(field)
      , Valid(false)
    {
    }

    const Self& Instance;
    const vtkm::cont::ArrayHandle<T, StorageTag>& Field;
    mutable bool Valid;

    template <typename DeviceAdapterTag>
    bool operator()(DeviceAdapterTag tag) const
    {
      this->Valid = this->Instance.DerivedClass->DoMapField(this->Instance.InputResult,
                                                            this->Field,
                                                            this->Instance.Metadata,
                                                            this->Instance.Policy,
                                                            tag);

      return this->Valid;
    }

  private:
    void operator=(const ResolveFieldTypeAndMapForDevice<T, StorageTag>&) = delete;
  };

public:
  template <typename T, typename StorageTag>
  void operator()(const vtkm::cont::ArrayHandle<T, StorageTag>& field) const
  {
    ResolveFieldTypeAndMapForDevice<T, StorageTag> doResolve(*this, field);
    vtkm::cont::TryExecute(doResolve, this->Tracker, typename DerivedPolicy::DeviceAdapterList());
    this->RanProperly = doResolve.Valid;
  }

private:
  void operator=(const ResolveFieldTypeAndMap<Derived, DerivedPolicy>&) = delete;
};
}
}
} // namespace vtkm::filter::internal

#endif //vtk_m_filter_internal_ResolveFieldTypeAndMap_h
