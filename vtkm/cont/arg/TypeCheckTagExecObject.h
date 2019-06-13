//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TypeCheckTagExecObject_h
#define vtk_m_cont_arg_TypeCheckTagExecObject_h

#include <vtkm/internal/ExportMacros.h>

#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/cont/ExecutionObjectBase.h>

#include <type_traits>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// The ExecObject type check passes for any object that inherits from \c
/// ExecutionObjectBase. This is supposed to signify that the object can be
/// used in the execution environment although there is no way to verify that.
///
struct TypeCheckTagExecObject
{
};

template <typename Type>
struct TypeCheck<TypeCheckTagExecObject, Type>
{
  static constexpr bool value = vtkm::cont::internal::IsExecutionObjectBase<Type>::value;
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheckTagExecObject_h
