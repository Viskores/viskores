//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_openmp_internal_ParallelRadixSortOpenMP_h
#define vtk_m_cont_openmp_internal_ParallelRadixSortOpenMP_h

#include <vtkm/cont/internal/ParallelRadixSortInterface.h>

namespace vtkm
{
namespace cont
{
namespace openmp
{
namespace sort
{
namespace radix
{

VTKM_DECLARE_RADIX_SORT()
}
}
}
}
} // end namespace vtkm::cont::openmp::sort::radix

#endif // vtk_m_cont_openmp_internal_ParallelRadixSortOpenMP_h
