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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/exec/internal/TaskSingular.h>

#include <vtkm/exec/arg/BasicArg.h>
#include <vtkm/exec/arg/ThreadIndicesBasic.h>
#include <vtkm/exec/FunctorBase.h>

#include <vtkm/StaticAssert.h>

#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/internal/Invocation.h>

#include <vtkm/testing/Testing.h>

namespace {

struct TestExecObject
{
  VTKM_EXEC_CONT
  TestExecObject() : Value(NULL) {  }

  VTKM_EXEC_CONT
  TestExecObject(vtkm::Id *value) : Value(value) {  }

  vtkm::Id *Value;
};

struct MyOutputToInputMapPortal
{
  typedef vtkm::Id ValueType;
  VTKM_EXEC_CONT
  vtkm::Id Get(vtkm::Id index) const { return index; }
};

struct MyVisitArrayPortal
{
  typedef vtkm::IdComponent ValueType;
  vtkm::IdComponent Get(vtkm::Id) const { return 1; }
};

struct TestFetchTagInput {  };
struct TestFetchTagOutput {  };

// Missing TransportTag, but we are not testing that so we can leave it out.
struct TestControlSignatureTagInput
{
  typedef TestFetchTagInput FetchTag;
};
struct TestControlSignatureTagOutput
{
  typedef TestFetchTagOutput FetchTag;
};

} // anonymous namespace

namespace vtkm {
namespace exec {
namespace arg {

template<>
struct Fetch<
    TestFetchTagInput,
    vtkm::exec::arg::AspectTagDefault,
    vtkm::exec::arg::ThreadIndicesBasic,
    TestExecObject>
{
  typedef vtkm::Id ValueType;

  VTKM_EXEC
  ValueType Load(const vtkm::exec::arg::ThreadIndicesBasic &indices,
                 const TestExecObject &execObject) const {
    return *execObject.Value + 10*indices.GetInputIndex();
  }

  VTKM_EXEC
  void Store(const vtkm::exec::arg::ThreadIndicesBasic &,
             const TestExecObject &,
             ValueType) const {
    // No-op
  }
};

template<>
struct Fetch<
    TestFetchTagOutput,
    vtkm::exec::arg::AspectTagDefault,
    vtkm::exec::arg::ThreadIndicesBasic,
    TestExecObject>
{
  typedef vtkm::Id ValueType;

  VTKM_EXEC
  ValueType Load(const vtkm::exec::arg::ThreadIndicesBasic &,
                 const TestExecObject &) const {
    // No-op
    return ValueType();
  }

  VTKM_EXEC
  void Store(const vtkm::exec::arg::ThreadIndicesBasic &indices,
             const TestExecObject &execObject,
             ValueType value) const {
    *execObject.Value = value + 20*indices.GetOutputIndex();
  }
};

}
}
} // vtkm::exec::arg

namespace {

typedef void TestControlSignature(TestControlSignatureTagInput,
                                  TestControlSignatureTagOutput);
typedef vtkm::internal::FunctionInterface<TestControlSignature>
    TestControlInterface;

typedef void TestExecutionSignature1(vtkm::exec::arg::BasicArg<1>,
                                     vtkm::exec::arg::BasicArg<2>);
typedef vtkm::internal::FunctionInterface<TestExecutionSignature1>
    TestExecutionInterface1;

typedef vtkm::exec::arg::BasicArg<2> TestExecutionSignature2(
    vtkm::exec::arg::BasicArg<1>);
typedef vtkm::internal::FunctionInterface<TestExecutionSignature2>
    TestExecutionInterface2;

typedef vtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)>
    ExecutionParameterInterface;

typedef vtkm::internal::Invocation<
    ExecutionParameterInterface,
    TestControlInterface,
    TestExecutionInterface1,
    1,
    MyOutputToInputMapPortal,
    MyVisitArrayPortal> InvocationType1;

typedef vtkm::internal::Invocation<
    ExecutionParameterInterface,
    TestControlInterface,
    TestExecutionInterface2,
    1,
    MyOutputToInputMapPortal,
    MyVisitArrayPortal> InvocationType2;

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletProxy : vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void operator()(vtkm::Id input, vtkm::Id &output) const
  {
    output = input + 100;
  }

  VTKM_EXEC
  vtkm::Id operator()(vtkm::Id input) const
  {
    return input + 200;
  }

  template<typename T, typename OutToInArrayType, typename VisitArrayType,
           typename InputDomainType, typename G>
  VTKM_EXEC
  vtkm::exec::arg::ThreadIndicesBasic
  GetThreadIndices(const T& threadIndex,
                   const OutToInArrayType& outToIn,
                   const VisitArrayType& visit,
                   const InputDomainType &,
                   const G& globalThreadIndexOffset) const
  {
    return vtkm::exec::arg::ThreadIndicesBasic(threadIndex,
                                               outToIn.Get(threadIndex),
                                               visit.Get(threadIndex),
                                               globalThreadIndexOffset );
  }
};

