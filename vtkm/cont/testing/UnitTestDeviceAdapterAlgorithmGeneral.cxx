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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

// This test makes sure that the algorithms specified in
// DeviceAdapterAlgorithmGeneral.h are working correctly. It does this by
// creating a test device adapter that uses the serial device adapter for the
// base schedule/scan/sort algorithms and using the general algorithms for
// everything else. Because this test is based of the serial device adapter,
// make sure that UnitTestDeviceAdapterSerial is working before trying to debug
// this one.

#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_ERROR

#include <vtkm/cont/DeviceAdapterSerial.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>

#include <vtkm/cont/testing/TestingDeviceAdapter.h>

VTKM_CREATE_DEVICE_ADAPTER(TestAlgorithmGeneral);

namespace vtkm {
namespace cont {

template<>
struct DeviceAdapterAlgorithm<
           vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral> :
    vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
        DeviceAdapterAlgorithm<
                   vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>,
        vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
{
private:
  typedef vtkm::cont::DeviceAdapterAlgorithm<
      vtkm::cont::DeviceAdapterTagSerial> Algorithm;

  typedef vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral
            DeviceAdapterTagTestAlgorithmGeneral;

public:

  template<class Functor>
  VTKM_CONT_EXPORT static void Schedule(Functor functor,
                                        vtkm::Id numInstances)
  {
    Algorithm::Schedule(functor, numInstances);
  }

  template<class Functor>
  VTKM_CONT_EXPORT static void Schedule(Functor functor,
                                        vtkm::Id3 rangeMax)
  {
    Algorithm::Schedule(functor, rangeMax);
  }

  VTKM_CONT_EXPORT static void Synchronize()
  {
    Algorithm::Synchronize();
  }
};

namespace internal {

template <typename T, class StorageTag>
class ArrayManagerExecution
    <T, StorageTag, vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
    : public vtkm::cont::internal::ArrayManagerExecution
          <T, StorageTag, vtkm::cont::DeviceAdapterTagSerial>
{
public:
  typedef vtkm::cont::internal::ArrayManagerExecution
      <T, StorageTag, vtkm::cont::DeviceAdapterTagSerial>
      Superclass;
  typedef typename Superclass::ValueType ValueType;
  typedef typename Superclass::PortalType PortalType;
  typedef typename Superclass::PortalConstType PortalConstType;
};


}
}
} // namespace vtkm::cont::testing

int UnitTestDeviceAdapterAlgorithmGeneral(int, char *[])
{
  return vtkm::cont::testing::TestingDeviceAdapter
      <vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>::Run();
}
