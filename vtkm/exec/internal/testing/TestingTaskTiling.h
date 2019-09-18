//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/StaticAssert.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/testing/Testing.h>

#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/arg/BasicArg.h>
#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ThreadIndicesBasic.h>

#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/internal/Invocation.h>

#include <algorithm>
#include <vector>

namespace vtkm
{
namespace exec
{
namespace internal
{
namespace testing
{

struct TestExecObject
{
  VTKM_EXEC_CONT
  TestExecObject()
    : Values(nullptr)
  {
  }

  VTKM_EXEC_CONT
  TestExecObject(std::vector<vtkm::Id>& values)
    : Values(&values[0])
  {
  }

  VTKM_EXEC_CONT
  TestExecObject(const TestExecObject& other) { Values = other.Values; }

  vtkm::Id* Values;
};

struct MyOutputToInputMapPortal
{
  using ValueType = vtkm::Id;
  VTKM_EXEC_CONT
  vtkm::Id Get(vtkm::Id index) const { return index; }
};

struct MyVisitArrayPortal
{
  using ValueType = vtkm::IdComponent;
  vtkm::IdComponent Get(vtkm::Id) const { return 1; }
};

struct MyThreadToOutputMapPortal
{
  using ValueType = vtkm::Id;
  VTKM_EXEC_CONT
  vtkm::Id Get(vtkm::Id index) const { return index; }
};

struct TestFetchTagInput
{
};
struct TestFetchTagOutput
{
};

// Missing TransportTag, but we are not testing that so we can leave it out.
struct TestControlSignatureTagInput
{
  using FetchTag = TestFetchTagInput;
};
struct TestControlSignatureTagOutput
{
  using FetchTag = TestFetchTagOutput;
};
}
}
}
}

namespace vtkm
{
namespace exec
{
namespace arg
{

using namespace vtkm::exec::internal::testing;

template <>
struct Fetch<TestFetchTagInput,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::arg::ThreadIndicesBasic,
             TestExecObject>
{
  using ValueType = vtkm::Id;

  VTKM_EXEC
  ValueType Load(const vtkm::exec::arg::ThreadIndicesBasic& indices,
                 const TestExecObject& execObject) const
  {
    return execObject.Values[indices.GetInputIndex()] + 10 * indices.GetInputIndex();
  }

  VTKM_EXEC
  void Store(const vtkm::exec::arg::ThreadIndicesBasic&, const TestExecObject&, ValueType) const
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
  using ValueType = vtkm::Id;

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
    execObject.Values[indices.GetOutputIndex()] = value + 20 * indices.GetOutputIndex();
  }
};
}
}
} // vtkm::exec::arg

namespace vtkm
{
namespace exec
{
namespace internal
{
namespace testing
{

using TestControlSignature = void(TestControlSignatureTagInput, TestControlSignatureTagOutput);
using TestControlInterface = vtkm::internal::FunctionInterface<TestControlSignature>;

using TestExecutionSignature1 = void(vtkm::exec::arg::BasicArg<1>, vtkm::exec::arg::BasicArg<2>);
using TestExecutionInterface1 = vtkm::internal::FunctionInterface<TestExecutionSignature1>;

using TestExecutionSignature2 = vtkm::exec::arg::BasicArg<2>(vtkm::exec::arg::BasicArg<1>);
using TestExecutionInterface2 = vtkm::internal::FunctionInterface<TestExecutionSignature2>;

using ExecutionParameterInterface =
  vtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)>;

using InvocationType1 = vtkm::internal::Invocation<ExecutionParameterInterface,
                                                   TestControlInterface,
                                                   TestExecutionInterface1,
                                                   1,
                                                   MyOutputToInputMapPortal,
                                                   MyVisitArrayPortal,
                                                   MyThreadToOutputMapPortal>;

using InvocationType2 = vtkm::internal::Invocation<ExecutionParameterInterface,
                                                   TestControlInterface,
                                                   TestExecutionInterface2,
                                                   1,
                                                   MyOutputToInputMapPortal,
                                                   MyVisitArrayPortal,
                                                   MyThreadToOutputMapPortal>;

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletProxy : vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void operator()(vtkm::Id input, vtkm::Id& output) const { output = input + 100; }

