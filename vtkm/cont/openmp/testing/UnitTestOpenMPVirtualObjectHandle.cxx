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
#include <vtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <vtkm/cont/testing/TestingVirtualObjectHandle.h>

namespace
{

void TestVirtualObjectHandle()
{
  using DeviceAdapterList = vtkm::List<vtkm::cont::DeviceAdapterTagOpenMP>;
  using DeviceAdapterList2 =
    vtkm::List<vtkm::cont::DeviceAdapterTagSerial, vtkm::cont::DeviceAdapterTagOpenMP>;

  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();

  tracker.ForceDevice(vtkm::cont::DeviceAdapterTagOpenMP{});
  vtkm::cont::testing::TestingVirtualObjectHandle<DeviceAdapterList>::Run();

  tracker.Reset();
  vtkm::cont::testing::TestingVirtualObjectHandle<DeviceAdapterList2>::Run();
}

} // anonymous namespace

int UnitTestOpenMPVirtualObjectHandle(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestVirtualObjectHandle, argc, argv);
}
