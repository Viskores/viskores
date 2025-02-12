//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/kokkos/DeviceAdapterKokkos.h>
#include <viskores/cont/testing/TestingDeviceAdapter.h>

int UnitTestKokkosDeviceAdapter(int argc, char* argv[])
{
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(viskores::cont::DeviceAdapterTagKokkos{});
  return viskores::cont::testing::TestingDeviceAdapter<viskores::cont::DeviceAdapterTagKokkos>::Run(argc,
                                                                                            argv);
}
