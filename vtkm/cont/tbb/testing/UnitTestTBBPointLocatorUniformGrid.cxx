//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/TestingPointLocatorUniformGrid.h>

int UnitTestTBBPointLocatorUniformGrid(int argc, char* argv[])
{
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(vtkm::cont::DeviceAdapterTagTBB{});
  return vtkm::cont::testing::Testing::Run(
    TestingPointLocatorUniformGrid<vtkm::cont::DeviceAdapterTagTBB>(), argc, argv);
}
