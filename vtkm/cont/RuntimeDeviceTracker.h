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
  VTKM_CONT bool CanRunOn(DeviceAdapterId deviceId) const;

  /// Report a failure to allocate memory on a device, this will flag the
  /// device as being unusable for all future invocations.
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
  /// caused by reported failures. Passing DeviceAdapterTagAny to this will
  /// reset all devices ( same as \c Reset ).
  ///
  VTKM_CONT void ResetDevice(vtkm::cont::DeviceAdapterId deviceId);

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
  /// Passing DeviceAdapterTagAny to this will disable all devices
  ///
  VTKM_CONT void DisableDevice(DeviceAdapterId deviceId);

  /// \brief Disable all devices except the specified one.
  ///
  /// The main intention of \c RuntimeDeviceTracker is to keep track of what
  /// devices are working for VTK-m. However, it can also be used to turn
  /// devices on and off. Use this method to disable all devices except one
  /// to effectively force VTK-m to use that device. Either pass the
  /// DeviceAdapterTagAny to this function or call \c Reset to restore
  /// all devices to their default state.
  ///
  /// This method will throw a \c ErrorBadValue if the given device does not
  /// exist on the system.
  ///
  VTKM_CONT void ForceDevice(DeviceAdapterId deviceId);

  VTKM_CONT void PrintSummary(std::ostream& out) const;

private:
  friend struct ScopedRuntimeDeviceTracker;

  detail::RuntimeDeviceTrackerInternals* Internals;

  VTKM_CONT
  RuntimeDeviceTracker(detail::RuntimeDeviceTrackerInternals* details, bool reset);

  VTKM_CONT
  RuntimeDeviceTracker(const RuntimeDeviceTracker&) = delete;

  VTKM_CONT
  RuntimeDeviceTracker& operator=(const RuntimeDeviceTracker&) = delete;

  VTKM_CONT
  void CheckDevice(vtkm::cont::DeviceAdapterId deviceId) const;

  VTKM_CONT
  void SetDeviceState(vtkm::cont::DeviceAdapterId deviceId, bool state);

  VTKM_CONT
  void LogEnabledDevices() const;
};


enum struct RuntimeDeviceTrackerMode
{
  Force,
  Enable,
  Disable
};

/// A class that can be used to determine or modify which device adapter
/// VTK-m algorithms should be run on. This class captures the state
/// of the per-thread device adapter and will revert any changes applied
/// during its lifetime on destruction.
///
///
struct VTKM_CONT_EXPORT ScopedRuntimeDeviceTracker : public vtkm::cont::RuntimeDeviceTracker
{
  /// Construct a ScopedRuntimeDeviceTracker where the state of the active devices
  /// for the current thread are determined by the parameters to the constructor.
  ///
  /// 'Force'
  ///   - Force-Enable the provided single device adapter
  ///   - Force-Enable all device adapters when using vtkm::cont::DeviceAdaterTagAny
  /// 'Enable'
  ///   - Enable the provided single device adapter if it was previously disabled
  ///   - Enable all device adapters that are currently disabled when using
  ///     vtkm::cont::DeviceAdaterTagAny
  /// 'Disable'
  ///   - Disable the provided single device adapter
  ///   - Disable all device adapters when using vtkm::cont::DeviceAdaterTagAny
  ///
  /// Constructor is not thread safe
  VTKM_CONT ScopedRuntimeDeviceTracker(
    vtkm::cont::DeviceAdapterId device,
    RuntimeDeviceTrackerMode mode = RuntimeDeviceTrackerMode::Force);

  /// Construct a ScopedRuntimeDeviceTracker associated with the thread
  /// associated with the provided tracker. The active devices
  /// for the current thread are determined by the parameters to the constructor.
  ///
  /// 'Force'
  ///   - Force-Enable the provided single device adapter
  ///   - Force-Enable all device adapters when using vtkm::cont::DeviceAdaterTagAny
  /// 'Enable'
  ///   - Enable the provided single device adapter if it was previously disabled
  ///   - Enable all device adapters that are currently disabled when using
  ///     vtkm::cont::DeviceAdaterTagAny
  /// 'Disable'
  ///   - Disable the provided single device adapter
  ///   - Disable all device adapters when using vtkm::cont::DeviceAdaterTagAny
  ///
  /// Any modifications to the ScopedRuntimeDeviceTracker will effect what
  /// ever thread the \c tracker is associated with, which might not be
  /// the thread which ScopedRuntimeDeviceTracker was constructed on.
  ///
  /// Constructor is not thread safe
  VTKM_CONT ScopedRuntimeDeviceTracker(vtkm::cont::DeviceAdapterId device,
                                       RuntimeDeviceTrackerMode mode,
                                       const vtkm::cont::RuntimeDeviceTracker& tracker);

  /// Construct a ScopedRuntimeDeviceTracker associated with the thread
  /// associated with the provided tracker.
  ///
  /// Any modifications to the ScopedRuntimeDeviceTracker will effect what
  /// ever thread the \c tracker is associated with, which might not be
  /// the thread which ScopedRuntimeDeviceTracker was constructed on.
  ///
  /// Constructor is not thread safe
  VTKM_CONT ScopedRuntimeDeviceTracker(const vtkm::cont::RuntimeDeviceTracker& tracker);

  /// Destructor is not thread safe
  VTKM_CONT ~ScopedRuntimeDeviceTracker();

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
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker();
}
} // namespace vtkm::cont

#endif //vtk_m_filter_RuntimeDeviceTracker_h
