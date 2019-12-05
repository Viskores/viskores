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

  tracker.ForceDevice(vtkm::cont::DeviceAdapterTagTBB{});
  using DeviceAdapterList = vtkm::List<vtkm::cont::DeviceAdapterTagTBB>;
  vtkm::cont::testing::TestingVirtualObjectHandle<DeviceAdapterList>::Run();

  tracker.Reset();
  using DeviceAdapterList2 =
    vtkm::List<vtkm::cont::DeviceAdapterTagSerial, vtkm::cont::DeviceAdapterTagTBB>;
  vtkm::cont::testing::TestingVirtualObjectHandle<DeviceAdapterList2>::Run();
}


} // anonymous namespace

int UnitTestTBBVirtualObjectHandle(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestVirtualObjectHandle, argc, argv);
}
