//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_Invoker_h
#define vtk_m_worklet_Invoker_h

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/DispatcherPointNeighborhood.h>
#include <vtkm/worklet/DispatcherReduceByKey.h>

#include <vtkm/cont/TryExecute.h>

namespace vtkm
{
namespace worklet
{


/// \brief Allows launching any worklet without a dispatcher.
///
/// \c Invoker is a generalized \c Dispatcher that is able to automatically
/// determine how to properly launch/invoke any worklet that is passed to it.
/// When an \c Invoker is constructed it is provided the desired device adapter
/// that all worklets invoked by it should be launched on.
///
/// \c Invoker is designed to not only reduce the verbosity of constructing
/// multiple dispatchers inside a block of logic, but also makes it easier to
/// make sure all worklets execute on the same device.
struct Invoker
{

  /// Constructs an Invoker that will try to launch worklets on any device
  /// that is enabled.
  ///
  explicit Invoker()
    : DeviceId(vtkm::cont::DeviceAdapterTagAny{})
  {
  }

  /// Constructs an Invoker that will try to launch worklets only on the
  /// provided device adapter.
  ///
  explicit Invoker(vtkm::cont::DeviceAdapterId device)
    : DeviceId(device)
  {
  }

  /// Launch the worklet that is provided as the first parameter. The additional
  /// parameters are the ControlSignature arguments for the worklet.
  ///
  template <typename Worklet, typename... Args>
  inline void operator()(Worklet&& worklet, Args&&... args) const
  {
    using WorkletType = typename std::decay<Worklet>::type;
    using DispatcherType = typename WorkletType::template Dispatcher<WorkletType>;

    DispatcherType dispatcher(worklet);
    dispatcher.SetDevice(this->DeviceId);
    dispatcher.Invoke(std::forward<Args>(args)...);
  }

  /// Get the device adapter that this Invoker is bound too
  ///
  vtkm::cont::DeviceAdapterId GetDevice() const { return DeviceId; }

private:
  vtkm::cont::DeviceAdapterId DeviceId;
};
}
}

#endif
