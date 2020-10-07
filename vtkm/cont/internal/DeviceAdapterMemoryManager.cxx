//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/internal/DeviceAdapterMemoryManager.h>

#include <vtkm/Math.h>

#include <atomic>
#include <cstring>

//----------------------------------------------------------------------------------------
// Special allocation/deallocation code

#if defined(VTKM_POSIX)
#define VTKM_MEMALIGN_POSIX
#elif defined(_WIN32)
#define VTKM_MEMALIGN_WIN
#elif defined(__SSE__)
#define VTKM_MEMALIGN_SSE
#else
#define VTKM_MEMALIGN_NONE
#endif

#if defined(VTKM_MEMALIGN_POSIX)
#include <stdlib.h>
#elif defined(VTKM_MEMALIGN_WIN)
#include <malloc.h>
#elif defined(VTKM_MEMALIGN_SSE)
#include <xmmintrin.h>
#else
#include <malloc.h>
#endif

#include <cstddef>
#include <cstdlib>

namespace
{

/// A deleter object that can be used with our aligned mallocs
void HostDeleter(void* memory)
{
  if (memory == nullptr)
  {
    return;
  }

#if defined(VTKM_MEMALIGN_POSIX)
  free(memory);
#elif defined(VTKM_MEMALIGN_WIN)
  _aligned_free(memory);
#elif defined(VTKM_MEMALIGN_SSE)
  _mm_free(memory);
#else
  free(memory);
#endif
}

/// Allocates a buffer of a specified size using VTK-m's preferred memory alignment.
/// Returns a void* pointer that should be deleted with `HostDeleter`.
void* HostAllocate(vtkm::BufferSizeType numBytes)
{
  VTKM_ASSERT(numBytes >= 0);
  if (numBytes <= 0)
  {
    return nullptr;
  }

  const std::size_t size = static_cast<std::size_t>(numBytes);
  constexpr std::size_t align = VTKM_ALLOCATION_ALIGNMENT;

#if defined(VTKM_MEMALIGN_POSIX)
  void* memory = nullptr;
  if (posix_memalign(&memory, align, size) != 0)
  {
    memory = nullptr;
  }
#elif defined(VTKM_MEMALIGN_WIN)
  void* memory = _aligned_malloc(size, align);
#elif defined(VTKM_MEMALIGN_SSE)
  void* memory = _mm_malloc(size, align);
#else
  void* memory = malloc(size);
#endif

  return memory;
}

/// Reallocates a buffer on the host.
void HostReallocate(void*& memory,
                    void*& container,
                    vtkm::BufferSizeType oldSize,
                    vtkm::BufferSizeType newSize)
{
  VTKM_ASSERT(memory == container);

  // If the new size is not much smaller than the old size, just reuse the buffer (and waste a
  // little memory).
  if ((newSize > ((3 * oldSize) / 4)) && (newSize <= oldSize))
  {
    return;
  }

  void* newBuffer = HostAllocate(newSize);
  std::memcpy(newBuffer, memory, static_cast<std::size_t>(vtkm::Min(newSize, oldSize)));

  if (memory != nullptr)
  {
    HostDeleter(memory);
  }

  memory = container = newBuffer;
}

} // anonymous namespace

namespace vtkm
{
namespace cont
{
namespace internal
{

VTKM_CONT void InvalidRealloc(void*&, void*&, vtkm::BufferSizeType, vtkm::BufferSizeType)
{
  vtkm::cont::ErrorBadAllocation("User provided memory does not have a reallocater.");
}


namespace detail
{

//----------------------------------------------------------------------------------------
// The BufferInfo internals behaves much like a std::shared_ptr. However, we do not use
// std::shared_ptr for compile efficiency issues.
struct BufferInfoInternals
{
  void* Memory;
  void* Container;
  BufferInfo::Deleter* Delete;
  BufferInfo::Reallocater* Reallocate;
  vtkm::BufferSizeType Size;

  using CountType = vtkm::IdComponent;
  std::atomic<CountType> Count;

  VTKM_CONT BufferInfoInternals(void* memory,
                                void* container,
                                vtkm::BufferSizeType size,
                                BufferInfo::Deleter deleter,
                                BufferInfo::Reallocater reallocater)
    : Memory(memory)
    , Container(container)
    , Delete(deleter)
    , Reallocate(reallocater)
    , Size(size)
    , Count(1)
  {
  }

