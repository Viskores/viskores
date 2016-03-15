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
#ifndef vtk_m_filter_internal_ResolveFieldTypeAndMap_h
#define vtk_m_filter_internal_ResolveFieldTypeAndMap_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/PolicyBase.h>
#include <vtkm/filter/internal/RuntimeDeviceTracker.h>

//forward declarations needed
namespace vtkm {
namespace filter {
  class DataSetResult;
}
}

namespace
{

template<bool> struct CanMap;

template<typename ClassType,
         typename ArrayType,
         typename DerivedPolicy,
         typename DeviceAdapterTag
        >
bool map_if_valid(ClassType* c,
                        vtkm::filter::DataSetResult& input,
                        const ArrayType &field,
                        const vtkm::filter::FieldMetadata& fieldMeta,
                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                        vtkm::filter::internal::RuntimeDeviceTracker& tracker,
                        DeviceAdapterTag tag)
{
  typedef vtkm::cont::DeviceAdapterTraits<
                                      DeviceAdapterTag> DeviceAdapterTraits;

  typedef CanMap<DeviceAdapterTraits::Valid> CanMapType;
  return CanMapType::Run(c,input,field,fieldMeta,policy,tracker,tag);
}


//Implementation that we call on device adapters we don't have support
//enabled for
template<>
struct CanMap<false>
{
  template<typename ClassType,
           typename ArrayType,
           typename DerivedPolicy,
           typename DeviceAdapterTag>
  static bool Run(ClassType*,
                  vtkm::filter::DataSetResult&,
                  const ArrayType &,
                  const vtkm::filter::FieldMetadata&,
                  const vtkm::filter::PolicyBase<DerivedPolicy>&,
                  vtkm::filter::internal::RuntimeDeviceTracker&,
                  DeviceAdapterTag)
  {
    return false;
  }
};

//Implementation that we call on device adapters we do have support
//enabled for
template<>
struct CanMap<true>
{
  template<typename ClassType,
           typename ArrayType,
           typename DerivedPolicy,
           typename DeviceAdapterTag>
  static bool Run(ClassType* c,
                  vtkm::filter::DataSetResult& input,
                  const ArrayType &field,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                  vtkm::filter::internal::RuntimeDeviceTracker& tracker,
                  DeviceAdapterTag tag)
  {
  const bool runtime_usable_device = tracker.CanRunOn(tag);

  bool valid = false;
  if(runtime_usable_device)
  {
    try
    {
      valid = c->DoMapField(input,field,fieldMeta,policy,tag);
    }
    catch(vtkm::cont::ErrorControlBadAllocation e)
    {
      std::cerr << "caught ErrorControlBadAllocation " << e.GetMessage() << std::endl;
      //currently we only consider OOM errors worth disabling a device for
      //than we fallback to another device
      tracker.ReportAllocationFailure(tag,e);
    }
    catch(vtkm::cont::ErrorControlBadType e)
    {
      //bad type errors should stop the filter, instead of deferring to
      //another device adapter
      std::cerr << "caught ErrorControlBadType : " << e.GetMessage() << std::endl;
    }
    catch(vtkm::cont::ErrorControlBadValue e)
    {
      //bad value errors should stop the filter, instead of deferring to
      //another device adapter
      std::cerr << "caught ErrorControlBadValue : " << e.GetMessage() << std::endl;
    }
    catch(vtkm::cont::ErrorControlAssert e)
    {
      //assert occurred, generally caused by going out of bounds on an array
      //this won't be solved by trying a different device adapter
      //so stop the filter
      std::cerr << "caught ErrorControlAssert : " << e.GetMessage() << std::endl;
    }
    catch(vtkm::cont::Error e)
    {
      //general errors should be caught and let us try the next device adapter.
      std::cerr << "exception is: " << e.GetMessage() << std::endl;
    }
  }

  return valid;
  }
};
}

namespace vtkm {
namespace filter {
namespace internal {

namespace
{
  template<typename Derived, typename DerivedPolicy>
  struct ResolveFieldTypeAndMap
  {
    Derived* DerivedClass;
    vtkm::filter::DataSetResult& InputResult;
    const vtkm::filter::FieldMetadata& Metadata;
    const vtkm::filter::PolicyBase<DerivedPolicy>& Policy;
    vtkm::filter::internal::RuntimeDeviceTracker& Tracker;
    bool& RanProperly;


    ResolveFieldTypeAndMap(Derived* derivedClass,
                           vtkm::filter::DataSetResult& inResult,
                           const vtkm::filter::FieldMetadata& fieldMeta,
                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                           vtkm::filter::internal::RuntimeDeviceTracker& tracker,
                           bool& ran):
      DerivedClass(derivedClass),
      InputResult(inResult),
      Metadata(fieldMeta),
      Policy(policy),
      Tracker(tracker),
      RanProperly(ran)
      {

      }

    template<typename T, typename StorageTag>
    void operator()(const vtkm::cont::ArrayHandle<T,StorageTag>& field) const
    {
      typedef vtkm::cont::DeviceAdapterTagCuda CudaTag;
      typedef vtkm::cont::DeviceAdapterTagTBB TBBTag;
      typedef vtkm::cont::DeviceAdapterTagSerial SerialTag;

      bool valid = false;

      {
        valid = map_if_valid(this->DerivedClass,
                             this->InputResult,
                             field,
                             this->Metadata,
                             this->Policy,
                             this->Tracker,
                             CudaTag() );
      }

      if( !valid )
      {
        valid = map_if_valid(this->DerivedClass,
                             this->InputResult,
                             field,
                             this->Metadata,
                             this->Policy,
                             this->Tracker,
                             TBBTag() );
      }
      if( !valid )
      {
        valid = map_if_valid(this->DerivedClass,
                             this->InputResult,
                             field,
                             this->Metadata,
                             this->Policy,
                             this->Tracker,
                             SerialTag() );
      }
      this->RanProperly = valid;
    }
  };
}

}
}
}  // namespace vtkm::filter::internal

#endif //vtk_m_filter_internal_ResolveFieldTypeAndExecute_h
