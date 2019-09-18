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
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/cuda/internal/testing/Testing.h>
#include <vtkm/cont/testing/TestingDeviceAdapter.h>

int UnitTestCudaDeviceAdapter(int argc, char* argv[])
{
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(vtkm::cont::DeviceAdapterTagCuda{});
  int result =
    vtkm::cont::testing::TestingDeviceAdapter<vtkm::cont::DeviceAdapterTagCuda>::Run(argc, argv);
  return vtkm::cont::cuda::internal::Testing::CheckCudaBeforeExit(result);
}
