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

#include <vtkm/worklet/internal/DispatcherBase.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

typedef vtkm::cont::DeviceAdapterTagSerial Device;

static const vtkm::Id ARRAY_SIZE = 10;

struct TestExecObject
{
  VTKM_EXEC_CONT
  TestExecObject()
    : Array(nullptr)
  {
  }

  VTKM_EXEC_CONT
  TestExecObject(vtkm::Id* array)
    : Array(array)
  {
  }

  vtkm::Id* Array;
};

struct TestExecObjectType : vtkm::exec::ExecutionObjectBase
{
  template <typename Functor>
  void CastAndCall(Functor f) const
  {
    f(*this);
  }
  vtkm::Id Value;
};

struct TestTypeCheckTag
{
};
struct TestTransportTag
{
};
struct TestFetchTagInput
{
};
struct TestFetchTagOutput
{
};

} // anonymous namespace

namespace vtkm
{
namespace cont
{
namespace arg
{

template <>
struct TypeCheck<TestTypeCheckTag, vtkm::Id*>
{
  static const bool value = true;
};

template <>
struct Transport<TestTransportTag, vtkm::Id*, Device>
{
  typedef TestExecObject ExecObjectType;

  VTKM_CONT
  ExecObjectType operator()(vtkm::Id* contData,
                            vtkm::Id*,
                            vtkm::Id inputRange,
                            vtkm::Id outputRange) const
  {
    VTKM_TEST_ASSERT(inputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    VTKM_TEST_ASSERT(outputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    return ExecObjectType(contData);
  }
};
}
}
} // namespace vtkm::cont::arg

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
struct DynamicTransformTraits<TestExecObjectType>
{
  typedef vtkm::cont::internal::DynamicTransformTagCastAndCall DynamicTag;
};
}
}
} // namespace vtkm::cont::internal

namespace vtkm
{
namespace exec
{
namespace arg
{

template <>
struct Fetch<TestFetchTagInput,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::arg::ThreadIndicesBasic,
             TestExecObject>
{
  typedef vtkm::Id ValueType;

  VTKM_EXEC
  ValueType Load(const vtkm::exec::arg::ThreadIndicesBasic indices,
                 const TestExecObject& execObject) const
  {
    return execObject.Array[indices.GetInputIndex()];
  }

  VTKM_EXEC
  void Store(const vtkm::exec::arg::ThreadIndicesBasic, const TestExecObject&, ValueType) const
  {
    // No-op
  }
};

template <>
struct Fetch<TestFetchTagOutput,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::arg::ThreadIndicesBasic,
             TestExecObject>
{
  typedef vtkm::Id ValueType;

  VTKM_EXEC
  ValueType Load(const vtkm::exec::arg::ThreadIndicesBasic&, const TestExecObject&) const
  {
    // No-op
    return ValueType();
  }

  VTKM_EXEC
  void Store(const vtkm::exec::arg::ThreadIndicesBasic& indices,
             const TestExecObject& execObject,
             ValueType value) const
  {
    execObject.Array[indices.GetOutputIndex()] = value;
  }
};
}
}
} // vtkm::exec::arg

namespace
{

static const vtkm::Id EXPECTED_EXEC_OBJECT_VALUE = 123;

class TestWorkletBase : public vtkm::worklet::internal::WorkletBase
{
public:
  struct TestIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    typedef TestTypeCheckTag TypeCheckTag;
    typedef TestTransportTag TransportTag;
    typedef TestFetchTagInput FetchTag;
  };
  struct TestOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    typedef TestTypeCheckTag TypeCheckTag;
    typedef TestTransportTag TransportTag;
    typedef TestFetchTagOutput FetchTag;
  };
};

class TestWorklet : public TestWorkletBase
{
public:
  typedef void ControlSignature(TestIn, ExecObject, TestOut);
  typedef _3 ExecutionSignature(_1, _2, WorkIndex);

  VTKM_EXEC
  vtkm::Id operator()(vtkm::Id value, TestExecObjectType execObject, vtkm::Id index) const
  {
    VTKM_TEST_ASSERT(value == TestValue(index, vtkm::Id()), "Got bad value in worklet.");
    VTKM_TEST_ASSERT(execObject.Value == EXPECTED_EXEC_OBJECT_VALUE,
                     "Got bad exec object in worklet.");
    return TestValue(index, vtkm::Id()) + 1000;
  }
};

#define ERROR_MESSAGE "Expected worklet error."

class TestErrorWorklet : public TestWorkletBase
{
public:
  typedef void ControlSignature(TestIn, ExecObject, TestOut);
  typedef void ExecutionSignature(_1, _2, _3);

  VTKM_EXEC
  void operator()(vtkm::Id, TestExecObjectType, vtkm::Id) const { this->RaiseError(ERROR_MESSAGE); }
};

