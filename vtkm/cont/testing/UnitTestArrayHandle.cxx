//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/TypeTraits.h>
#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/ArrayHandleExtractComponent.h>
#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{

template <class IteratorType, typename T>
void CheckValues(IteratorType begin, IteratorType end, T offset)
{

  vtkm::Id index = 0;
  for (IteratorType iter = begin; iter != end; iter++)
  {
    T expectedValue = TestValue(index, T()) + offset;
    if (!test_equal(*iter, expectedValue))
    {
      std::stringstream message;
      message << "Got unexpected value in array." << std::endl
              << "Expected: " << expectedValue << ", Found: " << *iter << std::endl;
      VTKM_TEST_FAIL(message.str().c_str());
    }

    index++;
  }
}

template <typename T>
void CheckArray(const vtkm::cont::ArrayHandle<T>& handle, T offset = T(0))
{
  CheckPortal(handle.ReadPortal(), offset);
}

// Use to get an arbitrarily different valuetype than T:
template <typename T>
struct OtherType
{
  using Type = vtkm::Int32;
};
template <>
struct OtherType<vtkm::Int32>
{
  using Type = vtkm::UInt8;
};

struct PassThrough : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  template <typename T>
  VTKM_EXEC T operator()(const T& value) const
  {
    return value;
  }
};

struct AssignTestValue : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn indices, FieldOut values);

  template <typename T>
  VTKM_EXEC void operator()(vtkm::Id index, T& valueOut) const
  {
    valueOut = TestValue(index, T());
  }
};

struct InplaceAdd1 : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);

  template <typename T>
  VTKM_EXEC void operator()(T& value) const
  {
    value = static_cast<T>(value + T(1));
  }
};

constexpr vtkm::Id ARRAY_SIZE = 100;

struct VerifyEmptyArrays
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Try operations on empty arrays." << std::endl;
    // After each operation, reinitialize array in case something gets
    // allocated.
    vtkm::cont::ArrayHandle<T> arrayHandle = vtkm::cont::ArrayHandle<T>();
    VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 0,
                     "Uninitialized array does not report zero values.");
    arrayHandle = vtkm::cont::ArrayHandle<T>();
    VTKM_TEST_ASSERT(arrayHandle.ReadPortal().GetNumberOfValues() == 0,
                     "Uninitialized array does not give portal with zero values.");
    vtkm::cont::Token token;
    arrayHandle = vtkm::cont::ArrayHandle<T>();
    arrayHandle.Allocate(0, vtkm::CopyFlag::On);
    arrayHandle = vtkm::cont::ArrayHandle<T>();
    arrayHandle.ReleaseResourcesExecution();
    arrayHandle = vtkm::cont::ArrayHandle<T>();
    arrayHandle.ReleaseResources();
    arrayHandle = vtkm::cont::make_ArrayHandleMove(std::vector<T>());
    arrayHandle.PrepareForInput(vtkm::cont::DeviceAdapterTagSerial{}, token);
    arrayHandle = vtkm::cont::ArrayHandle<T>();
    arrayHandle.PrepareForInPlace(vtkm::cont::DeviceAdapterTagSerial{}, token);
    arrayHandle = vtkm::cont::ArrayHandle<T>();
    arrayHandle.PrepareForOutput(ARRAY_SIZE, vtkm::cont::DeviceAdapterTagSerial{}, token);
  }
};

struct VerifyUserOwnedMemory
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    vtkm::cont::Invoker invoke;

    std::cout << "Creating array with user-allocated memory." << std::endl;
    std::vector<T> buffer(ARRAY_SIZE);
    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      buffer[static_cast<std::size_t>(index)] = TestValue(index, T());
    }

    vtkm::cont::ArrayHandle<T> arrayHandle =
      vtkm::cont::make_ArrayHandle(buffer, vtkm::CopyFlag::Off);

    VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                     "ArrayHandle has wrong number of entries.");

    std::cout << "Check array with user provided memory." << std::endl;
    CheckArray(arrayHandle);

    std::cout << "Check out execution array behavior." << std::endl;
    { //as input
      vtkm::cont::ArrayHandle<T> result;
      invoke(PassThrough{}, arrayHandle, result);
      CheckArray(result);
    }

    std::cout << "Check out inplace." << std::endl;
    { //as inplace
      invoke(InplaceAdd1{}, arrayHandle);
      CheckArray(arrayHandle, T(1));
    }

    //clear out user array for next test
    std::fill(buffer.begin(), buffer.end(), static_cast<T>(-1));

    std::cout << "Check out output." << std::endl;
    { //as output with same length as user provided. This should work
      //as no new memory needs to be allocated
      invoke(AssignTestValue{}, vtkm::cont::ArrayHandleIndex(ARRAY_SIZE), arrayHandle);

      //sync data, which should fill up the user buffer
      arrayHandle.SyncControlArray();

      //check that we got the proper values in the user array
      CheckValues(buffer.begin(), buffer.end(), T{ 0 });
    }

    std::cout << "Check invalid reallocation." << std::endl;
    { //as output with a length larger than the memory provided by the user
      //this should fail
      bool gotException = false;
      try
      {
        //you should not be able to allocate a size larger than the
        //user provided and get the results
        vtkm::cont::Token token;
        arrayHandle.PrepareForOutput(ARRAY_SIZE * 2, vtkm::cont::DeviceAdapterTagSerial{}, token);
        token.DetachFromAll();
        arrayHandle.WritePortal();
      }
      catch (vtkm::cont::Error&)
      {
        gotException = true;
      }
      VTKM_TEST_ASSERT(gotException,
                       "PrepareForOutput should fail when asked to "
                       "re-allocate user provided memory.");
    }
  }
};

