//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_Atomic_h
#define vtk_m_Atomic_h

#include <vtkm/List.h>

#include <vtkm/internal/Windows.h>

#include <atomic>

namespace vtkm
{

/// \brief Specifies memory order semantics for atomic operations.
///
/// The memory order parameter controls how all other memory operations are
/// ordered around a specific atomic instruction.
///
/// Memory access is complicated. Compilers can reorder instructions to optimize
/// scheduling, processors can speculatively read memory, and caches make
/// assumptions about coherency that we may not normally be aware of. Because of
/// this complexity, the order in which multiple updates to shared memory become
/// visible to other threads is not guaranteed, nor is it guaranteed that each
/// thread will see memory updates occur in the same order as any other thread.
/// This can lead to surprising behavior and cause problems when using atomics
/// to communicate between threads.
///
/// These problems are solved by using a standard set of memory orderings which
/// describe common access patterns used for shared memory programming. Their
/// goal is to provide guarantees that changes made in one thread will be visible
/// to another thread at a specific and predictable point in execution, regardless
/// of any hardware or compiler optimizations.
///
/// If unsure, use `SequentiallyConsistent` memory orderings. It will "do the right
/// thing", but at the cost of increased and possibly unnecessary memory ordering
/// restrictions. The other orderings are optimizations that are only applicable
/// in very specific situations.
///
/// See https://en.cppreference.com/w/cpp/atomic/memory_order for a detailed
/// description of the different orderings and their usage.
///
/// The memory order semantics follow those of other common atomic operations such as
/// the `std::memory_order` identifiers used for `std::atomic`.
///
/// Note that when a memory order is specified, the enforced memory order is guaranteed
/// to be as good or better than that requested.
///
enum class MemoryOrder
{
  /// An atomic operations with `Relaxed` memory order enforces no synchronization or ordering
  /// constraints on local reads and writes. That is, a read or write to a local, non-atomic
  /// variable may be moved to before or after an atomic operation with `Relaxed` memory order.
  ///
  Relaxed,

  /// A load operation with `Acquire` memory order will enforce that any local read or write
  /// operations listed in the program after the atomic will happen after the atomic.
  ///
  Acquire,

  /// A store operation with `Release` memory order will enforce that any local read or write
  /// operations listed in the program before the atomic will happen before the atomic.
  ///
  Release,

  /// A read-modify-write operation with `AcquireAndRelease` memory order will enforce that any
  /// local read or write operations listed in the program before the atomic will happen before the
  /// atomic and likewise any read or write operations listed in the program after the atomic will
  /// happen after the atomic.
  ///
  AcquireAndRelease,

  /// An atomic with `SequentiallyConsistent` memory order will enforce any appropriate semantics
  /// as `Acquire`, `Release`, and `AcquireAndRelease`. Additionally, `SequentiallyConsistent` will
  /// enforce a consistent ordering of atomic operations across all threads. That is, all threads
  /// observe the modifications in the same order.
  ///
  SequentiallyConsistent
};

namespace internal
{

VTKM_EXEC_CONT inline std::memory_order StdAtomicMemOrder(vtkm::MemoryOrder order)
{
  switch (order)
  {
    case vtkm::MemoryOrder::Relaxed:
      return std::memory_order_relaxed;
    case vtkm::MemoryOrder::Acquire:
      return std::memory_order_acquire;
    case vtkm::MemoryOrder::Release:
      return std::memory_order_release;
    case vtkm::MemoryOrder::AcquireAndRelease:
      return std::memory_order_acq_rel;
    case vtkm::MemoryOrder::SequentiallyConsistent:
      return std::memory_order_seq_cst;
  }

  // Should never reach here, but avoid compiler warnings
  return std::memory_order_seq_cst;
}

} // namespace internal

} // namespace vtkm


#if defined(VTKM_CUDA_DEVICE_PASS)

