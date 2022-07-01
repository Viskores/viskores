//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/kokkos/internal/KokkosAlloc.h>

#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/kokkos/internal/KokkosTypes.h>

#include <sstream>

namespace vtkm
{
namespace cont
{
namespace kokkos
{
namespace internal
{

void* Allocate(std::size_t size)
{
  try
  {
    return Kokkos::kokkos_malloc<ExecutionSpace::memory_space>(size);
  }
  catch (...) // the type of error thrown is not well documented
  {
    std::ostringstream err;
    err << "Failed to allocate " << size << " bytes on Kokkos device";
    throw vtkm::cont::ErrorBadAllocation(err.str());
  }
}

void Free(void* ptr)
{
  if (Kokkos::is_initialized())
  {
    GetExecutionSpaceInstance().fence();
    Kokkos::kokkos_free<ExecutionSpace::memory_space>(ptr);
  }
  else
  {
    // It is possible that a Buffer instance might try to free its Kokkos data after
    // Kokkos has been finalized. If that is the case, silently do nothing.
  }
}

void* Reallocate(void* ptr, std::size_t newSize)
{
  try
  {
    return Kokkos::kokkos_realloc<ExecutionSpace::memory_space>(ptr, newSize);
  }
  catch (...)
  {
    std::ostringstream err;
    err << "Failed to re-allocate " << newSize << " bytes on Kokkos device";
    throw vtkm::cont::ErrorBadAllocation(err.str());
  }
}

}
}
}
} // vtkm::cont::kokkos::internal
