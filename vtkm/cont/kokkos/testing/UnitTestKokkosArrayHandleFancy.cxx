//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/kokkos/DeviceAdapterKokkos.h>
#include <vtkm/cont/testing/TestingFancyArrayHandles.h>

int UnitTestKokkosArrayHandleFancy(int argc, char* argv[])
{
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(vtkm::cont::DeviceAdapterTagKokkos{});
  return vtkm::cont::testing::TestingFancyArrayHandles<vtkm::cont::DeviceAdapterTagKokkos>::Run(
    argc, argv);
}
