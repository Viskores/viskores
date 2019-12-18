//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

// cuda portals created from basic array handles should produce raw device
// pointers with ArrayPortalToIterator (see ArrayPortalFromThrust).
void TestIteratorSpecialization()
{
  vtkm::cont::ArrayHandle<int> handle;

  auto outputPortal = handle.PrepareForOutput(1, vtkm::cont::DeviceAdapterTagCuda{});
  auto inputPortal = handle.PrepareForInput(vtkm::cont::DeviceAdapterTagCuda{});
  auto inPlacePortal = handle.PrepareForInPlace(vtkm::cont::DeviceAdapterTagCuda{});

  auto outputIter = vtkm::cont::ArrayPortalToIteratorBegin(outputPortal);
  auto inputIter = vtkm::cont::ArrayPortalToIteratorBegin(inputPortal);
  auto inPlaceIter = vtkm::cont::ArrayPortalToIteratorBegin(inPlacePortal);

  (void)outputIter;
  (void)inputIter;
  (void)inPlaceIter;

  VTKM_TEST_ASSERT(std::is_same<decltype(outputIter), int*>::value);
  VTKM_TEST_ASSERT(std::is_same<decltype(inputIter), int const*>::value);
  VTKM_TEST_ASSERT(std::is_same<decltype(inPlaceIter), int*>::value);
}

} // end anon namespace

int UnitTestCudaIterators(int argc, char* argv[])
{
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(vtkm::cont::DeviceAdapterTagCuda{});
  return vtkm::cont::testing::Testing::Run(TestIteratorSpecialization, argc, argv);
}
