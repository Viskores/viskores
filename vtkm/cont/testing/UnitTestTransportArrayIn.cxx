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
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/TryExecute.h>

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
    ArrayHandleType handle = vtkm::cont::make_ArrayHandle(array, ARRAY_SIZE, vtkm::CopyFlag::Off);

    using PortalType = typename ArrayHandleType::ReadPortalType;

    vtkm::cont::arg::Transport<vtkm::cont::arg::TransportTagArrayIn, ArrayHandleType, Device>
      transport;

    vtkm::cont::Token token;

    TestKernelIn<PortalType> kernel;
    kernel.Portal = transport(handle, handle, ARRAY_SIZE, ARRAY_SIZE, token);

    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);
  }
};

template <typename Device>
bool TryArrayInTransport(Device device)
{
  std::cout << "Trying ArrayIn transport with " << device.GetName() << std::endl;
  vtkm::testing::Testing::TryTypes(TryArrayInType<Device>());
  return true;
}

void TestArrayInTransport()
{
  VTKM_TEST_ASSERT(vtkm::cont::TryExecute([](auto device) { return TryArrayInTransport(device); }));
}

} // Anonymous namespace

int UnitTestTransportArrayIn(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayInTransport, argc, argv);
}
