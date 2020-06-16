//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_kokkos_internal_AtomicInterfaceExecutionKokkos_h
#define vtk_m_cont_kokkos_internal_AtomicInterfaceExecutionKokkos_h

#include <vtkm/cont/kokkos/internal/DeviceAdapterTagKokkos.h>

#include <vtkm/cont/internal/AtomicInterfaceExecution.h>

#include <vtkm/List.h>
#include <vtkm/Types.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <Kokkos_Core.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class AtomicInterfaceExecution<DeviceAdapterTagKokkos>
{

public:
  // Note: There are 64-bit atomics available, but not on all devices. Stick
  // with 32-bit only until we require compute capability 3.5+
  using WordTypes = vtkm::List<vtkm::UInt32, vtkm::UInt64>;
  using WordTypePreferred = vtkm::UInt32;

#define VTKM_ATOMIC_OPS_FOR_TYPE(type)                                                             \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC static type Load(const type* addr)                         \
  {                                                                                                \
    return Kokkos::Impl::atomic_load(addr);                                                        \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC static void Store(type* addr, type value)                  \
  {                                                                                                \
    Kokkos::Impl::atomic_store(addr, value);                                                       \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC static type Add(type* addr, type arg)                      \
  {                                                                                                \
    return Kokkos::atomic_fetch_add(addr, arg);                                                    \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC static type Not(type* addr)                                \
  {                                                                                                \
    return Kokkos::atomic_fetch_xor(addr, static_cast<type>(~type{ 0u }));                         \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC static type And(type* addr, type mask)                     \
  {                                                                                                \
    return Kokkos::atomic_fetch_and(addr, mask);                                                   \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC static type Or(type* addr, type mask)                      \
  {                                                                                                \
    return Kokkos::atomic_fetch_or(addr, mask);                                                    \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC static type Xor(type* addr, type mask)                     \
  {                                                                                                \
    return Kokkos::atomic_fetch_xor(addr, mask);                                                   \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC static type CompareAndSwap(                                \
    type* addr, type newWord, type expected)                                                       \
  {                                                                                                \
    return Kokkos::atomic_compare_exchange(addr, expected, newWord);                               \
  }

  VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt32)
  VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt64)

#undef VTKM_ATOMIC_OPS_FOR_TYPE
};
}
}
} // end namespace vtkm::cont::internal

#endif // vtk_m_cont_kokkos_internal_AtomicInterfaceExecutionKokkos_h
