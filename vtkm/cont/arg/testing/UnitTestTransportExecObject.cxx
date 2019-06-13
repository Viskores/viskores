//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/arg/TransportTagExecObject.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/ExecutionObjectBase.h>

#include <vtkm/cont/testing/Testing.h>

#define EXPECTED_NUMBER 42

namespace
{

struct NotAnExecutionObject
{
};
struct InvalidExecutionObject : vtkm::cont::ExecutionObjectBase
{
};

template <typename Device>
struct ExecutionObject
{
  vtkm::Int32 Number;
};

struct TestExecutionObject : public vtkm::cont::ExecutionObjectBase
{
  vtkm::Int32 Number;

  template <typename Device>
  VTKM_CONT ExecutionObject<Device> PrepareForExecution(Device) const
  {
    ExecutionObject<Device> object;
    object.Number = this->Number;
    return object;
  }
};

template <typename Device>
struct TestKernel : public vtkm::exec::FunctorBase
{
  ExecutionObject<Device> Object;

  VTKM_EXEC
  void operator()(vtkm::Id) const
  {
    if (this->Object.Number != EXPECTED_NUMBER)
    {
      this->RaiseError("Got bad execution object.");
    }
  }
};

template <typename Device>
void TryExecObjectTransport(Device)
{
  TestExecutionObject contObject;
  contObject.Number = EXPECTED_NUMBER;

  vtkm::cont::arg::Transport<vtkm::cont::arg::TransportTagExecObject, TestExecutionObject, Device>
    transport;

  TestKernel<Device> kernel;
  kernel.Object = transport(contObject, nullptr, 1, 1);

  vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, 1);
}

void TestExecObjectTransport()
{
  std::cout << "Checking ExecObject queries." << std::endl;
  VTKM_TEST_ASSERT(!vtkm::cont::internal::IsExecutionObjectBase<NotAnExecutionObject>::value,
                   "Bad query");
  VTKM_TEST_ASSERT(vtkm::cont::internal::IsExecutionObjectBase<InvalidExecutionObject>::value,
                   "Bad query");
  VTKM_TEST_ASSERT(vtkm::cont::internal::IsExecutionObjectBase<TestExecutionObject>::value,
                   "Bad query");

  VTKM_TEST_ASSERT(!vtkm::cont::internal::HasPrepareForExecution<NotAnExecutionObject>::value,
                   "Bad query");
  VTKM_TEST_ASSERT(!vtkm::cont::internal::HasPrepareForExecution<InvalidExecutionObject>::value,
                   "Bad query");
  VTKM_TEST_ASSERT(vtkm::cont::internal::HasPrepareForExecution<TestExecutionObject>::value,
                   "Bad query");

  std::cout << "Trying ExecObject transport with serial device." << std::endl;
  TryExecObjectTransport(vtkm::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportExecObject(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestExecObjectTransport, argc, argv);
}
