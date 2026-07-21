//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/kokkos/internal/KokkosTypes.h>

namespace viskores
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
#if defined(VISKORES_KOKKOS_CUDA)
  static thread_local ExecutionSpace space(cudaStreamPerThread);
#else
  static thread_local ExecutionSpace space;
#endif
  return space;
}

}
}
}
} // viskores::cont::kokkos::internal
