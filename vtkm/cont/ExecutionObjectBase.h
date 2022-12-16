//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ExecutionObjectBase_h
#define vtk_m_cont_ExecutionObjectBase_h

#include <vtkm/Types.h>

#include <vtkm/cont/Token.h>

#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

namespace vtkm
{
namespace cont
{

/// Base `ExecutionObjectBase` for execution objects to inherit from so that
/// you can use an arbitrary object as a parameter in an execution environment
/// function. Any subclass of `ExecutionObjectBase` must implement a
/// `PrepareForExecution` method that takes a device adapter tag and a
/// `vtkm::cont::Token` and then returns an object for that device. The object
/// must be valid as long as the `Token` is in scope.
///
struct ExecutionObjectBase
{
};

namespace internal
{

namespace detail
{

struct CheckPrepareForExecution
{
  template <typename T>
  static auto check(T* p) -> decltype(p->PrepareForExecution(vtkm::cont::DeviceAdapterTagSerial{},
                                                             std::declval<vtkm::cont::Token&>()),
                                      std::true_type());

  template <typename T>
  static auto check(...) -> std::false_type;
};

} // namespace detail

template <typename T>
using IsExecutionObjectBase =
  typename std::is_base_of<vtkm::cont::ExecutionObjectBase, typename std::decay<T>::type>::type;

template <typename T>
struct HasPrepareForExecution
  : decltype(detail::CheckPrepareForExecution::check<typename std::decay<T>::type>(nullptr))
{
};

/// Checks that the argument is a proper execution object.
///
#define VTKM_IS_EXECUTION_OBJECT(execObject)                                            \
  static_assert(::vtkm::cont::internal::IsExecutionObjectBase<execObject>::value,       \
                "Provided type is not a subclass of vtkm::cont::ExecutionObjectBase."); \
  static_assert(::vtkm::cont::internal::HasPrepareForExecution<execObject>::value,      \
                "Provided type does not have requisite PrepareForExecution method.")

///@{
/// \brief Gets the object to use in the execution environment from an ExecutionObject.
///
/// An execution object (that is, an object inheriting from `vtkm::cont::ExecutionObjectBase`) is
/// really a control object factory that generates an object to be used in the execution
/// environment for a particular device. This function takes a subclass of `ExecutionObjectBase`
/// and returns the execution object for a given device.
///
template <typename T, typename Device>
VTKM_CONT auto CallPrepareForExecution(T&& execObject, Device device, vtkm::cont::Token& token)
  -> decltype(execObject.PrepareForExecution(device, token))
{
  VTKM_IS_EXECUTION_OBJECT(T);
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  return execObject.PrepareForExecution(device, token);
}

template <typename T>
VTKM_CONT auto CallPrepareForExecution(T&& execObject,
                                       vtkm::cont::DeviceAdapterId device,
                                       vtkm::cont::Token& token)
  -> decltype(execObject.PrepareForExecution(device, token))
{
  VTKM_IS_EXECUTION_OBJECT(T);

  return execObject.PrepareForExecution(device, token);
}
///@}

/// \brief Gets the type of the execution-side object for an ExecutionObject.
///
/// An execution object (that is, an object inheriting from `vtkm::cont::ExecutionObjectBase`) is
/// really a control object factory that generates an object to be used in the execution
/// environment for a particular device. This templated type gives the type for the class used
/// in the execution environment for a given ExecutionObject and device.
///
template <typename ExecutionObject, typename Device = vtkm::cont::DeviceAdapterId>
using ExecutionObjectType = decltype(CallPrepareForExecution(std::declval<ExecutionObject>(),
                                                             std::declval<Device>(),
                                                             std::declval<vtkm::cont::Token&>()));

} // namespace internal
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ExecutionObjectBase_h
