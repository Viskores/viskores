//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_RuntimeDeviceTracker_h
#define vtk_m_cont_RuntimeDeviceTracker_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/DeviceAdapterTag.h>
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
struct ScopedRuntimeDeviceTracker;

/// RuntimeDeviceTracker is the central location for determining
/// which device adapter will be active for algorithm execution.
/// Many features in VTK-m will attempt to run algorithms on the "best
/// available device." This generally is determined at runtime as some
/// backends require specific hardware, or failures in one device are
/// recorded and that device is disabled.
///
/// While vtkm::cont::RunimeDeviceInformation reports on the existence
/// of a device being supported, this tracks on a per-thread basis
/// when worklets fail, why the fail, and will update the list
/// of valid runtime devices based on that information.
///
///
class VTKM_CONT_EXPORT RuntimeDeviceTracker
{
  friend VTKM_CONT_EXPORT vtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker();

public:
  VTKM_CONT
  ~RuntimeDeviceTracker();

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  VTKM_CONT bool CanRunOn(DeviceAdapterId device) const { return this->CanRunOnImpl(device); }

  /// Report a failure to allocate memory on a device, this will flag the
  /// device as being unusable for all future invocations of the instance of
  /// the filter.
  ///
  VTKM_CONT void ReportAllocationFailure(vtkm::cont::DeviceAdapterId deviceId,
                                         const vtkm::cont::ErrorBadAllocation&)
  {
    this->SetDeviceState(deviceId, false);
  }


  /// Report a ErrorBadDevice failure and flag the device as unusable.
  VTKM_CONT void ReportBadDeviceFailure(vtkm::cont::DeviceAdapterId deviceId,
                                        const vtkm::cont::ErrorBadDevice&)
  {
    this->SetDeviceState(deviceId, false);
  }

  /// Reset the tracker for the given device. This will discard any updates
  /// caused by reported failures
  ///
  VTKM_CONT void ResetDevice(vtkm::cont::DeviceAdapterId device)
  {
    vtkm::cont::RuntimeDeviceInformation runtimeDevice;
    this->SetDeviceState(device, runtimeDevice.Exists(device));
  }

  /// Reset the tracker to its default state for default devices.
  /// Will discard any updates caused by reported failures.
  ///
  VTKM_CONT
  void Reset();

  /// \brief Disable the given device
  ///
  /// The main intention of \c RuntimeDeviceTracker is to keep track of what
  /// devices are working for VTK-m. However, it can also be used to turn
  /// devices on and off. Use this method to disable (turn off) a given device.
  /// Use \c ResetDevice to turn the device back on (if it is supported).
  ///
  VTKM_CONT void DisableDevice(DeviceAdapterId device) { this->SetDeviceState(device, false); }

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
  VTKM_CONT void ForceDevice(DeviceAdapterId id);

private:
  friend struct ScopedRuntimeDeviceTracker;

  std::shared_ptr<detail::RuntimeDeviceTrackerInternals> Internals;

  VTKM_CONT
  RuntimeDeviceTracker();

  VTKM_CONT
  void CheckDevice(vtkm::cont::DeviceAdapterId deviceId) const;

  VTKM_CONT
  bool CanRunOnImpl(vtkm::cont::DeviceAdapterId deviceId) const;

  VTKM_CONT
  void SetDeviceState(vtkm::cont::DeviceAdapterId deviceId, bool state);

  VTKM_CONT
  void ForceDeviceImpl(vtkm::cont::DeviceAdapterId deviceId, bool runtimeExists);
};

/// A class that can be used to determine or modify which device adapter
/// VTK-m algorithms should be run on. This class captures the state
/// of the per-thread device adapter and will revert any changes applied
/// during its lifetime on destruction.
///
///
struct VTKM_CONT_EXPORT ScopedRuntimeDeviceTracker : public vtkm::cont::RuntimeDeviceTracker
{
  /// Constructor is not thread safe
  VTKM_CONT ScopedRuntimeDeviceTracker();

  /// Constructor is not thread safe
  VTKM_CONT ScopedRuntimeDeviceTracker(const vtkm::cont::RuntimeDeviceTracker& tracker);

  /// Destructor is not thread safe
  VTKM_CONT ~ScopedRuntimeDeviceTracker();

  ScopedRuntimeDeviceTracker(const ScopedRuntimeDeviceTracker&) = delete;

private:
  std::unique_ptr<detail::RuntimeDeviceTrackerInternals> SavedState;
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
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker();
}
} // namespace vtkm::cont

#endif //vtk_m_filter_RuntimeDeviceTracker_h
