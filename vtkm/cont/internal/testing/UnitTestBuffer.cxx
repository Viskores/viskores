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
      VTKM_TEST_ASSERT(reinterpret_cast<T*>(p) == &this->Data->front());
      this->Data.reset();
    }
  }
};


void DoTest()
{
  constexpr vtkm::Id BUFFER_SIZE = ARRAY_SIZE * static_cast<vtkm::Id>(sizeof(T));
  constexpr vtkm::cont::DeviceAdapterTagSerial device;

  std::cout << "Initialize buffer" << std::endl;
  vtkm::cont::internal::Buffer buffer;
  buffer.SetNumberOfBytes(BUFFER_SIZE, vtkm::CopyFlag::Off);

  VTKM_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE);

  std::cout << "Fill up values on host" << std::endl;
  {
    vtkm::cont::Token token;
    SetPortal(MakePortal(buffer.WritePointerHost(token), ARRAY_SIZE));
  }
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnHost());
  VTKM_TEST_ASSERT(!buffer.IsAllocatedOnDevice(device));

  std::cout << "Check values" << std::endl;
  {
    vtkm::cont::Token token;
    std::cout << "  Host" << std::endl;
    CheckPortal(MakePortal(buffer.ReadPointerHost(token), ARRAY_SIZE));
    std::cout << "  Device" << std::endl;
    CheckPortal(MakePortal(buffer.ReadPointerDevice(device, token), ARRAY_SIZE));
  }
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnHost());
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnDevice(device));

  std::cout << "Resize array and access write on device" << std::endl;
  buffer.SetNumberOfBytes(BUFFER_SIZE / 2, vtkm::CopyFlag::On);
  VTKM_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE / 2);
  {
    vtkm::cont::Token token;
    CheckPortal(MakePortal(buffer.WritePointerDevice(device, token), ARRAY_SIZE / 2));
  }
  VTKM_TEST_ASSERT(!buffer.IsAllocatedOnHost());
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnDevice(device));

  std::cout << "Resize array and access write on host" << std::endl;
  // Note that this is a weird corner case where the array was resized while saving the data
  // and then requested on another device.
  buffer.SetNumberOfBytes(BUFFER_SIZE * 2, vtkm::CopyFlag::On);
  VTKM_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE * 2);
  {
    vtkm::cont::Token token;
    // Although the array is twice ARRAY_SIZE, the valid values are only ARRAY_SIZE/2
    CheckPortal(MakePortal(buffer.WritePointerHost(token), ARRAY_SIZE / 2));
  }
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnHost());
  VTKM_TEST_ASSERT(!buffer.IsAllocatedOnDevice(device));

  std::cout << "Reset with device data" << std::endl;
  VectorDeleter vectorDeleter(ARRAY_SIZE);
  void* devicePointer = &vectorDeleter.Data->front();
  SetPortal(MakePortal(devicePointer, ARRAY_SIZE));
  buffer.Reset(devicePointer, BUFFER_SIZE, std::move(vectorDeleter), device);
  VTKM_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE);
  VTKM_TEST_ASSERT(!buffer.IsAllocatedOnHost());
  VTKM_TEST_ASSERT(buffer.IsAllocatedOnDevice(device));

  std::cout << "Make sure device pointer is as expected" << std::endl;
  {
    vtkm::cont::Token token;
    VTKM_TEST_ASSERT(buffer.WritePointerDevice(device, token) == devicePointer);
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
