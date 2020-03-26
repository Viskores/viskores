//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/internal/DeviceAdapterMemoryManager.h>

#include <algorithm>

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
struct HostDeleter
{
  void operator()(void* memory)
  {
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
};

/// Allocates a buffer of a specified size using VTK-m's preferred memory alignment.
/// Returns as a shared_ptr with the proper deleter.
std::shared_ptr<vtkm::UInt8> HostAllocate(vtkm::BufferSizeType numBytes)
{
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

  return std::shared_ptr<vtkm::UInt8>(reinterpret_cast<vtkm::UInt8*>(memory), HostDeleter{});
}

} // anonymous namespace

//----------------------------------------------------------------------------------------
vtkm::cont::internal::BufferInfo::~BufferInfo()
{
}

vtkm::cont::internal::BufferInfoHost::BufferInfoHost()
  : Buffer()
  , Size(0)
{
}

vtkm::cont::internal::BufferInfoHost::BufferInfoHost(const std::shared_ptr<UInt8>& buffer,
                                                     vtkm::BufferSizeType size)
  : Buffer(buffer)
  , Size(size)
{
}

vtkm::cont::internal::BufferInfoHost::BufferInfoHost(vtkm::BufferSizeType size)
  : Buffer()
  , Size(0)
{
  this->Allocate(size, vtkm::CopyFlag::Off);
}

void* vtkm::cont::internal::BufferInfoHost::GetPointer() const
{
  return this->Buffer.get();
}

vtkm::BufferSizeType vtkm::cont::internal::BufferInfoHost::GetSize() const
{
  return this->Size;
}

void vtkm::cont::internal::BufferInfoHost::Allocate(vtkm::BufferSizeType size,
                                                    vtkm::CopyFlag preserve)
{
  if (size < 1)
  {
    // Compilers apparently have a problem calling std::shared_ptr::reset with nullptr.
    vtkm::UInt8* empty = nullptr;
    this->Buffer.reset(empty);
    this->Size = 0;
  }
  else if (preserve == vtkm::CopyFlag::Off)
  {
    this->Buffer = HostAllocate(size);
    this->Size = size;
  }
  else // preserve == vtkm::CopyFlag::On
  {
    std::shared_ptr<vtkm::UInt8> newBuffer = HostAllocate(size);
    std::copy(this->Buffer.get(), this->Buffer.get() + std::min(size, this->Size), newBuffer.get());
    this->Buffer = newBuffer;
    this->Size = size;
  }
}

std::shared_ptr<vtkm::UInt8> vtkm::cont::internal::BufferInfoHost::GetSharedPointer() const
{
  return this->Buffer;
}

vtkm::cont::internal::DeviceAdapterMemoryManagerBase::~DeviceAdapterMemoryManagerBase()
{
}