namespace vtkm
{
namespace detail
{

// Fence to ensure that previous non-atomic stores are visible to other threads.
VTKM_EXEC_CONT inline void AtomicStoreFence(vtkm::MemoryOrder order)
{
  if ((order == vtkm::MemoryOrder::Release) || (order == vtkm::MemoryOrder::AcquireAndRelease) ||
      (order == vtkm::MemoryOrder::SequentiallyConsistent))
  {
    __threadfence();
  }
}

// Fence to ensure that previous non-atomic stores are visible to other threads.
VTKM_EXEC_CONT inline void AtomicLoadFence(vtkm::MemoryOrder order)
{
  if ((order == vtkm::MemoryOrder::Acquire) || (order == vtkm::MemoryOrder::AcquireAndRelease) ||
      (order == vtkm::MemoryOrder::SequentiallyConsistent))
  {
    __threadfence();
  }
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicLoadImpl(const T* addr, vtkm::MemoryOrder order)
{
  const volatile T* vaddr = addr; /* volatile to bypass cache*/
  if (order == vtkm::MemoryOrder::SequentiallyConsistent)
  {
    __threadfence();
  }
  const T value = *vaddr;
  /* fence to ensure that dependent reads are correctly ordered */
  AtomicLoadFence(order);
  return value;
}

template <typename T>
VTKM_EXEC_CONT inline void AtomicStoreImpl(T* addr, T value, vtkm::MemoryOrder order)
{
  volatile T* vaddr = addr; /* volatile to bypass cache */
  /* fence to ensure that previous non-atomic stores are visible to other threads */
  AtomicStoreFence(order);
  *vaddr = value;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAddImpl(T* addr, T arg, vtkm::MemoryOrder order)
{
  AtomicStoreFence(order);
  auto result = atomicAdd(addr, arg);
  AtomicLoadFence(order);
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAndImpl(T* addr, T mask, vtkm::MemoryOrder order)
{
  AtomicStoreFence(order);
  auto result = atomicAnd(addr, mask);
  AtomicLoadFence(order);
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicOrImpl(T* addr, T mask, vtkm::MemoryOrder order)
{
  AtomicStoreFence(order);
  auto result = atomicOr(addr, mask);
  AtomicLoadFence(order);
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicXorImpl(T* addr, T mask, vtkm::MemoryOrder order)
{
  AtomicStoreFence(order);
  auto result = atomicXor(addr, mask);
  AtomicLoadFence(order);
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicNotImpl(T* addr, vtkm::MemoryOrder order)
{
  return AtomicXorImpl(addr, static_cast<T>(~T{ 0u }), order);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicCompareAndSwapImpl(T* addr,
                                                 T desired,
                                                 T expected,
                                                 vtkm::MemoryOrder order)
{
  AtomicStoreFence(order);
  auto result = atomicCAS(addr, expected, desired);
  AtomicLoadFence(order);
  return result;
}
}
} // namespace vtkm::detail

#elif defined(VTKM_ENABLE_KOKKOS)

VTKM_THIRDPARTY_PRE_INCLUDE
// Superhack! Kokkos_Macros.hpp defines macros to include modifiers like __device__.
// However, we don't want to actually use those if compiling this with a standard
// C++ compiler (because this particular code does not run on a device). Thus,
// we want to disable that behavior when not using the device compiler. To do that,
// we are going to have to load the KokkosCore_config.h file (which you are not
// supposed to do), then undefine the device enables if necessary, then load
// Kokkos_Macros.hpp to finish the state.
#ifndef KOKKOS_MACROS_HPP
#define KOKKOS_MACROS_HPP
#include <KokkosCore_config.h>
#undef KOKKOS_MACROS_HPP
#define KOKKOS_DONT_INCLUDE_CORE_CONFIG_H

#if defined(KOKKOS_ENABLE_CUDA) && !defined(VTKM_CUDA)
#undef KOKKOS_ENABLE_CUDA
#endif
#endif //KOKKOS_MACROS_HPP not loaded

#include <Kokkos_Core.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace detail
{

// Fence to ensure that previous non-atomic stores are visible to other threads.
VTKM_EXEC_CONT inline void AtomicStoreFence(vtkm::MemoryOrder order)
{
  if ((order == vtkm::MemoryOrder::Release) || (order == vtkm::MemoryOrder::AcquireAndRelease) ||
      (order == vtkm::MemoryOrder::SequentiallyConsistent))
  {
    Kokkos::memory_fence();
  }
}

// Fence to ensure that previous non-atomic stores are visible to other threads.
VTKM_EXEC_CONT inline void AtomicLoadFence(vtkm::MemoryOrder order)
{
  if ((order == vtkm::MemoryOrder::Acquire) || (order == vtkm::MemoryOrder::AcquireAndRelease) ||
      (order == vtkm::MemoryOrder::SequentiallyConsistent))
  {
    Kokkos::memory_fence();
  }
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicLoadImpl(const T* addr, vtkm::MemoryOrder order)
{
  switch (order)
  {
    case vtkm::MemoryOrder::Relaxed:
      return Kokkos::Impl::atomic_load(addr, Kokkos::Impl::memory_order_relaxed);
    case vtkm::MemoryOrder::Acquire:
    case vtkm::MemoryOrder::Release:           // Release doesn't make sense. Use Acquire.
    case vtkm::MemoryOrder::AcquireAndRelease: // Release doesn't make sense. Use Acquire.
      return Kokkos::Impl::atomic_load(addr, Kokkos::Impl::memory_order_acquire);
    case vtkm::MemoryOrder::SequentiallyConsistent:
      return Kokkos::Impl::atomic_load(addr, Kokkos::Impl::memory_order_seq_cst);
  }

  // Should never reach here, but avoid compiler warnings
  return Kokkos::Impl::atomic_load(addr, Kokkos::Impl::memory_order_seq_cst);
}

template <typename T>
VTKM_EXEC_CONT inline void AtomicStoreImpl(T* addr, T value, vtkm::MemoryOrder order)
{
  switch (order)
  {
    case vtkm::MemoryOrder::Relaxed:
      Kokkos::Impl::atomic_store(addr, value, Kokkos::Impl::memory_order_relaxed);
      break;
    case vtkm::MemoryOrder::Acquire: // Acquire doesn't make sense. Use Release.
    case vtkm::MemoryOrder::Release:
    case vtkm::MemoryOrder::AcquireAndRelease: // Acquire doesn't make sense. Use Release.
      Kokkos::Impl::atomic_store(addr, value, Kokkos::Impl::memory_order_release);
      break;
    case vtkm::MemoryOrder::SequentiallyConsistent:
      Kokkos::Impl::atomic_store(addr, value, Kokkos::Impl::memory_order_seq_cst);
      break;
  }
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAddImpl(T* addr, T arg, vtkm::MemoryOrder order)
{
  AtomicStoreFence(order);
  T result = Kokkos::atomic_fetch_add(addr, arg);
  AtomicLoadFence(order);
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAndImpl(T* addr, T mask, vtkm::MemoryOrder order)
{
  AtomicStoreFence(order);
  T result = Kokkos::atomic_fetch_and(addr, mask);
  AtomicLoadFence(order);
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicOrImpl(T* addr, T mask, vtkm::MemoryOrder order)
{
  AtomicStoreFence(order);
  T result = Kokkos::atomic_fetch_or(addr, mask);
  AtomicLoadFence(order);
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicXorImpl(T* addr, T mask, vtkm::MemoryOrder order)
{
  AtomicStoreFence(order);
  T result = Kokkos::atomic_fetch_xor(addr, mask);
  AtomicLoadFence(order);
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicNotImpl(T* addr, vtkm::MemoryOrder order)
{
  return AtomicXorImpl(addr, static_cast<T>(~T{ 0u }), order);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicCompareAndSwapImpl(T* addr,
                                                 T desired,
                                                 T expected,
                                                 vtkm::MemoryOrder order)
{
  AtomicStoreFence(order);
  T result = Kokkos::atomic_compare_exchange(addr, expected, desired);
  AtomicLoadFence(order);
  return result;
}
}
} // namespace vtkm::detail

#elif defined(VTKM_MSVC)

// Supports vtkm::UInt8, vtkm::UInt16, vtkm::UInt32, vtkm::UInt64

#include <cstdint>
#include <cstring>
#include <intrin.h> // For MSVC atomics

namespace vtkm
{
namespace detail
{

template <typename To, typename From>
VTKM_EXEC_CONT inline To BitCast(const From& src)
{
  // The memcpy should be removed by the compiler when possible, but this
  // works around a host of issues with bitcasting using reinterpret_cast.
  VTKM_STATIC_ASSERT(sizeof(From) == sizeof(To));
  To dst;
  std::memcpy(&dst, &src, sizeof(From));
  return dst;
}

template <typename T>
VTKM_EXEC_CONT inline T BitCast(T&& src)
{
  return std::forward<T>(src);
}

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

VTKM_EXEC_CONT inline vtkm::UInt8 AtomicLoadImpl(const vtkm::UInt8* addr, vtkm::MemoryOrder order)
{
  // This assumes that the memory interface is smart enough to load a 32-bit
  // word atomically and a properly aligned 8-bit word from it.
  // We could build address masks and do shifts to perform this manually if
  // this assumption is incorrect.
  auto result = *static_cast<volatile const vtkm::UInt8*>(addr);
  std::atomic_thread_fence(internal::StdAtomicMemOrder(order));
  return result;
}
VTKM_EXEC_CONT inline vtkm::UInt16 AtomicLoadImpl(const vtkm::UInt16* addr, vtkm::MemoryOrder order)
{
  // This assumes that the memory interface is smart enough to load a 32-bit
  // word atomically and a properly aligned 16-bit word from it.
  // We could build address masks and do shifts to perform this manually if
  // this assumption is incorrect.
  auto result = *static_cast<volatile const vtkm::UInt16*>(addr);
  std::atomic_thread_fence(internal::StdAtomicMemOrder(order));
  return result;
}
VTKM_EXEC_CONT inline vtkm::UInt32 AtomicLoadImpl(const vtkm::UInt32* addr, vtkm::MemoryOrder order)
{
  auto result = *static_cast<volatile const vtkm::UInt32*>(addr);
  std::atomic_thread_fence(internal::StdAtomicMemOrder(order));
  return result;
}
VTKM_EXEC_CONT inline vtkm::UInt64 AtomicLoadImpl(const vtkm::UInt64* addr, vtkm::MemoryOrder order)
{
  auto result = *static_cast<volatile const vtkm::UInt64*>(addr);
  std::atomic_thread_fence(internal::StdAtomicMemOrder(order));
  return result;
}

VTKM_EXEC_CONT inline void AtomicStoreImpl(vtkm::UInt8* addr,
                                           vtkm::UInt8 val,
                                           vtkm::MemoryOrder order)
{
  // There doesn't seem to be an atomic store instruction in the windows
  // API, so just exchange and discard the result.
  _InterlockedExchange8(reinterpret_cast<volatile CHAR*>(addr), BitCast<CHAR>(val));
}
VTKM_EXEC_CONT inline void AtomicStoreImpl(vtkm::UInt16* addr,
                                           vtkm::UInt16 val,
                                           vtkm::MemoryOrder order)
{
  // There doesn't seem to be an atomic store instruction in the windows
  // API, so just exchange and discard the result.
  _InterlockedExchange16(reinterpret_cast<volatile SHORT*>(addr), BitCast<SHORT>(val));
}
VTKM_EXEC_CONT inline void AtomicStoreImpl(vtkm::UInt32* addr,
                                           vtkm::UInt32 val,
                                           vtkm::MemoryOrder order)
{
  std::atomic_thread_fence(internal::StdAtomicMemOrder(order));
  *addr = val;
}
VTKM_EXEC_CONT inline void AtomicStoreImpl(vtkm::UInt64* addr,
                                           vtkm::UInt64 val,
                                           vtkm::MemoryOrder order)
{
  std::atomic_thread_fence(internal::StdAtomicMemOrder(order));
  *addr = val;
}

#define VTKM_ATOMIC_OP(vtkmName, winName, vtkmType, winType, suffix)                             \
  VTKM_EXEC_CONT inline vtkmType vtkmName(vtkmType* addr, vtkmType arg, vtkm::MemoryOrder order) \
  {                                                                                              \
    return BitCast<vtkmType>(                                                                    \
      winName##suffix(reinterpret_cast<volatile winType*>(addr), BitCast<winType>(arg)));        \
  }

#define VTKM_ATOMIC_OPS_FOR_TYPE(vtkmType, winType, suffix)                             \
  VTKM_ATOMIC_OP(AtomicAddImpl, _InterlockedExchangeAdd, vtkmType, winType, suffix)     \
  VTKM_ATOMIC_OP(AtomicAndImpl, _InterlockedAnd, vtkmType, winType, suffix)             \
  VTKM_ATOMIC_OP(AtomicOrImpl, _InterlockedOr, vtkmType, winType, suffix)               \
  VTKM_ATOMIC_OP(AtomicXorImpl, _InterlockedXor, vtkmType, winType, suffix)             \
  VTKM_EXEC_CONT inline vtkmType AtomicNotImpl(vtkmType* addr, vtkm::MemoryOrder order) \
  {                                                                                     \
    return AtomicXorImpl(addr, static_cast<vtkmType>(~vtkmType{ 0u }), order);          \
  }                                                                                     \
  VTKM_EXEC_CONT inline vtkmType AtomicCompareAndSwapImpl(                              \
    vtkmType* addr, vtkmType desired, vtkmType expected, vtkm::MemoryOrder order)       \
  {                                                                                     \
    return BitCast<vtkmType>(                                                           \
      _InterlockedCompareExchange##suffix(reinterpret_cast<volatile winType*>(addr),    \
                                          BitCast<winType>(desired),                    \
                                          BitCast<winType>(expected)));                 \
  }

VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt8, CHAR, 8)
VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt16, SHORT, 16)
VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt32, LONG, )
VTKM_ATOMIC_OPS_FOR_TYPE(vtkm::UInt64, LONG64, 64)

#undef VTKM_ATOMIC_OPS_FOR_TYPE
}
} // namespace vtkm::detail

#else // gcc/clang for CPU

// Supports vtkm::UInt8, vtkm::UInt16, vtkm::UInt32, vtkm::UInt64

#include <cstdint>
#include <cstring>

namespace vtkm
{
namespace detail
{

VTKM_EXEC_CONT inline int GccAtomicMemOrder(vtkm::MemoryOrder order)
{
  switch (order)
  {
    case vtkm::MemoryOrder::Relaxed:
      return __ATOMIC_RELAXED;
    case vtkm::MemoryOrder::Acquire:
      return __ATOMIC_ACQUIRE;
    case vtkm::MemoryOrder::Release:
      return __ATOMIC_RELEASE;
    case vtkm::MemoryOrder::AcquireAndRelease:
      return __ATOMIC_ACQ_REL;
    case vtkm::MemoryOrder::SequentiallyConsistent:
      return __ATOMIC_SEQ_CST;
  }

  // Should never reach here, but avoid compiler warnings
  return __ATOMIC_SEQ_CST;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicLoadImpl(const T* addr, vtkm::MemoryOrder order)
{
  return __atomic_load_n(addr, GccAtomicMemOrder(order));
}

template <typename T>
VTKM_EXEC_CONT inline void AtomicStoreImpl(T* addr, T value, vtkm::MemoryOrder order)
{
  return __atomic_store_n(addr, value, GccAtomicMemOrder(order));
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAddImpl(T* addr, T arg, vtkm::MemoryOrder order)
{
  return __atomic_fetch_add(addr, arg, GccAtomicMemOrder(order));
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAndImpl(T* addr, T mask, vtkm::MemoryOrder order)
{
  return __atomic_fetch_and(addr, mask, GccAtomicMemOrder(order));
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicOrImpl(T* addr, T mask, vtkm::MemoryOrder order)
{
  return __atomic_fetch_or(addr, mask, GccAtomicMemOrder(order));
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicXorImpl(T* addr, T mask, vtkm::MemoryOrder order)
{
  return __atomic_fetch_xor(addr, mask, GccAtomicMemOrder(order));
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicNotImpl(T* addr, vtkm::MemoryOrder order)
{
  return AtomicXorImpl(addr, static_cast<T>(~T{ 0u }), order);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicCompareAndSwapImpl(T* addr,
                                                 T desired,
                                                 T expected,
                                                 vtkm::MemoryOrder order)
{
  __atomic_compare_exchange_n(
    addr, &expected, desired, false, GccAtomicMemOrder(order), GccAtomicMemOrder(order));
  return expected;
}
}
} // namespace vtkm::detail

#endif // gcc/clang

namespace vtkm
{

namespace detail
{

template <typename T>
using OppositeSign = typename std::conditional<std::is_signed<T>::value,
                                               typename std::make_unsigned<T>::type,
                                               typename std::make_signed<T>::type>::type;

} // namespace detail

/// \brief The preferred type to use for atomic operations.
///
using AtomicTypePreferred = vtkm::UInt32;

/// \brief A list of types that can be used with atomic operations.
///
/// TODO: Adjust based on devices being compiled.
///
/// BUG: vtkm::UInt64 is provided in this list even though it is not supported on CUDA
/// before compute capability 3.5.
///
using AtomicTypesSupported = vtkm::List<vtkm::UInt32, vtkm::UInt64>;

/// \brief Atomic function to load a value from a shared memory location.
///
/// Given a pointer, returns the value in that pointer. If other threads are writing to
/// that same location, the returned value will be consistent to what was present before
/// or after that write.
///
template <typename T>
VTKM_EXEC_CONT inline T AtomicLoad(const T* pointer,
                                   vtkm::MemoryOrder order = vtkm::MemoryOrder::Acquire)
{
  return detail::AtomicLoadImpl(pointer, order);
}

///@{
/// \brief Atomic function to save a value to a shared memory location.
///
/// Given a pointer and a value, stores that value at the pointer's location. If two
/// threads are simultaneously using `AtomicStore` at the same location, the resulting
/// value will be one of the values or the other (as opposed to a mix of bits).
///
template <typename T>
VTKM_EXEC_CONT inline void AtomicStore(T* pointer,
                                       T value,
                                       vtkm::MemoryOrder order = vtkm::MemoryOrder::Release)
{
  detail::AtomicStoreImpl(pointer, value, order);
}
template <typename T>
VTKM_EXEC_CONT inline void AtomicStore(T* pointer,
                                       detail::OppositeSign<T> value,
                                       vtkm::MemoryOrder order = vtkm::MemoryOrder::Release)
{
  detail::AtomicStoreImpl(pointer, static_cast<T>(value), order);
}
///@}

///@{
/// \brief Atomic function to add a value to a shared memory location.
///
/// Given a pointer and an operand, adds the operand to the value at the given memory
/// location. The result of the addition is put into that memory location and the
/// _old_ value that was originally in the memory is returned. For example, if you
/// call `AtomicAdd` on a memory location that holds a 5 with an operand of 3, the
/// value of 8 is stored in the memory location and the value of 5 is returned.
///
/// If multiple threads call `AtomicAdd` simultaneously, they will not interfere with
/// each other. The result will be consistent as if one was called before the other
/// (although it is indeterminate which will be applied first).
///
template <typename T>
VTKM_EXEC_CONT inline T AtomicAdd(
  T* pointer,
  T operand,
  vtkm::MemoryOrder order = vtkm::MemoryOrder::SequentiallyConsistent)
{
  return detail::AtomicAddImpl(pointer, operand, order);
}
template <typename T>
VTKM_EXEC_CONT inline T AtomicAdd(
  T* pointer,
  detail::OppositeSign<T> operand,
  vtkm::MemoryOrder order = vtkm::MemoryOrder::SequentiallyConsistent)
{
  return detail::AtomicAddImpl(pointer, static_cast<T>(operand), order);
}
///@}

///@{
/// \brief Atomic function to AND bits to a shared memory location.
///
/// Given a pointer and an operand, performs a bitwise AND of the operand and thevalue at the given
/// memory location. The result of the AND is put into that memory location and the _old_ value
/// that was originally in the memory is returned. For example, if you call `AtomicAnd` on a memory
/// location that holds a 0x6 with an operand of 0x3, the value of 0x2 is stored in the memory
/// location and the value of 0x6 is returned.
///
/// If multiple threads call `AtomicAnd` simultaneously, they will not interfere with
/// each other. The result will be consistent as if one was called before the other
/// (although it is indeterminate which will be applied first).
///
template <typename T>
VTKM_EXEC_CONT inline T AtomicAnd(
  T* pointer,
  T operand,
  vtkm::MemoryOrder order = vtkm::MemoryOrder::SequentiallyConsistent)
{
  return detail::AtomicAndImpl(pointer, operand, order);
}
template <typename T>
VTKM_EXEC_CONT inline T AtomicAnd(
  T* pointer,
  detail::OppositeSign<T> operand,
  vtkm::MemoryOrder order = vtkm::MemoryOrder::SequentiallyConsistent)
{
  return detail::AtomicAndImpl(pointer, static_cast<T>(operand), order);
}
///@}

///@{
/// \brief Atomic function to OR bits to a shared memory location.
///
/// Given a pointer and an operand, performs a bitwise OR of the operand and the value at the given
/// memory location. The result of the OR is put into that memory location and the _old_ value
/// that was originally in the memory is returned. For example, if you call `AtomicOr` on a memory
/// location that holds a 0x6 with an operand of 0x3, the value of 0x7 is stored in the memory
/// location and the value of 0x6 is returned.
///
/// If multiple threads call `AtomicOr` simultaneously, they will not interfere with
/// each other. The result will be consistent as if one was called before the other
/// (although it is indeterminate which will be applied first).
///
template <typename T>
VTKM_EXEC_CONT inline T
AtomicOr(T* pointer, T operand, vtkm::MemoryOrder order = vtkm::MemoryOrder::SequentiallyConsistent)
{
  return detail::AtomicOrImpl(pointer, operand, order);
}
template <typename T>
VTKM_EXEC_CONT inline T AtomicOr(
  T* pointer,
  detail::OppositeSign<T> operand,
  vtkm::MemoryOrder order = vtkm::MemoryOrder::SequentiallyConsistent)
{
  return detail::AtomicOrImpl(pointer, static_cast<T>(operand), order);
}
///@}

///@{
/// \brief Atomic function to XOR bits to a shared memory location.
///
/// Given a pointer and an operand, performs a bitwise exclusive-OR of the operand and the value at
/// the given memory location. The result of the XOR is put into that memory location and the _old_
/// value that was originally in the memory is returned. For example, if you call `AtomicXor` on a
/// memory location that holds a 0x6 with an operand of 0x3, the value of 0x5 is stored in the
/// memory location and the value of 0x6 is returned.
///
/// If multiple threads call `AtomicXor` simultaneously, they will not interfere with
/// each other. The result will be consistent as if one was called before the other.
///
template <typename T>
VTKM_EXEC_CONT inline T AtomicXor(
  T* pointer,
  T operand,
  vtkm::MemoryOrder order = vtkm::MemoryOrder::SequentiallyConsistent)
{
  return detail::AtomicXorImpl(pointer, operand, order);
}
template <typename T>
VTKM_EXEC_CONT inline T AtomicXor(
  T* pointer,
  detail::OppositeSign<T> operand,
  vtkm::MemoryOrder order = vtkm::MemoryOrder::SequentiallyConsistent)
{
  return detail::AtomicXorImpl(pointer, static_cast<T>(operand), order);
}
///@}

/// \brief Atomic function to NOT bits to a shared memory location.
///
/// Given a pointer, performs a bitwise NOT of the value at the given
/// memory location. The result of the NOT is put into that memory location and the _old_ value
/// that was originally in the memory is returned.
///
/// If multiple threads call `AtomicNot` simultaneously, they will not interfere with
/// each other. The result will be consistent as if one was called before the other.
///
template <typename T>
VTKM_EXEC_CONT inline T AtomicNot(
  T* pointer,
  vtkm::MemoryOrder order = vtkm::MemoryOrder::SequentiallyConsistent)
{
  return detail::AtomicNotImpl(pointer, order);
}

/// \brief Atomic function that replaces a value given a condition.
///
/// Given a pointer, a new desired value, and an expected value, replaces the value at the
/// pointer if it is the same as the expected value with the new desired value. If the original
/// value in the pointer does not equal the expected value, then the memory at the pointer
/// remains unchanged. In either case, the function returns the _old_ original value that
/// was at the pointer.
///
/// If multiple threads call `AtomicCompareAndSwap` simultaneously, the result will be consistent
/// as if one was called before the other (although it is indeterminate which will be applied
/// first).
///
template <typename T>
VTKM_EXEC_CONT inline T AtomicCompareAndSwap(
  T* pointer,
  T desired,
  T expected,
  vtkm::MemoryOrder order = vtkm::MemoryOrder::SequentiallyConsistent)
{
  return detail::AtomicCompareAndSwapImpl(pointer, desired, expected, order);
}

} // namespace vtkm

#endif //vtk_m_Atomic_h