struct VerifyUserTransferredMemory
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    vtkm::cont::Invoker invoke;

    T* buffer = new T[ARRAY_SIZE];
    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      buffer[static_cast<std::size_t>(index)] = TestValue(index, T());
    }

    auto user_free_function = [](void* ptr) { delete[] static_cast<T*>(ptr); };
    vtkm::cont::ArrayHandleBasic<T> arrayHandle(buffer, ARRAY_SIZE, user_free_function);

    VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                     "ArrayHandle has wrong number of entries.");
    VTKM_TEST_ASSERT(arrayHandle.GetNumberOfComponentsFlat() == vtkm::VecFlat<T>::NUM_COMPONENTS);

    std::cout << "Check array with user transferred memory." << std::endl;
    CheckArray(arrayHandle);

    std::cout << "Check out execution array behavior." << std::endl;
    { //as input
      vtkm::cont::ArrayHandle<T> result;
      invoke(PassThrough{}, arrayHandle, result);
      CheckArray(result);
    }

    std::cout << "Check out inplace." << std::endl;
    { //as inplace
      invoke(InplaceAdd1{}, arrayHandle);
      CheckArray(arrayHandle, T(1));
    }

    std::cout << "Check out output." << std::endl;
    { //as output with same length as user provided. This should work
      //as no new memory needs to be allocated
      invoke(AssignTestValue{}, vtkm::cont::ArrayHandleIndex(ARRAY_SIZE), arrayHandle);

      //sync data, which should fill up the user buffer
      arrayHandle.SyncControlArray();

      //check that we got the proper values in the user array
      CheckValues(buffer, buffer + ARRAY_SIZE, T{ 0 });
    }

    { //as output with a length larger than the memory provided by the user
      //this should fail
      bool gotException = false;
      try
      {
        //you should not be able to allocate a size larger than the
        //user provided and get the results
        vtkm::cont::Token token;
        arrayHandle.PrepareForOutput(ARRAY_SIZE * 2, vtkm::cont::DeviceAdapterTagSerial{}, token);
        token.DetachFromAll();
        arrayHandle.WritePortal();
      }
      catch (vtkm::cont::Error&)
      {
        gotException = true;
      }
      VTKM_TEST_ASSERT(gotException,
                       "PrepareForOutput should fail when asked to "
                       "re-allocate user provided memory.");
    }
  }
};

struct VerifyVectorMovedMemory
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    vtkm::cont::Invoker invoke;

    std::cout << "Creating moved std::vector memory." << std::endl;
    std::vector<T> buffer(ARRAY_SIZE);
    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      buffer[static_cast<std::size_t>(index)] = TestValue(index, T());
    }

    vtkm::cont::ArrayHandle<T> arrayHandle = vtkm::cont::make_ArrayHandleMove(std::move(buffer));
    // buffer is now invalid

    VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                     "ArrayHandle has wrong number of entries.");

    std::cout << "Check array with moved std::vector memory." << std::endl;
    CheckArray(arrayHandle);

    std::cout << "Check out execution array behavior." << std::endl;
    { //as input
      vtkm::cont::ArrayHandle<T> result;
      invoke(PassThrough{}, arrayHandle, result);
      CheckArray(result);
    }

    std::cout << "Check out inplace." << std::endl;
    { //as inplace
      invoke(InplaceAdd1{}, arrayHandle);
      CheckArray(arrayHandle, T(1));
    }

    std::cout << "Check out output." << std::endl;
    { //as output with same length as user provided. This should work
      //as no new memory needs to be allocated
      invoke(AssignTestValue{}, vtkm::cont::ArrayHandleIndex(ARRAY_SIZE), arrayHandle);

      //check that we got the proper values in the user array
      CheckArray(arrayHandle);
    }

    { //as a vector moved to the ArrayHandle, reallocation should be possible
      invoke(AssignTestValue{}, vtkm::cont::ArrayHandleIndex(ARRAY_SIZE * 2), arrayHandle);
      VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2);

      //check that we got the proper values in the user array
      CheckArray(arrayHandle);
    }
  }
};

