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
#ifndef vtk_m_cont_arg_TypeCheckTagArray_h
#define vtk_m_cont_arg_TypeCheckTagArray_h

#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/ListTag.h>

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
