//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/internal/DispatcherBase.h>

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

struct TestExecObjectIn
{
  VTKM_EXEC_CONT
  TestExecObjectIn()
    : Array(nullptr)
  {
  }

  VTKM_EXEC_CONT
  TestExecObjectIn(const vtkm::Id* array)
    : Array(array)
  {
  }

  const vtkm::Id* Array;
};

struct TestExecObjectOut
{
  VTKM_EXEC_CONT
  TestExecObjectOut()
    : Array(nullptr)
  {
  }

  VTKM_EXEC_CONT
  TestExecObjectOut(vtkm::Id* array)
    : Array(array)
  {
  }

  vtkm::Id* Array;
};

template <typename Device>
struct ExecutionObject
{
  vtkm::Id Value;
};

struct TestExecObjectType : vtkm::cont::ExecutionObjectBase
{
  template <typename Functor, typename... Args>
  void CastAndCall(Functor f, Args&&... args) const
  {
    f(*this, std::forward<Args>(args)...);
  }
  template <typename Device>
  VTKM_CONT ExecutionObject<Device> PrepareForExecution(Device) const
  {
    ExecutionObject<Device> object;
    object.Value = this->Value;
    return object;
  }
  vtkm::Id Value;
};

struct TestExecObjectTypeBad
{ //this will fail as it doesn't inherit from vtkm::cont::ExecutionObjectBase
  template <typename Functor, typename... Args>
  void CastAndCall(Functor f, Args&&... args) const
  {
    f(*this, std::forward<Args>(args)...);
  }
};

struct TestTypeCheckTag
{
};
struct TestTransportTagIn
{
};
struct TestTransportTagOut
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
struct TypeCheck<TestTypeCheckTag, std::vector<vtkm::Id>>
{
  static constexpr bool value = true;
};

template <typename Device>
struct Transport<TestTransportTagIn, std::vector<vtkm::Id>, Device>
{
  using ExecObjectType = TestExecObjectIn;

