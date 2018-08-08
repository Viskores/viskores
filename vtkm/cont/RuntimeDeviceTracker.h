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
#ifndef vtk_m_cont_RuntimeDeviceTracker_h
#define vtk_m_cont_RuntimeDeviceTracker_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorBadDevice.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>

#include <memory>

namespace vtkm
{
namespace cont
{

namespace detail
{

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
  template <typename DeviceAdapterTag>
  VTKM_CONT bool CanRunOn(DeviceAdapterTag device) const
  {
    using Traits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    return this->CanRunOnImpl(device, Traits::GetName());
  }

  /// Report a failure to allocate memory on a device, this will flag the
  /// device as being unusable for all future invocations of the instance of
  /// the filter.
  ///
  template <typename DeviceAdapterTag>
  VTKM_CONT void ReportAllocationFailure(DeviceAdapterTag device,
                                         const vtkm::cont::ErrorBadAllocation&)
  {
    using Traits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    this->SetDeviceState(device, Traits::GetName(), false);
  }

  /// Report a failure to allocate memory on a device, this will flag the
  /// device as being unusable for all future invocations of the instance of
  /// the filter.
  ///
  VTKM_CONT void ReportAllocationFailure(vtkm::cont::DeviceAdapterId deviceId,
                                         const std::string& name,
                                         const vtkm::cont::ErrorBadAllocation&)
  {
    this->SetDeviceState(deviceId, name, false);
  }

  //@{
  /// Report a ErrorBadDevice failure and flag the device as unusable.
  template <typename DeviceAdapterTag>
  VTKM_CONT void ReportBadDeviceFailure(DeviceAdapterTag device, const vtkm::cont::ErrorBadDevice&)
  {
    using Traits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    this->SetDeviceState(device, Traits::GetName(), false);
  }

  VTKM_CONT void ReportBadDeviceFailure(vtkm::cont::DeviceAdapterId deviceId,
                                        const std::string& name,
                                        const vtkm::cont::ErrorBadDevice&)
  {
    this->SetDeviceState(deviceId, name, false);
  }
  //@}

  /// Reset the tracker for the given device. This will discard any updates
  /// caused by reported failures
  ///
  template <typename DeviceAdapterTag>
  VTKM_CONT void ResetDevice(DeviceAdapterTag device)
  {
    using Traits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    vtkm::cont::RuntimeDeviceInformation<DeviceAdapterTag> runtimeDevice;
    this->SetDeviceState(device, Traits::GetName(), runtimeDevice.Exists());
  }

  /// Reset the tracker to its default state for default devices.
  /// Will discard any updates caused by reported failures.
  ///
  VTKM_CONT_EXPORT
  VTKM_CONT
  void Reset();

  /// \brief Perform a deep copy of the \c RuntimeDeviceTracker state.
  ///
  /// Normally when you assign or copy a \c RuntimeDeviceTracker, they share
  /// state so that when you change the state of one (for example, find a
  /// device that does not work), the other is also implicitly updated. This
  /// important so that when you use the global runtime device tracker the
  /// state is synchronized across all the units using it.
  ///
  /// If you want a \c RuntimeDeviceTracker with independent state, just create
  /// one independently. If you want to start with the state of a source
  /// \c RuntimeDeviceTracker but update the state independently, you can use
  /// \c DeepCopy method to get the initial state. Further changes will
  /// not be shared.
  ///
  /// This version of \c DeepCopy creates a whole new \c RuntimeDeviceTracker
  /// with a state that is not shared with any other object.
  ///
  VTKM_CONT_EXPORT
  VTKM_CONT
  vtkm::cont::RuntimeDeviceTracker DeepCopy() const;

