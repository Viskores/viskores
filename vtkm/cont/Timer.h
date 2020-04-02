//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_Timer_h
#define vtk_m_cont_Timer_h

#include <vtkm/List.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterList.h>

#include <vtkm/cont/vtkm_cont_export.h>

#include <memory>

namespace vtkm
{
namespace cont
{
namespace detail
{
struct EnabledDeviceTimerImpls;
}

/// A class that can be used to time operations in VTK-m that might be occuring
/// in parallel. Users are recommended to provide a device adapter at construction
/// time which matches the one being used to execute algorithms to ensure that thread
/// synchronization is correct and accurate.
/// If no device adapter is provided at construction time, the maximum
/// elapsed time of all enabled deivces will be returned. Normally cuda is expected to
/// have the longest execution time if enabled.
/// Per device adapter time query is also supported. It's useful when users want to reuse
/// the same timer to measure the cuda kernal call as well as the cuda device execution.
/// It is also possible to change the device adapter after construction by calling the form
/// of the Reset method with a new DeviceAdapterId.
///
/// The there is no guaranteed resolution of the time but should generally be
/// good to about a millisecond.
///
class VTKM_CONT_EXPORT Timer
{
public:
  VTKM_CONT
  Timer();

  VTKM_CONT Timer(vtkm::cont::DeviceAdapterId device);

  VTKM_CONT ~Timer();

  /// Resets the timer.
  VTKM_CONT void Reset();

  /// Resets the timer and changes the device to time on.
  VTKM_CONT void Reset(vtkm::cont::DeviceAdapterId device);

  VTKM_CONT void Start();

  VTKM_CONT void Stop();

  VTKM_CONT bool Started() const;

  VTKM_CONT bool Stopped() const;

  /// Used to check if Timer has finished the synchronization to get the result from the device.
  VTKM_CONT bool Ready() const;

  /// Get the elapsed time measured by the given device adapter. If no device is
  /// specified, the max time of all device measurements will be returned.
  VTKM_CONT
  vtkm::Float64 GetElapsedTime() const;

  /// Returns the device for which this timer is synchronized. If the device adapter has the same
  /// id as DeviceAdapterTagAny, then the timer will synchronize all devices.
  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const { return this->Device; }

private:
  /// Some timers are ill-defined when copied, so disallow that for all timers.
  VTKM_CONT Timer(const Timer&) = delete;
  VTKM_CONT void operator=(const Timer&) = delete;

  DeviceAdapterId Device;
  std::unique_ptr<detail::EnabledDeviceTimerImpls> Internal;
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_Timer_h