  VTKM_CONT
  ExecObjectType operator()(const std::vector<vtkm::Id>& contData,
                            const std::vector<vtkm::Id>&,
                            vtkm::Id inputRange,
                            vtkm::Id outputRange) const
  {
    VTKM_TEST_ASSERT(inputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    VTKM_TEST_ASSERT(outputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    return ExecObjectType(contData.data());
  }
};

template <typename Device>
struct Transport<TestTransportTagOut, std::vector<vtkm::Id>, Device>
{
  using ExecObjectType = TestExecObjectOut;

  VTKM_CONT
  ExecObjectType operator()(const std::vector<vtkm::Id>& contData,
                            const std::vector<vtkm::Id>&,
                            vtkm::Id inputRange,
                            vtkm::Id outputRange) const
  {
    VTKM_TEST_ASSERT(inputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    VTKM_TEST_ASSERT(outputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    auto ptr = const_cast<vtkm::Id*>(contData.data());
    return ExecObjectType(ptr);
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
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};
template <>
struct DynamicTransformTraits<TestExecObjectTypeBad>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
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
             TestExecObjectIn>
{
  using ValueType = vtkm::Id;

  VTKM_EXEC
  ValueType Load(const vtkm::exec::arg::ThreadIndicesBasic indices,
                 const TestExecObjectIn& execObject) const
  {
    return execObject.Array[indices.GetInputIndex()];
  }

  VTKM_EXEC
  void Store(const vtkm::exec::arg::ThreadIndicesBasic, const TestExecObjectIn&, ValueType) const
  {
    // No-op
  }
};

template <>
struct Fetch<TestFetchTagOutput,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::arg::ThreadIndicesBasic,
             TestExecObjectOut>
{
  using ValueType = vtkm::Id;

  VTKM_EXEC
  ValueType Load(const vtkm::exec::arg::ThreadIndicesBasic&, const TestExecObjectOut&) const
  {
    // No-op
    return ValueType();
  }

  VTKM_EXEC
  void Store(const vtkm::exec::arg::ThreadIndicesBasic& indices,
             const TestExecObjectOut& execObject,
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

static constexpr vtkm::Id EXPECTED_EXEC_OBJECT_VALUE = 123;

class TestWorkletBase : public vtkm::worklet::internal::WorkletBase
{
public:
  struct TestIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = TestTypeCheckTag;
    using TransportTag = TestTransportTagIn;
    using FetchTag = TestFetchTagInput;
  };
  struct TestOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = TestTypeCheckTag;
    using TransportTag = TestTransportTagOut;
    using FetchTag = TestFetchTagOutput;
  };
};

class TestWorklet : public TestWorkletBase
{
public:
  using ControlSignature = void(TestIn, ExecObject, TestOut);
  using ExecutionSignature = _3(_1, _2, WorkIndex);

  template <typename ExecObjectType>
  VTKM_EXEC vtkm::Id operator()(vtkm::Id value, ExecObjectType execObject, vtkm::Id index) const
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
  using ControlSignature = void(TestIn, ExecObject, TestOut);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename ExecObjectType>
  VTKM_EXEC void operator()(vtkm::Id, ExecObjectType, vtkm::Id) const
  {
    this->RaiseError(ERROR_MESSAGE);
  }
};

template <typename WorkletType>
class TestDispatcher : public vtkm::worklet::internal::DispatcherBase<TestDispatcher<WorkletType>,
                                                                      WorkletType,
                                                                      TestWorkletBase>
{
  using Superclass = vtkm::worklet::internal::DispatcherBase<TestDispatcher<WorkletType>,
                                                             WorkletType,
                                                             TestWorkletBase>;
  using ScatterType = typename Superclass::ScatterType;

public:
  VTKM_CONT
  TestDispatcher(const WorkletType& worklet = WorkletType(),
                 const ScatterType& scatter = ScatterType())
    : Superclass(worklet, scatter)
  {
  }

  VTKM_CONT
  template <typename Invocation>
  void DoInvoke(Invocation&& invocation) const
  {
    std::cout << "In TestDispatcher::DoInvoke()" << std::endl;
    this->BasicInvoke(invocation, ARRAY_SIZE);
  }

private:
  WorkletType Worklet;
};

void TestBasicInvoke()
{
  std::cout << "Test basic invoke" << std::endl;
  std::cout << "  Set up data." << std::endl;
  std::vector<vtkm::Id> inputArray(ARRAY_SIZE);
  std::vector<vtkm::Id> outputArray(ARRAY_SIZE);
  TestExecObjectType execObject;
  execObject.Value = EXPECTED_EXEC_OBJECT_VALUE;

  std::size_t i = 0;
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++, i++)
  {
    inputArray[i] = TestValue(index, vtkm::Id());
    outputArray[i] = static_cast<vtkm::Id>(0xDEADDEAD);
  }

  std::cout << "  Create and run dispatcher." << std::endl;
  TestDispatcher<TestWorklet> dispatcher;
  dispatcher.Invoke(inputArray, execObject, &outputArray);

  std::cout << "  Check output of invoke." << std::endl;
  i = 0;
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++, i++)
  {
    VTKM_TEST_ASSERT(outputArray[i] == TestValue(index, vtkm::Id()) + 1000,
                     "Got bad value from testing.");
  }
}

void TestInvokeWithError()
{
  std::cout << "Test invoke with error raised" << std::endl;
  std::cout << "  Set up data." << std::endl;
  std::vector<vtkm::Id> inputArray(ARRAY_SIZE);
  std::vector<vtkm::Id> outputArray(ARRAY_SIZE);
  TestExecObjectType execObject;
  execObject.Value = EXPECTED_EXEC_OBJECT_VALUE;

  std::size_t i = 0;
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++, ++i)
  {
    inputArray[i] = TestValue(index, vtkm::Id());
    outputArray[i] = static_cast<vtkm::Id>(0xDEADDEAD);
  }

  try
  {
    std::cout << "  Create and run dispatcher that raises error." << std::endl;
    TestDispatcher<TestErrorWorklet> dispatcher;
    dispatcher.Invoke(&inputArray, execObject, outputArray);
    VTKM_TEST_FAIL("Exception not thrown.");
  }
  catch (vtkm::cont::ErrorExecution& error)
  {
    std::cout << "  Got expected exception." << std::endl;
    std::cout << "  Exception message: " << error.GetMessage() << std::endl;
    VTKM_TEST_ASSERT(error.GetMessage() == ERROR_MESSAGE, "Got unexpected error message.");
  }
}

void TestInvokeWithBadDynamicType()
{
  std::cout << "Test invoke with bad type" << std::endl;

  std::vector<vtkm::Id> inputArray(ARRAY_SIZE);
  std::vector<vtkm::Id> outputArray(ARRAY_SIZE);
  TestExecObjectTypeBad execObject;
  TestDispatcher<TestWorklet> dispatcher;

  try
  {
    std::cout << "  Second argument bad." << std::endl;
    dispatcher.Invoke(inputArray, execObject, outputArray);
    VTKM_TEST_FAIL("Dispatcher did not throw expected error.");
  }
  catch (vtkm::cont::ErrorBadType& error)
  {
    std::cout << "    Got expected exception." << std::endl;
    std::cout << "    " << error.GetMessage() << std::endl;
    VTKM_TEST_ASSERT(error.GetMessage().find(" 2 ") != std::string::npos,
                     "Parameter index not named in error message.");
  }
}

void TestDispatcherBase()
{
  TestBasicInvoke();
  TestInvokeWithError();
  TestInvokeWithBadDynamicType();
}

} // anonymous namespace

int UnitTestDispatcherBase(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestDispatcherBase, argc, argv);
}
