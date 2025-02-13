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
#include <viskores/cont/serial/DeviceAdapterSerial.h>
#include <viskores/cont/testing/TestingDeviceAdapter.h>

int UnitTestSerialDeviceAdapter(int argc, char* argv[])
{
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(viskores::cont::DeviceAdapterTagSerial{});
  return viskores::cont::testing::TestingDeviceAdapter<viskores::cont::DeviceAdapterTagSerial>::Run(
    argc, argv);
}