template <typename WorkletType>
class TestDispatcher : public vtkm::worklet::internal::DispatcherBase<TestDispatcher<WorkletType>,
                                                                      WorkletType,
                                                                      TestWorkletBase>
{
  typedef vtkm::worklet::internal::DispatcherBase<TestDispatcher<WorkletType>,
                                                  WorkletType,
                                                  TestWorkletBase>
    Superclass;
  typedef vtkm::internal::FunctionInterface<void(vtkm::Id*, TestExecObjectType, vtkm::Id*)>
    ParameterInterface;
  typedef vtkm::internal::Invocation<ParameterInterface,
                                     typename Superclass::ControlInterface,
                                     typename Superclass::ExecutionInterface,
                                     1>
    Invocation;

public:
  VTKM_CONT
  TestDispatcher(const WorkletType& worklet = WorkletType())
    : Superclass(worklet)
  {
  }

  VTKM_CONT
  void DoInvoke(const Invocation& invocation) const
  {
    std::cout << "In TestDispatcher::DoInvoke()" << std::endl;
    this->BasicInvoke(invocation, ARRAY_SIZE, Device());
  }

private:
  WorkletType Worklet;
};

void TestBasicInvoke()
{
  std::cout << "Test basic invoke" << std::endl;
  std::cout << "  Set up data." << std::endl;
  vtkm::Id inputArray[ARRAY_SIZE];
  vtkm::Id outputArray[ARRAY_SIZE];
  TestExecObjectType execObject;
  execObject.Value = EXPECTED_EXEC_OBJECT_VALUE;

  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    inputArray[index] = TestValue(index, vtkm::Id());
    outputArray[index] = static_cast<vtkm::Id>(0xDEADDEAD);
  }

  std::cout << "  Create and run dispatcher." << std::endl;
  TestDispatcher<TestWorklet> dispatcher;
  dispatcher.Invoke(inputArray, execObject, outputArray);

  std::cout << "  Check output of invoke." << std::endl;
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    VTKM_TEST_ASSERT(outputArray[index] == TestValue(index, vtkm::Id()) + 1000,
                     "Got bad value from testing.");
  }
}

void TestInvokeWithError()
{
  std::cout << "Test invoke with error raised" << std::endl;
  std::cout << "  Set up data." << std::endl;
  vtkm::Id inputArray[ARRAY_SIZE];
  vtkm::Id outputArray[ARRAY_SIZE];
  TestExecObjectType execObject;
  execObject.Value = EXPECTED_EXEC_OBJECT_VALUE;

  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    inputArray[index] = TestValue(index, vtkm::Id());
    outputArray[index] = static_cast<vtkm::Id>(0xDEADDEAD);
  }

  try
  {
    std::cout << "  Create and run dispatcher that raises error." << std::endl;
    TestDispatcher<TestErrorWorklet> dispatcher;
    dispatcher.Invoke(inputArray, execObject, outputArray);
    VTKM_TEST_FAIL("Exception not thrown.");
  }
  catch (vtkm::cont::ErrorExecution& error)
  {
    std::cout << "  Got expected exception." << std::endl;
    VTKM_TEST_ASSERT(error.GetMessage() == ERROR_MESSAGE, "Got unexpected error message.");
  }
}

void TestInvokeWithDynamicAndBadTypes()
{
  std::cout << "Test invoke with bad type" << std::endl;

  vtkm::Id array[ARRAY_SIZE];
  TestExecObjectType execObject;
  execObject.Value = EXPECTED_EXEC_OBJECT_VALUE;
  TestDispatcher<TestWorklet> dispatcher;

  try
  {
    std::cout << "  First argument bad." << std::endl;
    dispatcher.Invoke(nullptr, execObject, array);
    VTKM_TEST_FAIL("Dispatcher did not throw expected error.");
  }
  catch (vtkm::cont::ErrorBadType& error)
  {
    std::cout << "    Got expected exception." << std::endl;
    std::cout << "    " << error.GetMessage() << std::endl;
    VTKM_TEST_ASSERT(error.GetMessage().find(" 1 ") != std::string::npos,
                     "Parameter index not named in error message.");
  }

  try
  {
    std::cout << "  Third argument bad." << std::endl;
    dispatcher.Invoke(array, execObject, nullptr);
    VTKM_TEST_FAIL("Dispatcher did not throw expected error.");
  }
  catch (vtkm::cont::ErrorBadType& error)
  {
    std::cout << "    Got expected exception." << std::endl;
    std::cout << "    " << error.GetMessage() << std::endl;
    VTKM_TEST_ASSERT(error.GetMessage().find(" 3 ") != std::string::npos,
                     "Parameter index not named in error message.");
  }
}

void TestDispatcherBase()
{
  TestBasicInvoke();
  TestInvokeWithError();
  TestInvokeWithDynamicAndBadTypes();
}

} // anonymous namespace

int UnitTestDispatcherBase(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestDispatcherBase);
}