struct VerifyInitializerList
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    vtkm::cont::Invoker invoke;

    std::cout << "Creating array with initializer list memory." << std::endl;
    vtkm::cont::ArrayHandle<T> arrayHandle =
      vtkm::cont::make_ArrayHandle({ TestValue(0, T()), TestValue(1, T()), TestValue(2, T()) });

    VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 3,
                     "ArrayHandle has wrong number of entries.");

    std::cout << "Check array with initializer list memory." << std::endl;
    CheckArray(arrayHandle);

    std::cout << "Check out execution array behavior." << std::endl;
    { //as input
      vtkm::cont::ArrayHandle<T> result;
      invoke(PassThrough{}, arrayHandle, result);
      CheckArray(result);
    }

    std::cout << "Check out inplace." << std::endl;
    { //as inplace
      invoke(InplaceAdd1{}, arrayHandle);
      CheckArray(arrayHandle, T(1));
    }

    std::cout << "Check out output." << std::endl;
    { //as output with same length as user provided. This should work
      //as no new memory needs to be allocated
      invoke(AssignTestValue{}, vtkm::cont::ArrayHandleIndex(3), arrayHandle);

      //check that we got the proper values in the user array
      CheckArray(arrayHandle);
    }

    { //as a vector moved to the ArrayHandle, reallocation should be possible
      invoke(AssignTestValue{}, vtkm::cont::ArrayHandleIndex(ARRAY_SIZE * 2), arrayHandle);
      VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2);

      //check that we got the proper values in the user array
      CheckArray(arrayHandle);
    }
  }
};

struct VerifyVTKMAllocatedHandle
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    vtkm::cont::Invoker invoke;

    vtkm::cont::ArrayHandle<T> arrayHandle;

    VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 0,
                     "ArrayHandle has wrong number of entries.");
    invoke(AssignTestValue{}, vtkm::cont::ArrayHandleIndex(ARRAY_SIZE * 2), arrayHandle);

    VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2,
                     "Array not allocated correctly.");
    CheckArray(arrayHandle);

    std::cout << "Try shrinking the array." << std::endl;
    arrayHandle.Allocate(ARRAY_SIZE, vtkm::CopyFlag::On);
    VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                     "Array size did not shrink correctly.");
    CheckArray(arrayHandle);

    std::cout << "Try reallocating array." << std::endl;
    arrayHandle.Allocate(ARRAY_SIZE * 2);
    VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2,
                     "Array size did not allocate correctly.");
    // No point in checking values. This method can invalidate them.

    std::cout << "Try in place operation." << std::endl;
    // Reset array data.
    invoke(AssignTestValue{}, vtkm::cont::ArrayHandleIndex(ARRAY_SIZE * 2), arrayHandle);

    invoke(InplaceAdd1{}, arrayHandle);
    CheckArray(arrayHandle, T(1));

    VTKM_TEST_ASSERT(arrayHandle == arrayHandle, "Array handle does not equal itself.");
    VTKM_TEST_ASSERT(arrayHandle != vtkm::cont::ArrayHandle<T>(),
                     "Array handle equals different array.");
  }
};

struct VerifyVTKMTransferredOwnership
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    vtkm::cont::Invoker invoke;

    vtkm::cont::internal::TransferredBuffer transferredMemory;

    //Steal memory from a handle that has multiple copies to verify all
    //copies are updated correctly
    {
      vtkm::cont::ArrayHandle<T> arrayHandle;
      auto copyOfHandle = arrayHandle;

      VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 0,
                       "ArrayHandle has wrong number of entries.");
      invoke(AssignTestValue{}, vtkm::cont::ArrayHandleIndex(ARRAY_SIZE * 2), arrayHandle);

      transferredMemory = copyOfHandle.GetBuffers()[0].TakeHostBufferOwnership();

      VTKM_TEST_ASSERT(copyOfHandle.GetNumberOfValues() == ARRAY_SIZE * 2,
                       "Array not allocated correctly.");
      CheckArray(arrayHandle);

      std::cout << "Try in place operation." << std::endl;
      invoke(InplaceAdd1{}, arrayHandle);
      CheckArray(arrayHandle, T(1));
    }

    transferredMemory.Delete(transferredMemory.Container);
  }
};

