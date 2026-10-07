//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/tbb/DeviceAdapterTBB.h>
#include <viskores/cont/testing/TestingDeviceAdapter.h>

int UnitTestTBBDeviceAdapter(int argc, char* argv[])
{
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(viskores::cont::DeviceAdapterTagTBB{});
  return viskores::cont::testing::TestingDeviceAdapter<viskores::cont::DeviceAdapterTagTBB>::Run(
    argc, argv);
}
