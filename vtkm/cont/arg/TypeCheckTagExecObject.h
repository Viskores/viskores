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
  static constexpr bool value = std::is_base_of<vtkm::cont::ExecutionObjectBase, Type>::value;
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheckTagExecObject_h
