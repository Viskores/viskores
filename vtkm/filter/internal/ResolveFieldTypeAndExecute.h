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
#ifndef vtk_m_filter_internal_ResolveFieldTypeAndExecute_h
#define vtk_m_filter_internal_ResolveFieldTypeAndExecute_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/PolicyBase.h>
#include <vtkm/filter/internal/RuntimeDeviceTracker.h>

namespace
{

template<typename ReturnType, bool> struct CanExecute;

template<typename ReturnType,
         typename ClassType,
         typename ArrayType,
         typename DerivedPolicy,
         typename DeviceAdapterTag
        >
ReturnType run_if_valid(ClassType* c,
                        const vtkm::cont::DataSet& ds,
                        const ArrayType &field,
                        const vtkm::filter::FieldMetadata& fieldMeta,
                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                        vtkm::filter::internal::RuntimeDeviceTracker& tracker,
                        DeviceAdapterTag tag)
{
  typedef vtkm::cont::DeviceAdapterTraits<
                                      DeviceAdapterTag> DeviceAdapterTraits;

  typedef CanExecute<ReturnType, DeviceAdapterTraits::Valid> CanExecuteType;
  return CanExecuteType::Run(c,ds,field,fieldMeta,policy,tracker,tag);
}

template<typename ReturnType,
         typename ClassType,
         typename DerivedPolicy,
         typename DeviceAdapterTag
        >
ReturnType run_if_valid(ClassType* c,
                        const vtkm::cont::DataSet& ds,
                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                        vtkm::filter::internal::RuntimeDeviceTracker& tracker,
                        DeviceAdapterTag tag)
{
  typedef vtkm::cont::DeviceAdapterTraits<
                                      DeviceAdapterTag> DeviceAdapterTraits;

  typedef CanExecute<ReturnType, DeviceAdapterTraits::Valid> CanExecuteType;
  return CanExecuteType::Run(c,ds,policy,tracker,tag);
}


//Implementation that we call on device adapters we don't have support
//enabled for
template<typename ReturnType>
struct CanExecute<ReturnType, false>
{
  template<typename ClassType,
           typename ArrayType,
           typename DerivedPolicy,
           typename DeviceAdapterTag>
  static ReturnType Run(ClassType*,
                        const vtkm::cont::DataSet &,
                        const ArrayType &,
                        const vtkm::filter::FieldMetadata&,
                        const vtkm::filter::PolicyBase<DerivedPolicy>&,
                        vtkm::filter::internal::RuntimeDeviceTracker&,
                        DeviceAdapterTag)
  {
    return ReturnType();
  }

  template<typename ClassType,
           typename DerivedPolicy,
           typename DeviceAdapterTag>
  static ReturnType Run(ClassType*,
                        const vtkm::cont::DataSet &,
                        const vtkm::filter::PolicyBase<DerivedPolicy>&,
                        vtkm::filter::internal::RuntimeDeviceTracker&,
                        DeviceAdapterTag)
  {
    return ReturnType();
  }
};

//Implementation that we call on device adapters we do have support
//enabled for
template<typename ReturnType>
struct CanExecute<ReturnType,true>
{
  template<typename ClassType,
           typename ArrayType,
           typename DerivedPolicy,
           typename DeviceAdapterTag>
  static ReturnType Run(ClassType* c,
                        const vtkm::cont::DataSet &input,
                        const ArrayType &field,
                        const vtkm::filter::FieldMetadata& fieldMeta,
                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                        vtkm::filter::internal::RuntimeDeviceTracker& tracker,
                        DeviceAdapterTag tag)
  {
  const bool runtime_usable_device = tracker.CanRunOn(tag);

  ReturnType result;
  if(runtime_usable_device)
  {
    try
    {
      result = c->DoExecute(input,field,fieldMeta,policy,tag);
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
    catch(vtkm::cont::Error e)
    {
      //general errors should be caught and let us try the next device adapter.
      std::cerr << "exception is: " << e.GetMessage() << std::endl;
    }
  }

  return result;
  }

  template<typename ClassType,
           typename DerivedPolicy,
           typename DeviceAdapterTag>
  static ReturnType Run(ClassType* c,
                        const vtkm::cont::DataSet &input,
                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                        vtkm::filter::internal::RuntimeDeviceTracker& tracker,
                        DeviceAdapterTag tag)
  {
  const bool runtime_usable_device = tracker.CanRunOn(tag);

  ReturnType result;
  if(runtime_usable_device)
  {
    try
    {
      result = c->DoExecute(input,policy,tag);
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
    catch(vtkm::cont::Error e)
    {
      //general errors should be caught and let us try the next device adapter.
      std::cerr << "exception is: " << e.GetMessage() << std::endl;
    }
  }

  return result;
  }
};
}

namespace vtkm {
namespace filter {
namespace internal {

namespace
{
  template<typename Derived, typename DerivedPolicy, typename ResultType>
  struct ResolveFieldTypeAndExecute
  {
    Derived* DerivedClass;
    const vtkm::cont::DataSet &InputData;
    const vtkm::filter::FieldMetadata& Metadata;
    const vtkm::filter::PolicyBase<DerivedPolicy>& Policy;
    vtkm::filter::internal::RuntimeDeviceTracker& Tracker;
    ResultType& Result;

    ResolveFieldTypeAndExecute(Derived* derivedClass,
                               const vtkm::cont::DataSet &inputData,
                               const vtkm::filter::FieldMetadata& fieldMeta,
                               const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                               vtkm::filter::internal::RuntimeDeviceTracker& tracker,
                               ResultType& result):
      DerivedClass(derivedClass),
      InputData(inputData),
      Metadata(fieldMeta),
      Policy(policy),
      Tracker(tracker),
      Result(result)
      {

      }

    template<typename T, typename StorageTag>
    void operator()(const vtkm::cont::ArrayHandle<T,StorageTag>& field) const
    {
      typedef vtkm::cont::DeviceAdapterTagCuda CudaTag;
      typedef vtkm::cont::DeviceAdapterTagTBB TBBTag;
      typedef vtkm::cont::DeviceAdapterTagSerial SerialTag;

      {
        Result = run_if_valid<ResultType>( this->DerivedClass,
                                           this->InputData,
                                           field,
                                           this->Metadata,
                                           this->Policy,
                                           this->Tracker,
                                           CudaTag() );
      }

      if( !Result.IsValid() )
      {
        Result = run_if_valid<ResultType>( this->DerivedClass,
                                           this->InputData,
                                           field,
                                           this->Metadata,
                                           this->Policy,
                                           this->Tracker,
                                           TBBTag() );
      }
      if( !Result.IsValid() )
      {
        Result = run_if_valid<ResultType>( this->DerivedClass,
                                           this->InputData,
                                           field,
                                           this->Metadata,
                                           this->Policy,
                                           this->Tracker,
                                           SerialTag() );
      }
    }
  };
}

}
}
}  // namespace vtkm::filter::internal

#endif //vtk_m_filter_internal_ResolveFieldTypeAndExecute_h