struct VerifyEqualityOperators
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Verify that shallow copied array handles compare equal:\n";
    {
      vtkm::cont::ArrayHandle<T> a1;
      vtkm::cont::ArrayHandle<T> a2 = a1; // shallow copy
      vtkm::cont::ArrayHandle<T> a3;
      VTKM_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
      VTKM_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");
      VTKM_TEST_ASSERT(a1 != a3, "Distinct arrays compared equal.");
      VTKM_TEST_ASSERT(!(a1 == a3), "Distinct arrays compared equal.");

      // Operations on a1 shouldn't affect equality
      a1.Allocate(200);
      VTKM_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
      VTKM_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");

      a1.ReadPortal();
      VTKM_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
      VTKM_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");

      vtkm::cont::Token token;
      a1.PrepareForInPlace(vtkm::cont::DeviceAdapterTagSerial{}, token);
      VTKM_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
      VTKM_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");
    }

    std::cout << "Verify that handles with different storage types are not equal.\n";
    {
      vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> a1;
      vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, vtkm::cont::StorageTagBasic> tmp;
      auto a2 = vtkm::cont::make_ArrayHandleExtractComponent(tmp, 1);

      VTKM_TEST_ASSERT(a1 != a2, "Arrays with different storage type compared equal.");
      VTKM_TEST_ASSERT(!(a1 == a2), "Arrays with different storage type compared equal.");
    }

    std::cout << "Verify that handles with different value types are not equal.\n";
    {
      vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> a1;
      vtkm::cont::ArrayHandle<typename OtherType<T>::Type, vtkm::cont::StorageTagBasic> a2;

      VTKM_TEST_ASSERT(a1 != a2, "Arrays with different value type compared equal.");
      VTKM_TEST_ASSERT(!(a1 == a2), "Arrays with different value type compared equal.");
    }

    std::cout << "Verify that handles with different storage and value types are not equal.\n";
    {
      vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> a1;
      vtkm::cont::ArrayHandle<vtkm::Vec<typename OtherType<T>::Type, 3>,
                              vtkm::cont::StorageTagBasic>
        tmp;
      auto a2 = vtkm::cont::make_ArrayHandleExtractComponent(tmp, 1);

      VTKM_TEST_ASSERT(a1 != a2, "Arrays with different storage and value type compared equal.");
      VTKM_TEST_ASSERT(!(a1 == a2), "Arrays with different storage and value type compared equal.");
    }
  }
};

struct VerifyFill
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Initialize values of array." << std::endl;
    const T testValue1 = TestValue(13, T{});
    vtkm::cont::ArrayHandle<T> array;
    array.AllocateAndFill(ARRAY_SIZE, testValue1);
    {
      auto portal = array.ReadPortal();
      for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        VTKM_TEST_ASSERT(portal.Get(index) == testValue1);
      }
    }

    std::cout << "Grow array with new values." << std::endl;
    const T testValue2 = TestValue(42, T{});
    array.AllocateAndFill(ARRAY_SIZE * 2, testValue2, vtkm::CopyFlag::On);
    {
      auto portal = array.ReadPortal();
      for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        VTKM_TEST_ASSERT(portal.Get(index) == testValue1);
      }
      for (vtkm::Id index = ARRAY_SIZE; index < ARRAY_SIZE * 2; ++index)
      {
        VTKM_TEST_ASSERT(portal.Get(index) == testValue2);
      }
    }
  }
};

VTKM_CONT void Run()
{
  vtkm::testing::Testing::TryTypes(VerifyEmptyArrays{});
  vtkm::testing::Testing::TryTypes(VerifyUserOwnedMemory{});
  vtkm::testing::Testing::TryTypes(VerifyUserTransferredMemory{});
  vtkm::testing::Testing::TryTypes(VerifyVectorMovedMemory{});
  vtkm::testing::Testing::TryTypes(VerifyInitializerList{});
  vtkm::testing::Testing::TryTypes(VerifyVTKMAllocatedHandle{});
  vtkm::testing::Testing::TryTypes(VerifyVTKMTransferredOwnership{});
  vtkm::testing::Testing::TryTypes(VerifyEqualityOperators{});
  vtkm::testing::Testing::TryTypes(VerifyFill{});
}

} // anonymous namespace

int UnitTestArrayHandle(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
