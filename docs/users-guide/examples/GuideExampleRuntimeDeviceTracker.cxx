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

#include <vtkm/cont/ArrayCopy.h>

#include <vtkm/cont/DeviceAdapterTag.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static const vtkm::Id ARRAY_SIZE = 10;

void CopyWithRuntime()
{
  std::cout << "Checking runtime in copy." << std::endl;

  using T = vtkm::Float32;
  vtkm::cont::ArrayHandle<T> srcArray;
  srcArray.Allocate(ARRAY_SIZE);
  SetPortal(srcArray.WritePortal());

  vtkm::cont::ArrayHandle<T> destArray;

  ////
  //// BEGIN-EXAMPLE RestrictCopyDevice
  ////
  vtkm::cont::ScopedRuntimeDeviceTracker tracker(
    vtkm::cont::DeviceAdapterTagKokkos(), vtkm::cont::RuntimeDeviceTrackerMode::Disable);

  ////
  //// BEGIN-EXAMPLE ArrayCopy
  ////
  vtkm::cont::ArrayCopy(srcArray, destArray);
  ////
  //// END-EXAMPLE ArrayCopy
  ////
  ////
  //// END-EXAMPLE RestrictCopyDevice
  ////

  VTKM_TEST_ASSERT(destArray.GetNumberOfValues() == ARRAY_SIZE, "Bad array size.");
  CheckPortal(destArray.ReadPortal());
}

////
//// BEGIN-EXAMPLE ForceThreadLocalDevice
////
void ChangeDefaultRuntime()
{
  std::cout << "Checking changing default runtime." << std::endl;

  //// PAUSE-EXAMPLE
#ifdef VTKM_ENABLE_KOKKOS
  //// RESUME-EXAMPLE
  ////
  //// BEGIN-EXAMPLE SpecifyDeviceAdapter
  ////
  vtkm::cont::ScopedRuntimeDeviceTracker(vtkm::cont::DeviceAdapterTagKokkos{});
  ////
  //// END-EXAMPLE SpecifyDeviceAdapter
  ////
  //// PAUSE-EXAMPLE
#endif //VTKM_ENABLE_KOKKOS
  //// RESUME-EXAMPLE

  // VTK-m operations limited to Kokkos devices here...

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
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
