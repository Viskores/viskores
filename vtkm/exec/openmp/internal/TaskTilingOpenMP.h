//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
