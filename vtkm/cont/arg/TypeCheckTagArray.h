//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TypeCheckTagArray_h
#define vtk_m_cont_arg_TypeCheckTagArray_h

#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/List.h>

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// The Array type check passes for any object that behaves like an \c
/// ArrayHandle class and can be passed to the ArrayIn and ArrayOut transports.
///
struct TypeCheckTagArray
{
};

template <typename ArrayType>
struct TypeCheck<TypeCheckTagArray, ArrayType>
{
  static constexpr bool value = vtkm::cont::internal::ArrayHandleCheck<ArrayType>::type::value;
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheckTagArray_h
