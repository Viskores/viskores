//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/internal/Buffer.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

using T = vtkm::FloatDefault;
constexpr vtkm::Id ARRAY_SIZE = 20;

using PortalType = vtkm::cont::internal::ArrayPortalFromIterators<T*>;
using PortalTypeConst = vtkm::cont::internal::ArrayPortalFromIterators<const T*>;

struct BufferMetaDataTest : vtkm::cont::internal::BufferMetaData
{
  vtkm::Id Value;

  std::unique_ptr<vtkm::cont::internal::BufferMetaData> DeepCopy() const override
  {
    return std::unique_ptr<vtkm::cont::internal::BufferMetaData>(new BufferMetaDataTest(*this));
  }
};

constexpr vtkm::Id METADATA_VALUE = 42;

bool CheckMetaData(const vtkm::cont::internal::Buffer& buffer)
{
  vtkm::cont::internal::BufferMetaData* generalMetaData = buffer.GetMetaData();
  if (!generalMetaData)
  {
    return false;
  }
  BufferMetaDataTest* metadata = dynamic_cast<BufferMetaDataTest*>(generalMetaData);
  if (!metadata)
  {
    return false;
  }

  return metadata->Value == METADATA_VALUE;
}

PortalType MakePortal(void* buffer, vtkm::Id numValues)
{
  return PortalType(static_cast<T*>(buffer),
                    static_cast<T*>(buffer) + static_cast<std::size_t>(numValues));
};

PortalTypeConst MakePortal(const void* buffer, vtkm::Id numValues)
{
  return PortalTypeConst(static_cast<const T*>(buffer),
                         static_cast<const T*>(buffer) + static_cast<std::size_t>(numValues));
};

void VectorDeleter(void* container)
{
  std::vector<T>* v = reinterpret_cast<std::vector<T>*>(container);
  delete v;
}

void VectorReallocator(void*& memory,
                       void*& container,
                       vtkm::BufferSizeType oldSize,
                       vtkm::BufferSizeType newSize)
{
  std::vector<T>* v = reinterpret_cast<std::vector<T>*>(container);
  VTKM_TEST_ASSERT(v->size() == static_cast<std::size_t>(oldSize));
  VTKM_TEST_ASSERT(v->empty() || (memory == v->data()));

  v->resize(static_cast<std::size_t>(newSize));
  memory = v->data();
}
struct VectorDeleter
{
  std::shared_ptr<std::vector<T>> Data;

  VectorDeleter(vtkm::Id numValues)
    : Data(new std::vector<T>(static_cast<size_t>(numValues)))
  {
  }

  template <typename U>
  void operator()(U* p)
  {
    if (this->Data)
    {
      VTKM_TEST_ASSERT(reinterpret_cast<T*>(p) == this->Data->data());
      this->Data.reset();
    }
  }
};


