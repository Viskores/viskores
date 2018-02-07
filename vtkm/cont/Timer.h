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
#ifndef vtk_m_cont_Timer_h
#define vtk_m_cont_Timer_h

#include <vtkm/cont/DeviceAdapter.h>

namespace vtkm
{
namespace cont
{

/// A class that can be used to time operations in VTK-m that might be occurring
/// in parallel. You should make sure that the device adapter for the timer
/// matches that being used to execute algorithms to ensure that the thread
/// synchronization is correct.
///
/// The there is no guaranteed resolution of the time but should generally be
/// good to about a millisecond.
///
template <class Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class Timer
{
public:
  /// When a timer is constructed, all threads are synchronized and the
  /// current time is marked so that GetElapsedTime returns the number of
  /// seconds elapsed since the construction.
  VTKM_CONT
  Timer()
    : TimerImplementation()
  {
  }

  /// Resets the timer. All further calls to GetElapsedTime will report the
  /// number of seconds elapsed since the call to this. This method
  /// synchronizes all asynchronous operations.
  ///
  VTKM_CONT
  void Reset() { this->TimerImplementation.Reset(); }

  /// Returns the elapsed time in seconds between the construction of this
  /// class or the last call to Reset and the time this function is called. The
  /// time returned is measured in wall time. GetElapsedTime may be called any
  /// number of times to get the progressive time. This method synchronizes all
  /// asynchronous operations.
  ///
  VTKM_CONT
  vtkm::Float64 GetElapsedTime() { return this->TimerImplementation.GetElapsedTime(); }

private:
  /// Some timers are ill-defined when copied, so disallow that for all timers.
  VTKM_CONT Timer(const Timer<Device>&) = delete;
  VTKM_CONT void operator=(const Timer<Device>&) = delete;

  vtkm::cont::DeviceAdapterTimerImplementation<Device> TimerImplementation;
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_Timer_h
