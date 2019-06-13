//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

  /// Launch the worklet that is provided as the first parameter.
  /// Optional second parameter is the scatter type associated with the worklet.
  /// Any additional parameters are the ControlSignature arguments for the worklet.
  ///
  template <typename Worklet,
            typename T,
            typename... Args,
            typename std::enable_if<
              std::is_base_of<internal::ScatterBase, internal::detail::remove_cvref<T>>::value,
              int>::type* = nullptr>
  inline void operator()(Worklet&& worklet, T&& scatter, Args&&... args) const
  {
    using WorkletType = internal::detail::remove_cvref<Worklet>;
    using DispatcherType = typename WorkletType::template Dispatcher<WorkletType>;

    DispatcherType dispatcher(worklet, scatter);
    dispatcher.SetDevice(this->DeviceId);
    dispatcher.Invoke(std::forward<Args>(args)...);
  }

  /// Launch the worklet that is provided as the first parameter.
  /// Optional second parameter is the scatter type associated with the worklet.
  /// Any additional parameters are the ControlSignature arguments for the worklet.
  ///
  template <typename Worklet,
            typename T,
            typename... Args,
            typename std::enable_if<
              !std::is_base_of<internal::ScatterBase, internal::detail::remove_cvref<T>>::value,
              int>::type* = nullptr>
  inline void operator()(Worklet&& worklet, T&& t, Args&&... args) const
  {
    using WorkletType = internal::detail::remove_cvref<Worklet>;
    using DispatcherType = typename WorkletType::template Dispatcher<WorkletType>;

    DispatcherType dispatcher(worklet);
    dispatcher.SetDevice(this->DeviceId);
    dispatcher.Invoke(std::forward<T>(t), std::forward<Args>(args)...);
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
