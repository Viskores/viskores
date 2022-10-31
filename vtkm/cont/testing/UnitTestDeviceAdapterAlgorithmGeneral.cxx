//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// This test makes sure that the algorithms specified in
// DeviceAdapterAlgorithmGeneral.h are working correctly. It does this by
// creating a test device adapter that uses the serial device adapter for the
// base schedule/scan/sort algorithms and using the general algorithms for
// everything else. Because this test is based of the serial device adapter,
// make sure that UnitTestDeviceAdapterSerial is working before trying to debug
// this one.

// It's OK to compile this without the device compiler.
#ifndef VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG
#define VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG 1
#endif

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/TestingDeviceAdapter.h>

// Hijack the serial device id so that precompiled units (like memory management) still work.
VTKM_VALID_DEVICE_ADAPTER(TestAlgorithmGeneral, VTKM_DEVICE_ADAPTER_SERIAL);

namespace vtkm
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
  : vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>,
      vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
{
private:
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagSerial>;

  using DeviceAdapterTagTestAlgorithmGeneral = vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral;

public:
  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, vtkm::Id numInstances)
  {
    Algorithm::Schedule(functor, numInstances);
  }

  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, vtkm::Id3 rangeMax)
  {
    Algorithm::Schedule(functor, rangeMax);
  }

  VTKM_CONT static void Synchronize() { Algorithm::Synchronize(); }
};

template <>
class DeviceAdapterRuntimeDetector<vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
{
public:
  /// Returns true as the General Algorithm Device can always be used.
  VTKM_CONT bool Exists() const { return true; }
};


namespace internal
{

template <>
class DeviceAdapterMemoryManager<vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
  : public vtkm::cont::internal::DeviceAdapterMemoryManagerShared
{
  VTKM_CONT vtkm::cont::DeviceAdapterId GetDevice() const override
  {
    return vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral{};
  }
};

}
}
} // namespace vtkm::cont::internal

int UnitTestDeviceAdapterAlgorithmGeneral(int argc, char* argv[])
{
  //need to enable DeviceAdapterTagTestAlgorithmGeneral as it
  //is not part of the default set of devices
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  tracker.ResetDevice(vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral{});

  return vtkm::cont::testing::TestingDeviceAdapter<
    vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>::Run(argc, argv);
}
