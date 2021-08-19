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
#include <vtkm/cont/kokkos/DeviceAdapterKokkos.h>
#include <vtkm/cont/testing/TestingRuntimeDeviceConfiguration.h>

int UnitTestKokkosRuntimeDeviceConfiguration(int argc, char* argv[])
{
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagKokkos{});
  return vtkm::cont::testing::TestingRuntimeDeviceConfiguration<
    vtkm::cont::DeviceAdapterTagKokkos>::Run(argc, argv);
}
