//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/internal/DeviceAdapterMemoryManagerShared.h>

#include <cstring>

namespace vtkm
{
namespace cont
{
namespace internal
{

vtkm::cont::internal::BufferInfo DeviceAdapterMemoryManagerShared::Allocate(
  vtkm::BufferSizeType size) const
{
  return vtkm::cont::internal::BufferInfo(vtkm::cont::internal::AllocateOnHost(size),
                                          this->GetDevice());
}

vtkm::cont::internal::BufferInfo DeviceAdapterMemoryManagerShared::CopyHostToDevice(
  const vtkm::cont::internal::BufferInfo& src) const
{
  VTKM_ASSERT(src.GetDevice() == vtkm::cont::DeviceAdapterTagUndefined{});
  return vtkm::cont::internal::BufferInfo(src, this->GetDevice());
}

vtkm::cont::internal::BufferInfo DeviceAdapterMemoryManagerShared::CopyDeviceToHost(
  const vtkm::cont::internal::BufferInfo& src) const
{
  VTKM_ASSERT(src.GetDevice() == this->GetDevice());
  return vtkm::cont::internal::BufferInfo(src, vtkm::cont::DeviceAdapterTagUndefined{});
}

vtkm::cont::internal::BufferInfo DeviceAdapterMemoryManagerShared::CopyDeviceToDevice(
  const vtkm::cont::internal::BufferInfo& src) const
{
  VTKM_ASSERT(src.GetDevice() == this->GetDevice());

  vtkm::BufferSizeType size = src.GetSize();
  vtkm::cont::internal::BufferInfo dest = this->Allocate(size);
  std::memcpy(dest.GetPointer(), src.GetPointer(), static_cast<std::size_t>(size));
  return dest;
}
}
}
} // namespace vtkm::cont::internal
