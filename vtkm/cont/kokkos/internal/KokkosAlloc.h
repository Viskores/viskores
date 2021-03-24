//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_kokkos_internal_KokkosAlloc_h
#define vtk_m_cont_kokkos_internal_KokkosAlloc_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <cstddef>

namespace vtkm
{
namespace cont
{
namespace kokkos
{
namespace internal
{

VTKM_CONT_EXPORT void* Allocate(std::size_t size);
VTKM_CONT_EXPORT void Free(void* ptr);
VTKM_CONT_EXPORT void* Reallocate(void* ptr, std::size_t newSize);

}
}
}
} // vtkm::cont::kokkos::internal

#endif // vtk_m_cont_kokkos_internal_KokkosAlloc_h
