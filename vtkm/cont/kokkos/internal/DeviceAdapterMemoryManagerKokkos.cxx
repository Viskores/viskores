//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/kokkos/internal/DeviceAdapterMemoryManagerKokkos.h>

#include <vtkm/cont/kokkos/DeviceAdapterKokkos.h>
#include <vtkm/cont/kokkos/internal/KokkosAlloc.h>
#include <vtkm/cont/kokkos/internal/KokkosTypes.h>

namespace
{

void* KokkosAllocate(vtkm::BufferSizeType size)
{
  return vtkm::cont::kokkos::internal::Allocate(static_cast<std::size_t>(size));
}

void KokkosDelete(void* memory)
{
  vtkm::cont::kokkos::internal::Free(memory);
}

void KokkosReallocate(void*& memory,
                      void*& container,
                      vtkm::BufferSizeType oldSize,
                      vtkm::BufferSizeType newSize)
{
  VTKM_ASSERT(memory == container);
  if (newSize > oldSize)
  {
    if (oldSize == 0)
    {
      memory = container = vtkm::cont::kokkos::internal::Allocate(newSize);
    }
    else
    {
      memory = container = vtkm::cont::kokkos::internal::Reallocate(container, newSize);
    }
  }
}
}

namespace vtkm
{
namespace cont
{
namespace internal
{

vtkm::cont::internal::BufferInfo DeviceAdapterMemoryManager<
  vtkm::cont::DeviceAdapterTagKokkos>::Allocate(vtkm::BufferSizeType size) const
{
  void* memory = KokkosAllocate(size);
  return vtkm::cont::internal::BufferInfo(
    vtkm::cont::DeviceAdapterTagKokkos{}, memory, memory, size, KokkosDelete, KokkosReallocate);
}

vtkm::cont::DeviceAdapterId
DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagKokkos>::GetDevice() const
{
  return vtkm::cont::DeviceAdapterTagKokkos{};
}

vtkm::cont::internal::BufferInfo
DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagKokkos>::CopyHostToDevice(
  const vtkm::cont::internal::BufferInfo& src) const
{
  VTKM_ASSERT(src.GetDevice() == vtkm::cont::DeviceAdapterTagUndefined{});

  // Make a new buffer
  vtkm::cont::internal::BufferInfo dest = this->Allocate(src.GetSize());
  this->CopyHostToDevice(src, dest);

  return dest;
}

void DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagKokkos>::CopyHostToDevice(
  const vtkm::cont::internal::BufferInfo& src,
  const vtkm::cont::internal::BufferInfo& dest) const
{
  vtkm::BufferSizeType size = vtkm::Min(src.GetSize(), dest.GetSize());

  VTKM_LOG_F(vtkm::cont::LogLevel::MemTransfer,
             "Copying host --> Kokkos dev: %s (%lld bytes)",
             vtkm::cont::GetHumanReadableSize(static_cast<std::size_t>(size)).c_str(),
             size);

  vtkm::cont::kokkos::internal::KokkosViewConstCont<vtkm::UInt8> srcView(
    static_cast<vtkm::UInt8*>(src.GetPointer()), static_cast<std::size_t>(size));
  vtkm::cont::kokkos::internal::KokkosViewExec<vtkm::UInt8> destView(
    static_cast<vtkm::UInt8*>(dest.GetPointer()), static_cast<std::size_t>(size));
  Kokkos::deep_copy(vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), destView, srcView);
}

vtkm::cont::internal::BufferInfo
DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagKokkos>::CopyDeviceToHost(
  const vtkm::cont::internal::BufferInfo& src) const
{
  VTKM_ASSERT(src.GetDevice() == vtkm::cont::DeviceAdapterTagKokkos{});

  // Make a new buffer
  vtkm::cont::internal::BufferInfo dest;
  dest = vtkm::cont::internal::AllocateOnHost(src.GetSize());
  this->CopyDeviceToHost(src, dest);

  return dest;
}

void DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagKokkos>::CopyDeviceToHost(
  const vtkm::cont::internal::BufferInfo& src,
  const vtkm::cont::internal::BufferInfo& dest) const
{
  vtkm::BufferSizeType size = vtkm::Min(src.GetSize(), dest.GetSize());

  VTKM_LOG_F(vtkm::cont::LogLevel::MemTransfer,
             "Copying Kokkos dev --> host: %s (%lld bytes)",
             vtkm::cont::GetHumanReadableSize(static_cast<std::size_t>(size)).c_str(),
             size);

  vtkm::cont::kokkos::internal::KokkosViewConstExec<vtkm::UInt8> srcView(
    static_cast<vtkm::UInt8*>(src.GetPointer()), static_cast<std::size_t>(size));
  vtkm::cont::kokkos::internal::KokkosViewCont<vtkm::UInt8> destView(
    static_cast<vtkm::UInt8*>(dest.GetPointer()), static_cast<std::size_t>(size));
  Kokkos::deep_copy(vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), destView, srcView);
  vtkm::cont::kokkos::internal::GetExecutionSpaceInstance().fence();
}

vtkm::cont::internal::BufferInfo
DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagKokkos>::CopyDeviceToDevice(
  const vtkm::cont::internal::BufferInfo& src) const
{
  vtkm::cont::internal::BufferInfo dest = this->Allocate(src.GetSize());
  this->CopyDeviceToDevice(src, dest);

  return dest;
}

void DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagKokkos>::CopyDeviceToDevice(
  const vtkm::cont::internal::BufferInfo& src,
  const vtkm::cont::internal::BufferInfo& dest) const
{
  vtkm::BufferSizeType size = vtkm::Min(src.GetSize(), dest.GetSize());

  vtkm::cont::kokkos::internal::KokkosViewConstExec<vtkm::UInt8> srcView(
    static_cast<vtkm::UInt8*>(src.GetPointer()), static_cast<std::size_t>(size));
  vtkm::cont::kokkos::internal::KokkosViewExec<vtkm::UInt8> destView(
    static_cast<vtkm::UInt8*>(dest.GetPointer()), static_cast<std::size_t>(size));
  Kokkos::deep_copy(vtkm::cont::kokkos::internal::GetExecutionSpaceInstance(), destView, srcView);
}
}
}
} // vtkm::cont::internal
