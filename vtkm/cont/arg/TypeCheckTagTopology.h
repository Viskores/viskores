//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_arg_TypeCheckTagTopology_h
#define vtk_m_cont_arg_TypeCheckTagTopology_h

#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/ListTag.h>

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm {
namespace cont {
namespace arg {

/// Check for a topology-like object.
///
struct TypeCheckTagTopology
{
};

template<typename ArrayType>
struct TypeCheck<TypeCheckTagTopology, ArrayType>
{
  static const bool value = true;
};

}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheckTagTopology_h