  VTKM_EXEC
  vtkm::Id operator()(vtkm::Id input) const { return input + 200; }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutputArrayType,
            typename InputDomainType,
            typename G>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const vtkm::Id& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutputArrayType& threadToOut,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            typename G>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const vtkm::Id3& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    const vtkm::Id flatThreadIndex = vtkm::Dot(threadIndex, vtkm::Id3(1, 8, 64));
    const vtkm::Id outIndex = threadToOut.Get(flatThreadIndex);
    return vtkm::exec::arg::ThreadIndicesBasic(flatThreadIndex,
                                               outToIn.Get(outIndex),
                                               visit.Get(outIndex),
                                               outIndex,
                                               globalThreadIndexOffset);
  }
};

#define ERROR_MESSAGE "Expected worklet error."

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletErrorProxy : vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void operator()(vtkm::Id, vtkm::Id) const { this->RaiseError(ERROR_MESSAGE); }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            typename G>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const vtkm::Id& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutputArrayType,
            typename InputDomainType,
            typename G>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const vtkm::Id3& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutputArrayType& threadToOutput,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    const vtkm::Id index = vtkm::Dot(threadIndex, vtkm::Id3(1, 8, 64));
    const vtkm::Id outputIndex = threadToOutput.Get(index);
    return vtkm::exec::arg::ThreadIndicesBasic(index,
                                               outToIn.Get(outputIndex),
                                               visit.Get(outputIndex),
                                               outputIndex,
                                               globalThreadIndexOffset);
  }
};

template <typename DeviceAdapter>
void Test1DNormalTaskTilingInvoke()
{

  std::cout << "Testing TaskTiling1D." << std::endl;

  std::vector<vtkm::Id> inputTestValues(100, 5);
  std::vector<vtkm::Id> outputTestValues(100, static_cast<vtkm::Id>(0xDEADDEAD));
  vtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    vtkm::internal::make_FunctionInterface<void>(TestExecObject(inputTestValues),
                                                 TestExecObject(outputTestValues));

  std::cout << "  Try void return." << std::endl;
  TestWorkletProxy worklet;
  InvocationType1 invocation1(execObjects);

  using TaskTypes = typename vtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation1, vtkm::Id());

  vtkm::exec::internal::ErrorMessageBuffer errorMessage(nullptr, 0);
  task1.SetErrorMessageBuffer(errorMessage);

  task1(0, 90);
  task1(90, 99);
  task1(99, 100); //verify single value ranges work

  for (std::size_t i = 0; i < 100; ++i)
  {
    VTKM_TEST_ASSERT(inputTestValues[i] == 5, "Input value changed.");
    VTKM_TEST_ASSERT(outputTestValues[i] ==
                       inputTestValues[i] + 100 + (30 * static_cast<vtkm::Id>(i)),
                     "Output value not set right.");
  }

  std::cout << "  Try return value." << std::endl;
  std::fill(inputTestValues.begin(), inputTestValues.end(), 6);
  std::fill(outputTestValues.begin(), outputTestValues.end(), static_cast<vtkm::Id>(0xDEADDEAD));

  InvocationType2 invocation2(execObjects);

  using TaskTypes = typename vtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task2 = TaskTypes::MakeTask(worklet, invocation2, vtkm::Id());

  task2.SetErrorMessageBuffer(errorMessage);

  task2(0, 0); //verify zero value ranges work
  task2(0, 90);
  task2(90, 100);

  task2(0, 100); //verify that you can invoke worklets multiple times

  for (std::size_t i = 0; i < 100; ++i)
  {
    VTKM_TEST_ASSERT(inputTestValues[i] == 6, "Input value changed.");
    VTKM_TEST_ASSERT(outputTestValues[i] ==
                       inputTestValues[i] + 200 + (30 * static_cast<vtkm::Id>(i)),
                     "Output value not set right.");
  }
}