void DoTest()
{
  constexpr vtkm::Id BUFFER_SIZE = ARRAY_SIZE * static_cast<vtkm::Id>(sizeof(T));
  constexpr vtkm::cont::DeviceAdapterTagSerial device;

  vtkm::cont::internal::Buffer buffer;

  {
    BufferMetaDataTest metadata;
    metadata.Value = METADATA_VALUE;
    buffer.SetMetaData(metadata);
    VTKM_TEST_ASSERT(CheckMetaData(buffer));
  }

  std::cout << "Copy uninitialized buffer" << std::endl;
  {
    vtkm::cont::internal::Buffer copy;
    copy.DeepCopyFrom(buffer);
    VTKM_TEST_ASSERT(copy.GetNumberOfBytes() == 0);
    VTKM_TEST_ASSERT(CheckMetaData(copy));
  }

  std::cout << "Initialize buffer" << std::endl;
  {
    vtkm::cont::Token token;
    buffer.SetNumberOfBytes(BUFFER_SIZE, vtkm::CopyFlag::Off, token);
  }

  VTKM_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE);

  std::cout << "Copy sized but uninitialized buffer" << std::endl;
  {
    vtkm::cont::internal::Buffer copy;
    copy.DeepCopyFrom(buffer);
    VTKM_TEST_ASSERT(copy.GetNumberOfBytes() == BUFFER_SIZE);
    VTKM_TEST_ASSERT(CheckMetaData(copy));
    VTKM_TEST_ASSERT(!copy.IsAllocatedOnHost());
    VTKM_TEST_ASSERT(!copy.IsAllocatedOnDevice(device));
  }

  std::cout << "Fill up values on host" << std::endl;
  {
    vtkm::cont::Token token;
    SetPortal(MakePortal(buffer.WritePointerHost(token), ARRAY_SIZE));
  }
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnHost());
  VTKM_TEST_ASSERT(!buffer.IsAllocatedOnDevice(device));

  std::cout << "Check values on host" << std::endl;
  {
    vtkm::cont::Token token;
    CheckPortal(MakePortal(buffer.ReadPointerHost(token), ARRAY_SIZE));
  }
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnHost());
  VTKM_TEST_ASSERT(!buffer.IsAllocatedOnDevice(device));

  std::cout << "Copy buffer with host data" << std::endl;
  {
    vtkm::cont::Token token;
    vtkm::cont::internal::Buffer copy;
    copy.DeepCopyFrom(buffer);
    VTKM_TEST_ASSERT(copy.GetNumberOfBytes() == BUFFER_SIZE);
    VTKM_TEST_ASSERT(CheckMetaData(copy));
    VTKM_TEST_ASSERT(copy.IsAllocatedOnHost());
    VTKM_TEST_ASSERT(!copy.IsAllocatedOnDevice(device));
    CheckPortal(MakePortal(buffer.ReadPointerHost(token), ARRAY_SIZE));
  }

  std::cout << "Check values on device" << std::endl;
  {
    vtkm::cont::Token token;
    CheckPortal(MakePortal(buffer.ReadPointerDevice(device, token), ARRAY_SIZE));
  }
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnHost());
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnDevice(device));

  std::cout << "Resize array and access write on device" << std::endl;
  {
    vtkm::cont::Token token;
    buffer.SetNumberOfBytes(BUFFER_SIZE / 2, vtkm::CopyFlag::On, token);
    VTKM_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE / 2);
    CheckPortal(MakePortal(buffer.WritePointerDevice(device, token), ARRAY_SIZE / 2));
  }
  VTKM_TEST_ASSERT(!buffer.IsAllocatedOnHost());
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnDevice(device));

  std::cout << "Resize array and access write on host" << std::endl;
  // Note that this is a weird corner case where the array was resized while saving the data
  // and then requested on another device.
  {
    vtkm::cont::Token token;
    buffer.SetNumberOfBytes(BUFFER_SIZE * 2, vtkm::CopyFlag::On, token);
    VTKM_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE * 2);
    // Although the array is twice ARRAY_SIZE, the valid values are only ARRAY_SIZE/2
    CheckPortal(MakePortal(buffer.WritePointerHost(token), ARRAY_SIZE / 2));
  }
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnHost());
  VTKM_TEST_ASSERT(!buffer.IsAllocatedOnDevice(device));

  std::cout << "Reset with device data" << std::endl;
  std::vector<T> v(ARRAY_SIZE);
  void* devicePointer = v.data();
  SetPortal(MakePortal(devicePointer, ARRAY_SIZE));
  buffer.Reset(vtkm::cont::internal::BufferInfo(device,
                                                devicePointer,
                                                new std::vector<T>(std::move(v)),
                                                BUFFER_SIZE,
                                                VectorDeleter,
                                                VectorReallocator));
  VTKM_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE);
  VTKM_TEST_ASSERT(!buffer.IsAllocatedOnHost());
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnDevice(device));

  std::cout << "Make sure device pointer is as expected" << std::endl;
  {
    vtkm::cont::Token token;
    VTKM_TEST_ASSERT(buffer.WritePointerDevice(device, token) == devicePointer);
  }

  std::cout << "Copy buffer with device data" << std::endl;
  {
    vtkm::cont::Token token;
    vtkm::cont::internal::Buffer copy;
    copy.DeepCopyFrom(buffer);
    VTKM_TEST_ASSERT(copy.GetNumberOfBytes() == BUFFER_SIZE);
    VTKM_TEST_ASSERT(CheckMetaData(copy));
    VTKM_TEST_ASSERT(!copy.IsAllocatedOnHost());
    VTKM_TEST_ASSERT(copy.IsAllocatedOnDevice(device));
    CheckPortal(MakePortal(buffer.ReadPointerDevice(device, token), ARRAY_SIZE));
  }

  std::cout << "Pull data to host" << std::endl;
  {
    vtkm::cont::Token token;
    CheckPortal(MakePortal(buffer.ReadPointerHost(token), ARRAY_SIZE));
  }
}

} // anonymous namespace

int UnitTestBuffer(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