  BufferInfoInternals(const BufferInfoInternals&) = delete;
  void operator=(const BufferInfoInternals&) = delete;
};

} // namespace detail

void* BufferInfo::GetPointer() const
{
  return this->Internals->Memory;
}

vtkm::BufferSizeType BufferInfo::GetSize() const
{
  return this->Internals->Size;
}

vtkm::cont::DeviceAdapterId BufferInfo::GetDevice() const
{
  return this->Device;
}

BufferInfo::BufferInfo()
  : Internals(new detail::BufferInfoInternals(nullptr, nullptr, 0, HostDeleter, HostReallocate))
  , Device(vtkm::cont::DeviceAdapterTagUndefined{})
{
}

BufferInfo::~BufferInfo()
{
  if (this->Internals != nullptr)
  {
    detail::BufferInfoInternals::CountType oldCount =
      this->Internals->Count.fetch_sub(1, std::memory_order::memory_order_seq_cst);
    if (oldCount == 1)
    {
      this->Internals->Delete(this->Internals->Container);
      delete this->Internals;
      this->Internals = nullptr;
    }
  }
}

BufferInfo::BufferInfo(const BufferInfo& src)
  : Internals(src.Internals)
  , Device(src.Device)
{
  // Can add with relaxed because order does not matter. (But order does matter for decrement.)
  this->Internals->Count.fetch_add(1, std::memory_order::memory_order_relaxed);
}

BufferInfo::BufferInfo(BufferInfo&& src)
  : Internals(src.Internals)
  , Device(src.Device)
{
  src.Internals = nullptr;
}

BufferInfo& BufferInfo::operator=(const BufferInfo& src)
{
  detail::BufferInfoInternals::CountType oldCount =
    this->Internals->Count.fetch_sub(1, std::memory_order::memory_order_seq_cst);
  if (oldCount == 1)
  {
    this->Internals->Delete(this->Internals->Container);
    delete this->Internals;
    this->Internals = nullptr;
  }

  this->Internals = src.Internals;
  this->Device = src.Device;

  // Can add with relaxed because order does not matter. (But order does matter for decrement.)
  this->Internals->Count.fetch_add(1, std::memory_order::memory_order_relaxed);

  return *this;
}

BufferInfo& BufferInfo::operator=(BufferInfo&& src)
{
  detail::BufferInfoInternals::CountType oldCount =
    this->Internals->Count.fetch_sub(1, std::memory_order::memory_order_seq_cst);
  if (oldCount == 1)
  {
    this->Internals->Delete(this->Internals->Container);
    delete this->Internals;
    this->Internals = nullptr;
  }

  this->Internals = src.Internals;
  this->Device = src.Device;

  src.Internals = nullptr;

  return *this;
}

BufferInfo::BufferInfo(const BufferInfo& src, vtkm::cont::DeviceAdapterId device)
  : Internals(src.Internals)
  , Device(device)
{
  // Can add with relaxed because order does not matter. (But order does matter for decrement.)
  this->Internals->Count.fetch_add(1, std::memory_order::memory_order_relaxed);
}

BufferInfo::BufferInfo(BufferInfo&& src, vtkm::cont::DeviceAdapterId device)
  : Internals(src.Internals)
  , Device(device)
{
  src.Internals = nullptr;
}

BufferInfo::BufferInfo(vtkm::cont::DeviceAdapterId device,
                       void* memory,
                       void* container,
                       vtkm::BufferSizeType size,
                       Deleter deleter,
                       Reallocater reallocater)
  : Internals(new detail::BufferInfoInternals(memory, container, size, deleter, reallocater))
  , Device(device)
{
}

void BufferInfo::Reallocate(vtkm::BufferSizeType newSize)
{
  this->Internals->Reallocate(
    this->Internals->Memory, this->Internals->Container, this->Internals->Size, newSize);
  this->Internals->Size = newSize;
}

TransferredBuffer BufferInfo::TransferOwnership()
{
  TransferredBuffer tbufffer = { this->Internals->Memory,
                                 this->Internals->Container,
                                 this->Internals->Delete,
                                 this->Internals->Reallocate,
                                 this->Internals->Size };

  this->Internals->Delete = [](void*) {};
  this->Internals->Reallocate = vtkm::cont::internal::InvalidRealloc;

  return tbufffer;
}



//----------------------------------------------------------------------------------------
vtkm::cont::internal::BufferInfo AllocateOnHost(vtkm::BufferSizeType size)
{
  void* memory = HostAllocate(size);

  return vtkm::cont::internal::BufferInfo(
    vtkm::cont::DeviceAdapterTagUndefined{}, memory, memory, size, HostDeleter, HostReallocate);
}

//----------------------------------------------------------------------------------------
DeviceAdapterMemoryManagerBase::~DeviceAdapterMemoryManagerBase() {}

void DeviceAdapterMemoryManagerBase::Reallocate(vtkm::cont::internal::BufferInfo& buffer,
                                                vtkm::BufferSizeType newSize) const
{
  VTKM_ASSERT(buffer.GetDevice() == this->GetDevice());
  buffer.Reallocate(newSize);
}

vtkm::cont::internal::BufferInfo DeviceAdapterMemoryManagerBase::ManageArray(
  void* memory,
  void* container,
  vtkm::BufferSizeType size,
  BufferInfo::Deleter deleter,
  BufferInfo::Reallocater reallocater) const
{
  return vtkm::cont::internal::BufferInfo(
    this->GetDevice(), memory, container, size, deleter, reallocater);
}
}
}
} // namespace vtkm::cont::internal
