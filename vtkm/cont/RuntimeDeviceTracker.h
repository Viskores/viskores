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
#ifndef vtk_m_cont_RuntimeDeviceTracker_h
#define vtk_m_cont_RuntimeDeviceTracker_h

#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>

namespace vtkm {
namespace cont {

namespace detail {

struct RuntimeDeviceTrackerInternals;

}

/// A class that can be used to determine if a given device adapter
/// is supported on the current machine at runtime. This is a more
/// complex version of vtkm::cont::RunimeDeviceInformation, as this can
/// also track when worklets fail, why the fail, and will update the list
/// of valid runtime devices based on that information.
///
///
class VTKM_ALWAYS_EXPORT RuntimeDeviceTracker
{
public:
  VTKM_CONT_EXPORT
  VTKM_CONT
  RuntimeDeviceTracker();

  VTKM_CONT_EXPORT
  VTKM_CONT
  ~RuntimeDeviceTracker();

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  template<typename DeviceAdapterTag>
  VTKM_CONT
  bool CanRunOn(DeviceAdapterTag) const
  {
    using Traits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    return this->CanRunOnImpl(Traits::GetId(), Traits::GetName());
  }

  /// Report a failure to allocate memory on a device, this will flag the
  /// device as being unusable for all future invocations of the instance of
  /// the filter.
  ///
  template<typename DeviceAdapterTag>
  VTKM_CONT
  void ReportAllocationFailure(DeviceAdapterTag,
                               const vtkm::cont::ErrorBadAllocation&)
  {
    using Traits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    this->SetDeviceState(Traits::GetId(), Traits::GetName(), false);
  }

  /// Reset the tracker for the given device. This will discard any updates
  /// caused by reported failures
  ///
  template<typename DeviceAdapterTag>
  VTKM_CONT
  void ResetDevice(DeviceAdapterTag)
  {
    using Traits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    vtkm::cont::RuntimeDeviceInformation<DeviceAdapterTag> runtimeDevice;
    this->SetDeviceState(Traits::GetId(),
                         Traits::GetName(),
                         runtimeDevice.Exists());
  }

  /// Reset the tracker to its default state for default devices.
  /// Will discard any updates caused by reported failures.
  ///
  VTKM_CONT_EXPORT
  VTKM_CONT
  void Reset();


private:
  std::shared_ptr<detail::RuntimeDeviceTrackerInternals> Internals;

  VTKM_CONT_EXPORT
  VTKM_CONT
  void CheckDevice(vtkm::cont::DeviceAdapterId deviceId,
                   const vtkm::cont::DeviceAdapterNameType &deviceName) const;

  VTKM_CONT_EXPORT
  VTKM_CONT
  bool CanRunOnImpl(vtkm::cont::DeviceAdapterId deviceId,
                    const vtkm::cont::DeviceAdapterNameType &deviceName) const;

  VTKM_CONT_EXPORT
  VTKM_CONT
  void SetDeviceState(vtkm::cont::DeviceAdapterId deviceId,
                      const vtkm::cont::DeviceAdapterNameType &deviceName,
                      bool state);
};

}
}  // namespace vtkm::cont

#endif //vtk_m_filter_RuntimeDeviceTracker_h
