//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/interop/anari/ANARILoadDevice.h>

#include <viskores/testing/Testing.h>

#include <cstdlib>
#include <string>

namespace
{

void TestExternalDeviceLifetime()
{
  const char* libraryName = std::getenv("VISKORES_TEST_ANARI_LIBRARY");
  if ((libraryName == nullptr) || (std::string(libraryName) == "viskores"))
  {
    VISKORES_TEST_SKIP("Set VISKORES_TEST_ANARI_LIBRARY to the name of an external ANARI library.");
  }

  auto loadedDevice = viskores::interop::anari::ANARILoadDevice(libraryName);
  VISKORES_TEST_ASSERT(static_cast<bool>(loadedDevice),
                       "Failed to load the requested external ANARI device.");

  auto device = loadedDevice.GetDevice();
  viskores::Int32 version = -1;
  VISKORES_TEST_ASSERT(anari_cpp::getProperty(device, device, "version", version),
                       "Failed to query the loaded ANARI device.");

  auto world = anari_cpp::newObject<anari_cpp::World>(device);
  VISKORES_TEST_ASSERT(world != nullptr, "Failed to create an object on the loaded ANARI device.");
  anari_cpp::release(device, world);
}

} // anonymous namespace

int UnitTestANARIExternalDevice(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestExternalDeviceLifetime, argc, argv);
}
