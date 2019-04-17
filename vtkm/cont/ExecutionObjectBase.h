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

#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

namespace vtkm
{
namespace cont
{

/// Base \c ExecutionObjectBase for execution objects to inherit from so that
/// you can use an arbitrary object as a parameter in an execution environment
/// function. Any subclass of \c ExecutionObjectBase must implement a
/// \c PrepareForExecution method that takes a device adapter tag and returns
/// an object for that device.
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
  static auto check(T* p)
    -> decltype(p->PrepareForExecution(vtkm::cont::DeviceAdapterTagSerial{}), std::true_type());

  template <typename T>
  static auto check(...) -> std::false_type;
};

} // namespace detail

template <typename T>
using IsExecutionObjectBase =
  std::is_base_of<vtkm::cont::ExecutionObjectBase, typename std::decay<T>::type>;

template <typename T>
struct HasPrepareForExecution
  : decltype(detail::CheckPrepareForExecution::check<typename std::decay<T>::type>(nullptr))
{
};

} // namespace internal
}
} // namespace vtkm::cont

/// Checks that the argument is a proper execution object.
///
#define VTKM_IS_EXECUTION_OBJECT(execObject)                                                       \
  static_assert(::vtkm::cont::internal::IsExecutionObjectBase<execObject>::value,                  \
                "Provided type is not a subclass of vtkm::cont::ExecutionObjectBase.");            \
  static_assert(::vtkm::cont::internal::HasPrepareForExecution<execObject>::value,                 \
                "Provided type does not have requisite PrepareForExecution method.")

#endif //vtk_m_cont_ExecutionObjectBase_h
