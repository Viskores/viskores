//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
