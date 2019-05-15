//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>
#include <vtkm/cont/testing/TestingBitField.h>

int UnitTestTBBBitField(int argc, char* argv[])
{
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(vtkm::cont::DeviceAdapterTagTBB{});
  return vtkm::cont::testing::TestingBitField<vtkm::cont::DeviceAdapterTagTBB>::Run(argc, argv);
}
