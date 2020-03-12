//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_AtomicInterfaceControl_h
#define vtk_m_cont_internal_AtomicInterfaceControl_h

#include <vtkm/internal/Configure.h>
#include <vtkm/internal/Windows.h>

#include <vtkm/List.h>
#include <vtkm/Types.h>

#if defined(VTKM_MSVC) && !defined(VTKM_CUDA)
#include <intrin.h> // For MSVC atomics
#endif

#include <atomic>
#include <cstdint>
#include <cstring>

namespace vtkm
{
namespace cont
{
namespace internal
{

/**
 * Implementation of AtomicInterfaceDevice that uses control-side atomics.
 */
class AtomicInterfaceControl
{
public:
  using WordTypes = vtkm::List<vtkm::UInt8, vtkm::UInt16, vtkm::UInt32, vtkm::UInt64>;

  // TODO These support UInt64, too. This should be benchmarked to see which
  // is faster.
  using WordTypePreferred = vtkm::UInt32;

#ifdef VTKM_MSVC
private:
  template <typename To, typename From>
  VTKM_EXEC_CONT static To BitCast(const From& src)
  {
    // The memcpy should be removed by the compiler when possible, but this
    // works around a host of issues with bitcasting using reinterpret_cast.
    VTKM_STATIC_ASSERT(sizeof(From) == sizeof(To));
    To dst;
    std::memcpy(&dst, &src, sizeof(From));
    return dst;
  }

