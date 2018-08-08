//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/RuntimeDeviceInformation.h>

//include all backends
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template <bool>
struct DoesExist;

template <typename DeviceAdapterTag>
void detect_if_exists(DeviceAdapterTag tag)
{
  using DeviceAdapterTraits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
  std::cout << "testing runtime support for " << DeviceAdapterTraits::GetName() << std::endl;
  DoesExist<tag.IsEnabled> exist;
  exist.Exist(tag);
}

template <>
struct DoesExist<false>
{
  template <typename DeviceAdapterTag>
  void Exist(DeviceAdapterTag) const
  {

    //runtime information for this device should return false
    vtkm::cont::RuntimeDeviceInformation<DeviceAdapterTag> runtime;
    VTKM_TEST_ASSERT(runtime.Exists() == false,
                     "A backend with zero compile time support, can't have runtime support");
  }

  void Exist(vtkm::cont::DeviceAdapterTagCuda) const
  {
    //Since we are in a C++ compilation unit the Device Adapter
    //trait should be false. But CUDA could still be enabled.
    //That is why we check VTKM_ENABLE_CUDA.
    vtkm::cont::RuntimeDeviceInformation<vtkm::cont::DeviceAdapterTagCuda> runtime;
#ifdef VTKM_ENABLE_CUDA
    VTKM_TEST_ASSERT(runtime.Exists() == true,
                     "with cuda backend enabled, runtime support should be enabled");
#else
    VTKM_TEST_ASSERT(runtime.Exists() == false,
                     "with cuda backend disabled, runtime support should be disabled");
#endif
  }
};

template <>
struct DoesExist<true>
{
  template <typename DeviceAdapterTag>
  void Exist(DeviceAdapterTag) const
  {
    //runtime information for this device should return true
    vtkm::cont::RuntimeDeviceInformation<DeviceAdapterTag> runtime;
    VTKM_TEST_ASSERT(runtime.Exists() == true,
                     "A backend with compile time support, should have runtime support");
  }
};

void Detection()
{
  using SerialTag = ::vtkm::cont::DeviceAdapterTagSerial;
  using TBBTag = ::vtkm::cont::DeviceAdapterTagTBB;
  using CudaTag = ::vtkm::cont::DeviceAdapterTagCuda;

  //Verify that for each device adapter we compile code for, that it
  //has valid runtime support.
  detect_if_exists(CudaTag());
  detect_if_exists(TBBTag());
  detect_if_exists(SerialTag());
}

} // anonymous namespace

int UnitTestRuntimeDeviceInformation(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(Detection);
}
