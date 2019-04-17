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

/// A class that can be used to determine if a given device adapter
/// is supported on the current machine at runtime. This is a more
/// complex version of vtkm::cont::RunimeDeviceInformation, as this can
/// also track when worklets fail, why the fail, and will update the list
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
  VTKM_CONT
  void DeepCopy(const vtkm::cont::RuntimeDeviceTracker& src);

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
  std::shared_ptr<detail::RuntimeDeviceTrackerInternals> Internals;

  VTKM_CONT
  RuntimeDeviceTracker();

  // Deep Copy constructor.
  VTKM_CONT
  RuntimeDeviceTracker(const std::shared_ptr<detail::RuntimeDeviceTrackerInternals>& internals);

  VTKM_CONT
  void CheckDevice(vtkm::cont::DeviceAdapterId deviceId) const;

  VTKM_CONT
  bool CanRunOnImpl(vtkm::cont::DeviceAdapterId deviceId) const;

  VTKM_CONT
  void SetDeviceState(vtkm::cont::DeviceAdapterId deviceId, bool state);

  VTKM_CONT
  void ForceDeviceImpl(vtkm::cont::DeviceAdapterId deviceId, bool runtimeExists);
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

struct ScopedRuntimeDeviceTracker
{
  vtkm::cont::RuntimeDeviceTracker SavedTracker;

  VTKM_CONT ScopedRuntimeDeviceTracker()
    : SavedTracker(vtkm::cont::GetRuntimeDeviceTracker().DeepCopy())
  {
  }

  VTKM_CONT ScopedRuntimeDeviceTracker(vtkm::cont::RuntimeDeviceTracker tracker)
    : SavedTracker(vtkm::cont::GetRuntimeDeviceTracker().DeepCopy())
  {
    vtkm::cont::GetRuntimeDeviceTracker().DeepCopy(tracker);
  }

  VTKM_CONT ~ScopedRuntimeDeviceTracker()
  {
    vtkm::cont::GetRuntimeDeviceTracker().DeepCopy(this->SavedTracker);
  }

  ScopedRuntimeDeviceTracker(const ScopedRuntimeDeviceTracker&) = delete;
};
}
} // namespace vtkm::cont

#endif //vtk_m_filter_RuntimeDeviceTracker_h
