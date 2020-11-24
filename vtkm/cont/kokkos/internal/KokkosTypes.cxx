//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/kokkos/internal/KokkosTypes.h>

namespace vtkm
{
namespace cont
{
namespace kokkos
{
namespace internal
{

const ExecutionSpace& GetExecutionSpaceInstance()
{
  // We use per-thread execution spaces so that the threads can execute independently without
  // requiring global synchronizations.
#if defined(VTKM_KOKKOS_CUDA)
  static thread_local ExecutionSpace space(cudaStreamPerThread);
#else
  static thread_local ExecutionSpace space;
#endif
  return space;
}

}
}
}
} // vtkm::cont::kokkos::internal
