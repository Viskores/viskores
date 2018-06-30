//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_openmp_internal_TaskTilingOpenMP_h
#define vtk_m_exec_openmp_internal_TaskTilingOpenMP_h

#include <vtkm/exec/serial/internal/TaskTiling.h>

namespace vtkm
{
namespace exec
{
namespace openmp
{
namespace internal
{

using TaskTiling1D = vtkm::exec::serial::internal::TaskTiling1D;
using TaskTiling3D = vtkm::exec::serial::internal::TaskTiling3D;
}
}
}
} // namespace vtkm::exec::tbb::internal

#endif //vtk_m_exec_tbb_internal_TaskTiling_h
