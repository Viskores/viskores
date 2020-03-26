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

#include <algorithm>

namespace
{

class BufferInfoShared : public vtkm::cont::internal::BufferInfo
{
  std::shared_ptr<vtkm::cont::internal::BufferInfoHost> HostBuffer;

public:
  VTKM_CONT BufferInfoShared()
    : HostBuffer(new vtkm::cont::internal::BufferInfoHost)
  {
  }

  VTKM_CONT BufferInfoShared(std::shared_ptr<vtkm::cont::internal::BufferInfoHost> hostBuffer)
    : HostBuffer(hostBuffer)
  {
  }

  template <typename... Ts>
  VTKM_CONT BufferInfoShared(Ts&&... bufferInfoHostArgs)
    : HostBuffer(new vtkm::cont::internal::BufferInfoHost(std::forward<Ts>(bufferInfoHostArgs)...))
  {
  }

  VTKM_CONT void* GetPointer() const override { return this->HostBuffer->GetPointer(); }

  VTKM_CONT vtkm::BufferSizeType GetSize() const override { return this->HostBuffer->GetSize(); }

  VTKM_CONT std::shared_ptr<vtkm::cont::internal::BufferInfoHost> GetHostBuffer() const
  {
    return this->HostBuffer;
  }
};

} // anonymous namespace

std::shared_ptr<vtkm::cont::internal::BufferInfo>
vtkm::cont::internal::DeviceAdapterMemoryManagerShared::Allocate(vtkm::BufferSizeType size)
{
  return std::shared_ptr<vtkm::cont::internal::BufferInfo>(new BufferInfoShared(size));
}

std::shared_ptr<vtkm::cont::internal::BufferInfo>
vtkm::cont::internal::DeviceAdapterMemoryManagerShared::ManageArray(
  std::shared_ptr<vtkm::UInt8> buffer,
  vtkm::BufferSizeType size)
{
  return std::shared_ptr<vtkm::cont::internal::BufferInfo>(new BufferInfoShared(buffer, size));
}

void vtkm::cont::internal::DeviceAdapterMemoryManagerShared::Reallocate(
  std::shared_ptr<vtkm::cont::internal::BufferInfo> b,
  vtkm::BufferSizeType newSize)
{
  BufferInfoShared* buffer = dynamic_cast<BufferInfoShared*>(b.get());
  VTKM_ASSERT(buffer != nullptr);

  buffer->GetHostBuffer()->Allocate(newSize, vtkm::CopyFlag::On);
}

std::shared_ptr<vtkm::cont::internal::BufferInfo>
vtkm::cont::internal::DeviceAdapterMemoryManagerShared::CopyHostToDevice(
  std::shared_ptr<vtkm::cont::internal::BufferInfoHost> src)
{
  return std::shared_ptr<vtkm::cont::internal::BufferInfo>(new BufferInfoShared(src));
}

std::shared_ptr<vtkm::cont::internal::BufferInfoHost>
vtkm::cont::internal::DeviceAdapterMemoryManagerShared::CopyDeviceToHost(
  std::shared_ptr<vtkm::cont::internal::BufferInfo> src)
{
  BufferInfoShared* buffer = dynamic_cast<BufferInfoShared*>(src.get());
  VTKM_ASSERT(buffer != nullptr);

  return buffer->GetHostBuffer();
}

std::shared_ptr<vtkm::cont::internal::BufferInfo>
vtkm::cont::internal::DeviceAdapterMemoryManagerShared::CopyDeviceToDevice(
  std::shared_ptr<vtkm::cont::internal::BufferInfo> src)
{
  BufferInfoShared* dest = new BufferInfoShared(src->GetSize());
  vtkm::UInt8* srcP = reinterpret_cast<vtkm::UInt8*>(src->GetPointer());
  vtkm::UInt8* destP = reinterpret_cast<vtkm::UInt8*>(dest->GetPointer());
  std::copy(srcP, srcP + src->GetSize(), destP);
  return std::shared_ptr<vtkm::cont::internal::BufferInfo>(dest);
}
