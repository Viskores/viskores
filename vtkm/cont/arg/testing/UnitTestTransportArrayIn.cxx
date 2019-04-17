//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/arg/TransportTagArrayIn.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename PortalType>
struct TestKernelIn : public vtkm::exec::FunctorBase
{
  PortalType Portal;

  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using ValueType = typename PortalType::ValueType;
    if (!test_equal(this->Portal.Get(index), TestValue(index, ValueType())))
    {
      this->RaiseError("Got bad execution object.");
    }
  }
};

template <typename Device>
struct TryArrayInType
{
  template <typename T>
  void operator()(T) const
  {
    T array[ARRAY_SIZE];
    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      array[index] = TestValue(index, T());
    }

    using ArrayHandleType = vtkm::cont::ArrayHandle<T>;
    ArrayHandleType handle = vtkm::cont::make_ArrayHandle(array, ARRAY_SIZE);

    using PortalType = typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst;

    vtkm::cont::arg::Transport<vtkm::cont::arg::TransportTagArrayIn, ArrayHandleType, Device>
      transport;

    TestKernelIn<PortalType> kernel;
    kernel.Portal = transport(handle, handle, ARRAY_SIZE, ARRAY_SIZE);

    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);
  }
};

template <typename Device>
void TryArrayInTransport(Device)
{
  vtkm::testing::Testing::TryTypes(TryArrayInType<Device>());
}

void TestArrayInTransport()
{
  std::cout << "Trying ArrayIn transport with serial device." << std::endl;
  TryArrayInTransport(vtkm::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportArrayIn(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayInTransport, argc, argv);
}
