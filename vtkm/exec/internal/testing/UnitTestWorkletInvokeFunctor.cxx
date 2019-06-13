//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/internal/TaskSingular.h>

#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/arg/BasicArg.h>
#include <vtkm/exec/arg/ThreadIndicesBasic.h>

#include <vtkm/StaticAssert.h>

#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/internal/Invocation.h>

#include <vtkm/testing/Testing.h>

namespace
{

struct TestExecObject
{
  VTKM_EXEC_CONT
  TestExecObject()
    : Value(nullptr)
  {
  }

  VTKM_EXEC_CONT
  TestExecObject(vtkm::Id* value)
    : Value(value)
  {
  }

  vtkm::Id* Value;
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

} // anonymous namespace

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
  using ValueType = vtkm::Id;

  VTKM_EXEC
  ValueType Load(const vtkm::exec::arg::ThreadIndicesBasic& indices,
                 const TestExecObject& execObject) const
  {
    return *execObject.Value + 10 * indices.GetInputIndex();
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
    *execObject.Value = value + 20 * indices.GetOutputIndex();
  }
};
}
}
} // vtkm::exec::arg

namespace
{

using TestControlSignature = void(TestControlSignatureTagInput, TestControlSignatureTagOutput);
using TestControlInterface = vtkm::internal::FunctionInterface<TestControlSignature>;

using TestExecutionSignature1 = void(vtkm::exec::arg::BasicArg<1>, vtkm::exec::arg::BasicArg<2>);
using TestExecutionInterface1 = vtkm::internal::FunctionInterface<TestExecutionSignature1>;

using TestExecutionSignature2 = vtkm::exec::arg::BasicArg<2>(vtkm::exec::arg::BasicArg<1>);
using TestExecutionInterface2 = vtkm::internal::FunctionInterface<TestExecutionSignature2>;

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletProxy : vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void operator()(vtkm::Id input, vtkm::Id& output) const { output = input + 100; }

  VTKM_EXEC
  vtkm::Id operator()(vtkm::Id input) const { return input + 200; }

  template <typename T,
            typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            typename G>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const T& threadIndex,
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
};

template <typename Invocation>
void CallDoWorkletInvokeFunctor(const Invocation& invocation, vtkm::Id index)
{
  const vtkm::Id outputIndex = invocation.ThreadToOutputMap.Get(index);
  vtkm::exec::internal::detail::DoWorkletInvokeFunctor(
    TestWorkletProxy(),
    invocation,
    vtkm::exec::arg::ThreadIndicesBasic(index,
                                        invocation.OutputToInputMap.Get(outputIndex),
                                        invocation.VisitArray.Get(outputIndex),
                                        outputIndex));
}

void TestDoWorkletInvoke()
{
  std::cout << "Testing internal worklet invoke." << std::endl;

  vtkm::Id inputTestValue;
  vtkm::Id outputTestValue;
  vtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    vtkm::internal::make_FunctionInterface<void>(TestExecObject(&inputTestValue),
                                                 TestExecObject(&outputTestValue));

  std::cout << "  Try void return." << std::endl;
  inputTestValue = 5;
  outputTestValue = static_cast<vtkm::Id>(0xDEADDEAD);
  CallDoWorkletInvokeFunctor(vtkm::internal::make_Invocation<1>(execObjects,
                                                                TestControlInterface(),
                                                                TestExecutionInterface1(),
                                                                MyOutputToInputMapPortal(),
                                                                MyVisitArrayPortal(),
                                                                MyThreadToOutputMapPortal()),
                             1);
  VTKM_TEST_ASSERT(inputTestValue == 5, "Input value changed.");
  VTKM_TEST_ASSERT(outputTestValue == inputTestValue + 100 + 30, "Output value not set right.");

  std::cout << "  Try return value." << std::endl;
  inputTestValue = 6;
  outputTestValue = static_cast<vtkm::Id>(0xDEADDEAD);
  CallDoWorkletInvokeFunctor(vtkm::internal::make_Invocation<1>(execObjects,
                                                                TestControlInterface(),
                                                                TestExecutionInterface2(),
                                                                MyOutputToInputMapPortal(),
                                                                MyVisitArrayPortal(),
                                                                MyThreadToOutputMapPortal()),
                             2);
  VTKM_TEST_ASSERT(inputTestValue == 6, "Input value changed.");
  VTKM_TEST_ASSERT(outputTestValue == inputTestValue + 200 + 30 * 2, "Output value not set right.");
}

void TestWorkletInvokeFunctor()
{
  TestDoWorkletInvoke();
}

} // anonymous namespace

int UnitTestWorkletInvokeFunctor(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestWorkletInvokeFunctor, argc, argv);
}
