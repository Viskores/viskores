//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TypeCheckTagKeys_h
#define vtk_m_cont_arg_TypeCheckTagKeys_h

#include <vtkm/cont/arg/TypeCheck.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// Check for a Keys object.
///
struct TypeCheckTagKeys
{
};

// A more specific specialization that actually checks for Keys types is
// implemented in vtkm/worklet/Keys.h. That class is not accessible from here
// due to VTK-m package dependencies.
template <typename Type>
struct TypeCheck<TypeCheckTagKeys, Type>
{
  static constexpr bool value = false;
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheckTagKeys_h
