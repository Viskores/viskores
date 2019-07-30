//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/testing/TestingArrayHandleMultiplexer.h>

int UnitTestCudaArrayHandleMultiplexer(int argc, char* argv[])
{
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(vtkm::cont::DeviceAdapterTagCuda{});
  return vtkm::cont::testing::TestingArrayHandleMultiplexer<vtkm::cont::DeviceAdapterTagCuda>::Run(
    argc, argv);
}
