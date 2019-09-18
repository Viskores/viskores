//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/testing/Testing.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/arg/BasicArg.h>
#include <vtkm/exec/arg/ThreadIndicesBasic.h>
#include <vtkm/exec/cuda/internal/TaskStrided.h>

#include <vtkm/StaticAssert.h>

#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/internal/Invocation.h>

#if defined(VTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4068) //unknown pragma
#endif

#if defined(__NVCC__) && defined(__CUDACC_VER_MAJOR__)
// Disable warning "declared but never referenced"
// This file produces several false-positive warnings
// Eg: TestExecObject::TestExecObject, MyOutputToInputMapPortal::Get,
//     TestWorkletProxy::operator()
#pragma push
#pragma diag_suppress 177
#endif

namespace
{

struct TestExecObject
{
  VTKM_EXEC_CONT
  TestExecObject(vtkm::exec::cuda::internal::ArrayPortalFromThrust<vtkm::Id> portal)
    : Portal(portal)
  {
  }

  vtkm::exec::cuda::internal::ArrayPortalFromThrust<vtkm::Id> Portal;
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
  VTKM_EXEC_CONT
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
    return execObject.Portal.Get(indices.GetInputIndex()) + 10 * indices.GetInputIndex();
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
    execObject.Portal.Set(indices.GetOutputIndex(), value + 20 * indices.GetOutputIndex());
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

template <typename TaskType>
static __global__ void ScheduleTaskStrided(TaskType task, vtkm::Id start, vtkm::Id end)
{

  const vtkm::Id index = blockIdx.x * blockDim.x + threadIdx.x;
  const vtkm::Id inc = blockDim.x * gridDim.x;
  if (index >= start && index < end)
  {
    task(index, end, inc);
  }
}

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
    vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }
};

#define ERROR_MESSAGE "Expected worklet error."

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletErrorProxy : vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void operator()(vtkm::Id, vtkm::Id) const { this->RaiseError(ERROR_MESSAGE); }

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
    vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }
};

// Check behavior of InvocationToFetch helper class.

VTKM_STATIC_ASSERT(
  (std::is_same<vtkm::exec::internal::detail::
                  InvocationToFetch<vtkm::exec::arg::ThreadIndicesBasic, InvocationType1, 1>::type,
                vtkm::exec::arg::Fetch<TestFetchTagInput,
                                       vtkm::exec::arg::AspectTagDefault,
                                       vtkm::exec::arg::ThreadIndicesBasic,
                                       TestExecObject>>::type::value));

VTKM_STATIC_ASSERT(
  (std::is_same<vtkm::exec::internal::detail::
                  InvocationToFetch<vtkm::exec::arg::ThreadIndicesBasic, InvocationType1, 2>::type,
                vtkm::exec::arg::Fetch<TestFetchTagOutput,
                                       vtkm::exec::arg::AspectTagDefault,
                                       vtkm::exec::arg::ThreadIndicesBasic,
                                       TestExecObject>>::type::value));

VTKM_STATIC_ASSERT(
  (std::is_same<vtkm::exec::internal::detail::
                  InvocationToFetch<vtkm::exec::arg::ThreadIndicesBasic, InvocationType2, 0>::type,
                vtkm::exec::arg::Fetch<TestFetchTagOutput,
                                       vtkm::exec::arg::AspectTagDefault,
                                       vtkm::exec::arg::ThreadIndicesBasic,
                                       TestExecObject>>::type::value));

