//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingArrayHandles_h
#define vtk_m_cont_testing_TestingArrayHandles_h

#include <vtkm/TypeTraits.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/ArrayHandleExtractComponent.h>
#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace vtkm
{
namespace cont
{
namespace testing
{

namespace array_handle_testing
{
template <class IteratorType, typename T>
void CheckValues(IteratorType begin, IteratorType end, T)
{

  vtkm::Id index = 0;
  for (IteratorType iter = begin; iter != end; iter++)
  {
    T expectedValue = TestValue(index, T());
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
void CheckArray(const vtkm::cont::ArrayHandle<T>& handle)
{
  CheckPortal(handle.ReadPortal());
}
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

/// This class has a single static member, Run, that tests that all Fancy Array
/// Handles work with the given DeviceAdapter
///
template <class DeviceAdapterTag>
struct TestingArrayHandles
{
  // Make sure deprecated types still work (while applicable)
  VTKM_DEPRECATED_SUPPRESS_BEGIN
  VTKM_STATIC_ASSERT(
    (std::is_same<typename vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType,
                  typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<
                    DeviceAdapterTag>::PortalConst>::value));
  VTKM_STATIC_ASSERT((std::is_same<typename vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType,
                                   typename vtkm::cont::ArrayHandle<vtkm::Id>::
                                     template ExecutionTypes<DeviceAdapterTag>::Portal>::value));
  VTKM_DEPRECATED_SUPPRESS_END

  template <typename PortalType>
  struct PortalExecObjectWrapper : vtkm::cont::ExecutionObjectBase
  {
    PortalType Portal;
    PortalExecObjectWrapper(const PortalType& portal)
      : Portal(portal)
    {
    }

    PortalType PrepareForExecution(DeviceAdapterTag, vtkm::cont::Token&) const
    {
      return this->Portal;
    }

    template <typename OtherDevice>
    PortalType PrepareForExecution(OtherDevice, vtkm::cont::Token&) const
    {
      VTKM_TEST_FAIL("Executing on wrong device.");
      return this->Portal;
    }
  };

  template <typename PortalType>
  static PortalExecObjectWrapper<PortalType> WrapPortal(const PortalType& portal)
  {
    return PortalExecObjectWrapper<PortalType>(portal);
  }

  struct PassThrough : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn, ExecObject, FieldOut);
    using ExecutionSignature = _3(_2, InputIndex);

    template <typename PortalType>
    VTKM_EXEC typename PortalType::ValueType operator()(const PortalType& portal,
                                                        vtkm::Id index) const
    {
      return portal.Get(index);
    }
  };

  template <typename T, typename ExecutionPortalType>
  struct AssignTestValue : public vtkm::exec::FunctorBase
  {
    ExecutionPortalType Portal;
    VTKM_CONT
    AssignTestValue(ExecutionPortalType p)
      : Portal(p)
    {
    }

    VTKM_EXEC
    void operator()(vtkm::Id index) const { this->Portal.Set(index, TestValue(index, T())); }
  };

  template <typename T, typename ExecutionPortalType>
  struct InplaceFunctor : public vtkm::exec::FunctorBase
  {
    ExecutionPortalType Portal;
    VTKM_CONT
    InplaceFunctor(const ExecutionPortalType& p)
      : Portal(p)
    {
    }

    VTKM_EXEC
    void operator()(vtkm::Id index) const
    {
      this->Portal.Set(index, T(this->Portal.Get(index) + T(1)));
    }
  };

private:
  static constexpr vtkm::Id ARRAY_SIZE = 100;

  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;

  using DispatcherPassThrough = vtkm::worklet::DispatcherMapField<PassThrough>;
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
      arrayHandle.PrepareForInput(DeviceAdapterTag(), token);
      arrayHandle = vtkm::cont::ArrayHandle<T>();
      arrayHandle.PrepareForInPlace(DeviceAdapterTag(), token);
      arrayHandle = vtkm::cont::ArrayHandle<T>();
      arrayHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token);
    }
  };

  struct VerifyUserOwnedMemory
  {
    template <typename T>
    VTKM_CONT void operator()(T) const
    {
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
      array_handle_testing::CheckArray(arrayHandle);

      std::cout << "Check out execution array behavior." << std::endl;
      { //as input
        vtkm::cont::Token token;
        typename vtkm::cont::ArrayHandle<T>::ReadPortalType executionPortal =
          arrayHandle.PrepareForInput(DeviceAdapterTag(), token);
        token.DetachFromAll();
        static_cast<void>(executionPortal);

        //use a worklet to verify the input transfer worked properly
        vtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, WrapPortal(executionPortal), result);
        array_handle_testing::CheckArray(result);
      }

      std::cout << "Check out inplace." << std::endl;
      { //as inplace
        vtkm::cont::Token token;
        typename vtkm::cont::ArrayHandle<T>::WritePortalType executionPortal =
          arrayHandle.PrepareForInPlace(DeviceAdapterTag(), token);
        token.DetachFromAll();
        static_cast<void>(executionPortal);

        //use a worklet to verify the inplace transfer worked properly
        vtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, WrapPortal(executionPortal), result);
        array_handle_testing::CheckArray(result);
      }

      //clear out user array for next test
      std::fill(buffer.begin(), buffer.end(), static_cast<T>(-1));

      std::cout << "Check out output." << std::endl;
      { //as output with same length as user provided. This should work
        //as no new memory needs to be allocated
        vtkm::cont::Token token;
        auto outputPortal = arrayHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token);

        //fill array on device
        Algorithm::Schedule(AssignTestValue<T, decltype(outputPortal)>{ outputPortal }, ARRAY_SIZE);

        //sync data, which should fill up the user buffer
        token.DetachFromAll();
        arrayHandle.SyncControlArray();

        //check that we got the proper values in the user array
        array_handle_testing::CheckValues(buffer.begin(), buffer.end(), T{});
      }

      { //as output with a length larger than the memory provided by the user
        //this should fail
        bool gotException = false;
        try
        {
          //you should not be able to allocate a size larger than the
          //user provided and get the results
          vtkm::cont::Token token;
          arrayHandle.PrepareForOutput(ARRAY_SIZE * 2, DeviceAdapterTag(), token);
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
      T* buffer = new T[ARRAY_SIZE];
      for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
      {
        buffer[static_cast<std::size_t>(index)] = TestValue(index, T());
      }

      auto user_free_function = [](void* ptr) { delete[] static_cast<T*>(ptr); };
      vtkm::cont::ArrayHandleBasic<T> arrayHandle(buffer, ARRAY_SIZE, user_free_function);

      VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                       "ArrayHandle has wrong number of entries.");

      std::cout << "Check array with user transferred memory." << std::endl;
      array_handle_testing::CheckArray(arrayHandle);

      std::cout << "Check out execution array behavior." << std::endl;
      { //as input
        vtkm::cont::Token token;
        typename vtkm::cont::ArrayHandle<T>::ReadPortalType executionPortal =
          arrayHandle.PrepareForInput(DeviceAdapterTag(), token);
        token.DetachFromAll();
        static_cast<void>(executionPortal);

        //use a worklet to verify the input transfer worked properly
        vtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, WrapPortal(executionPortal), result);
        array_handle_testing::CheckArray(result);
      }

      std::cout << "Check out inplace." << std::endl;
      { //as inplace
        vtkm::cont::Token token;
        typename vtkm::cont::ArrayHandle<T>::WritePortalType executionPortal =
          arrayHandle.PrepareForInPlace(DeviceAdapterTag(), token);
        token.DetachFromAll();
        static_cast<void>(executionPortal);

        //use a worklet to verify the inplace transfer worked properly
        vtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, WrapPortal(executionPortal), result);
        array_handle_testing::CheckArray(result);
      }

      std::cout << "Check out output." << std::endl;
      { //as output with same length as user provided. This should work
        //as no new memory needs to be allocated
        vtkm::cont::Token token;
        arrayHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token);
        token.DetachFromAll();

        //we can't verify output contents as those aren't fetched, we
        //can just make sure the allocation didn't throw an exception
      }

      { //as output with a length larger than the memory provided by the user
        //this should fail
        bool gotException = false;
        try
        {
          //you should not be able to allocate a size larger than the
          //user provided and get the results
          vtkm::cont::Token token;
          arrayHandle.PrepareForOutput(ARRAY_SIZE * 2, DeviceAdapterTag(), token);
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
      array_handle_testing::CheckArray(arrayHandle);

      std::cout << "Check out execution array behavior." << std::endl;
      { //as input
        vtkm::cont::Token token;
        typename vtkm::cont::ArrayHandle<T>::ReadPortalType executionPortal;
        executionPortal = arrayHandle.PrepareForInput(DeviceAdapterTag(), token);
        token.DetachFromAll();

        //use a worklet to verify the input transfer worked properly
        vtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, WrapPortal(executionPortal), result);
        array_handle_testing::CheckArray(result);
      }

      std::cout << "Check out inplace." << std::endl;
      { //as inplace
        vtkm::cont::Token token;
        typename vtkm::cont::ArrayHandle<T>::WritePortalType executionPortal;
        executionPortal = arrayHandle.PrepareForInPlace(DeviceAdapterTag(), token);
        token.DetachFromAll();

        //use a worklet to verify the inplace transfer worked properly
        vtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, WrapPortal(executionPortal), result);
        array_handle_testing::CheckArray(result);
      }

      std::cout << "Check out output." << std::endl;
      { //as output with same length as user provided. This should work
        //as no new memory needs to be allocated
        vtkm::cont::Token token;
        arrayHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token);
        token.DetachFromAll();

        //we can't verify output contents as those aren't fetched, we
        //can just make sure the allocation didn't throw an exception
      }

      { //as a vector moved to the ArrayHandle, reallocation should be possible
        vtkm::cont::Token token;
        arrayHandle.PrepareForOutput(ARRAY_SIZE * 2, DeviceAdapterTag(), token);
        token.DetachFromAll();

        //we can't verify output contents as those aren't fetched, we
        //can just make sure the allocation didn't throw an exception
        VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2);
      }
    }
  };

  struct VerifyInitializerList
  {
    template <typename T>
    VTKM_CONT void operator()(T) const
    {
      std::cout << "Creating array with initializer list memory." << std::endl;
      vtkm::cont::ArrayHandle<T> arrayHandle =
        vtkm::cont::make_ArrayHandle({ TestValue(0, T()), TestValue(1, T()), TestValue(2, T()) });

      VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 3,
                       "ArrayHandle has wrong number of entries.");

      std::cout << "Check array with initializer list memory." << std::endl;
      array_handle_testing::CheckArray(arrayHandle);

      std::cout << "Check out execution array behavior." << std::endl;
      { //as input
        vtkm::cont::Token token;
        typename vtkm::cont::ArrayHandle<T>::ReadPortalType executionPortal;
        executionPortal = arrayHandle.PrepareForInput(DeviceAdapterTag(), token);
        token.DetachFromAll();

        //use a worklet to verify the input transfer worked properly
        vtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, WrapPortal(executionPortal), result);
        array_handle_testing::CheckArray(result);
      }

      std::cout << "Check out inplace." << std::endl;
      { //as inplace
        vtkm::cont::Token token;
        typename vtkm::cont::ArrayHandle<T>::WritePortalType executionPortal;
        executionPortal = arrayHandle.PrepareForInPlace(DeviceAdapterTag(), token);
        token.DetachFromAll();

        //use a worklet to verify the inplace transfer worked properly
        vtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, WrapPortal(executionPortal), result);
        array_handle_testing::CheckArray(result);
      }

      std::cout << "Check out output." << std::endl;
      { //as output with same length as user provided. This should work
        //as no new memory needs to be allocated
        vtkm::cont::Token token;
        arrayHandle.PrepareForOutput(3, DeviceAdapterTag(), token);
        token.DetachFromAll();

        //we can't verify output contents as those aren't fetched, we
        //can just make sure the allocation didn't throw an exception
      }

      { //as a vector moved to the ArrayHandle, reallocation should be possible
        vtkm::cont::Token token;
        arrayHandle.PrepareForOutput(ARRAY_SIZE * 2, DeviceAdapterTag(), token);
        token.DetachFromAll();

        //we can't verify output contents as those aren't fetched, we
        //can just make sure the allocation didn't throw an exception
        VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2);
      }
    }
  };

  struct VerifyVTKMAllocatedHandle
  {
    template <typename T>
    VTKM_CONT void operator()(T) const
    {
      vtkm::cont::ArrayHandle<T> arrayHandle;

      VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 0,
                       "ArrayHandle has wrong number of entries.");
      {
        vtkm::cont::Token token;
        using ExecutionPortalType = typename vtkm::cont::ArrayHandle<T>::WritePortalType;
        ExecutionPortalType executionPortal =
          arrayHandle.PrepareForOutput(ARRAY_SIZE * 2, DeviceAdapterTag(), token);

        //we drop down to manually scheduling so that we don't need
        //need to bring in array handle counting
        AssignTestValue<T, ExecutionPortalType> functor(executionPortal);
        Algorithm::Schedule(functor, ARRAY_SIZE * 2);
      }

      VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2,
                       "Array not allocated correctly.");
      array_handle_testing::CheckArray(arrayHandle);

      std::cout << "Try shrinking the array." << std::endl;
      arrayHandle.Allocate(ARRAY_SIZE, vtkm::CopyFlag::On);
      VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                       "Array size did not shrink correctly.");
      array_handle_testing::CheckArray(arrayHandle);

      std::cout << "Try reallocating array." << std::endl;
      arrayHandle.Allocate(ARRAY_SIZE * 2);
      VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2,
                       "Array size did not allocate correctly.");
      // No point in checking values. This method can invalidate them.

      std::cout << "Try in place operation." << std::endl;
      {
        vtkm::cont::Token token;
        using ExecutionPortalType = typename vtkm::cont::ArrayHandle<T>::WritePortalType;

        // Reset array data.
        Algorithm::Schedule(AssignTestValue<T, ExecutionPortalType>{ arrayHandle.PrepareForOutput(
                              ARRAY_SIZE * 2, DeviceAdapterTag{}, token) },
                            ARRAY_SIZE * 2);

        ExecutionPortalType executionPortal =
          arrayHandle.PrepareForInPlace(DeviceAdapterTag(), token);

        //in place can't be done through the dispatcher
        //instead we have to drop down to manually scheduling
        InplaceFunctor<T, ExecutionPortalType> functor(executionPortal);
        Algorithm::Schedule(functor, ARRAY_SIZE * 2);
      }
      typename vtkm::cont::ArrayHandle<T>::ReadPortalType controlPortal = arrayHandle.ReadPortal();
      for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
      {
        VTKM_TEST_ASSERT(test_equal(controlPortal.Get(index), TestValue(index, T()) + T(1)),
                         "Did not get result from in place operation.");
      }

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

      vtkm::cont::internal::TransferredBuffer transferredMemory;

      //Steal memory from a handle that has multiple copies to verify all
      //copies are updated correctly
      {
        vtkm::cont::ArrayHandle<T> arrayHandle;
        auto copyOfHandle = arrayHandle;

        VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 0,
                         "ArrayHandle has wrong number of entries.");
        {
          vtkm::cont::Token token;
          using ExecutionPortalType = typename vtkm::cont::ArrayHandle<T>::WritePortalType;
          ExecutionPortalType executionPortal =
            arrayHandle.PrepareForOutput(ARRAY_SIZE * 2, DeviceAdapterTag(), token);

          //we drop down to manually scheduling so that we don't need
          //need to bring in array handle counting
          AssignTestValue<T, ExecutionPortalType> functor(executionPortal);
          Algorithm::Schedule(functor, ARRAY_SIZE * 2);
        }

        transferredMemory = copyOfHandle.GetBuffers()->TakeHostBufferOwnership();

        VTKM_TEST_ASSERT(copyOfHandle.GetNumberOfValues() == ARRAY_SIZE * 2,
                         "Array not allocated correctly.");
        array_handle_testing::CheckArray(arrayHandle);

        std::cout << "Try in place operation." << std::endl;
        {
          vtkm::cont::Token token;
          using ExecutionPortalType = typename vtkm::cont::ArrayHandle<T>::WritePortalType;

          // Reset array data.
          Algorithm::Schedule(AssignTestValue<T, ExecutionPortalType>{ arrayHandle.PrepareForOutput(
                                ARRAY_SIZE * 2, DeviceAdapterTag{}, token) },
                              ARRAY_SIZE * 2);

          ExecutionPortalType executionPortal =
            arrayHandle.PrepareForInPlace(DeviceAdapterTag(), token);

          //in place can't be done through the dispatcher
          //instead we have to drop down to manually scheduling
          InplaceFunctor<T, ExecutionPortalType> functor(executionPortal);
          Algorithm::Schedule(functor, ARRAY_SIZE * 2);
        }
        typename vtkm::cont::ArrayHandle<T>::ReadPortalType controlPortal =
          arrayHandle.ReadPortal();
        for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
        {
          VTKM_TEST_ASSERT(test_equal(controlPortal.Get(index), TestValue(index, T()) + T(1)),
                           "Did not get result from in place operation.");
        }
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
        a1.PrepareForInPlace(DeviceAdapterTag(), token);
        VTKM_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
        VTKM_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");
      }

      std::cout << "Verify that handles with different storage types are not equal.\n";
      {
        vtkm::cont::ArrayHandle<T, StorageTagBasic> a1;
        vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageTagBasic> tmp;
        auto a2 = vtkm::cont::make_ArrayHandleExtractComponent(tmp, 1);

        VTKM_TEST_ASSERT(a1 != a2, "Arrays with different storage type compared equal.");
        VTKM_TEST_ASSERT(!(a1 == a2), "Arrays with different storage type compared equal.");
      }

      std::cout << "Verify that handles with different value types are not equal.\n";
      {
        vtkm::cont::ArrayHandle<T, StorageTagBasic> a1;
        vtkm::cont::ArrayHandle<typename OtherType<T>::Type, StorageTagBasic> a2;

        VTKM_TEST_ASSERT(a1 != a2, "Arrays with different value type compared equal.");
        VTKM_TEST_ASSERT(!(a1 == a2), "Arrays with different value type compared equal.");
      }

      std::cout << "Verify that handles with different storage and value types are not equal.\n";
      {
        vtkm::cont::ArrayHandle<T, StorageTagBasic> a1;
        vtkm::cont::ArrayHandle<vtkm::Vec<typename OtherType<T>::Type, 3>, StorageTagBasic> tmp;
        auto a2 = vtkm::cont::make_ArrayHandleExtractComponent(tmp, 1);

        VTKM_TEST_ASSERT(a1 != a2, "Arrays with different storage and value type compared equal.");
        VTKM_TEST_ASSERT(!(a1 == a2),
                         "Arrays with different storage and value type compared equal.");
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

  struct TryArrayHandleType
  {
    void operator()() const
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
  };

public:
  static VTKM_CONT int Run(int argc, char* argv[])
  {
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapterTag());
    return vtkm::cont::testing::Testing::Run(TryArrayHandleType(), argc, argv);
  }
};
}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_TestingArrayHandles_h
