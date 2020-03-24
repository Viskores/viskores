//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TypeCheckTagArrayIn_h
#define vtk_m_cont_arg_TypeCheckTagArrayIn_h

#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/List.h>

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/internal/ArrayPortalHelpers.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// The Array type check passes for any object that behaves like an \c
/// ArrayHandle class and can be passed to the ArrayIn transport.
///
struct TypeCheckTagArrayIn
{
};

namespace detail
{

template <typename ArrayType,
          bool IsArrayHandle = vtkm::cont::internal::ArrayHandleCheck<ArrayType>::type::value>
struct IsArrayHandleIn;

template <typename ArrayType>
struct IsArrayHandleIn<ArrayType, true>
{
  static constexpr bool value =
    vtkm::internal::PortalSupportsGets<typename ArrayType::ReadPortalType>::value;
};

template <typename ArrayType>
struct IsArrayHandleIn<ArrayType, false>
{
  static constexpr bool value = false;
};

} // namespace detail

template <typename ArrayType>
struct TypeCheck<TypeCheckTagArrayIn, ArrayType>
{
  static constexpr bool value = detail::IsArrayHandleIn<ArrayType>::value;
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheckTagArray_h
