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


#if defined(VTKM_ENABLE_KOKKOS)

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

template <typename T>
VTKM_EXEC_CONT inline T AtomicLoadImpl(const T* addr)
{
  return Kokkos::Impl::atomic_load(addr);
}

template <typename T>
VTKM_EXEC_CONT inline void AtomicStoreImpl(T* addr, T value)
{
  Kokkos::Impl::atomic_store(addr, value);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAddImpl(T* addr, T arg)
{
  return Kokkos::atomic_fetch_add(addr, arg);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAndImpl(T* addr, T mask)
{
  return Kokkos::atomic_fetch_and(addr, mask);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicOrImpl(T* addr, T mask)
{
  return Kokkos::atomic_fetch_or(addr, mask);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicXorImpl(T* addr, T mask)
{
  return Kokkos::atomic_fetch_xor(addr, mask);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicNotImpl(T* addr)
{
  return Kokkos::atomic_fetch_xor(addr, static_cast<T>(~T{ 0u }));
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicCompareAndSwapImpl(T* addr, T desired, T expected)
{
  return Kokkos::atomic_compare_exchange(addr, expected, desired);
}
}
} // namespace vtkm::detail

#elif defined(VTKM_CUDA_DEVICE_PASS)

namespace vtkm
{
namespace detail
{

template <typename T>
VTKM_EXEC_CONT inline T AtomicLoadImpl(const T* addr)
{
  const volatile T* vaddr = addr; /* volatile to bypass cache*/
  const T value = *vaddr;
  /* fence to ensure that dependent reads are correctly ordered */
  __threadfence();
  return value;
}

template <typename T>
VTKM_EXEC_CONT inline void AtomicStoreImpl(T* addr, T value)
{
  volatile T* vaddr = addr; /* volatile to bypass cache */
  /* fence to ensure that previous non-atomic stores are visible to other threads */
  __threadfence();
  *vaddr = value;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAddImpl(T* addr, T arg)
{
  __threadfence();
  auto result = atomicAdd(addr, arg);
  __threadfence();
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAndImpl(T* addr, T mask)
{
  __threadfence();
  auto result = atomicAnd(addr, mask);
  __threadfence();
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicOrImpl(T* addr, T mask)
{
  __threadfence();
  auto result = atomicOr(addr, mask);
  __threadfence();
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicXorImpl(T* addr, T mask)
{
  __threadfence();
  auto result = atomicXor(addr, mask);
  __threadfence();
  return result;
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicNotImpl(T* addr)
{
  return AtomicXorImpl(addr, static_cast<T>(~T{ 0u }));
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicCompareAndSwapImpl(T* addr, T desired, T expected)
{
  __threadfence();
  auto result = atomicCAS(addr, expected, desired);
  __threadfence();
  return result;
}
}
} // namespace vtkm::detail

#elif defined(VTKM_MSVC)

// Supports vtkm::UInt8, vtkm::UInt16, vtkm::UInt32, vtkm::UInt64

#include <atomic>
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

VTKM_EXEC_CONT inline vtkm::UInt8 AtomicLoadImpl(const vtkm::UInt8* addr)
{
  // This assumes that the memory interface is smart enough to load a 32-bit
  // word atomically and a properly aligned 8-bit word from it.
  // We could build address masks and do shifts to perform this manually if
  // this assumption is incorrect.
  auto result = *static_cast<volatile const vtkm::UInt8*>(addr);
  std::atomic_thread_fence(std::memory_order_acquire);
  return result;
}
VTKM_EXEC_CONT inline vtkm::UInt16 AtomicLoadImpl(const vtkm::UInt16* addr)
{
  // This assumes that the memory interface is smart enough to load a 32-bit
  // word atomically and a properly aligned 16-bit word from it.
  // We could build address masks and do shifts to perform this manually if
  // this assumption is incorrect.
  auto result = *static_cast<volatile const vtkm::UInt16*>(addr);
  std::atomic_thread_fence(std::memory_order_acquire);
  return result;
}
VTKM_EXEC_CONT inline vtkm::UInt32 AtomicLoadImpl(const vtkm::UInt32* addr)
{
  auto result = *static_cast<volatile const vtkm::UInt32*>(addr);
  std::atomic_thread_fence(std::memory_order_acquire);
  return result;
}
VTKM_EXEC_CONT inline vtkm::UInt64 AtomicLoadImpl(const vtkm::UInt64* addr)
{
  auto result = *static_cast<volatile const vtkm::UInt64*>(addr);
  std::atomic_thread_fence(std::memory_order_acquire);
  return result;
}

VTKM_EXEC_CONT inline void AtomicStoreImpl(vtkm::UInt8* addr, vtkm::UInt8 val)
{
  // There doesn't seem to be an atomic store instruction in the windows
  // API, so just exchange and discard the result.
  _InterlockedExchange8(reinterpret_cast<volatile CHAR*>(addr), BitCast<CHAR>(val));
}
VTKM_EXEC_CONT inline void AtomicStoreImpl(vtkm::UInt16* addr, vtkm::UInt16 val)
{
  // There doesn't seem to be an atomic store instruction in the windows
  // API, so just exchange and discard the result.
  _InterlockedExchange16(reinterpret_cast<volatile SHORT*>(addr), BitCast<SHORT>(val));
}
VTKM_EXEC_CONT inline void AtomicStoreImpl(vtkm::UInt32* addr, vtkm::UInt32 val)
{
  std::atomic_thread_fence(std::memory_order_release);
  *addr = val;
}
VTKM_EXEC_CONT inline void AtomicStoreImpl(vtkm::UInt64* addr, vtkm::UInt64 val)
{
  std::atomic_thread_fence(std::memory_order_release);
  *addr = val;
}

#define VTKM_ATOMIC_OPS_FOR_TYPE(vtkmType, winType, suffix)                                        \
  VTKM_EXEC_CONT inline vtkmType AtomicAddImpl(vtkmType* addr, vtkmType arg)                       \
  {                                                                                                \
    return BitCast<vtkmType>(_InterlockedExchangeAdd##suffix(                                      \
      reinterpret_cast<volatile winType*>(addr), BitCast<winType>(arg)));                          \
  }                                                                                                \
  VTKM_EXEC_CONT inline vtkmType AtomicAndImpl(vtkmType* addr, vtkmType mask)                      \
  {                                                                                                \
    return BitCast<vtkmType>(                                                                      \
      _InterlockedAnd##suffix(reinterpret_cast<volatile winType*>(addr), BitCast<winType>(mask))); \
  }                                                                                                \
  VTKM_EXEC_CONT inline vtkmType AtomicOrImpl(vtkmType* addr, vtkmType mask)                       \
  {                                                                                                \
    return BitCast<vtkmType>(                                                                      \
      _InterlockedOr##suffix(reinterpret_cast<volatile winType*>(addr), BitCast<winType>(mask)));  \
  }                                                                                                \
  VTKM_EXEC_CONT inline vtkmType AtomicXorImpl(vtkmType* addr, vtkmType mask)                      \
  {                                                                                                \
    return BitCast<vtkmType>(                                                                      \
      _InterlockedXor##suffix(reinterpret_cast<volatile winType*>(addr), BitCast<winType>(mask))); \
  }                                                                                                \
  VTKM_EXEC_CONT inline vtkmType AtomicNotImpl(vtkmType* addr)                                     \
  {                                                                                                \
    return AtomicXorImpl(addr, static_cast<vtkmType>(~vtkmType{ 0u }));                            \
  }                                                                                                \
  VTKM_EXEC_CONT inline vtkmType AtomicCompareAndSwapImpl(                                         \
    vtkmType* addr, vtkmType desired, vtkmType expected)                                           \
  {                                                                                                \
    return BitCast<vtkmType>(                                                                      \
      _InterlockedCompareExchange##suffix(reinterpret_cast<volatile winType*>(addr),               \
                                          BitCast<winType>(desired),                               \
                                          BitCast<winType>(expected)));                            \
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

#include <atomic>
#include <cstdint>
#include <cstring>

namespace vtkm
{
namespace detail
{

template <typename T>
VTKM_EXEC_CONT inline T AtomicLoadImpl(const T* addr)
{
  return __atomic_load_n(addr, __ATOMIC_ACQUIRE);
}

template <typename T>
VTKM_EXEC_CONT inline void AtomicStoreImpl(T* addr, T value)
{
  return __atomic_store_n(addr, value, __ATOMIC_RELEASE);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAddImpl(T* addr, T arg)
{
  return __atomic_fetch_add(addr, arg, __ATOMIC_SEQ_CST);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicAndImpl(T* addr, T mask)
{
  return __atomic_fetch_and(addr, mask, __ATOMIC_SEQ_CST);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicOrImpl(T* addr, T mask)
{
  return __atomic_fetch_or(addr, mask, __ATOMIC_SEQ_CST);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicXorImpl(T* addr, T mask)
{
  return __atomic_fetch_xor(addr, mask, __ATOMIC_SEQ_CST);
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicNotImpl(T* addr)
{
  return AtomicXorImpl(addr, static_cast<T>(~T{ 0u }));
}

template <typename T>
VTKM_EXEC_CONT inline T AtomicCompareAndSwapImpl(T* addr, T desired, T expected)
{
  __atomic_compare_exchange_n(addr, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
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
VTKM_EXEC_CONT inline T AtomicLoad(const T* pointer)
{
  return detail::AtomicLoadImpl(pointer);
}

///@{
/// \brief Atomic function to save a value to a shared memory location.
///
/// Given a pointer and a value, stores that value at the pointer's location. If two
/// threads are simultaneously using `AtomicStore` at the same location, the resulting
/// value will be one of the values or the other (as opposed to a mix of bits).
///
template <typename T>
VTKM_EXEC_CONT inline void AtomicStore(T* pointer, T value)
{
  detail::AtomicStoreImpl(pointer, value);
}
template <typename T>
VTKM_EXEC_CONT inline void AtomicStore(T* pointer, detail::OppositeSign<T> value)
{
  detail::AtomicStoreImpl(pointer, static_cast<T>(value));
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
VTKM_EXEC_CONT inline T AtomicAdd(T* pointer, T operand)
{
  return detail::AtomicAddImpl(pointer, operand);
}
template <typename T>
VTKM_EXEC_CONT inline T AtomicAdd(T* pointer, detail::OppositeSign<T> operand)
{
  return detail::AtomicAddImpl(pointer, static_cast<T>(operand));
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
VTKM_EXEC_CONT inline T AtomicAnd(T* pointer, T operand)
{
  return detail::AtomicAndImpl(pointer, operand);
}
template <typename T>
VTKM_EXEC_CONT inline T AtomicAnd(T* pointer, detail::OppositeSign<T> operand)
{
  return detail::AtomicAndImpl(pointer, static_cast<T>(operand));
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
VTKM_EXEC_CONT inline T AtomicOr(T* pointer, T operand)
{
  return detail::AtomicOrImpl(pointer, operand);
}
template <typename T>
VTKM_EXEC_CONT inline T AtomicOr(T* pointer, detail::OppositeSign<T> operand)
{
  return detail::AtomicOrImpl(pointer, static_cast<T>(operand));
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
VTKM_EXEC_CONT inline T AtomicXor(T* pointer, T operand)
{
  return detail::AtomicXorImpl(pointer, operand);
}
template <typename T>
VTKM_EXEC_CONT inline T AtomicXor(T* pointer, detail::OppositeSign<T> operand)
{
  return detail::AtomicXorImpl(pointer, static_cast<T>(operand));
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
VTKM_EXEC_CONT inline T AtomicNot(T* pointer)
{
  return detail::AtomicNotImpl(pointer);
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
VTKM_EXEC_CONT inline T AtomicCompareAndSwap(T* pointer, T desired, T expected)
{
  return detail::AtomicCompareAndSwapImpl(pointer, desired, expected);
}

} // namespace vtkm

#endif //vtk_m_Atomic_h
