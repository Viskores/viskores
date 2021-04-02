//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/testing/TestingVirtualObjectHandle.h>

namespace
{

void TestVirtualObjectHandle()
{
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();

  tracker.ForceDevice(vtkm::cont::DeviceAdapterTagKokkos{});
  using DeviceAdapterList = vtkm::List<vtkm::cont::DeviceAdapterTagKokkos>;
  vtkm::cont::testing::TestingVirtualObjectHandle<DeviceAdapterList>::Run();

  tracker.Reset();
  using DeviceAdapterList2 =
    vtkm::List<vtkm::cont::DeviceAdapterTagSerial, vtkm::cont::DeviceAdapterTagKokkos>;
  vtkm::cont::testing::TestingVirtualObjectHandle<DeviceAdapterList2>::Run();
}

} // anonymous namespace

int UnitTestKokkosVirtualObjectHandle(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestVirtualObjectHandle, argc, argv);
}
