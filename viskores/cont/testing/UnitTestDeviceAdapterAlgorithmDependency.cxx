//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


// This tests a previous problem where code templated on the device adapter and
// used one of the device adapter algorithms (for example, the dispatcher) had
// to be declared after any device adapter it was ever used with.
#include <viskores/cont/DeviceAdapter.h>

#include <viskores/cont/testing/Testing.h>

#include <viskores/cont/ArrayHandle.h>

// Important for this test!
//This file must be included after ArrayHandle.h
#include <viskores/cont/serial/DeviceAdapterSerial.h>

namespace
{

struct ExampleWorklet
{
  template <typename T>
  void operator()(T viskoresNotUsed(v)) const
  {
  }
};

void CheckPostDefinedDeviceAdapter()
{
  // Nothing to really check. If this compiles, then the test is probably
  // successful.
  viskores::cont::ArrayHandle<viskores::Id> test;
  (void)test;
}

} // anonymous namespace

int UnitTestDeviceAdapterAlgorithmDependency(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(CheckPostDefinedDeviceAdapter, argc, argv);
}
