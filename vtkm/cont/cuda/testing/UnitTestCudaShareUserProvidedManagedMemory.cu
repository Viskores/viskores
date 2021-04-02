//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/cuda/internal/testing/Testing.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/cuda/ErrorCuda.h>

#include <vtkm/cont/cuda/internal/CudaAllocator.h>
#include <vtkm/cont/cuda/internal/testing/Testing.h>

#include <cuda_runtime.h>

using vtkm::cont::cuda::internal::CudaAllocator;

namespace
{

template <typename ValueType>
ValueType* AllocateManagedPointer(vtkm::Id numValues)
{
  void* result;
  VTKM_CUDA_CALL(cudaMallocManaged(&result, static_cast<size_t>(numValues) * sizeof(ValueType)));
  // Some sanity checks:
  VTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(result),
                   "Allocated pointer is not a device pointer.");
  VTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(result), "Allocated pointer is not managed.");
  return static_cast<ValueType*>(result);
}

void DeallocateManagedPointer(void* ptr)
{
  VTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(ptr), "Pointer to delete is not device pointer.");
  VTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(ptr), "Pointer to delete is not managed.");
  VTKM_CUDA_CALL(cudaFree(ptr));
}

template <typename ValueType>
ValueType* AllocateDevicePointer(vtkm::Id numValues)
{
  void* result;
  VTKM_CUDA_CALL(cudaMalloc(&result, static_cast<size_t>(numValues) * sizeof(ValueType)));
  // Some sanity checks:
  VTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(result),
                   "Allocated pointer is not a device pointer.");
  VTKM_TEST_ASSERT(!CudaAllocator::IsManagedPointer(result), "Allocated pointer is managed.");
  return static_cast<ValueType*>(result);
}

void DeallocateDevicePointer(void* ptr)
{
  VTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(ptr),
                   "Pointer to delete is not a device pointer.");
  VTKM_TEST_ASSERT(!CudaAllocator::IsManagedPointer(ptr), "Pointer to delete is managed.");
  VTKM_CUDA_CALL(cudaFree(ptr));
}

template <typename ValueType>
vtkm::cont::ArrayHandle<ValueType> CreateArrayHandle(vtkm::Id numValues, bool managed)
{
  if (managed)
  {
    return vtkm::cont::ArrayHandleBasic<ValueType>(
      AllocateManagedPointer<ValueType>(numValues), numValues, [](void* ptr) {
        DeallocateManagedPointer(ptr);
      });
  }
  else
  {
    return vtkm::cont::ArrayHandleBasic<ValueType>(AllocateDevicePointer<ValueType>(numValues),
                                                   numValues,
                                                   vtkm::cont::DeviceAdapterTagCuda{},
                                                   [](void* ptr) { DeallocateDevicePointer(ptr); });
  }
}

template <typename ValueType>
void TestPrepareForInput(bool managed)
{
  vtkm::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  vtkm::cont::Token token;
  auto portal = handle.PrepareForInput(vtkm::cont::DeviceAdapterTagCuda(), token);
  const void* execArray = portal.GetIteratorBegin();
  VTKM_TEST_ASSERT(execArray != nullptr, "No execution array after PrepareForInput.");
  if (managed)
  {
    VTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray));
  }
  token.DetachFromAll();

  VTKM_TEST_ASSERT(handle.IsOnDevice(vtkm::cont::DeviceAdapterTagCuda{}),
                   "No execution array after PrepareForInput.");
  if (managed)
  {
    const void* contArray = handle.ReadPortal().GetIteratorBegin();
    VTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray), "Control array unmanaged.");
    VTKM_TEST_ASSERT(execArray == contArray, "PrepareForInput managed arrays not shared.");
  }
}

template <typename ValueType>
void TestPrepareForInPlace(bool managed)
{
  vtkm::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  vtkm::cont::Token token;
  auto portal = handle.PrepareForInPlace(vtkm::cont::DeviceAdapterTagCuda(), token);
  const void* execArray = portal.GetIteratorBegin();
  VTKM_TEST_ASSERT(execArray != nullptr, "No execution array after PrepareForInPlace.");
  if (managed)
  {
    VTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray));
  }
  token.DetachFromAll();

  VTKM_TEST_ASSERT(!handle.IsOnHost(), "Control array still exists after PrepareForInPlace.");
  VTKM_TEST_ASSERT(handle.IsOnDevice(vtkm::cont::DeviceAdapterTagCuda{}),
                   "No execution array after PrepareForInPlace.");
  if (managed)
  {
    const void* contArray = handle.ReadPortal().GetIteratorBegin();
    VTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray), "Control array unmanaged.");
    VTKM_TEST_ASSERT(execArray == contArray, "PrepareForInPlace managed arrays not shared.");
  }
}

