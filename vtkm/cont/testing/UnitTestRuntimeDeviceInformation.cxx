//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/RuntimeDeviceInformation.h>

//include all backends
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>
#include <vtkm/cont/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

template<bool> struct DoesExist;

template<typename DeviceAdapterTag>
void detect_if_exists(DeviceAdapterTag tag)
{
  typedef vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag> DeviceAdapterTraits;
  DoesExist<DeviceAdapterTraits::Valid>::Exist(tag);
}

template<>
struct DoesExist<false>
{
  template<typename DeviceAdapterTag>
  static void Exist(DeviceAdapterTag)
  {
    //runtime information for this device should return false
    vtkm::cont::RuntimeDeviceInformation<DeviceAdapterTag> runtime;
    VTKM_TEST_ASSERT(runtime.Exists() == false,
      "A backend with zero compile time support, can't have runtime support");
  }
};

template<>
struct DoesExist<true>
{
  template<typename DeviceAdapterTag>
  static void Exist(DeviceAdapterTag)
  {
    //runtime information for this device should return true
    vtkm::cont::RuntimeDeviceInformation<DeviceAdapterTag> runtime;
    VTKM_TEST_ASSERT(runtime.Exists() == true,
      "A backend with compile time support, should have runtime support");
  }
};

void Detection()
{
  typedef ::vtkm::cont::DeviceAdapterTagSerial SerialTag;
  typedef ::vtkm::cont::DeviceAdapterTagTBB TBBTag;
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;

  //Verify that for each device adapter we compile code for, that it
  //has valid runtime support.
  detect_if_exists(CudaTag());
  detect_if_exists(TBBTag());
  detect_if_exists(SerialTag());

}

} // anonymous namespace

int UnitTestRuntimeDeviceInformation(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(Detection);
}