  template <typename T>
  VTKM_EXEC_CONT static T BitCast(T&& src)
  {
    return std::forward<T>(src);
  }

public:
  // Note about Load and Store implementations:
  //
  // "Simple reads and writes to properly-aligned 32-bit variables are atomic
  //  operations"
  //
  // "Simple reads and writes to properly aligned 64-bit variables are atomic on
  // 64-bit Windows. Reads and writes to 64-bit values are not guaranteed to be
  // atomic on 32-bit Windows."
  //
  // "Reads and writes to variables of other sizes [than 32 or 64 bits] are not
  // guaranteed to be atomic on any platform."
  //
  // https://docs.microsoft.com/en-us/windows/desktop/sync/interlocked-variable-access

  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static vtkm::UInt8 Load(const vtkm::UInt8* addr)
  {
    // This assumes that the memory interface is smart enough to load a 32-bit
    // word atomically and a properly aligned 8-bit word from it.
    // We could build address masks and do shifts to perform this manually if
    // this assumption is incorrect.
    auto result = *static_cast<volatile const vtkm::UInt8*>(addr);
    std::atomic_thread_fence(std::memory_order_acquire);
    return result;
  }
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static vtkm::UInt16 Load(const vtkm::UInt16* addr)
  {
    // This assumes that the memory interface is smart enough to load a 32-bit
    // word atomically and a properly aligned 16-bit word from it.
    // We could build address masks and do shifts to perform this manually if
    // this assumption is incorrect.
    auto result = *static_cast<volatile const vtkm::UInt16*>(addr);
    std::atomic_thread_fence(std::memory_order_acquire);
    return result;
  }
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static vtkm::UInt32 Load(const vtkm::UInt32* addr)
  {
    auto result = *static_cast<volatile const vtkm::UInt32*>(addr);
    std::atomic_thread_fence(std::memory_order_acquire);
    return result;
  }
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static vtkm::UInt64 Load(const vtkm::UInt64* addr)
  {
    auto result = *static_cast<volatile const vtkm::UInt64*>(addr);
    std::atomic_thread_fence(std::memory_order_acquire);
    return result;
  }
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static void Store(vtkm::UInt8* addr, vtkm::UInt8 val)
  {
    // There doesn't seem to be an atomic store instruction in the windows
    // API, so just exchange and discard the result.
    _InterlockedExchange8(reinterpret_cast<volatile CHAR*>(addr), BitCast<CHAR>(val));
  }
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static void Store(vtkm::UInt16* addr, vtkm::UInt16 val)
  {
    // There doesn't seem to be an atomic store instruction in the windows
    // API, so just exchange and discard the result.
    _InterlockedExchange16(reinterpret_cast<volatile SHORT*>(addr), BitCast<SHORT>(val));
  }
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static void Store(vtkm::UInt32* addr, vtkm::UInt32 val)
  {
    std::atomic_thread_fence(std::memory_order_release);
    *addr = val;
  }
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static void Store(vtkm::UInt64* addr, vtkm::UInt64 val)
  {
    std::atomic_thread_fence(std::memory_order_release);
    *addr = val;
  }

#define VTKM_ATOMIC_OPS_FOR_TYPE(vtkmType, winType, suffix)                                        \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static vtkmType Add(vtkmType* addr, vtkmType arg)     \
  {                                                                                                \
    return BitCast<vtkmType>(_InterlockedExchangeAdd##suffix(                                      \
      reinterpret_cast<volatile winType*>(addr), BitCast<winType>(arg)));                          \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static vtkmType Not(vtkmType* addr)                   \
  {                                                                                                \
    return Xor(addr, static_cast<vtkmType>(~vtkmType{ 0u }));                                      \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static vtkmType And(vtkmType* addr, vtkmType mask)    \
  {                                                                                                \
    return BitCast<vtkmType>(                                                                      \
      _InterlockedAnd##suffix(reinterpret_cast<volatile winType*>(addr), BitCast<winType>(mask))); \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static vtkmType Or(vtkmType* addr, vtkmType mask)     \
  {                                                                                                \
    return BitCast<vtkmType>(                                                                      \
      _InterlockedOr##suffix(reinterpret_cast<volatile winType*>(addr), BitCast<winType>(mask)));  \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static vtkmType Xor(vtkmType* addr, vtkmType mask)    \
  {                                                                                                \
    return BitCast<vtkmType>(                                                                      \
      _InterlockedXor##suffix(reinterpret_cast<volatile winType*>(addr), BitCast<winType>(mask))); \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static vtkmType CompareAndSwap(                       \
    vtkmType* addr, vtkmType newWord, vtkmType expected)                                           \
  {                                                                                                \
    return BitCast<vtkmType>(                                                                      \
      _InterlockedCompareExchange##suffix(reinterpret_cast<volatile winType*>(addr),               \
                                          BitCast<winType>(newWord),                               \
                                          BitCast<winType>(expected)));                            \
  }

  VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt8, CHAR, 8)
  VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt16, SHORT, 16)
  VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt32, LONG, )
  VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt64, LONG64, 64)

#undef VTKM_ATOMIC_OPS_FOR_TYPE

#else // gcc/clang

#define VTKM_ATOMIC_OPS_FOR_TYPE(type)                                                             \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static type Load(const type* addr)                    \
  {                                                                                                \
    return __atomic_load_n(addr, __ATOMIC_ACQUIRE);                                                \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static void Store(type* addr, type value)             \
  {                                                                                                \
    return __atomic_store_n(addr, value, __ATOMIC_RELEASE);                                        \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static type Add(type* addr, type arg)                 \
  {                                                                                                \
    return __atomic_fetch_add(addr, arg, __ATOMIC_SEQ_CST);                                        \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static type Not(type* addr)                           \
  {                                                                                                \
    return Xor(addr, static_cast<type>(~type{ 0u }));                                              \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static type And(type* addr, type mask)                \
  {                                                                                                \
    return __atomic_fetch_and(addr, mask, __ATOMIC_SEQ_CST);                                       \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static type Or(type* addr, type mask)                 \
  {                                                                                                \
    return __atomic_fetch_or(addr, mask, __ATOMIC_SEQ_CST);                                        \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static type Xor(type* addr, type mask)                \
  {                                                                                                \
    return __atomic_fetch_xor(addr, mask, __ATOMIC_SEQ_CST);                                       \
  }                                                                                                \
  VTKM_SUPPRESS_EXEC_WARNINGS VTKM_EXEC_CONT static type CompareAndSwap(                           \
    type* addr, type newWord, type expected)                                                       \
  {                                                                                                \
    __atomic_compare_exchange_n(                                                                   \
      addr, &expected, newWord, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);                        \
    return expected;                                                                               \
  }

  VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt8)
  VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt16)
  VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt32)
  VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt64)

#undef VTKM_ATOMIC_OPS_FOR_TYPE

#endif
};
}
}
} // end namespace vtkm::cont::internal

#endif // vtk_m_cont_internal_AtomicInterfaceControl_h