#define ERROR_MESSAGE "Expected worklet error."

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletErrorProxy : vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void operator()(vtkm::Id, vtkm::Id) const
  {
    this->RaiseError(ERROR_MESSAGE);
  }

  template<typename T, typename OutToInArrayType, typename VisitArrayType,
           typename InputDomainType, typename G>
  VTKM_EXEC
  vtkm::exec::arg::ThreadIndicesBasic
  GetThreadIndices(const T& threadIndex,
                   const OutToInArrayType& outToIn,
                   const VisitArrayType& visit,
                   const InputDomainType &,
                   const G& globalThreadIndexOffset) const
  {
    return vtkm::exec::arg::ThreadIndicesBasic(threadIndex,
                                               outToIn.Get(threadIndex),
                                               visit.Get(threadIndex),
                                               globalThreadIndexOffset );
  }
};

// Check behavior of InvocationToFetch helper class.

VTKM_STATIC_ASSERT(( std::is_same<
                        vtkm::exec::internal::detail::InvocationToFetch<vtkm::exec::arg::ThreadIndicesBasic,InvocationType1,1>::type,
                        vtkm::exec::arg::Fetch<TestFetchTagInput,vtkm::exec::arg::AspectTagDefault,vtkm::exec::arg::ThreadIndicesBasic,TestExecObject> >::type::value ));

VTKM_STATIC_ASSERT(( std::is_same<
                        vtkm::exec::internal::detail::InvocationToFetch<vtkm::exec::arg::ThreadIndicesBasic,InvocationType1,2>::type,
                        vtkm::exec::arg::Fetch<TestFetchTagOutput,vtkm::exec::arg::AspectTagDefault,vtkm::exec::arg::ThreadIndicesBasic,TestExecObject> >::type::value ));

VTKM_STATIC_ASSERT(( std::is_same<
                        vtkm::exec::internal::detail::InvocationToFetch<vtkm::exec::arg::ThreadIndicesBasic,InvocationType2,0>::type,
                        vtkm::exec::arg::Fetch<TestFetchTagOutput,vtkm::exec::arg::AspectTagDefault,vtkm::exec::arg::ThreadIndicesBasic,TestExecObject> >::type::value ));


void TestNormalFunctorInvoke()
{
  std::cout << "Testing normal worklet invoke." << std::endl;

  vtkm::Id inputTestValue;
  vtkm::Id outputTestValue;
  vtkm::internal::FunctionInterface<void(TestExecObject,TestExecObject)> execObjects =
      vtkm::internal::make_FunctionInterface<void>(TestExecObject(&inputTestValue),
                                                   TestExecObject(&outputTestValue));

  std::cout << "  Try void return." << std::endl;
  inputTestValue = 5;
  outputTestValue = static_cast<vtkm::Id>(0xDEADDEAD);
  typedef vtkm::exec::internal::TaskSingular<TestWorkletProxy,InvocationType1> TaskSingular1;
  TaskSingular1 taskInvokeWorklet1 =
      TaskSingular1(TestWorkletProxy(), InvocationType1(execObjects));
  taskInvokeWorklet1(1);
  VTKM_TEST_ASSERT(inputTestValue == 5, "Input value changed.");
  VTKM_TEST_ASSERT(outputTestValue == inputTestValue + 100 + 30,
                   "Output value not set right.");

  std::cout << "  Try return value." << std::endl;
  inputTestValue = 6;
  outputTestValue = static_cast<vtkm::Id>(0xDEADDEAD);
  typedef vtkm::exec::internal::TaskSingular<TestWorkletProxy,InvocationType2> TaskSingular2;
  TaskSingular2 taskInvokeWorklet2 =
      TaskSingular2(TestWorkletProxy(), InvocationType2(execObjects));
  taskInvokeWorklet2(2);
  VTKM_TEST_ASSERT(inputTestValue == 6, "Input value changed.");
  VTKM_TEST_ASSERT(outputTestValue == inputTestValue + 200 + 30*2,
                   "Output value not set right.");
}

void TestErrorFunctorInvoke()
{
  std::cout << "Testing invoke with an error raised in the worklet." << std::endl;

  vtkm::Id inputTestValue = 5;
  vtkm::Id outputTestValue = static_cast<vtkm::Id>(0xDEADDEAD);
  vtkm::internal::FunctionInterface<void(TestExecObject,TestExecObject)> execObjects =
      vtkm::internal::make_FunctionInterface<void>(TestExecObject(&inputTestValue),
                                                   TestExecObject(&outputTestValue));

  typedef vtkm::exec::internal::TaskSingular<TestWorkletErrorProxy,InvocationType1> TaskSingular1;
  TaskSingular1 taskInvokeWorklet1 =
      TaskSingular1(TestWorkletErrorProxy(), InvocationType1(execObjects));
  char message[1024];
  message[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(message, 1024);
  taskInvokeWorklet1.SetErrorMessageBuffer(errorMessage);
  taskInvokeWorklet1(1);

  VTKM_TEST_ASSERT(errorMessage.IsErrorRaised(), "Error not raised correctly.");
  VTKM_TEST_ASSERT(message == std::string(ERROR_MESSAGE),
                   "Got wrong error message.");
}

void TestTaskSingular()
{
  TestNormalFunctorInvoke();
  TestErrorFunctorInvoke();
}

} // anonymous namespace

int UnitTestTaskSingular(int, char *[])
{
  return vtkm::testing::Testing::Run(TestTaskSingular);
}
