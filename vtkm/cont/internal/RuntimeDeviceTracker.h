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
#ifndef vtk_m_cont_internal_RuntimeDeviceTracker_h
#define vtk_m_cont_internal_RuntimeDeviceTracker_h

#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include <cstring>

namespace vtkm {
namespace cont {
namespace internal {

/// A class that can be used to determine if a given device adapter
/// is supported on the current machine at runtime. This is a more
/// complex version of vtkm::cont::RunimeDeviceInformation, as this can
/// also track when worklets fail, why the fail, and will update the list
/// of valid runtime devices based on that information.
///
///
class RuntimeDeviceTracker
{
public:
  VTKM_CONT
  RuntimeDeviceTracker()
  {
    this->Reset();
  }

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  template<typename DeviceAdapterTag>
  VTKM_CONT
  bool CanRunOn(DeviceAdapterTag) const
  {
    typedef vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag> Traits;
    return this->RuntimeValid[ Traits::GetId() ];
  }

  ///Report a failure to allocate memory on a device, this will flag the device
  ///as being unusable for all future invocations of the instance of the filter.
  ///
  template<typename DeviceAdapterTag>
  VTKM_CONT
  void ReportAllocationFailure(DeviceAdapterTag,
                               const vtkm::cont::ErrorBadAllocation&)
  {
    typedef vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag> Traits;
    this->RuntimeValid[ Traits::GetId() ] = false;
  }

  ///Reset the tracker to its default state.
  /// Will discard any updates caused by reported failures.
  ///
  VTKM_CONT
  void Reset()
  {
    std::memset(this->RuntimeValid, 0, sizeof(bool)*8 );

    //for each device determine the current runtime status at mark it
    //self in the validity array
    {
    typedef vtkm::cont::DeviceAdapterTagCuda CudaTag;
    typedef vtkm::cont::DeviceAdapterTraits<CudaTag> CudaTraits;

    vtkm::cont::RuntimeDeviceInformation<CudaTag> runtimeDevice;
    this->RuntimeValid[ CudaTraits::GetId() ] = runtimeDevice.Exists();
    }

    {
    typedef vtkm::cont::DeviceAdapterTagTBB TBBTag;
    typedef vtkm::cont::DeviceAdapterTraits<TBBTag> TBBTraits;

    vtkm::cont::RuntimeDeviceInformation<TBBTag> runtimeDevice;
    this->RuntimeValid[ TBBTraits::GetId() ] = runtimeDevice.Exists();
    }

    {
    typedef vtkm::cont::DeviceAdapterTagSerial SerialTag;
    typedef vtkm::cont::DeviceAdapterTraits<SerialTag> SerialTraits;

    vtkm::cont::RuntimeDeviceInformation<SerialTag> runtimeDevice;
    this->RuntimeValid[ SerialTraits::GetId() ] = runtimeDevice.Exists();
    }
  }



private:
  //make the array size 8 so the sizeof the class doesn't change when
  //we add more device adapters.
  bool RuntimeValid[8];
};

}
}
}  // namespace vtkm::cont::internal

#endif //vtk_m_filter_internal_RuntimeDeviceTracker_h
