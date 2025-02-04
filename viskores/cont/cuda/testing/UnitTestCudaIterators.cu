//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/ArrayHandle.h>

#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/cuda/DeviceAdapterCuda.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

// cuda portals created from basic array handles should produce raw device
// pointers with ArrayPortalToIterator (see ArrayPortalFromThrust).
void TestIteratorSpecialization()
{
  viskores::cont::ArrayHandle<int> handle;

  viskores::cont::Token token;

  auto outputPortal = handle.PrepareForOutput(1, viskores::cont::DeviceAdapterTagCuda{}, token);
  auto inputPortal = handle.PrepareForInput(viskores::cont::DeviceAdapterTagCuda{}, token);
  auto inPlacePortal = handle.PrepareForInPlace(viskores::cont::DeviceAdapterTagCuda{}, token);

  auto outputIter = viskores::cont::ArrayPortalToIteratorBegin(outputPortal);
  auto inputIter = viskores::cont::ArrayPortalToIteratorBegin(inputPortal);
  auto inPlaceIter = viskores::cont::ArrayPortalToIteratorBegin(inPlacePortal);

  (void)outputIter;
  (void)inputIter;
  (void)inPlaceIter;

  VISKORES_TEST_ASSERT(std::is_same<decltype(outputIter), int*>::value);
  VISKORES_TEST_ASSERT(std::is_same<decltype(inputIter), int const*>::value);
  VISKORES_TEST_ASSERT(std::is_same<decltype(inPlaceIter), int*>::value);
}

} // end anon namespace

int UnitTestCudaIterators(int argc, char* argv[])
{
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(viskores::cont::DeviceAdapterTagCuda{});
  return viskores::cont::testing::Testing::Run(TestIteratorSpecialization, argc, argv);
}
