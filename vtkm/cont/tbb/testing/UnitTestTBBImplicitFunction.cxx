//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/TestingImplicitFunction.h>

namespace
{

void TestImplicitFunctions()
{
  vtkm::cont::testing::TestingImplicitFunction testing;
  testing.Run(vtkm::cont::DeviceAdapterTagTBB());
}

} // anonymous namespace

int UnitTestTBBImplicitFunction(int argc, char* argv[])
{
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(vtkm::cont::DeviceAdapterTagTBB{});
  return vtkm::cont::testing::Testing::Run(TestImplicitFunctions, argc, argv);
}
