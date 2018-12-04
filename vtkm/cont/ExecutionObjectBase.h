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
#ifndef vtk_m_cont_ExecutionObjectBase_h
#define vtk_m_cont_ExecutionObjectBase_h

#include <vtkm/Types.h>

#include <vtkm/cont/DeviceAdapter.h>

#if ((VTKM_DEVICE_ADAPTER > 0) && (VTKM_DEVICE_ADAPTER < VTKM_MAX_DEVICE_ADAPTER_ID))
// Use the default device adapter tag for testing whether execution objects are valid.
#define VTK_M_DEVICE_ADAPTER_TO_TEST_EXEC_OBJECT VTKM_DEFAULT_DEVICE_ADAPTER_TAG
#else
// The default device adapter is invalid. Perhaps the error device adapter is being used.
// In this case, try the serial device adapter instead. It should always be valid.
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#define VTK_M_DEVICE_ADAPTER_TO_TEST_EXEC_OBJECT ::vtkm::cont::DeviceAdapterTagSerial
#endif

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
    -> decltype(p->PrepareForExecution(VTK_M_DEVICE_ADAPTER_TO_TEST_EXEC_OBJECT()),
                std::true_type());

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
