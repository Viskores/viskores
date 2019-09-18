//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/arg/TransportTagArrayOut.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename PortalType>
struct TestKernelOut : public vtkm::exec::FunctorBase
{
  PortalType Portal;

  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    using ValueType = typename PortalType::ValueType;
    this->Portal.Set(index, TestValue(index, ValueType()));
  }
};

template <typename Device>
struct TryArrayOutType
{
  template <typename T>
  void operator()(T) const
  {
    using ArrayHandleType = vtkm::cont::ArrayHandle<T>;
    ArrayHandleType handle;

    using PortalType = typename ArrayHandleType::template ExecutionTypes<Device>::Portal;

    vtkm::cont::arg::Transport<vtkm::cont::arg::TransportTagArrayOut, ArrayHandleType, Device>
      transport;

    TestKernelOut<PortalType> kernel;
    kernel.Portal =
      transport(handle, vtkm::cont::ArrayHandleIndex(ARRAY_SIZE), ARRAY_SIZE, ARRAY_SIZE);

    VTKM_TEST_ASSERT(handle.GetNumberOfValues() == ARRAY_SIZE,
                     "ArrayOut transport did not allocate array correctly.");

    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);

    CheckPortal(handle.GetPortalConstControl());
  }
};

template <typename Device>
void TryArrayOutTransport(Device)
{
  vtkm::testing::Testing::TryTypes(TryArrayOutType<Device>());
}

void TestArrayOutTransport()
{
  std::cout << "Trying ArrayOut transport with serial device." << std::endl;
  TryArrayOutTransport(vtkm::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportArrayOut(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayOutTransport, argc, argv);
}
