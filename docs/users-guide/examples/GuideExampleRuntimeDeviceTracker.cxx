//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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

#include <viskores/cont/ArrayCopy.h>

#include <viskores/cont/DeviceAdapterTag.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

static const viskores::Id ARRAY_SIZE = 10;

void CopyWithRuntime()
{
  std::cout << "Checking runtime in copy." << std::endl;

  using T = viskores::Float32;
  viskores::cont::ArrayHandle<T> srcArray;
  srcArray.Allocate(ARRAY_SIZE);
  SetPortal(srcArray.WritePortal());

  viskores::cont::ArrayHandle<T> destArray;

  ////
  //// BEGIN-EXAMPLE RestrictCopyDevice
  ////
  viskores::cont::ScopedRuntimeDeviceTracker tracker(
    viskores::cont::DeviceAdapterTagKokkos(),
    viskores::cont::RuntimeDeviceTrackerMode::Disable);

  ////
  //// BEGIN-EXAMPLE ArrayCopy
  ////
  viskores::cont::ArrayCopy(srcArray, destArray);
  ////
  //// END-EXAMPLE ArrayCopy
  ////
  ////
  //// END-EXAMPLE RestrictCopyDevice
  ////

  VISKORES_TEST_ASSERT(destArray.GetNumberOfValues() == ARRAY_SIZE, "Bad array size.");
  CheckPortal(destArray.ReadPortal());
}

////
//// BEGIN-EXAMPLE ForceThreadLocalDevice
////
void ChangeDefaultRuntime()
{
  std::cout << "Checking changing default runtime." << std::endl;

  //// PAUSE-EXAMPLE
#ifdef VISKORES_ENABLE_KOKKOS
  //// RESUME-EXAMPLE
  ////
  //// BEGIN-EXAMPLE SpecifyDeviceAdapter
  ////
  viskores::cont::ScopedRuntimeDeviceTracker(viskores::cont::DeviceAdapterTagKokkos{});
  ////
  //// END-EXAMPLE SpecifyDeviceAdapter
  ////
  //// PAUSE-EXAMPLE
#endif //VISKORES_ENABLE_KOKKOS
  //// RESUME-EXAMPLE

  // Viskores operations limited to Kokkos devices here...

  // Devices restored as we leave scope.
}
////
//// END-EXAMPLE ForceThreadLocalDevice
////

void Run()
{
  CopyWithRuntime();
  ChangeDefaultRuntime();
}

} // anonymous namespace

int GuideExampleRuntimeDeviceTracker(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
