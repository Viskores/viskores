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

// This test makes sure that the algorithms specified in
// DeviceAdapterAlgorithmGeneral.h are working correctly. It does this by
// creating a test device adapter that uses the serial device adapter for the
// base schedule/scan/sort algorithms and using the general algorithms for
// everything else. Because this test is based of the serial device adapter,
// make sure that UnitTestDeviceAdapterSerial is working before trying to debug
// this one.

#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_ERROR

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/TestingDeviceAdapter.h>

VTKM_VALID_DEVICE_ADAPTER(TestAlgorithmGeneral, -3);

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

namespace internal
{

template <typename T, class StorageTag>
class ArrayManagerExecution<T, StorageTag, vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
  : public vtkm::cont::internal::ArrayManagerExecution<T,
                                                       StorageTag,
                                                       vtkm::cont::DeviceAdapterTagSerial>
{
public:
  using Superclass =
    vtkm::cont::internal::ArrayManagerExecution<T, StorageTag, vtkm::cont::DeviceAdapterTagSerial>;
  using ValueType = typename Superclass::ValueType;
  using PortalType = typename Superclass::PortalType;
  using PortalConstType = typename Superclass::PortalConstType;

  ArrayManagerExecution(vtkm::cont::internal::Storage<T, StorageTag>* storage)
    : Superclass(storage)
  {
  }
};

template <typename VirtualObject, typename TargetClass>
struct VirtualObjectTransfer<VirtualObject,
                             TargetClass,
                             vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>
  : public VirtualObjectTransferShareWithControl<VirtualObject, TargetClass>
{
};

template <typename T>
struct ExecutionPortalFactoryBasic<T, DeviceAdapterTagTestAlgorithmGeneral>
  : public ExecutionPortalFactoryBasicShareWithControl<T>
{
  using Superclass = ExecutionPortalFactoryBasicShareWithControl<T>;

  using typename Superclass::ValueType;
  using typename Superclass::PortalType;
  using typename Superclass::PortalConstType;
  using Superclass::CreatePortal;
  using Superclass::CreatePortalConst;
};

template <>
struct ExecutionArrayInterfaceBasic<DeviceAdapterTagTestAlgorithmGeneral>
  : public ExecutionArrayInterfaceBasicShareWithControl
{
  using Superclass = ExecutionArrayInterfaceBasicShareWithControl;

  VTKM_CONT
  ExecutionArrayInterfaceBasic(StorageBasicBase& storage)
    : Superclass(storage)
  {
  }

  VTKM_CONT
  virtual DeviceAdapterId GetDeviceId() const final { return -3; }
};
}
}
} // namespace vtkm::cont::internal

int UnitTestDeviceAdapterAlgorithmGeneral(int, char* [])
{
  return vtkm::cont::testing::TestingDeviceAdapter<
    vtkm::cont::DeviceAdapterTagTestAlgorithmGeneral>::Run();
}