template <typename DeviceAdapter>
void Test1DErrorTaskTilingInvoke()
{

  std::cout << "Testing TaskTiling1D with an error raised in the worklet." << std::endl;

  std::vector<vtkm::Id> inputTestValues(100, 5);
  std::vector<vtkm::Id> outputTestValues(100, static_cast<vtkm::Id>(0xDEADDEAD));

  TestExecObject arg1(inputTestValues);
  TestExecObject arg2(outputTestValues);

  vtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    vtkm::internal::make_FunctionInterface<void>(arg1, arg2);

  TestWorkletErrorProxy worklet;
  InvocationType1 invocation(execObjects);

  using TaskTypes = typename vtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task = TaskTypes::MakeTask(worklet, invocation, vtkm::Id());

  char message[1024];
  message[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(message, 1024);
  task.SetErrorMessageBuffer(errorMessage);

  task(0, 100);

  VTKM_TEST_ASSERT(errorMessage.IsErrorRaised(), "Error not raised correctly.");
  VTKM_TEST_ASSERT(message == std::string(ERROR_MESSAGE), "Got wrong error message.");
}

template <typename DeviceAdapter>
void Test3DNormalTaskTilingInvoke()
{
  std::cout << "Testing TaskTiling3D." << std::endl;

  std::vector<vtkm::Id> inputTestValues((8 * 8 * 8), 5);
  std::vector<vtkm::Id> outputTestValues((8 * 8 * 8), static_cast<vtkm::Id>(0xDEADDEAD));
  vtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    vtkm::internal::make_FunctionInterface<void>(TestExecObject(inputTestValues),
                                                 TestExecObject(outputTestValues));

  std::cout << "  Try void return." << std::endl;

  TestWorkletProxy worklet;
  InvocationType1 invocation1(execObjects);

  using TaskTypes = typename vtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation1, vtkm::Id3());
  for (vtkm::Id k = 0; k < 8; ++k)
  {
    for (vtkm::Id j = 0; j < 8; j += 2)
    {
      //verify that order is not required
      task1(0, 8, j + 1, k);
      task1(0, 8, j, k);
    }
  }

  for (std::size_t i = 0; i < (8 * 8 * 8); ++i)
  {
    VTKM_TEST_ASSERT(inputTestValues[i] == 5, "Input value changed.");
    VTKM_TEST_ASSERT(outputTestValues[i] ==
                       inputTestValues[i] + 100 + (30 * static_cast<vtkm::Id>(i)),
                     "Output value not set right.");
  }

  std::cout << "  Try return value." << std::endl;
  std::fill(inputTestValues.begin(), inputTestValues.end(), 6);
  std::fill(outputTestValues.begin(), outputTestValues.end(), static_cast<vtkm::Id>(0xDEADDEAD));

  InvocationType2 invocation2(execObjects);
  using TaskTypes = typename vtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task2 = TaskTypes::MakeTask(worklet, invocation2, vtkm::Id3());

  //verify that linear order of values being processed is not presumed
  for (vtkm::Id i = 0; i < 8; ++i)
  {
    for (vtkm::Id j = 0; j < 8; ++j)
    {
      for (vtkm::Id k = 0; k < 8; ++k)
      {
        task2(i, i + 1, j, k);
      }
    }
  }

  for (std::size_t i = 0; i < (8 * 8 * 8); ++i)
  {
    VTKM_TEST_ASSERT(inputTestValues[i] == 6, "Input value changed.");
    VTKM_TEST_ASSERT(outputTestValues[i] ==
                       inputTestValues[i] + 200 + (30 * static_cast<vtkm::Id>(i)),
                     "Output value not set right.");
  }
}

template <typename DeviceAdapter>
void Test3DErrorTaskTilingInvoke()
{
  std::cout << "Testing TaskTiling3D with an error raised in the worklet." << std::endl;

  std::vector<vtkm::Id> inputTestValues((8 * 8 * 8), 5);
  std::vector<vtkm::Id> outputTestValues((8 * 8 * 8), static_cast<vtkm::Id>(0xDEADDEAD));
  vtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    vtkm::internal::make_FunctionInterface<void>(TestExecObject(inputTestValues),
                                                 TestExecObject(outputTestValues));

  TestWorkletErrorProxy worklet;
  InvocationType1 invocation(execObjects);

  using TaskTypes = typename vtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation, vtkm::Id3());

  char message[1024];
  message[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(message, 1024);
  task1.SetErrorMessageBuffer(errorMessage);

  for (vtkm::Id k = 0; k < 8; ++k)
  {
    for (vtkm::Id j = 0; j < 8; ++j)
    {
      task1(0, 8, j, k);
    }
  }

  VTKM_TEST_ASSERT(errorMessage.IsErrorRaised(), "Error not raised correctly.");
  VTKM_TEST_ASSERT(message == std::string(ERROR_MESSAGE), "Got wrong error message.");
}

template <typename DeviceAdapter>
void TestTaskTiling()
{
  Test1DNormalTaskTilingInvoke<DeviceAdapter>();
  Test1DErrorTaskTilingInvoke<DeviceAdapter>();

  Test3DNormalTaskTilingInvoke<DeviceAdapter>();
  Test3DErrorTaskTilingInvoke<DeviceAdapter>();
}
}
}
}
}