template <typename DeviceAdapter>
void TestNormalFunctorInvoke()
{
  std::cout << "Testing normal worklet invoke." << std::endl;

  vtkm::Id inputTestValues[3] = { 5, 5, 6 };

  vtkm::cont::ArrayHandle<vtkm::Id> input = vtkm::cont::make_ArrayHandle(inputTestValues, 3);
  vtkm::cont::ArrayHandle<vtkm::Id> output;

  vtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    vtkm::internal::make_FunctionInterface<void>(
      TestExecObject(input.PrepareForInPlace(DeviceAdapter())),
      TestExecObject(output.PrepareForOutput(3, DeviceAdapter())));

  std::cout << "  Try void return." << std::endl;
  TestWorkletProxy worklet;
  InvocationType1 invocation1(execObjects);

  using TaskTypes = typename vtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation1, vtkm::Id());

  ScheduleTaskStrided<decltype(task1)><<<32, 256>>>(task1, 1, 2);
  cudaDeviceSynchronize();
  input.SyncControlArray();
  output.SyncControlArray();

  VTKM_TEST_ASSERT(inputTestValues[1] == 5, "Input value changed.");
  VTKM_TEST_ASSERT(output.GetPortalConstControl().Get(1) == inputTestValues[1] + 100 + 30,
                   "Output value not set right.");

  std::cout << "  Try return value." << std::endl;

  execObjects = vtkm::internal::make_FunctionInterface<void>(
    TestExecObject(input.PrepareForInPlace(DeviceAdapter())),
    TestExecObject(output.PrepareForOutput(3, DeviceAdapter())));

  InvocationType2 invocation2(execObjects);

  using TaskTypes = typename vtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task2 = TaskTypes::MakeTask(worklet, invocation2, vtkm::Id());

  ScheduleTaskStrided<decltype(task2)><<<32, 256>>>(task2, 2, 3);
  cudaDeviceSynchronize();
  input.SyncControlArray();
  output.SyncControlArray();

  VTKM_TEST_ASSERT(inputTestValues[2] == 6, "Input value changed.");
  VTKM_TEST_ASSERT(output.GetPortalConstControl().Get(2) == inputTestValues[2] + 200 + 30 * 2,
                   "Output value not set right.");
}

template <typename DeviceAdapter>
void TestErrorFunctorInvoke()
{
  std::cout << "Testing invoke with an error raised in the worklet." << std::endl;

  vtkm::Id inputTestValue = 5;
  vtkm::Id outputTestValue = static_cast<vtkm::Id>(0xDEADDEAD);

  vtkm::cont::ArrayHandle<vtkm::Id> input = vtkm::cont::make_ArrayHandle(&inputTestValue, 1);
  vtkm::cont::ArrayHandle<vtkm::Id> output = vtkm::cont::make_ArrayHandle(&outputTestValue, 1);

  vtkm::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    vtkm::internal::make_FunctionInterface<void>(
      TestExecObject(input.PrepareForInPlace(DeviceAdapter())),
      TestExecObject(output.PrepareForInPlace(DeviceAdapter())));

  using TaskStrided1 =
    vtkm::exec::cuda::internal::TaskStrided1D<TestWorkletErrorProxy, InvocationType1>;
  TestWorkletErrorProxy worklet;
  InvocationType1 invocation(execObjects);

  using TaskTypes = typename vtkm::cont::DeviceTaskTypes<DeviceAdapter>;
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  auto task = TaskTypes::MakeTask(worklet, invocation, vtkm::Id());

  auto errorArray = Algorithm::GetPinnedErrorArray();
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(errorArray.DevicePtr, errorArray.Size);
  task.SetErrorMessageBuffer(errorMessage);

  ScheduleTaskStrided<decltype(task)><<<32, 256>>>(task, 1, 2);
  cudaDeviceSynchronize();

  VTKM_TEST_ASSERT(errorMessage.IsErrorRaised(), "Error not raised correctly.");
  VTKM_TEST_ASSERT(errorArray.HostPtr == std::string(ERROR_MESSAGE), "Got wrong error message.");
}

template <typename DeviceAdapter>
void TestTaskStrided()
{
  TestNormalFunctorInvoke<DeviceAdapter>();
  TestErrorFunctorInvoke<DeviceAdapter>();
}

} // anonymous namespace

int UnitTestTaskStrided(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestTaskStrided<vtkm::cont::DeviceAdapterTagCuda>, argc, argv);
}

#if defined(__NVCC__) && defined(__CUDACC_VER_MAJOR__)
#pragma pop
#endif

#if defined(VTKM_MSVC)
#pragma warning(pop)
#endif
