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

#include <functional>
#include <memory>

namespace vtkm
{
namespace cont
{
namespace detail
{

struct RuntimeDeviceTrackerInternals;
}
class ScopedRuntimeDeviceTracker;

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
  /// reset all devices (same as `Reset()`).
  ///
  VTKM_CONT void ResetDevice(vtkm::cont::DeviceAdapterId deviceId);

  /// Reset the tracker to its default state for default devices.
  /// Will discard any updates caused by reported failures.
  ///
  VTKM_CONT
  void Reset();

  /// @brief Disable the given device.
  ///
  /// The main intention of `RuntimeDeviceTracker` is to keep track of what
  /// devices are working for VTK-m. However, it can also be used to turn
  /// devices on and off. Use this method to disable (turn off) a given device.
  /// Use `ResetDevice()` to turn the device back on (if it is supported).
  ///
  /// Passing DeviceAdapterTagAny to this will disable all devices.
  ///
  VTKM_CONT void DisableDevice(DeviceAdapterId deviceId);

  /// \brief Disable all devices except the specified one.
  ///
  /// The main intention of `RuntimeDeviceTracker` is to keep track of what
  /// devices are working for VTK-m. However, it can also be used to turn
  /// devices on and off. Use this method to disable all devices except one
  /// to effectively force VTK-m to use that device. Either pass the
  /// DeviceAdapterTagAny to this function or call `Reset()` to restore
  /// all devices to their default state.
  ///
  /// This method will throw a `vtkm::cont::ErrorBadValue` if the given device
  /// does not exist on the system.
  ///
  VTKM_CONT void ForceDevice(DeviceAdapterId deviceId);

  /// @brief Get/Set use of thread-friendly memory allocation for a device.
  ///
  ///
  VTKM_CONT bool GetThreadFriendlyMemAlloc() const;
  /// @copydoc GetThreadFriendlyMemAlloc
  VTKM_CONT void SetThreadFriendlyMemAlloc(bool state);

  /// @brief Copies the state from the given device.
  ///
  /// This is a convenient way to allow the `RuntimeDeviceTracker` on one thread
  /// copy the behavior from another thread.
  ///
  VTKM_CONT void CopyStateFrom(const vtkm::cont::RuntimeDeviceTracker& tracker);

  /// @brief Set/Clear the abort checker functor.
  ///
  /// If set the abort checker functor is called by `vtkm::cont::TryExecute()`
  /// before scheduling a task on a device from the associated the thread. If
  /// the functor returns `true`, an exception is thrown.
  VTKM_CONT void SetAbortChecker(const std::function<bool()>& func);
  /// @copydoc SetAbortChecker
  VTKM_CONT void ClearAbortChecker();

  VTKM_CONT bool CheckForAbortRequest() const;

  /// @brief Produce a human-readable report on the state of the runtime device tracker.
  VTKM_CONT void PrintSummary(std::ostream& out) const;

private:
  friend class ScopedRuntimeDeviceTracker;

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

///----------------------------------------------------------------------------
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

/// @brief Identifier used to specify whether to enable or disable a particular device.
enum struct RuntimeDeviceTrackerMode
{
  // Documentation is below (for better layout in generated documents).
  Force,
  Enable,
  Disable
};

/// @var RuntimeDeviceTrackerMode Force
/// @brief Replaces the current list of devices to try with the device specified.
///
/// This has the effect of forcing VTK-m to use the provided device.
/// This is the default behavior for `vtkm::cont::ScopedRuntimeDeviceTracker`.

/// @var RuntimeDeviceTrackerMode Enable
/// @brief Adds the provided device adapter to the list of devices to try.

/// @var RuntimeDeviceTrackerMode Disable
/// @brief Removes the provided device adapter from the list of devices to try.

//----------------------------------------------------------------------------
/// A class to create a scoped runtime device tracker object. This object captures the state
/// of the per-thread device tracker and will revert any changes applied
/// during its lifetime on destruction.
///
class VTKM_CONT_EXPORT ScopedRuntimeDeviceTracker : public vtkm::cont::RuntimeDeviceTracker
{
public:
  /// Construct a ScopedRuntimeDeviceTracker associated with the thread,
  /// associated with the provided tracker (defaults to current thread's tracker).
  ///
  /// Any modifications to the ScopedRuntimeDeviceTracker will effect what
  /// ever thread the \c tracker is associated with, which might not be
  /// the thread on which the ScopedRuntimeDeviceTracker was constructed.
  ///
  /// Constructors are not thread safe
  /// @{
  ///
  VTKM_CONT ScopedRuntimeDeviceTracker(
    const vtkm::cont::RuntimeDeviceTracker& tracker = GetRuntimeDeviceTracker());

  /// Use this constructor to modify the state of the device adapters associated with
  /// the provided tracker. Use \p mode with \p device as follows:
  ///
  /// 'Force' (default)
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
  VTKM_CONT ScopedRuntimeDeviceTracker(
    vtkm::cont::DeviceAdapterId device,
    RuntimeDeviceTrackerMode mode = RuntimeDeviceTrackerMode::Force,
    const vtkm::cont::RuntimeDeviceTracker& tracker = GetRuntimeDeviceTracker());

  /// Use this constructor to set the abort checker functor for the provided tracker.
  ///
  VTKM_CONT ScopedRuntimeDeviceTracker(
    const std::function<bool()>& abortChecker,
    const vtkm::cont::RuntimeDeviceTracker& tracker = GetRuntimeDeviceTracker());

  /// Destructor is not thread safe
  VTKM_CONT ~ScopedRuntimeDeviceTracker();

private:
  std::unique_ptr<detail::RuntimeDeviceTrackerInternals> SavedState;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_RuntimeDeviceTracker_h