template <typename ValueType>
void TestPrepareForOutput(bool managed)
{
  // Should reuse a managed control pointer if buffer is large enough.
  vtkm::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  vtkm::cont::Token token;
  auto portal = handle.PrepareForOutput(32, vtkm::cont::DeviceAdapterTagCuda(), token);
  const void* execArray = portal.GetIteratorBegin();
  VTKM_TEST_ASSERT(execArray != nullptr, "No execution array after PrepareForOutput.");
  if (managed)
  {
    VTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray));
  }
  token.DetachFromAll();

  VTKM_TEST_ASSERT(!handle.IsOnHost(), "Control array still exists after PrepareForOutput.");
  VTKM_TEST_ASSERT(handle.IsOnDevice(vtkm::cont::DeviceAdapterTagCuda{}),
                   "No execution array after PrepareForOutput.");
  if (managed)
  {
    const void* contArray = handle.ReadPortal().GetIteratorBegin();
    VTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray), "Control array unmanaged.");
    VTKM_TEST_ASSERT(execArray == contArray, "PrepareForOutput managed arrays not shared.");
  }
}

template <typename ValueType>
void TestReleaseResourcesExecution(bool managed)
{
  vtkm::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  vtkm::cont::Token token;
  auto portal = handle.PrepareForInput(vtkm::cont::DeviceAdapterTagCuda(), token);
  const void* origArray = portal.GetIteratorBegin();
  token.DetachFromAll();

  handle.ReleaseResourcesExecution();

  VTKM_TEST_ASSERT(handle.IsOnHost(),
                   "Control array does not exist after ReleaseResourcesExecution.");
  VTKM_TEST_ASSERT(!handle.IsOnDevice(vtkm::cont::DeviceAdapterTagCuda{}),
                   "Execution array still exists after ReleaseResourcesExecution.");

  if (managed)
  {
    const void* contArray = handle.ReadPortal().GetIteratorBegin();
    VTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray), "Control array unmanaged.");
    VTKM_TEST_ASSERT(origArray == contArray, "Managed arrays not shared.");
  }
}

template <typename ValueType>
void TestRoundTrip(bool managed)
{
  vtkm::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  const void* origExecArray;
  {
    vtkm::cont::Token token;
    auto portal = handle.PrepareForOutput(32, vtkm::cont::DeviceAdapterTagCuda(), token);
    origExecArray = portal.GetIteratorBegin();
  }

  VTKM_TEST_ASSERT(!handle.IsOnHost());
  VTKM_TEST_ASSERT(handle.IsOnDevice(vtkm::cont::DeviceAdapterTagCuda{}));

  const void* contArray;
  {
    auto portal = handle.WritePortal();
    contArray = portal.GetIteratorBegin();
  }

  VTKM_TEST_ASSERT(handle.IsOnHost());
  VTKM_TEST_ASSERT(!handle.IsOnDevice(vtkm::cont::DeviceAdapterTagCuda{}));
  if (managed)
  {
    VTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray));
    VTKM_TEST_ASSERT(contArray == origExecArray);
  }

  const void* execArray;
  {
    vtkm::cont::Token token;
    auto portal = handle.PrepareForInput(vtkm::cont::DeviceAdapterTagCuda(), token);
    execArray = portal.GetIteratorBegin();
  }

  VTKM_TEST_ASSERT(handle.IsOnHost());
  VTKM_TEST_ASSERT(handle.IsOnDevice(vtkm::cont::DeviceAdapterTagCuda{}));
  if (managed)
  {
    VTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray));
    VTKM_TEST_ASSERT(execArray == contArray);
  }
}

template <typename ValueType>
void DoTests()
{
  TestPrepareForInput<ValueType>(false);
  TestPrepareForInPlace<ValueType>(false);
  TestPrepareForOutput<ValueType>(false);
  TestReleaseResourcesExecution<ValueType>(false);
  TestRoundTrip<ValueType>(false);


  // If this device does not support managed memory, skip the managed tests.
  if (!CudaAllocator::UsingManagedMemory())
  {
    std::cerr << "Skipping some tests -- device does not support managed memory.\n";
  }
  else
  {
    TestPrepareForInput<ValueType>(true);
    TestPrepareForInPlace<ValueType>(true);
    TestPrepareForOutput<ValueType>(true);
    TestReleaseResourcesExecution<ValueType>(true);
    TestRoundTrip<ValueType>(true);
  }
}

struct ArgToTemplateType
{
  template <typename ValueType>
  void operator()(ValueType) const
  {
    DoTests<ValueType>();
  }
};

void Launch()
{
  using Types = vtkm::List<vtkm::UInt8,
                           vtkm::Vec<vtkm::UInt8, 3>,
                           vtkm::Float32,
                           vtkm::Vec<vtkm::Float32, 4>,
                           vtkm::Float64,
                           vtkm::Vec<vtkm::Float64, 4>>;
  vtkm::testing::Testing::TryTypes(ArgToTemplateType(), Types());
}

} // end anon namespace

int UnitTestCudaShareUserProvidedManagedMemory(int argc, char* argv[])
{
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(vtkm::cont::DeviceAdapterTagCuda{});
  int ret = vtkm::cont::testing::Testing::Run(Launch, argc, argv);
  return vtkm::cont::cuda::internal::Testing::CheckCudaBeforeExit(ret);
}
