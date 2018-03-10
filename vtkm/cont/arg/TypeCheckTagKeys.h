//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