  /// \brief Perform a deep copy of the \c RuntimeDeviceTracker state.
  ///
  /// Normally when you assign or copy a \c RuntimeDeviceTracker, they share
  /// state so that when you change the state of one (for example, find a
  /// device that does not work), the other is also implicitly updated. This
  /// important so that when you use the global runtime device tracker the
  /// state is synchronized across all the units using it.
  ///
  /// If you want a \c RuntimeDeviceTracker with independent state, just create
  /// one independently. If you want to start with the state of a source
  /// \c RuntimeDeviceTracker but update the state independently, you can use
  /// \c DeepCopy method to get the initial state. Further changes will
  /// not be shared.
  ///
  /// This version of \c DeepCopy sets the state of the current object to
  /// the one given in the argument. Any other \c RuntimeDeviceTrackers sharing
  /// state with this object will also get updated. This method is good for
  /// restoring a state that was previously saved.
  ///
  VTKM_CONT_EXPORT
  VTKM_CONT
  void DeepCopy(const vtkm::cont::RuntimeDeviceTracker& src);

  /// \brief Disable the given device
  ///
  /// The main intention of \c RuntimeDeviceTracker is to keep track of what
  /// devices are working for VTK-m. However, it can also be used to turn
  /// devices on and off. Use this method to disable (turn off) a given device.
  /// Use \c ResetDevice to turn the device back on (if it is supported).
  ///
  template <typename DeviceAdapterTag>
  VTKM_CONT void DisableDevice(DeviceAdapterTag device)
  {
    using Traits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    this->SetDeviceState(device, Traits::GetName(), false);
  }

  /// \brief Disable all devices except the specified one.
  ///
  /// The main intention of \c RuntimeDeviceTracker is to keep track of what
  /// devices are working for VTK-m. However, it can also be used to turn
  /// devices on and off. Use this method to disable all devices except one
  /// to effectively force VTK-m to use that device. Use \c Reset restore
  /// all devices to their default values. You can also use the \c DeepCopy
  /// methods to save and restore the state.
  ///
  /// This method will throw a \c ErrorBadValue if the given device does not
  /// exist on the system.
  ///
  template <typename DeviceAdapterTag>
  VTKM_CONT void ForceDevice(DeviceAdapterTag device)
  {
    using Traits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    vtkm::cont::RuntimeDeviceInformation<DeviceAdapterTag> runtimeDevice;
    this->ForceDeviceImpl(device, Traits::GetName(), runtimeDevice.Exists());
  }

private:
  std::shared_ptr<detail::RuntimeDeviceTrackerInternals> Internals;

  VTKM_CONT_EXPORT
  VTKM_CONT
  void CheckDevice(vtkm::cont::DeviceAdapterId deviceId,
                   const vtkm::cont::DeviceAdapterNameType& deviceName) const;

  VTKM_CONT_EXPORT
  VTKM_CONT
  bool CanRunOnImpl(vtkm::cont::DeviceAdapterId deviceId,
                    const vtkm::cont::DeviceAdapterNameType& deviceName) const;

  VTKM_CONT_EXPORT
  VTKM_CONT
  void SetDeviceState(vtkm::cont::DeviceAdapterId deviceId,
                      const vtkm::cont::DeviceAdapterNameType& deviceName,
                      bool state);

  VTKM_CONT_EXPORT
  VTKM_CONT
  void ForceDeviceImpl(vtkm::cont::DeviceAdapterId deviceId,
                       const vtkm::cont::DeviceAdapterNameType& deviceName,
                       bool runtimeExists);
};

/// \brief Get the \c RuntimeDeviceTracker for the current thread.
///
/// Many features in VTK-m will attempt to run algorithms on the "best
/// available device." This often is determined at runtime as failures in
/// one device are recorded and that device is disabled. To prevent having
/// to check over and over again, VTK-m uses per thread runtime device tracker
/// so that these choices are marked and shared.
///
/// Xcode's clang only supports thread_local from version 8
#if !(defined(VTKM_CLANG) && (__apple_build_version__ < 8000000))
thread_local static vtkm::cont::RuntimeDeviceTracker runtimeDeviceTracker;
#endif
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::RuntimeDeviceTracker GetGlobalRuntimeDeviceTracker();
}
} // namespace vtkm::cont

#endif //vtk_m_filter_RuntimeDeviceTracker_h
