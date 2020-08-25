//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/BitField.h>
#include <vtkm/cont/Logging.h>

namespace
{

struct DeviceCheckFunctor
{
  vtkm::cont::DeviceAdapterId FoundDevice = vtkm::cont::DeviceAdapterTagUndefined{};

  VTKM_CONT void operator()(vtkm::cont::DeviceAdapterId device,
                            const vtkm::cont::internal::Buffer& buffer)
  {
    if (this->FoundDevice == vtkm::cont::DeviceAdapterTagUndefined{})
    {
      if (buffer.IsAllocatedOnDevice(device))
      {
        this->FoundDevice = device;
      }
    }
  }
};

} // anonymous namespace

namespace vtkm
{
namespace cont
{

namespace detail
{

vtkm::cont::internal::BitFieldMetaData* GetBitFieldMetaData(
  const vtkm::cont::internal::Buffer& buffer)
{
  vtkm::cont::internal::BufferMetaData* generalMetaData = buffer.GetMetaData();
  if (generalMetaData == nullptr)
  {
    VTKM_LOG_F(vtkm::cont::LogLevel::Warn, "BitField has buffer with no metadata. Setting.");
    const_cast<vtkm::cont::internal::Buffer&>(buffer).SetMetaData(
      vtkm::cont::internal::BitFieldMetaData{});
    generalMetaData = buffer.GetMetaData();
    VTKM_ASSERT(generalMetaData != nullptr);
  }

  vtkm::cont::internal::BitFieldMetaData* metadata =
    dynamic_cast<vtkm::cont::internal::BitFieldMetaData*>(generalMetaData);
  if (metadata == nullptr)
  {
    VTKM_LOG_F(vtkm::cont::LogLevel::Error,
               "BitField has a buffer with metadata of the wrong type. "
               "Replacing, but this will likely cause problems.");
    const_cast<vtkm::cont::internal::Buffer&>(buffer).SetMetaData(
      vtkm::cont::internal::BitFieldMetaData{});
    generalMetaData = buffer.GetMetaData();
    metadata = dynamic_cast<vtkm::cont::internal::BitFieldMetaData*>(generalMetaData);
    VTKM_ASSERT(metadata != nullptr);
  }

  // Check to make sure that the buffer is at least as larger as needed for buffer size.
  VTKM_ASSERT(buffer.GetNumberOfBytes() * CHAR_BIT >= metadata->NumberOfBits);

  return metadata;
}

} // namespace detail

namespace internal
{

BitFieldMetaData::~BitFieldMetaData() {}

std::unique_ptr<vtkm::cont::internal::BufferMetaData> BitFieldMetaData::DeepCopy() const
{
  return std::unique_ptr<vtkm::cont::internal::BufferMetaData>(new BitFieldMetaData(*this));
}

} // namespace internal

BitField::BitField()
{
  this->Buffer.SetMetaData(internal::BitFieldMetaData{});
}

vtkm::Id BitField::GetNumberOfBits() const
{
  auto metadata = detail::GetBitFieldMetaData(this->Buffer);
  return metadata->NumberOfBits;
}

void BitField::Allocate(vtkm::Id numberOfBits,
                        vtkm::CopyFlag preserve,
                        vtkm::cont::Token& token) const
{
  const vtkm::BufferSizeType bytesNeeded = (numberOfBits + CHAR_BIT - 1) / CHAR_BIT;
  const vtkm::BufferSizeType blocksNeeded = (bytesNeeded + BlockSize - 1) / BlockSize;
  const vtkm::BufferSizeType numBytes = blocksNeeded * BlockSize;

  VTKM_LOG_F(vtkm::cont::LogLevel::MemCont,
             "BitField Allocation: %llu bits, blocked up to %s bytes.",
             static_cast<unsigned long long>(numberOfBits),
             vtkm::cont::GetSizeString(static_cast<vtkm::UInt64>(numBytes)).c_str());

  this->Buffer.SetNumberOfBytes(numBytes, preserve, token);
  detail::GetBitFieldMetaData(this->Buffer)->NumberOfBits = numberOfBits;
}

void BitField::ReleaseResourcesExecution()
{
  this->Buffer.ReleaseDeviceResources();
}

void BitField::ReleaseResources()
{
  vtkm::cont::Token token;
  this->Buffer.SetNumberOfBytes(0, vtkm::CopyFlag::Off, token);
  detail::GetBitFieldMetaData(this->Buffer)->NumberOfBits = 0;
}

void BitField::SyncControlArray() const
{
  vtkm::cont::Token token;
  this->Buffer.ReadPointerHost(token);
}

bool BitField::IsOnDevice(vtkm::cont::DeviceAdapterId device) const
{
  return this->Buffer.IsAllocatedOnDevice(device);
}

vtkm::cont::DeviceAdapterId BitField::GetDeviceAdapterId() const
{
  DeviceCheckFunctor functor;
  vtkm::ListForEach(functor, VTKM_DEFAULT_DEVICE_ADAPTER_LIST{}, this->Buffer);
  return functor.FoundDevice;
}

BitField::WritePortalType BitField::WritePortal() const
{
  vtkm::cont::Token token;
  return WritePortalType(this->Buffer.WritePointerHost(token),
                         detail::GetBitFieldMetaData(this->Buffer)->NumberOfBits);
}

BitField::ReadPortalType BitField::ReadPortal() const
{
  vtkm::cont::Token token;
  return ReadPortalType(this->Buffer.ReadPointerHost(token),
                        detail::GetBitFieldMetaData(this->Buffer)->NumberOfBits);
}

BitField::ReadPortalType BitField::PrepareForInput(vtkm::cont::DeviceAdapterId device,
                                                   vtkm::cont::Token& token) const
{
  return ReadPortalType(this->Buffer.ReadPointerDevice(device, token),
                        detail::GetBitFieldMetaData(this->Buffer)->NumberOfBits);
}

BitField::WritePortalType BitField::PrepareForOutput(vtkm::Id numBits,
                                                     vtkm::cont::DeviceAdapterId device,
                                                     vtkm::cont::Token& token) const
{
  this->Allocate(numBits, vtkm::CopyFlag::Off, token);
  return WritePortalType(this->Buffer.WritePointerDevice(device, token),
                         detail::GetBitFieldMetaData(this->Buffer)->NumberOfBits);
}

BitField::WritePortalType BitField::PrepareForInPlace(vtkm::cont::DeviceAdapterId device,
                                                      vtkm::cont::Token& token) const
{
  return WritePortalType(this->Buffer.WritePointerDevice(device, token),
                         detail::GetBitFieldMetaData(this->Buffer)->NumberOfBits);
}

}
} // namespace vtkm::cont
