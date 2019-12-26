//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingDeviceAdapter_h
#define vtk_m_cont_testing_TestingDeviceAdapter_h

#include <vtkm/BinaryOperators.h>
#include <vtkm/BinaryPredicates.h>
#include <vtkm/TypeTraits.h>

#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>
#include <vtkm/cont/StorageBasic.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/cont/internal/VirtualObjectTransfer.h>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/AtomicArray.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <random>
#include <thread>
#include <utility>
#include <vector>

#include <vtkm/internal/Windows.h>

namespace vtkm
{
namespace cont
{
namespace testing
{

#define ERROR_MESSAGE "Got an error."
#define ARRAY_SIZE 100000
#define OFFSET 1000
#define DIM_SIZE 128

/// This class has a single static member, Run, that tests the templated
/// DeviceAdapter for conformance.
///
template <class DeviceAdapterTag>
struct TestingDeviceAdapter
{
private:
  using StorageTag = vtkm::cont::StorageTagBasic;

  using IdArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id, StorageTag>;
  using IdComponentArrayHandle = vtkm::cont::ArrayHandle<vtkm::IdComponent, StorageTag>;
  using ScalarArrayHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault, StorageTag>;

  using IdPortalType = typename IdArrayHandle::template ExecutionTypes<DeviceAdapterTag>::Portal;
  using IdPortalConstType =
    typename IdArrayHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst;

  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;

public:
  // Cuda kernels have to be public (in Cuda 4.0).

  struct CopyArrayKernel
  {
    VTKM_CONT
    CopyArrayKernel(const IdPortalConstType& input, const IdPortalType& output)
      : InputArray(input)
      , OutputArray(output)
    {
    }

    VTKM_EXEC void operator()(vtkm::Id index, const vtkm::exec::internal::ErrorMessageBuffer&) const
    {
      this->OutputArray.Set(index, this->InputArray.Get(index));
    }

    VTKM_CONT void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}

    IdPortalConstType InputArray;
    IdPortalType OutputArray;
  };

  template <typename PortalType>
  struct GenericClearArrayKernel
  {
    using ValueType = typename PortalType::ValueType;

    VTKM_CONT
    GenericClearArrayKernel(const PortalType& array,
                            const ValueType& fillValue = static_cast<ValueType>(OFFSET))
      : Array(array)
      , Dims()
      , FillValue(fillValue)
    {
    }

    VTKM_CONT
    GenericClearArrayKernel(const PortalType& array,
                            const vtkm::Id3& dims,
                            const ValueType& fillValue = static_cast<ValueType>(OFFSET))
      : Array(array)
      , Dims(dims)
      , FillValue(fillValue)
    {
    }

    VTKM_EXEC void operator()(vtkm::Id index) const { this->Array.Set(index, this->FillValue); }

    VTKM_EXEC void operator()(vtkm::Id3 index) const
    {
      //convert from id3 to id
      vtkm::Id flatIndex = index[0] + this->Dims[0] * (index[1] + this->Dims[1] * index[2]);
      this->operator()(flatIndex);
    }

    VTKM_CONT void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}

    PortalType Array;
    vtkm::Id3 Dims;
    ValueType FillValue;
  };

  using ClearArrayKernel = GenericClearArrayKernel<IdPortalType>;

  struct ClearArrayMapKernel //: public vtkm::exec::WorkletMapField
  {

    // using ControlSignature = void(Field(Out));
    // using ExecutionSignature = void(_1);

    template <typename T>
    VTKM_EXEC void operator()(T& value) const
    {
      value = OFFSET;
    }
  };

  struct AddArrayKernel
  {
    VTKM_CONT
    AddArrayKernel(const IdPortalType& array)
      : Array(array)
      , Dims()
    {
    }

    VTKM_CONT
    AddArrayKernel(const IdPortalType& array, const vtkm::Id3& dims)
      : Array(array)
      , Dims(dims)
    {
    }

    VTKM_EXEC void operator()(vtkm::Id index) const
    {
      this->Array.Set(index, this->Array.Get(index) + index);
    }

    VTKM_EXEC void operator()(vtkm::Id3 index) const
    {
      //convert from id3 to id
      vtkm::Id flatIndex = index[0] + this->Dims[0] * (index[1] + this->Dims[1] * index[2]);
      this->operator()(flatIndex);
    }

    VTKM_CONT void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}

    IdPortalType Array;
    vtkm::Id3 Dims;
  };

  // Checks that each instance is only visited once:
  struct OverlapKernel
  {
    using ArrayType = ArrayHandle<bool>;
    using PortalType = typename ArrayType::template ExecutionTypes<DeviceAdapterTag>::Portal;

    PortalType TrackerPortal;
    PortalType ValidPortal;
    vtkm::Id3 Dims;

    VTKM_CONT
    OverlapKernel(const PortalType& trackerPortal,
                  const PortalType& validPortal,
                  const vtkm::Id3& dims)
      : TrackerPortal(trackerPortal)
      , ValidPortal(validPortal)
      , Dims(dims)
    {
    }

    VTKM_CONT
    OverlapKernel(const PortalType& trackerPortal, const PortalType& validPortal)
      : TrackerPortal(trackerPortal)
      , ValidPortal(validPortal)
      , Dims()
    {
    }

    VTKM_EXEC void operator()(vtkm::Id index) const
    {
      if (this->TrackerPortal.Get(index))
      { // this index has already been visited, that's an error
        this->ValidPortal.Set(index, false);
      }
      else
      {
        this->TrackerPortal.Set(index, true);
        this->ValidPortal.Set(index, true);
      }
    }

    VTKM_EXEC void operator()(vtkm::Id3 index) const
    {
      //convert from id3 to id
      vtkm::Id flatIndex = index[0] + this->Dims[0] * (index[1] + this->Dims[1] * index[2]);
      this->operator()(flatIndex);
    }

    VTKM_CONT void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}
  };

  struct OneErrorKernel
  {
    VTKM_EXEC void operator()(vtkm::Id index) const
    {
      if (index == ARRAY_SIZE / 2)
      {
        this->ErrorMessage.RaiseError(ERROR_MESSAGE);
      }
    }

    VTKM_CONT void SetErrorMessageBuffer(
      const vtkm::exec::internal::ErrorMessageBuffer& errorMessage)
    {
      this->ErrorMessage = errorMessage;
    }

    vtkm::exec::internal::ErrorMessageBuffer ErrorMessage;
  };

  struct AllErrorKernel
  {
    VTKM_EXEC void operator()(vtkm::Id vtkmNotUsed(index)) const
    {
      this->ErrorMessage.RaiseError(ERROR_MESSAGE);
    }

    VTKM_CONT void SetErrorMessageBuffer(
      const vtkm::exec::internal::ErrorMessageBuffer& errorMessage)
    {
      this->ErrorMessage = errorMessage;
    }

    vtkm::exec::internal::ErrorMessageBuffer ErrorMessage;
  };

  struct OffsetPlusIndexKernel
  {
    VTKM_CONT
    OffsetPlusIndexKernel(const IdPortalType& array)
      : Array(array)
    {
    }

    VTKM_EXEC void operator()(vtkm::Id index) const { this->Array.Set(index, OFFSET + index); }

    VTKM_CONT void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}

    IdPortalType Array;
  };

  struct MarkOddNumbersKernel
  {
    VTKM_CONT
    MarkOddNumbersKernel(const IdPortalType& array)
      : Array(array)
    {
    }

    VTKM_EXEC void operator()(vtkm::Id index) const { this->Array.Set(index, index % 2); }

    VTKM_CONT void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}

    IdPortalType Array;
  };

  struct FuseAll
  {
    template <typename T>
    VTKM_EXEC bool operator()(const T&, const T&) const
    {
      //binary predicates for unique return true if they are the same
      return true;
    }
  };

  template <typename T>
  struct AtomicKernel
  {
    VTKM_CONT
    AtomicKernel(const vtkm::cont::AtomicArray<T>& array)
      : AArray(array.PrepareForExecution(DeviceAdapterTag()))
    {
    }

    VTKM_EXEC void operator()(vtkm::Id index) const
    {
      T value = (T)index;
      this->AArray.Add(0, value);
    }

    VTKM_CONT void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}

    vtkm::exec::AtomicArrayExecutionObject<T, DeviceAdapterTag> AArray;
  };

  template <typename T>
  struct AtomicCASKernel
  {
    VTKM_CONT
    AtomicCASKernel(const vtkm::cont::AtomicArray<T>& array)
      : AArray(array.PrepareForExecution(DeviceAdapterTag()))
    {
    }

    VTKM_EXEC void operator()(vtkm::Id index) const
    {
      T value = (T)index;
      //Get the old value from the array
      T oldValue = this->AArray.Get(0);
      //This creates an atomic add using the CAS operatoin
      T assumed = T(0);
      do
      {
        assumed = oldValue;
        oldValue = this->AArray.CompareAndSwap(0, (assumed + value), assumed);

      } while (assumed != oldValue);
    }

    VTKM_CONT void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}

    vtkm::exec::AtomicArrayExecutionObject<T, DeviceAdapterTag> AArray;
  };

  class VirtualObjectTransferKernel
  {
  public:
    struct Interface : public vtkm::VirtualObjectBase
    {
      VTKM_EXEC virtual vtkm::Id Foo() const = 0;
    };

    struct Concrete : public Interface
    {
      VTKM_EXEC vtkm::Id Foo() const override { return this->Value; }

      vtkm::Id Value = 0;
    };

    VirtualObjectTransferKernel(const Interface* vo, IdArrayHandle& result)
      : Virtual(vo)
      , Result(result.PrepareForInPlace(DeviceAdapterTag()))
    {
    }

    VTKM_EXEC
    void operator()(vtkm::Id) const { this->Result.Set(0, this->Virtual->Foo()); }

    VTKM_CONT void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer&) {}

  private:
    const Interface* Virtual;
    IdPortalType Result;
  };

  struct CustomPairOp
  {
    using ValueType = vtkm::Pair<vtkm::Id, vtkm::Float32>;

    VTKM_EXEC
    ValueType operator()(const vtkm::Id& a) const { return ValueType(a, 0.0f); }

    VTKM_EXEC
    ValueType operator()(const vtkm::Id& a, const vtkm::Id& b) const
    {
      return ValueType(vtkm::Max(a, b), 0.0f);
    }

    VTKM_EXEC
    ValueType operator()(const ValueType& a, const ValueType& b) const
    {
      return ValueType(vtkm::Max(a.first, b.first), 0.0f);
    }

    VTKM_EXEC
    ValueType operator()(const vtkm::Id& a, const ValueType& b) const
    {
      return ValueType(vtkm::Max(a, b.first), 0.0f);
    }

    VTKM_EXEC
    ValueType operator()(const ValueType& a, const vtkm::Id& b) const
    {
      return ValueType(vtkm::Max(a.first, b), 0.0f);
    }
  };

  struct CustomTForReduce
  {
    constexpr CustomTForReduce()
      : Value(0.0f)
    {
    }

    constexpr CustomTForReduce(float f)
      : Value(f)
    {
    }

    VTKM_EXEC_CONT
    constexpr float value() const { return this->Value; }

    float Value;
  };

  template <typename T>
  struct CustomMinAndMax
  {
    VTKM_EXEC_CONT
    vtkm::Vec<float, 2> operator()(const T& a) const
    {
      return vtkm::make_Vec(a.value(), a.value());
    }

    VTKM_EXEC_CONT
    vtkm::Vec<float, 2> operator()(const T& a, const T& b) const
    {
      return vtkm::make_Vec(vtkm::Min(a.value(), b.value()), vtkm::Max(a.value(), b.value()));
    }

    VTKM_EXEC_CONT
    vtkm::Vec<float, 2> operator()(const vtkm::Vec<float, 2>& a, const vtkm::Vec<float, 2>& b) const
    {
      return vtkm::make_Vec(vtkm::Min(a[0], b[0]), vtkm::Max(a[1], b[1]));
    }

    VTKM_EXEC_CONT
    vtkm::Vec<float, 2> operator()(const T& a, const vtkm::Vec<float, 2>& b) const
    {
      return vtkm::make_Vec(vtkm::Min(a.value(), b[0]), vtkm::Max(a.value(), b[1]));
    }

    VTKM_EXEC_CONT
    vtkm::Vec<float, 2> operator()(const vtkm::Vec<float, 2>& a, const T& b) const
    {
      return vtkm::make_Vec(vtkm::Min(a[0], b.value()), vtkm::Max(a[1], b.value()));
    }
  };


private:
  static VTKM_CONT void TestDeviceAdapterTag()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing device adapter tag" << std::endl;

    constexpr DeviceAdapterTag deviceTag;
    constexpr vtkm::cont::DeviceAdapterTagUndefined undefinedTag;

    VTKM_TEST_ASSERT(deviceTag.GetValue() == deviceTag.GetValue(),
                     "Device adapter Id does not equal itself.");
    VTKM_TEST_ASSERT(deviceTag.GetValue() != undefinedTag.GetValue(),
                     "Device adapter Id not distinguishable from others.");

    using Traits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    VTKM_TEST_ASSERT(Traits::GetName() == Traits::GetName(),
                     "Device adapter Name does not equal itself.");
  }

  // Note: this test does not actually test to make sure the data is available
  // in the execution environment. It tests to make sure data gets to the array
  // and back, but it is possible that the data is not available in the
  // execution environment.
  static VTKM_CONT void TestArrayTransfer()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing ArrayHandle Transfer" << std::endl;

    using StorageType = vtkm::cont::internal::Storage<vtkm::Id, StorageTagBasic>;

    // Create original input array.
    StorageType storage;
    storage.Allocate(ARRAY_SIZE * 2);

    StorageType::PortalType portal = storage.GetPortal();
    VTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE * 2,
                     "Storage portal has unexpected size.");

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      portal.Set(index, TestValue(index, vtkm::Id()));
    }

    vtkm::cont::ArrayHandle<vtkm::Id> handle(std::move(storage));

    // Do an operation just so we know the values are placed in the execution
    // environment and they change. We are only calling on half the array
    // because we are about to shrink.
    Algorithm::Schedule(AddArrayKernel(handle.PrepareForInPlace(DeviceAdapterTag{})), ARRAY_SIZE);

    // Change size.
    handle.Shrink(ARRAY_SIZE);

    VTKM_TEST_ASSERT(handle.GetNumberOfValues() == ARRAY_SIZE,
                     "Shrink did not set size of array handle correctly.");

    // Get the array back and check its values.
    StorageType::PortalConstType checkPortal = handle.GetPortalConstControl();
    VTKM_TEST_ASSERT(checkPortal.GetNumberOfValues() == ARRAY_SIZE, "Storage portal wrong size.");

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      VTKM_TEST_ASSERT(checkPortal.Get(index) == TestValue(index, vtkm::Id()) + index,
                       "Did not get correct values from array.");
    }
  }

  static VTKM_CONT void TestOutOfMemory()
  {
// Only test out of memory with 64 bit ids.  If there are 32 bit ids on
// a 64 bit OS (common), it is simply too hard to get a reliable allocation
// that is too much memory.
#ifdef VTKM_USE_64BIT_IDS
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Out of Memory" << std::endl;
    try
    {
      std::cout << "Do array allocation that should fail." << std::endl;
      vtkm::cont::ArrayHandle<vtkm::Vec4f_32, StorageTagBasic> bigArray;
      const vtkm::Id bigSize = 0x7FFFFFFFFFFFFFFFLL;
      bigArray.PrepareForOutput(bigSize, DeviceAdapterTag{});
      // It does not seem reasonable to get here.  The previous call should fail.
      VTKM_TEST_FAIL("A ridiculously sized allocation succeeded.  Either there "
                     "was a failure that was not reported but should have been "
                     "or the width of vtkm::Id is not large enough to express all "
                     "array sizes.");
    }
    catch (vtkm::cont::ErrorBadAllocation& error)
    {
      std::cout << "Got the expected error: " << error.GetMessage() << std::endl;
    }
#else
    std::cout << "--------- Skipping out of memory test" << std::endl;
#endif
  }

  VTKM_CONT
  static void TestTimer()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Timer" << std::endl;
    auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
    if (tracker.CanRunOn(DeviceAdapterTag()))
    {
      vtkm::cont::Timer timer{ DeviceAdapterTag() };
      timer.Start();

      std::cout << "Timer started. Sleeping..." << std::endl;

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      std::cout << "Woke up. Check time." << std::endl;

      timer.Stop();
      vtkm::Float64 elapsedTime = timer.GetElapsedTime();

      std::cout << "Elapsed time: " << elapsedTime << std::endl;

      VTKM_TEST_ASSERT(elapsedTime > 0.499, "Timer did not capture full second wait.");
      VTKM_TEST_ASSERT(elapsedTime < 1.0, "Timer counted too far or system really busy.");
    }
  }

  VTKM_CONT
  static void TestVirtualObjectTransfer()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing VirtualObjectTransfer" << std::endl;

    using BaseType = typename VirtualObjectTransferKernel::Interface;
    using TargetType = typename VirtualObjectTransferKernel::Concrete;
    using Transfer = vtkm::cont::internal::VirtualObjectTransfer<TargetType, DeviceAdapterTag>;

    IdArrayHandle result;
    result.Allocate(1);
    result.GetPortalControl().Set(0, 0);

    TargetType target;
    target.Value = 5;

    Transfer transfer(&target);
    const BaseType* base = static_cast<const BaseType*>(transfer.PrepareForExecution(false));

    Algorithm::Schedule(VirtualObjectTransferKernel(base, result), 1);
    VTKM_TEST_ASSERT(result.GetPortalConstControl().Get(0) == 5, "Did not get expected result");

    target.Value = 10;
    base = static_cast<const BaseType*>(transfer.PrepareForExecution(true));
    Algorithm::Schedule(VirtualObjectTransferKernel(base, result), 1);
    VTKM_TEST_ASSERT(result.GetPortalConstControl().Get(0) == 10, "Did not get expected result");

    transfer.ReleaseResources();
  }

  static VTKM_CONT void TestAlgorithmSchedule()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing single value Scheduling with vtkm::Id" << std::endl;

    {
      std::cout << "Allocating execution array" << std::endl;
      vtkm::cont::ArrayHandle<vtkm::Id> handle;

      std::cout << "Running clear." << std::endl;
      Algorithm::Schedule(ClearArrayKernel(handle.PrepareForOutput(1, DeviceAdapterTag{})), 1);

      std::cout << "Running add." << std::endl;
      Algorithm::Schedule(AddArrayKernel(handle.PrepareForInPlace(DeviceAdapterTag{})), 1);

      std::cout << "Checking results." << std::endl;
      for (vtkm::Id index = 0; index < 1; index++)
      {
        vtkm::Id value = handle.GetPortalConstControl().Get(index);
        VTKM_TEST_ASSERT(value == index + OFFSET,
                         "Got bad value for single value scheduled kernel.");
      }
    } //release memory

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule with vtkm::Id" << std::endl;

    {
      std::cout << "Allocating execution array" << std::endl;
      vtkm::cont::ArrayHandle<vtkm::Id> handle;

      std::cout << "Running clear." << std::endl;
      Algorithm::Schedule(ClearArrayKernel(handle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag{})),
                          ARRAY_SIZE);

      std::cout << "Running add." << std::endl;
      Algorithm::Schedule(AddArrayKernel(handle.PrepareForInPlace(DeviceAdapterTag{})), ARRAY_SIZE);

      std::cout << "Checking results." << std::endl;
      for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
      {
        vtkm::Id value = handle.GetPortalConstControl().Get(index);
        VTKM_TEST_ASSERT(value == index + OFFSET, "Got bad value for scheduled kernels.");
      }
    } //release memory

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule with a vary large Id value" << std::endl;

    {
      std::cout << "Allocating execution array" << std::endl;
      vtkm::cont::ArrayHandle<vtkm::Id> handle;

      std::cout << "Running clear." << std::endl;

      //size is selected to be larger than the CUDA backend can launch in a
      //single invocation when compiled for SM_2 support
      const vtkm::Id size = 8400000;
      Algorithm::Schedule(ClearArrayKernel(handle.PrepareForOutput(size, DeviceAdapterTag{})),
                          size);

      std::cout << "Running add." << std::endl;
      Algorithm::Schedule(AddArrayKernel(handle.PrepareForInPlace(DeviceAdapterTag{})), size);

      std::cout << "Checking results." << std::endl;
      //Rather than testing for correctness every value of a large array,
      // we randomly test a subset of that array.
      std::default_random_engine generator(static_cast<unsigned int>(std::time(nullptr)));
      std::uniform_int_distribution<vtkm::Id> distribution(0, size - 1);
      vtkm::Id numberOfSamples = size / 100;
      for (vtkm::Id i = 0; i < numberOfSamples; ++i)
      {
        vtkm::Id randomIndex = distribution(generator);
        vtkm::Id value = handle.GetPortalConstControl().Get(randomIndex);
        VTKM_TEST_ASSERT(value == randomIndex + OFFSET, "Got bad value for scheduled kernels.");
      }
    } //release memory

    //verify that the schedule call works with id3
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule with vtkm::Id3" << std::endl;

    {
      std::cout << "Allocating execution array" << std::endl;
      vtkm::cont::ArrayHandle<vtkm::Id> handle;
      vtkm::Id3 maxRange(DIM_SIZE);

      std::cout << "Running clear." << std::endl;
      Algorithm::Schedule(
        ClearArrayKernel(
          handle.PrepareForOutput(DIM_SIZE * DIM_SIZE * DIM_SIZE, DeviceAdapterTag{}), maxRange),
        maxRange);

      std::cout << "Running add." << std::endl;
      Algorithm::Schedule(AddArrayKernel(handle.PrepareForInPlace(DeviceAdapterTag{}), maxRange),
                          maxRange);

      std::cout << "Checking results." << std::endl;
      const vtkm::Id maxId = DIM_SIZE * DIM_SIZE * DIM_SIZE;
      for (vtkm::Id index = 0; index < maxId; index++)
      {
        vtkm::Id value = handle.GetPortalConstControl().Get(index);
        VTKM_TEST_ASSERT(value == index + OFFSET, "Got bad value for scheduled vtkm::Id3 kernels.");
      }
    } //release memory

    // Ensure that each element is only visited once:
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule for overlap" << std::endl;

    {
      using BoolArray = ArrayHandle<bool>;
      using BoolPortal = typename BoolArray::template ExecutionTypes<DeviceAdapterTag>::Portal;
      BoolArray tracker;
      BoolArray valid;

      // Initialize tracker with 'false' values
      std::cout << "Allocating and initializing memory" << std::endl;
      Algorithm::Schedule(GenericClearArrayKernel<BoolPortal>(
                            tracker.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag()), false),
                          ARRAY_SIZE);
      Algorithm::Schedule(GenericClearArrayKernel<BoolPortal>(
                            valid.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag()), false),
                          ARRAY_SIZE);

      std::cout << "Running Overlap kernel." << std::endl;
      Algorithm::Schedule(OverlapKernel(tracker.PrepareForInPlace(DeviceAdapterTag()),
                                        valid.PrepareForInPlace(DeviceAdapterTag())),
                          ARRAY_SIZE);

      std::cout << "Checking results." << std::endl;

      auto vPortal = valid.GetPortalConstControl();
      for (vtkm::Id i = 0; i < ARRAY_SIZE; i++)
      {
        bool isValid = vPortal.Get(i);
        VTKM_TEST_ASSERT(isValid, "Schedule executed some elements more than once.");
      }
    } // release memory

    // Ensure that each element is only visited once:
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule for overlap with vtkm::Id3" << std::endl;

    {
      static constexpr vtkm::Id numElems{ DIM_SIZE * DIM_SIZE * DIM_SIZE };
      static const vtkm::Id3 dims{ DIM_SIZE, DIM_SIZE, DIM_SIZE };

      using BoolArray = ArrayHandle<bool>;
      using BoolPortal = typename BoolArray::template ExecutionTypes<DeviceAdapterTag>::Portal;
      BoolArray tracker;
      BoolArray valid;

      // Initialize tracker with 'false' values
      std::cout << "Allocating and initializing memory" << std::endl;
      Algorithm::Schedule(GenericClearArrayKernel<BoolPortal>(
                            tracker.PrepareForOutput(numElems, DeviceAdapterTag()), dims, false),
                          numElems);
      Algorithm::Schedule(GenericClearArrayKernel<BoolPortal>(
                            valid.PrepareForOutput(numElems, DeviceAdapterTag()), dims, false),
                          numElems);

      std::cout << "Running Overlap kernel." << std::endl;
      Algorithm::Schedule(OverlapKernel(tracker.PrepareForInPlace(DeviceAdapterTag()),
                                        valid.PrepareForInPlace(DeviceAdapterTag()),
                                        dims),
                          dims);

      std::cout << "Checking results." << std::endl;

      auto vPortal = valid.GetPortalConstControl();
      for (vtkm::Id i = 0; i < numElems; i++)
      {
        bool isValid = vPortal.Get(i);
        VTKM_TEST_ASSERT(isValid, "Id3 Schedule executed some elements more than once.");
      }
    } // release memory
  }

  static VTKM_CONT void TestCopyIf()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing CopyIf" << std::endl;

    IdArrayHandle array;
    IdArrayHandle stencil;
    IdArrayHandle result;

    std::cout << "  Standard call" << std::endl;
    //construct the index array
    Algorithm::Schedule(
      OffsetPlusIndexKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
    Algorithm::Schedule(
      MarkOddNumbersKernel(stencil.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);

    Algorithm::CopyIf(array, stencil, result);
    VTKM_TEST_ASSERT(result.GetNumberOfValues() == array.GetNumberOfValues() / 2,
                     "result of CopyIf has an incorrect size");

    for (vtkm::Id index = 0; index < result.GetNumberOfValues(); index++)
    {
      const vtkm::Id value = result.GetPortalConstControl().Get(index);
      VTKM_TEST_ASSERT(value == (OFFSET + (index * 2) + 1), "Incorrect value in CopyIf result.");
    }

    std::cout << "  CopyIf on zero size arrays." << std::endl;
    array.Shrink(0);
    stencil.Shrink(0);
    Algorithm::CopyIf(array, stencil, result);
    VTKM_TEST_ASSERT(result.GetNumberOfValues() == 0, "result of CopyIf has an incorrect size");
  }

  static VTKM_CONT void TestOrderedUniqueValues()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing Sort, Unique, LowerBounds and UpperBounds" << std::endl;
    std::vector<vtkm::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<vtkm::Id>(OFFSET + (i % 50));
    }

    IdArrayHandle input = vtkm::cont::make_ArrayHandle(&(*testData.begin()), ARRAY_SIZE);

    //make a deep copy of input and place it into temp
    IdArrayHandle temp;
    Algorithm::Copy(input, temp);

    Algorithm::Sort(temp);
    Algorithm::Unique(temp);

    IdArrayHandle handle;
    IdArrayHandle handle1;

    //verify lower and upper bounds work
    Algorithm::LowerBounds(temp, input, handle);
    Algorithm::UpperBounds(temp, input, handle1);

    // Check to make sure that temp was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    temp.GetPortalConstControl();     // Forces copy back to control.
    temp.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VTKM_TEST_ASSERT(temp.GetNumberOfValues() == 50,
                     "Unique did not resize array (or size did not copy to control).");

    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id value = handle.GetPortalConstControl().Get(i);
      vtkm::Id value1 = handle1.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(value == i % 50, "Got bad value (LowerBounds)");
      VTKM_TEST_ASSERT(value1 >= i % 50, "Got bad value (UpperBounds)");
    }

    std::cout << "Testing Sort, Unique, LowerBounds and UpperBounds with random values"
              << std::endl;
    //now test it works when the id are not incrementing
    const vtkm::Id RANDOMDATA_SIZE = 6;
    vtkm::Id randomData[RANDOMDATA_SIZE];
    randomData[0] = 500; // 2 (lower), 3 (upper)
    randomData[1] = 955; // 3 (lower), 4 (upper)
    randomData[2] = 955; // 3 (lower), 4 (upper)
    randomData[3] = 120; // 0 (lower), 1 (upper)
    randomData[4] = 320; // 1 (lower), 2 (upper)
    randomData[5] = 955; // 3 (lower), 4 (upper)

    //change the control structure under the handle
    input = vtkm::cont::make_ArrayHandle(randomData, RANDOMDATA_SIZE);
    Algorithm::Copy(input, handle);
    VTKM_TEST_ASSERT(handle.GetNumberOfValues() == RANDOMDATA_SIZE,
                     "Handle incorrect size after setting new control data");

    Algorithm::Copy(input, handle1);
    VTKM_TEST_ASSERT(handle.GetNumberOfValues() == RANDOMDATA_SIZE,
                     "Handle incorrect size after setting new control data");

    Algorithm::Copy(handle, temp);
    VTKM_TEST_ASSERT(temp.GetNumberOfValues() == RANDOMDATA_SIZE, "Copy failed");
    Algorithm::Sort(temp);
    Algorithm::Unique(temp);
    Algorithm::LowerBounds(temp, handle);
    Algorithm::UpperBounds(temp, handle1);

    VTKM_TEST_ASSERT(handle.GetNumberOfValues() == RANDOMDATA_SIZE,
                     "LowerBounds returned incorrect size");

    std::copy(vtkm::cont::ArrayPortalToIteratorBegin(handle.GetPortalConstControl()),
              vtkm::cont::ArrayPortalToIteratorEnd(handle.GetPortalConstControl()),
              randomData);
    VTKM_TEST_ASSERT(randomData[0] == 2, "Got bad value - LowerBounds");
    VTKM_TEST_ASSERT(randomData[1] == 3, "Got bad value - LowerBounds");
    VTKM_TEST_ASSERT(randomData[2] == 3, "Got bad value - LowerBounds");
    VTKM_TEST_ASSERT(randomData[3] == 0, "Got bad value - LowerBounds");
    VTKM_TEST_ASSERT(randomData[4] == 1, "Got bad value - LowerBounds");
    VTKM_TEST_ASSERT(randomData[5] == 3, "Got bad value - LowerBounds");

    VTKM_TEST_ASSERT(handle1.GetNumberOfValues() == RANDOMDATA_SIZE,
                     "UppererBounds returned incorrect size");

    std::copy(vtkm::cont::ArrayPortalToIteratorBegin(handle1.GetPortalConstControl()),
              vtkm::cont::ArrayPortalToIteratorEnd(handle1.GetPortalConstControl()),
              randomData);
    VTKM_TEST_ASSERT(randomData[0] == 3, "Got bad value - UpperBound");
    VTKM_TEST_ASSERT(randomData[1] == 4, "Got bad value - UpperBound");
    VTKM_TEST_ASSERT(randomData[2] == 4, "Got bad value - UpperBound");
    VTKM_TEST_ASSERT(randomData[3] == 1, "Got bad value - UpperBound");
    VTKM_TEST_ASSERT(randomData[4] == 2, "Got bad value - UpperBound");
    VTKM_TEST_ASSERT(randomData[5] == 4, "Got bad value - UpperBound");
  }

  static VTKM_CONT void TestSort()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort" << std::endl;
    std::vector<vtkm::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<vtkm::Id>(OFFSET + ((ARRAY_SIZE - i) % 50));
    }

    IdArrayHandle unsorted = vtkm::cont::make_ArrayHandle(testData);
    IdArrayHandle sorted;
    Algorithm::Copy(unsorted, sorted);

    //Validate the standard inplace sort is correct
    Algorithm::Sort(sorted);

    for (vtkm::Id i = 0; i < ARRAY_SIZE - 1; ++i)
    {
      vtkm::Id sorted1 = sorted.GetPortalConstControl().Get(i);
      vtkm::Id sorted2 = sorted.GetPortalConstControl().Get(i + 1);
      VTKM_TEST_ASSERT(sorted1 <= sorted2, "Values not properly sorted.");
    }

    //Try zero sized array
    sorted.Shrink(0);
    Algorithm::Sort(sorted);
  }

  static VTKM_CONT void TestSortWithComparisonObject()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort with comparison object" << std::endl;
    std::vector<vtkm::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<vtkm::Id>(OFFSET + ((ARRAY_SIZE - i) % 50));
    }

    //sort the users memory in-place
    IdArrayHandle sorted = vtkm::cont::make_ArrayHandle(testData);
    Algorithm::Sort(sorted);

    //copy the sorted array into our own memory, if use the same user ptr
    //we would also sort the 'sorted' handle
    IdArrayHandle comp_sorted;
    Algorithm::Copy(sorted, comp_sorted);
    Algorithm::Sort(comp_sorted, vtkm::SortGreater());

    //Validate that sorted and comp_sorted are sorted in the opposite directions
    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id sorted1 = sorted.GetPortalConstControl().Get(i);
      vtkm::Id sorted2 = comp_sorted.GetPortalConstControl().Get(ARRAY_SIZE - (i + 1));
      VTKM_TEST_ASSERT(sorted1 == sorted2, "Got bad sort values when using SortGreater");
    }

    //validate that sorted and comp_sorted are now equal
    Algorithm::Sort(comp_sorted, vtkm::SortLess());
    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id sorted1 = sorted.GetPortalConstControl().Get(i);
      vtkm::Id sorted2 = comp_sorted.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(sorted1 == sorted2, "Got bad sort values when using SortLess");
    }
  }

  static VTKM_CONT void TestSortWithFancyArrays()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort of a ArrayHandleZip" << std::endl;

    std::vector<vtkm::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<vtkm::Id>(OFFSET + ((ARRAY_SIZE - i) % 50));
    }

    IdArrayHandle unsorted = vtkm::cont::make_ArrayHandle(testData);
    IdArrayHandle sorted;
    Algorithm::Copy(unsorted, sorted);

    //verify that we can use ArrayHandleZip inplace
    vtkm::cont::ArrayHandleZip<IdArrayHandle, IdArrayHandle> zipped(unsorted, sorted);

    //verify we can use sort with zip handle
    Algorithm::Sort(zipped, vtkm::SortGreater());
    Algorithm::Sort(zipped);

    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Pair<vtkm::Id, vtkm::Id> kv_sorted = zipped.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT((OFFSET + (i / (ARRAY_SIZE / 50))) == kv_sorted.first,
                       "ArrayZipHandle improperly sorted");
    }

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort of a ArrayHandlePermutation" << std::endl;

    //verify that we can use ArrayHandlePermutation inplace
    vtkm::cont::ArrayHandleIndex index(ARRAY_SIZE);
    vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandleIndex, IdArrayHandle> perm(index,
                                                                                         sorted);

    //verify we can use a custom operator sort with permutation handle
    Algorithm::Sort(perm, vtkm::SortGreater());
    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id sorted_value = perm.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT((OFFSET + ((ARRAY_SIZE - (i + 1)) / (ARRAY_SIZE / 50))) == sorted_value,
                       "ArrayZipPermutation improperly sorted");
    }

    //verify we can use the default sort with permutation handle
    Algorithm::Sort(perm);
    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id sorted_value = perm.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT((OFFSET + (i / (ARRAY_SIZE / 50))) == sorted_value,
                       "ArrayZipPermutation improperly sorted");
    }
  }

  static VTKM_CONT void TestSortByKey()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort by keys" << std::endl;

    using Vec3 = vtkm::Vec<FloatDefault, 3>;
    using Vec3ArrayHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f, StorageTag>;

    std::vector<vtkm::Id> testKeys(ARRAY_SIZE);
    std::vector<Vec3> testValues(testKeys.size());

    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      std::size_t index = static_cast<size_t>(i);
      testKeys[index] = ARRAY_SIZE - i;
      testValues[index] = TestValue(i, Vec3());
    }

    IdArrayHandle keys = vtkm::cont::make_ArrayHandle(testKeys);
    Vec3ArrayHandle values = vtkm::cont::make_ArrayHandle(testValues);

    Algorithm::SortByKey(keys, values);

    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      //keys should be sorted from 1 to ARRAY_SIZE
      //values should be sorted from (ARRAY_SIZE-1) to 0
      Vec3 sorted_value = values.GetPortalConstControl().Get(i);
      vtkm::Id sorted_key = keys.GetPortalConstControl().Get(i);

      VTKM_TEST_ASSERT((sorted_key == (i + 1)), "Got bad SortByKeys key");
      VTKM_TEST_ASSERT(test_equal(sorted_value, TestValue(ARRAY_SIZE - 1 - i, Vec3())),
                       "Got bad SortByKeys value");
    }

    // this will return everything back to what it was before sorting
    Algorithm::SortByKey(keys, values, vtkm::SortGreater());
    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      //keys should be sorted from ARRAY_SIZE to 1
      //values should be sorted from 0 to (ARRAY_SIZE-1)
      Vec3 sorted_value = values.GetPortalConstControl().Get(i);
      vtkm::Id sorted_key = keys.GetPortalConstControl().Get(i);

      VTKM_TEST_ASSERT((sorted_key == (ARRAY_SIZE - i)), "Got bad SortByKeys key");
      VTKM_TEST_ASSERT(test_equal(sorted_value, TestValue(i, Vec3())), "Got bad SortByKeys value");
    }

    //this is here to verify we can sort by vtkm::Vec
    Algorithm::SortByKey(values, keys);
    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      //keys should be sorted from ARRAY_SIZE to 1
      //values should be sorted from 0 to (ARRAY_SIZE-1)
      Vec3 sorted_value = values.GetPortalConstControl().Get(i);
      vtkm::Id sorted_key = keys.GetPortalConstControl().Get(i);

      VTKM_TEST_ASSERT((sorted_key == (ARRAY_SIZE - i)), "Got bad SortByKeys key");
      VTKM_TEST_ASSERT(test_equal(sorted_value, TestValue(i, Vec3())), "Got bad SortByKeys value");
    }
  }

  static VTKM_CONT void TestLowerBoundsWithComparisonObject()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing LowerBounds with comparison object" << std::endl;
    std::vector<vtkm::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<vtkm::Id>(OFFSET + (i % 50));
    }
    IdArrayHandle input = vtkm::cont::make_ArrayHandle(testData);

    //make a deep copy of input and place it into temp
    IdArrayHandle temp;
    Algorithm::Copy(input, temp);

    Algorithm::Sort(temp);
    Algorithm::Unique(temp);

    IdArrayHandle handle;
    //verify lower bounds work
    Algorithm::LowerBounds(temp, input, handle, vtkm::SortLess());

    // Check to make sure that temp was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    temp.GetPortalConstControl();     // Forces copy back to control.
    temp.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VTKM_TEST_ASSERT(temp.GetNumberOfValues() == 50,
                     "Unique did not resize array (or size did not copy to control).");

    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id value = handle.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(value == i % 50, "Got bad LowerBounds value with SortLess");
    }
  }

  static VTKM_CONT void TestUpperBoundsWithComparisonObject()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing UpperBounds with comparison object" << std::endl;
    std::vector<vtkm::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<vtkm::Id>(OFFSET + (i % 50));
    }
    IdArrayHandle input = vtkm::cont::make_ArrayHandle(testData);

    //make a deep copy of input and place it into temp
    IdArrayHandle temp;
    Algorithm::Copy(input, temp);

    Algorithm::Sort(temp);
    Algorithm::Unique(temp);

    IdArrayHandle handle;
    //verify upper bounds work
    Algorithm::UpperBounds(temp, input, handle, vtkm::SortLess());

    // Check to make sure that temp was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    temp.GetPortalConstControl();     // Forces copy back to control.
    temp.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VTKM_TEST_ASSERT(temp.GetNumberOfValues() == 50,
                     "Unique did not resize array (or size did not copy to control).");

    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id value = handle.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(value == (i % 50) + 1, "Got bad UpperBounds value with SortLess");
    }
  }

  static VTKM_CONT void TestUniqueWithComparisonObject()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing Unique with comparison object" << std::endl;
    std::vector<vtkm::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<vtkm::Id>(OFFSET + (i % 50));
    }
    IdArrayHandle input = vtkm::cont::make_ArrayHandle(testData);
    Algorithm::Sort(input);
    Algorithm::Unique(input, FuseAll());

    // Check to make sure that input was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    input.GetPortalConstControl();     // Forces copy back to control.
    input.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VTKM_TEST_ASSERT(input.GetNumberOfValues() == 1,
                     "Unique did not resize array (or size did not copy to control).");

    vtkm::Id value = input.GetPortalConstControl().Get(0);
    VTKM_TEST_ASSERT(value == OFFSET, "Got bad unique value");
  }

  static VTKM_CONT void TestReduce()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce" << std::endl;

    //construct the index array
    IdArrayHandle array;
    Algorithm::Schedule(ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                        ARRAY_SIZE);

    //the output of reduce and scan inclusive should be the same
    std::cout << "  Reduce with initial value of 0." << std::endl;
    vtkm::Id reduce_sum = Algorithm::Reduce(array, vtkm::Id(0));
    std::cout << "  Reduce with initial value." << std::endl;
    vtkm::Id reduce_sum_with_intial_value = Algorithm::Reduce(array, vtkm::Id(ARRAY_SIZE));
    std::cout << "  Inclusive scan to check" << std::endl;
    vtkm::Id inclusive_sum = Algorithm::ScanInclusive(array, array);
    std::cout << "  Reduce with 1 value." << std::endl;
    array.Shrink(1);
    vtkm::Id reduce_sum_one_value = Algorithm::Reduce(array, vtkm::Id(0));
    std::cout << "  Reduce with 0 values." << std::endl;
    array.Shrink(0);
    vtkm::Id reduce_sum_no_values = Algorithm::Reduce(array, vtkm::Id(0));
    VTKM_TEST_ASSERT(reduce_sum == OFFSET * ARRAY_SIZE, "Got bad sum from Reduce");
    VTKM_TEST_ASSERT(reduce_sum_with_intial_value == reduce_sum + ARRAY_SIZE,
                     "Got bad sum from Reduce with initial value");
    VTKM_TEST_ASSERT(reduce_sum_one_value == OFFSET, "Got bad single sum from Reduce");
    VTKM_TEST_ASSERT(reduce_sum_no_values == 0, "Got bad empty sum from Reduce");

    VTKM_TEST_ASSERT(reduce_sum == inclusive_sum,
                     "Got different sums from Reduce and ScanInclusive");
  }

  static VTKM_CONT void TestReduceWithComparisonObject()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce with comparison object " << std::endl;


    std::cout << "  Reduce vtkm::Id array with vtkm::MinAndMax to compute range." << std::endl;
    //construct the index array. Assign an abnormally large value
    //to the middle of the array, that should be what we see as our sum.
    std::vector<vtkm::Id> testData(ARRAY_SIZE);
    const vtkm::Id maxValue = ARRAY_SIZE * 2;
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id index = static_cast<vtkm::Id>(i);
      testData[i] = index;
    }
    testData[ARRAY_SIZE / 2] = maxValue;

    IdArrayHandle input = vtkm::cont::make_ArrayHandle(testData);
    vtkm::Id2 range = Algorithm::Reduce(input, vtkm::Id2(0, 0), vtkm::MinAndMax<vtkm::Id>());

    VTKM_TEST_ASSERT(maxValue == range[1], "Got bad value from Reduce with comparison object");
    VTKM_TEST_ASSERT(0 == range[0], "Got bad value from Reduce with comparison object");


    std::cout << "  Reduce vtkm::Id array with custom functor that returns vtkm::Pair<>."
              << std::endl;
    auto pairInit = vtkm::Pair<vtkm::Id, vtkm::Float32>(0, 0.0f);
    vtkm::Pair<vtkm::Id, vtkm::Float32> pairRange =
      Algorithm::Reduce(input, pairInit, CustomPairOp());

    VTKM_TEST_ASSERT(maxValue == pairRange.first,
                     "Got bad value from Reduce with pair comparison object");
    VTKM_TEST_ASSERT(0.0f == pairRange.second,
                     "Got bad value from Reduce with pair comparison object");


    std::cout << "  Reduce bool array with vtkm::LogicalAnd to see if all values are true."
              << std::endl;
    //construct an array of bools and verify that they aren't all true
    constexpr vtkm::Id inputLength = 60;
    constexpr bool inputValues[inputLength] = {
      true, true, true, true, true, true, false, true, true, true, true, true, true, true, true,
      true, true, true, true, true, true, true,  true, true, true, true, true, true, true, true,
      true, true, true, true, true, true, true,  true, true, true, true, true, true, true, true,
      true, true, true, true, true, true, true,  true, true, true, true, true, true, true, true
    };
    auto barray = vtkm::cont::make_ArrayHandle(inputValues, inputLength);
    bool all_true = Algorithm::Reduce(barray, true, vtkm::LogicalAnd());
    VTKM_TEST_ASSERT(all_true == false, "reduction with vtkm::LogicalAnd should return false");

    std::cout << "  Reduce with custom value type and custom comparison operator." << std::endl;
    //test with a custom value type with the reduction value being a vtkm::Vec<float,2>
    constexpr CustomTForReduce inputFValues[inputLength] = {
      13.1f, -2.1f, -1.0f,  13.1f, -2.1f, -1.0f, 413.1f, -2.1f, -1.0f, 13.1f,  -2.1f,   -1.0f,
      13.1f, -2.1f, -1.0f,  13.1f, -2.1f, -1.0f, 13.1f,  -2.1f, -1.0f, 13.1f,  -2.1f,   -1.0f,
      13.1f, -2.1f, -11.0f, 13.1f, -2.1f, -1.0f, 13.1f,  -2.1f, -1.0f, 13.1f,  -2.1f,   -1.0f,
      13.1f, -2.1f, -1.0f,  13.1f, -2.1f, -1.0f, 13.1f,  -2.1f, -1.0f, 13.1f,  -211.1f, -1.0f,
      13.1f, -2.1f, -1.0f,  13.1f, -2.1f, -1.0f, 13.1f,  -2.1f, -1.0f, 113.1f, -2.1f,   -1.0f
    };
    auto farray = vtkm::cont::make_ArrayHandle(inputFValues, inputLength);
    vtkm::Vec2f_32 frange =
      Algorithm::Reduce(farray, vtkm::Vec2f_32(0.0f, 0.0f), CustomMinAndMax<CustomTForReduce>());
    VTKM_TEST_ASSERT(-211.1f == frange[0],
                     "Got bad float value from Reduce with comparison object");
    VTKM_TEST_ASSERT(413.1f == frange[1], "Got bad float value from Reduce with comparison object");
  }

  static VTKM_CONT void TestReduceWithFancyArrays()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce with ArrayHandleZip" << std::endl;
    {
      IdArrayHandle keys, values;
      Algorithm::Schedule(ClearArrayKernel(keys.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                          ARRAY_SIZE);

      Algorithm::Schedule(ClearArrayKernel(values.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                          ARRAY_SIZE);

      vtkm::cont::ArrayHandleZip<IdArrayHandle, IdArrayHandle> zipped(keys, values);

      //the output of reduce and scan inclusive should be the same
      using ResultType = vtkm::Pair<vtkm::Id, vtkm::Id>;
      ResultType reduce_sum_with_intial_value =
        Algorithm::Reduce(zipped, ResultType(ARRAY_SIZE, ARRAY_SIZE));

      ResultType expectedResult(OFFSET * ARRAY_SIZE + ARRAY_SIZE, OFFSET * ARRAY_SIZE + ARRAY_SIZE);
      VTKM_TEST_ASSERT((reduce_sum_with_intial_value == expectedResult),
                       "Got bad sum from Reduce with initial value");
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce with ArrayHandlePermutation" << std::endl;
    {
      //lastly test with heterogeneous zip values ( vec3, and constant array handle),
      //and a custom reduce binary functor
      const vtkm::Id indexLength = 30;
      const vtkm::Id valuesLength = 10;
      using ValueType = vtkm::Float32;

      vtkm::Id indexs[indexLength] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4,
                                       5, 5, 5, 1, 4, 9, 7, 7, 7, 8, 8, 8, 0, 1, 2 };
      ValueType values[valuesLength] = {
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, -2.0f
      };
      const ValueType expectedSum = 125;

      IdArrayHandle indexHandle = vtkm::cont::make_ArrayHandle(indexs, indexLength);
      vtkm::cont::ArrayHandle<ValueType> valueHandle =
        vtkm::cont::make_ArrayHandle(values, valuesLength);

      vtkm::cont::ArrayHandlePermutation<IdArrayHandle, vtkm::cont::ArrayHandle<ValueType>> perm;
      perm = vtkm::cont::make_ArrayHandlePermutation(indexHandle, valueHandle);

      const ValueType sum = Algorithm::Reduce(perm, ValueType(0.0f));

      std::cout << "sum: " << sum << std::endl;
      VTKM_TEST_ASSERT((sum == expectedSum), "Got bad sum from Reduce with permutation handle");
    }
  }

  static VTKM_CONT void TestReduceByKey()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce By Key" << std::endl;

    //first test with very basic integer key / values
    {
      const vtkm::Id inputLength = 12;
      const vtkm::Id expectedLength = 6;
      vtkm::IdComponent inputKeys[inputLength] = { 0, 0, 0, 1, 1, 4, 0, 2, 2, 2, 2, -1 }; // in keys
      vtkm::Id inputValues[inputLength] = { 13, -2, -1, 1, 1, 0, 3, 1, 2, 3, 4, -42 }; // in values
      vtkm::IdComponent expectedKeys[expectedLength] = { 0, 1, 4, 0, 2, -1 };
      vtkm::Id expectedValues[expectedLength] = { 10, 2, 0, 3, 10, -42 };

      IdComponentArrayHandle keys = vtkm::cont::make_ArrayHandle(inputKeys, inputLength);
      IdArrayHandle values = vtkm::cont::make_ArrayHandle(inputValues, inputLength);

      IdComponentArrayHandle keysOut;
      IdArrayHandle valuesOut;
      Algorithm::ReduceByKey(keys, values, keysOut, valuesOut, vtkm::Add());

      VTKM_TEST_ASSERT(keysOut.GetNumberOfValues() == expectedLength,
                       "Got wrong number of output keys");

      VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                       "Got wrong number of output values");

      for (vtkm::Id i = 0; i < expectedLength; ++i)
      {
        const vtkm::Id k = keysOut.GetPortalConstControl().Get(i);
        const vtkm::Id v = valuesOut.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(expectedKeys[i] == k, "Incorrect reduced key");
        VTKM_TEST_ASSERT(expectedValues[i] == v, "Incorrect reduced value");
      }
    }

    //next test with a single key across the entire set, using vec3 as the
    //value, using a custom reduce binary functor
    {
      const vtkm::Id inputLength = 3;
      const vtkm::Id expectedLength = 1;
      vtkm::Id inputKeys[inputLength] = { 0, 0, 0 }; // input keys
      vtkm::Vec3f_64 inputValues[inputLength];
      inputValues[0] = vtkm::make_Vec(13.1, 13.3, 13.5);
      inputValues[1] = vtkm::make_Vec(-2.1, -2.3, -2.5);
      inputValues[2] = vtkm::make_Vec(-1.0, -1.0, 1.0); // input keys
      vtkm::Id expectedKeys[expectedLength] = { 0 };

      vtkm::Vec3f_64 expectedValues[expectedLength];
      expectedValues[0] = vtkm::make_Vec(27.51, 30.59, -33.75);

      IdArrayHandle keys = vtkm::cont::make_ArrayHandle(inputKeys, inputLength);
      vtkm::cont::ArrayHandle<vtkm::Vec3f_64, StorageTag> values =
        vtkm::cont::make_ArrayHandle(inputValues, inputLength);

      IdArrayHandle keysOut;
      vtkm::cont::ArrayHandle<vtkm::Vec3f_64, StorageTag> valuesOut;
      Algorithm::ReduceByKey(keys, values, keysOut, valuesOut, vtkm::Multiply());

      VTKM_TEST_ASSERT(keysOut.GetNumberOfValues() == expectedLength,
                       "Got wrong number of output keys");

      VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                       "Got wrong number of output values");

      for (vtkm::Id i = 0; i < expectedLength; ++i)
      {
        const vtkm::Id k = keysOut.GetPortalConstControl().Get(i);
        const vtkm::Vec3f_64 v = valuesOut.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(expectedKeys[i] == k, "Incorrect reduced key");
        VTKM_TEST_ASSERT(expectedValues[i] == v, "Incorrect reduced vale");
      }
    }
  }

  static VTKM_CONT void TestReduceByKeyWithFancyArrays()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce By Key with Fancy Arrays" << std::endl;

    //lastly test with heterogeneous zip values ( vec3, and constant array handle),
    //and a custom reduce binary functor
    const vtkm::Id inputLength = 30;
    const vtkm::Id expectedLength = 10;
    using ValueType = vtkm::Float32;
    vtkm::Id inputKeys[inputLength] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4,
                                        5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9 }; // input keys
    ValueType inputValues1[inputLength] = {
      13.1f, -2.1f, -1.0f, 13.1f, -2.1f, -1.0f, 13.1f, -2.1f, -1.0f, 13.1f,
      -2.1f, -1.0f, 13.1f, -2.1f, -1.0f, 13.1f, -2.1f, -1.0f, 13.1f, -2.1f,
      -1.0f, 13.1f, -2.1f, -1.0f, 13.1f, -2.1f, -1.0f, 13.1f, -2.1f, -1.0f
    }; // input values array1
    vtkm::Id expectedKeys[expectedLength] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    ValueType expectedValues1[expectedLength] = { 10.f, 10.f, 10.f, 10.f, 10.f,
                                                  10.f, 10.f, 10.f, 10.f, 10.f }; // output values 1
    ValueType expectedValues2[expectedLength] = {
      3.f, 3.f, 3.f, 3.f, 3.f, 3.f, 3.f, 3.f, 3.f, 3.f
    }; // output values 2

    IdArrayHandle keys = vtkm::cont::make_ArrayHandle(inputKeys, inputLength);
    using ValueArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
    ValueArrayType values1 = vtkm::cont::make_ArrayHandle(inputValues1, inputLength);
    using ConstValueArrayType = vtkm::cont::ArrayHandleConstant<ValueType>;
    ConstValueArrayType constOneArray(1.f, inputLength);

    vtkm::cont::ArrayHandleZip<ValueArrayType, ConstValueArrayType> valuesZip;
    valuesZip = make_ArrayHandleZip(values1, constOneArray); // values in zip

    IdArrayHandle keysOut;
    ValueArrayType valuesOut1;
    ValueArrayType valuesOut2;
    vtkm::cont::ArrayHandleZip<ValueArrayType, ValueArrayType> valuesOutZip(valuesOut1, valuesOut2);

    Algorithm::ReduceByKey(keys, valuesZip, keysOut, valuesOutZip, vtkm::Add());

    VTKM_TEST_ASSERT(keysOut.GetNumberOfValues() == expectedLength,
                     "Got wrong number of output keys");

    VTKM_TEST_ASSERT(valuesOutZip.GetNumberOfValues() == expectedLength,
                     "Got wrong number of output values");

    for (vtkm::Id i = 0; i < expectedLength; ++i)
    {
      const vtkm::Id k = keysOut.GetPortalConstControl().Get(i);
      const vtkm::Pair<ValueType, ValueType> v = valuesOutZip.GetPortalConstControl().Get(i);
      std::cout << "key=" << k << ","
                << "expectedValues1[i] = " << expectedValues1[i] << ","
                << "computed value1 = " << v.first << std::endl;
      VTKM_TEST_ASSERT(expectedKeys[i] == k, "Incorrect reduced key");
      VTKM_TEST_ASSERT(expectedValues1[i] == v.first, "Incorrect reduced value1");
      VTKM_TEST_ASSERT(expectedValues2[i] == v.second, "Incorrect reduced value2");
    }
  }

  static VTKM_CONT void TestScanInclusiveByKeyOne()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Inclusive By Key with 1 elements" << std::endl;

    const vtkm::Id inputLength = 1;
    vtkm::Id inputKeys[inputLength] = { 0 };
    vtkm::Id inputValues[inputLength] = { 5 };

    const vtkm::Id expectedLength = 1;

    IdArrayHandle keys = vtkm::cont::make_ArrayHandle(inputKeys, inputLength);
    IdArrayHandle values = vtkm::cont::make_ArrayHandle(inputValues, inputLength);

    IdArrayHandle valuesOut;

    Algorithm::ScanInclusiveByKey(keys, values, valuesOut, vtkm::Add());

    VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                     "Got wrong number of output values");
    const vtkm::Id v = valuesOut.GetPortalConstControl().Get(0);
    VTKM_TEST_ASSERT(5 == v, "Incorrect scanned value");
  }

  static VTKM_CONT void TestScanInclusiveByKeyTwo()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Exclusive By Key with 2 elements" << std::endl;

    const vtkm::Id inputLength = 2;
    vtkm::Id inputKeys[inputLength] = { 0, 1 };
    vtkm::Id inputValues[inputLength] = { 1, 1 };

    const vtkm::Id expectedLength = 2;
    vtkm::Id expectedValues[expectedLength] = { 1, 1 };

    IdArrayHandle keys = vtkm::cont::make_ArrayHandle(inputKeys, inputLength);
    IdArrayHandle values = vtkm::cont::make_ArrayHandle(inputValues, inputLength);

    IdArrayHandle valuesOut;

    Algorithm::ScanInclusiveByKey(keys, values, valuesOut, vtkm::Add());

    VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                     "Got wrong number of output values");
    for (vtkm::Id i = 0; i < expectedLength; i++)
    {
      const vtkm::Id v = valuesOut.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v, "Incorrect scanned value");
    }
  }
  static VTKM_CONT void TestScanInclusiveByKeyLarge()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Inclusive By Key with " << ARRAY_SIZE << " elements" << std::endl;

    const vtkm::Id inputLength = ARRAY_SIZE;
    std::vector<vtkm::Id> inputKeys(inputLength);

    for (vtkm::Id i = 0; i < ARRAY_SIZE; i++)
    {
      if (i % 100 < 98)
        inputKeys[static_cast<std::size_t>(i)] = static_cast<vtkm::Id>(i / 100);
      else
        inputKeys[static_cast<std::size_t>(i)] = static_cast<vtkm::Id>(i);
    }
    std::vector<vtkm::Id> inputValues(inputLength, 1);

    const vtkm::Id expectedLength = ARRAY_SIZE;
    std::vector<vtkm::Id> expectedValues(expectedLength);
    for (std::size_t i = 0; i < ARRAY_SIZE; i++)
    {
      if (i % 100 < 98)
        expectedValues[i] = static_cast<vtkm::Id>(1 + i % 100);
      else
        expectedValues[i] = static_cast<vtkm::Id>(1);
    }

    IdArrayHandle keys = vtkm::cont::make_ArrayHandle(inputKeys);
    IdArrayHandle values = vtkm::cont::make_ArrayHandle(inputValues);

    IdArrayHandle valuesOut;

    Algorithm::ScanInclusiveByKey(keys, values, valuesOut, vtkm::Add());

    VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                     "Got wrong number of output values");
    for (auto i = 0; i < expectedLength; i++)
    {
      const vtkm::Id v = valuesOut.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v, "Incorrect scanned value");
    }
  }
  static VTKM_CONT void TestScanInclusiveByKey()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Inclusive By Key" << std::endl;

    const vtkm::Id inputLength = 10;
    vtkm::IdComponent inputKeys[inputLength] = { 0, 0, 0, 1, 1, 2, 3, 3, 3, 3 };
    vtkm::Id inputValues[inputLength] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

    const vtkm::Id expectedLength = 10;
    vtkm::Id expectedValues[expectedLength] = { 1, 2, 3, 1, 2, 1, 1, 2, 3, 4 };

    IdComponentArrayHandle keys = vtkm::cont::make_ArrayHandle(inputKeys, inputLength);
    IdArrayHandle values = vtkm::cont::make_ArrayHandle(inputValues, inputLength);

    IdArrayHandle valuesOut;

    Algorithm::ScanInclusiveByKey(keys, values, valuesOut);
    VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                     "Got wrong number of output values");
    for (auto i = 0; i < expectedLength; i++)
    {
      const vtkm::Id v = valuesOut.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v, "Incorrect scanned value");
    }
  }

  static VTKM_CONT void TestScanExclusiveByKeyOne()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Exclusive By Key with 1 elements" << std::endl;

    const vtkm::Id inputLength = 1;
    vtkm::Id inputKeys[inputLength] = { 0 };
    vtkm::Id inputValues[inputLength] = { 0 };
    vtkm::Id init = 5;

    const vtkm::Id expectedLength = 1;

    IdArrayHandle keys = vtkm::cont::make_ArrayHandle(inputKeys, inputLength);
    IdArrayHandle values = vtkm::cont::make_ArrayHandle(inputValues, inputLength);

    IdArrayHandle valuesOut;

    Algorithm::ScanExclusiveByKey(keys, values, valuesOut, init, vtkm::Add());

    VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                     "Got wrong number of output values");
    const vtkm::Id v = valuesOut.GetPortalConstControl().Get(0);
    VTKM_TEST_ASSERT(init == v, "Incorrect scanned value");
  }

  static VTKM_CONT void TestScanExclusiveByKeyTwo()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Exclusive By Key with 2 elements" << std::endl;

    const vtkm::Id inputLength = 2;
    vtkm::Id inputKeys[inputLength] = { 0, 1 };
    vtkm::Id inputValues[inputLength] = { 1, 1 };
    vtkm::Id init = 5;

    const vtkm::Id expectedLength = 2;
    vtkm::Id expectedValues[expectedLength] = { 5, 5 };

    IdArrayHandle keys = vtkm::cont::make_ArrayHandle(inputKeys, inputLength);
    IdArrayHandle values = vtkm::cont::make_ArrayHandle(inputValues, inputLength);

    IdArrayHandle valuesOut;

    Algorithm::ScanExclusiveByKey(keys, values, valuesOut, init, vtkm::Add());

    VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                     "Got wrong number of output values");
    for (auto i = 0; i < expectedLength; i++)
    {
      const vtkm::Id v = valuesOut.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(expectedValues[i] == v, "Incorrect scanned value");
    }
  }

  static VTKM_CONT void TestScanExclusiveByKeyLarge()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Exclusive By Key with " << ARRAY_SIZE << " elements" << std::endl;

    const vtkm::Id inputLength = ARRAY_SIZE;
    std::vector<vtkm::Id> inputKeys(inputLength);
    for (std::size_t i = 0; i < ARRAY_SIZE; i++)
    {
      if (i % 100 < 98)
        inputKeys[i] = static_cast<vtkm::Id>(i / 100);
      else
        inputKeys[i] = static_cast<vtkm::Id>(i);
    }
    std::vector<vtkm::Id> inputValues(inputLength, 1);
    vtkm::Id init = 5;

    const vtkm::Id expectedLength = ARRAY_SIZE;
    std::vector<vtkm::Id> expectedValues(expectedLength);
    for (vtkm::Id i = 0; i < ARRAY_SIZE; i++)
    {
      if (i % 100 < 98)
        expectedValues[static_cast<std::size_t>(i)] = static_cast<vtkm::Id>(init + i % 100);
      else
        expectedValues[static_cast<std::size_t>(i)] = init;
    }

    IdArrayHandle keys = vtkm::cont::make_ArrayHandle(inputKeys);
    IdArrayHandle values = vtkm::cont::make_ArrayHandle(inputValues);

    IdArrayHandle valuesOut;

    Algorithm::ScanExclusiveByKey(keys, values, valuesOut, init, vtkm::Add());

    VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                     "Got wrong number of output values");
    for (vtkm::Id i = 0; i < expectedLength; i++)
    {
      const vtkm::Id v = valuesOut.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v, "Incorrect scanned value");
    }
  }

  static VTKM_CONT void TestScanExclusiveByKey()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Exclusive By Key" << std::endl;

    const vtkm::Id inputLength = 10;
    vtkm::IdComponent inputKeys[inputLength] = { 0, 0, 0, 1, 1, 2, 3, 3, 3, 3 };
    vtkm::Id inputValues[inputLength] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    vtkm::Id init = 5;

    const vtkm::Id expectedLength = 10;
    vtkm::Id expectedValues[expectedLength] = { 5, 6, 7, 5, 6, 5, 5, 6, 7, 8 };

    IdComponentArrayHandle keys = vtkm::cont::make_ArrayHandle(inputKeys, inputLength);
    IdArrayHandle values = vtkm::cont::make_ArrayHandle(inputValues, inputLength);

    IdArrayHandle valuesOut;

    Algorithm::ScanExclusiveByKey(keys, values, valuesOut, init, vtkm::Add());

    VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                     "Got wrong number of output values");
    for (vtkm::Id i = 0; i < expectedLength; i++)
    {
      const vtkm::Id v = valuesOut.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v, "Incorrect scanned value");
    }
  }

  static VTKM_CONT void TestScanInclusive()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Inclusive Scan" << std::endl;

    {
      std::cout << "  size " << ARRAY_SIZE << std::endl;
      //construct the index array
      IdArrayHandle array;
      Algorithm::Schedule(ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                          ARRAY_SIZE);

      //we know have an array whose sum is equal to OFFSET * ARRAY_SIZE,
      //let's validate that
      vtkm::Id sum = Algorithm::ScanInclusive(array, array);
      VTKM_TEST_ASSERT(sum == OFFSET * ARRAY_SIZE, "Got bad sum from Inclusive Scan");

      for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        const vtkm::Id value = array.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(value == (i + 1) * OFFSET, "Incorrect partial sum");
      }

      std::cout << "  size 1" << std::endl;
      array.Shrink(1);
      sum = Algorithm::ScanInclusive(array, array);
      VTKM_TEST_ASSERT(sum == OFFSET, "Incorrect partial sum");
      const vtkm::Id value = array.GetPortalConstControl().Get(0);
      VTKM_TEST_ASSERT(value == OFFSET, "Incorrect partial sum");

      std::cout << "  size 0" << std::endl;
      array.Shrink(0);
      sum = Algorithm::ScanInclusive(array, array);
      VTKM_TEST_ASSERT(sum == 0, "Incorrect partial sum");
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Inclusive Scan with multiplication operator" << std::endl;
    {
      std::vector<vtkm::Float64> inputValues(ARRAY_SIZE);
      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        inputValues[i] = 1.01;
      }

      std::size_t mid = ARRAY_SIZE / 2;
      inputValues[mid] = 0.0;

      vtkm::cont::ArrayHandle<vtkm::Float64> array =
        vtkm::cont::make_ArrayHandle(&inputValues[0], ARRAY_SIZE);

      vtkm::Float64 product = Algorithm::ScanInclusive(array, array, vtkm::Multiply());

      VTKM_TEST_ASSERT(product == 0.0f, "ScanInclusive product result not 0.0");
      for (std::size_t i = 0; i < mid; ++i)
      {
        vtkm::Id index = static_cast<vtkm::Id>(i);
        vtkm::Float64 expected = pow(1.01, static_cast<vtkm::Float64>(i + 1));
        vtkm::Float64 got = array.GetPortalConstControl().Get(index);
        VTKM_TEST_ASSERT(test_equal(got, expected), "Incorrect results for ScanInclusive");
      }
      for (std::size_t i = mid; i < ARRAY_SIZE; ++i)
      {
        vtkm::Id index = static_cast<vtkm::Id>(i);
        VTKM_TEST_ASSERT(array.GetPortalConstControl().Get(index) == 0.0f,
                         "Incorrect results for ScanInclusive");
      }
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Inclusive Scan with a vtkm::Vec" << std::endl;

    {
      using Vec3 = vtkm::Vec<Float64, 3>;
      using Vec3ArrayHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f_64, StorageTag>;

      std::vector<Vec3> testValues(ARRAY_SIZE);

      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        testValues[i] = TestValue(1, Vec3());
      }
      Vec3ArrayHandle values = vtkm::cont::make_ArrayHandle(testValues);

      Vec3 sum = Algorithm::ScanInclusive(values, values);
      std::cout << "Sum that was returned " << sum << std::endl;
      VTKM_TEST_ASSERT(test_equal(sum, TestValue(1, Vec3()) * ARRAY_SIZE),
                       "Got bad sum from Inclusive Scan");
    }
  }

  static VTKM_CONT void TestScanInclusiveWithComparisonObject()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Inclusive Scan with comparison object " << std::endl;

    //construct the index array
    IdArrayHandle array;
    Algorithm::Schedule(ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                        ARRAY_SIZE);

    Algorithm::Schedule(AddArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                        ARRAY_SIZE);
    //we know have an array whose sum is equal to OFFSET * ARRAY_SIZE,
    //let's validate that
    IdArrayHandle result;
    vtkm::Id sum = Algorithm::ScanInclusive(array, result, vtkm::Maximum());
    VTKM_TEST_ASSERT(sum == OFFSET + (ARRAY_SIZE - 1),
                     "Got bad sum from Inclusive Scan with comparison object");

    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      const vtkm::Id input_value = array.GetPortalConstControl().Get(i);
      const vtkm::Id result_value = result.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(input_value == result_value, "Incorrect partial sum");
    }

    //now try it inline
    sum = Algorithm::ScanInclusive(array, array, vtkm::Maximum());
    VTKM_TEST_ASSERT(sum == OFFSET + (ARRAY_SIZE - 1),
                     "Got bad sum from Inclusive Scan with comparison object");

    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      const vtkm::Id input_value = array.GetPortalConstControl().Get(i);
      const vtkm::Id result_value = result.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(input_value == result_value, "Incorrect partial sum");
    }
  }

  static VTKM_CONT void TestScanExclusive()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Exclusive Scan" << std::endl;

    {
      std::cout << "  size " << ARRAY_SIZE << std::endl;
      //construct the index array
      IdArrayHandle array;
      Algorithm::Schedule(ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                          ARRAY_SIZE);

      // we know have an array whose sum = (OFFSET * ARRAY_SIZE),
      // let's validate that
      vtkm::Id sum = Algorithm::ScanExclusive(array, array);
      std::cout << "  Sum that was returned " << sum << std::endl;
      VTKM_TEST_ASSERT(sum == (OFFSET * ARRAY_SIZE), "Got bad sum from Exclusive Scan");

      for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        const vtkm::Id value = array.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(value == i * OFFSET, "Incorrect partial sum");
      }

      std::cout << "  size 1" << std::endl;
      array.Shrink(1);
      array.GetPortalControl().Set(0, OFFSET);
      sum = Algorithm::ScanExclusive(array, array);
      VTKM_TEST_ASSERT(sum == OFFSET, "Incorrect partial sum");
      const vtkm::Id value = array.GetPortalConstControl().Get(0);
      VTKM_TEST_ASSERT(value == 0, "Incorrect partial sum");

      std::cout << "  size 0" << std::endl;
      array.Shrink(0);
      sum = Algorithm::ScanExclusive(array, array);
      VTKM_TEST_ASSERT(sum == 0, "Incorrect partial sum");
    }

    // Enable when Exclusive Scan with custom operator is implemented for all
    // device adaptors
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Exclusive Scan with multiplication operator" << std::endl;
    {
      std::vector<vtkm::Float64> inputValues(ARRAY_SIZE);
      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        inputValues[i] = 1.01;
      }

      std::size_t mid = ARRAY_SIZE / 2;
      inputValues[mid] = 0.0;

      vtkm::cont::ArrayHandle<vtkm::Float64> array = vtkm::cont::make_ArrayHandle(inputValues);

      vtkm::Float64 initialValue = 2.00;
      vtkm::Float64 product =
        Algorithm::ScanExclusive(array, array, vtkm::Multiply(), initialValue);

      VTKM_TEST_ASSERT(product == 0.0f, "ScanExclusive product result not 0.0");
      VTKM_TEST_ASSERT(array.GetPortalConstControl().Get(0) == initialValue,
                       "ScanExclusive result's first value != initialValue");
      for (std::size_t i = 1; i <= mid; ++i)
      {
        vtkm::Id index = static_cast<vtkm::Id>(i);
        vtkm::Float64 expected = pow(1.01, static_cast<vtkm::Float64>(i)) * initialValue;
        vtkm::Float64 got = array.GetPortalConstControl().Get(index);
        VTKM_TEST_ASSERT(test_equal(got, expected), "Incorrect results for ScanExclusive");
      }
      for (std::size_t i = mid + 1; i < ARRAY_SIZE; ++i)
      {
        vtkm::Id index = static_cast<vtkm::Id>(i);
        VTKM_TEST_ASSERT(array.GetPortalConstControl().Get(index) == 0.0f,
                         "Incorrect results for ScanExclusive");
      }
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Exclusive Scan with a vtkm::Vec" << std::endl;

    {
      using Vec3 = vtkm::Vec<Float64, 3>;
      using Vec3ArrayHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f_64, StorageTag>;

      std::vector<Vec3> testValues(ARRAY_SIZE);

      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        testValues[i] = TestValue(1, Vec3());
      }
      Vec3ArrayHandle values = vtkm::cont::make_ArrayHandle(testValues);

      Vec3 sum = Algorithm::ScanExclusive(values, values);
      std::cout << "Sum that was returned " << sum << std::endl;
      VTKM_TEST_ASSERT(test_equal(sum, (TestValue(1, Vec3()) * ARRAY_SIZE)),
                       "Got bad sum from Exclusive Scan");
    }
  }

  static VTKM_CONT void TestScanExtended()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Extended Scan" << std::endl;

    {
      std::cout << "  size " << ARRAY_SIZE << std::endl;

      //construct the index array
      IdArrayHandle array;
      Algorithm::Schedule(ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                          ARRAY_SIZE);

      // we now have an array whose sum = (OFFSET * ARRAY_SIZE),
      // let's validate that
      Algorithm::ScanExtended(array, array);
      VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE + 1, "Output size incorrect.");
      auto portal = array.GetPortalConstControl();
      for (vtkm::Id i = 0; i < ARRAY_SIZE + 1; ++i)
      {
        const vtkm::Id value = portal.Get(i);
        VTKM_TEST_ASSERT(value == i * OFFSET, "Incorrect partial sum");
      }

      std::cout << "  size 1" << std::endl;
      array.Shrink(1);
      array.GetPortalControl().Set(0, OFFSET);
      Algorithm::ScanExtended(array, array);
      VTKM_TEST_ASSERT(array.GetNumberOfValues() == 2);
      portal = array.GetPortalConstControl();
      VTKM_TEST_ASSERT(portal.Get(0) == 0, "Incorrect initial value");
      VTKM_TEST_ASSERT(portal.Get(1) == OFFSET, "Incorrect total sum");

      std::cout << "  size 0" << std::endl;
      array.Shrink(0);
      Algorithm::ScanExtended(array, array);
      VTKM_TEST_ASSERT(array.GetNumberOfValues() == 1);
      portal = array.GetPortalConstControl();
      VTKM_TEST_ASSERT(portal.Get(0) == 0, "Incorrect initial value");
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Extended Scan with multiplication operator" << std::endl;
    {
      std::vector<vtkm::Float64> inputValues(ARRAY_SIZE);
      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        inputValues[i] = 1.01;
      }

      std::size_t mid = ARRAY_SIZE / 2;
      inputValues[mid] = 0.0;

      vtkm::cont::ArrayHandle<vtkm::Float64> array =
        vtkm::cont::make_ArrayHandle(inputValues, vtkm::CopyFlag::On);

      vtkm::Float64 initialValue = 2.00;
      Algorithm::ScanExtended(array, array, vtkm::Multiply(), initialValue);

      VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE + 1,
                       "ScanExtended output size incorrect.");

      auto portal = array.GetPortalConstControl();
      VTKM_TEST_ASSERT(portal.Get(0) == initialValue,
                       "ScanExtended result's first value != initialValue");

      for (std::size_t i = 1; i <= mid; ++i)
      {
        vtkm::Id index = static_cast<vtkm::Id>(i);
        vtkm::Float64 expected = pow(1.01, static_cast<vtkm::Float64>(i)) * initialValue;
        vtkm::Float64 got = portal.Get(index);
        VTKM_TEST_ASSERT(test_equal(got, expected), "Incorrect results for ScanExtended");
      }
      for (std::size_t i = mid + 1; i < ARRAY_SIZE + 1; ++i)
      {
        vtkm::Id index = static_cast<vtkm::Id>(i);
        VTKM_TEST_ASSERT(portal.Get(index) == 0.0f, "Incorrect results for ScanExtended");
      }
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Extended Scan with a vtkm::Vec" << std::endl;

    {
      using Vec3 = vtkm::Vec3f_64;
      using Vec3ArrayHandle = vtkm::cont::ArrayHandle<Vec3, StorageTag>;

      std::vector<Vec3> testValues(ARRAY_SIZE);

      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        testValues[i] = TestValue(1, Vec3());
      }
      Vec3ArrayHandle values = vtkm::cont::make_ArrayHandle(testValues, vtkm::CopyFlag::On);

      Algorithm::ScanExtended(values, values);
      VTKM_TEST_ASSERT(test_equal(vtkm::cont::ArrayGetValue(ARRAY_SIZE, values),
                                  (TestValue(1, Vec3()) * ARRAY_SIZE)),
                       "Got bad sum from ScanExtended");
    }
  }

  static VTKM_CONT void TestErrorExecution()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Exceptions in Execution Environment" << std::endl;

    std::cout << "Generating one error." << std::endl;
    std::string message;
    try
    {
      Algorithm::Schedule(OneErrorKernel(), ARRAY_SIZE);
      Algorithm::Synchronize();
    }
    catch (vtkm::cont::ErrorExecution& error)
    {
      std::cout << "Got expected error: " << error.GetMessage() << std::endl;
      message = error.GetMessage();
    }
    VTKM_TEST_ASSERT(message == ERROR_MESSAGE, "Did not get expected error message.");

    std::cout << "Generating lots of errors." << std::endl;
    message = "";
    try
    {
      Algorithm::Schedule(AllErrorKernel(), ARRAY_SIZE);
      Algorithm::Synchronize();
    }
    catch (vtkm::cont::ErrorExecution& error)
    {
      std::cout << "Got expected error: " << error.GetMessage() << std::endl;
      message = error.GetMessage();
    }
    VTKM_TEST_ASSERT(message == ERROR_MESSAGE, "Did not get expected error message.");

    // This is spcifically to test the cuda-backend but should pass for all backends
    std::cout << "Testing if execution errors are eventually propagated to the host "
              << "without explicit synchronization\n";
    message = "";
    int nkernels = 0;
    try
    {
      IdArrayHandle idArray;
      idArray.Allocate(ARRAY_SIZE);
      auto portal = idArray.PrepareForInPlace(DeviceAdapterTag{});

      Algorithm::Schedule(OneErrorKernel(), ARRAY_SIZE);
      for (; nkernels < 100; ++nkernels)
      {
        Algorithm::Schedule(AddArrayKernel(portal), ARRAY_SIZE);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
      Algorithm::Synchronize();
    }
    catch (vtkm::cont::ErrorExecution& error)
    {
      std::cout << "Got expected error: \"" << error.GetMessage() << "\" ";
      if (nkernels < 100)
      {
        std::cout << "after " << nkernels << " invocations of other kernel" << std::endl;
      }
      else
      {
        std::cout << "only after explicit synchronization" << std::endl;
      }
      message = error.GetMessage();
    }
    std::cout << "\n";
    VTKM_TEST_ASSERT(message == ERROR_MESSAGE, "Did not get expected error message.");
  }

  template <typename T, int N = 0>
  struct TestCopy
  {
  };

  template <typename T>
  struct TestCopy<T>
  {
    static T get(vtkm::Id i) { return static_cast<T>(i); }
  };

  template <typename T, int N>
  struct TestCopy<vtkm::Vec<T, N>>
  {
    static vtkm::Vec<T, N> get(vtkm::Id i)
    {
      vtkm::Vec<T, N> temp;
      for (int j = 0; j < N; ++j)
      {
        temp[j] = static_cast<T>(OFFSET + (i % 50));
      }
      return temp;
    }
  };

  template <typename T, typename U>
  struct TestCopy<vtkm::Pair<T, U>>
  {
    static vtkm::Pair<T, U> get(vtkm::Id i)
    {
      return vtkm::make_Pair(TestCopy<T>::get(i), TestCopy<U>::get(i));
    }
  };

  template <typename T>
  static VTKM_CONT void TestCopyArrays()
  {
#define COPY_ARRAY_SIZE 10000

    std::vector<T> testData(COPY_ARRAY_SIZE);
    std::default_random_engine generator(static_cast<unsigned int>(std::time(nullptr)));

    vtkm::Id index = 0;
    for (std::size_t i = 0; i < COPY_ARRAY_SIZE; ++i, ++index)
    {
      testData[i] = TestCopy<T>::get(index);
    }

    vtkm::cont::ArrayHandle<T> input = vtkm::cont::make_ArrayHandle(&testData[0], COPY_ARRAY_SIZE);

    //make a deep copy of input and place it into temp
    {
      vtkm::cont::ArrayHandle<T> temp;
      temp.Allocate(COPY_ARRAY_SIZE * 2);
      Algorithm::Copy(input, temp);
      VTKM_TEST_ASSERT(temp.GetNumberOfValues() == COPY_ARRAY_SIZE, "Copy Needs to Resize Array");

      const auto& portal = temp.GetPortalConstControl();

      std::uniform_int_distribution<vtkm::Id> distribution(0, COPY_ARRAY_SIZE - 1);
      vtkm::Id numberOfSamples = COPY_ARRAY_SIZE / 50;
      for (vtkm::Id i = 0; i < numberOfSamples; ++i)
      {
        vtkm::Id randomIndex = distribution(generator);
        T value = portal.Get(randomIndex);
        VTKM_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex)],
                         "Got bad value (Copy)");
      }
    }

    //Verify copy of empty array works
    {
      vtkm::cont::ArrayHandle<T> tempIn;
      vtkm::cont::ArrayHandle<T> tempOut;

      tempOut.Allocate(COPY_ARRAY_SIZE);
      Algorithm::Copy(tempIn, tempOut);
      VTKM_TEST_ASSERT(tempIn.GetNumberOfValues() == tempOut.GetNumberOfValues(),
                       "Copy sized wrong");

      // Actually allocate input array to 0 in case that makes a difference.
      tempIn.Allocate(0);
      tempOut.Allocate(COPY_ARRAY_SIZE);
      Algorithm::Copy(tempIn, tempOut);
      VTKM_TEST_ASSERT(tempIn.GetNumberOfValues() == tempOut.GetNumberOfValues(),
                       "Copy sized wrong");
    }

    //CopySubRange tests:

    //1. Verify invalid input start position fails
    {
      vtkm::cont::ArrayHandle<T> output;
      bool result = Algorithm::CopySubRange(input, COPY_ARRAY_SIZE * 4, 1, output);
      VTKM_TEST_ASSERT(result == false, "CopySubRange when given bad input offset");
    }

    //2. Verify unallocated output gets allocated
    {
      vtkm::cont::ArrayHandle<T> output;
      bool result = Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output);
      VTKM_TEST_ASSERT(result == true, "CopySubRange should succeed");
      VTKM_TEST_ASSERT(output.GetNumberOfValues() == COPY_ARRAY_SIZE,
                       "CopySubRange needs to allocate output");
    }

    //3. Verify under allocated output gets resized properly
    {
      vtkm::cont::ArrayHandle<T> output;
      output.Allocate(2);
      bool result = Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output);
      VTKM_TEST_ASSERT(result == true, "CopySubRange should succeed");
      VTKM_TEST_ASSERT(output.GetNumberOfValues() == COPY_ARRAY_SIZE,
                       "CopySubRange needs to re-allocate output");
    }

    //4. Verify invalid input length gets shortened
    {
      vtkm::cont::ArrayHandle<T> output;
      bool result = Algorithm::CopySubRange(input, 100, COPY_ARRAY_SIZE, output);
      VTKM_TEST_ASSERT(result == true, "CopySubRange needs to shorten input range");
      VTKM_TEST_ASSERT(output.GetNumberOfValues() == (COPY_ARRAY_SIZE - 100),
                       "CopySubRange needs to shorten input range");

      std::uniform_int_distribution<vtkm::Id> distribution(0, COPY_ARRAY_SIZE - 100 - 1);
      vtkm::Id numberOfSamples = (COPY_ARRAY_SIZE - 100) / 100;
      for (vtkm::Id i = 0; i < numberOfSamples; ++i)
      {
        vtkm::Id randomIndex = distribution(generator);
        T value = output.GetPortalConstControl().Get(randomIndex);
        VTKM_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex) + 100],
                         "Got bad value (CopySubRange 2)");
      }
    }

    //5. Verify sub range copy works when copying into a larger output
    {
      vtkm::cont::ArrayHandle<T> output;
      output.Allocate(COPY_ARRAY_SIZE * 2);
      Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output);
      Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output, COPY_ARRAY_SIZE);
      VTKM_TEST_ASSERT(output.GetNumberOfValues() == (COPY_ARRAY_SIZE * 2),
                       "CopySubRange needs to not resize array");

      std::uniform_int_distribution<vtkm::Id> distribution(0, COPY_ARRAY_SIZE - 1);
      vtkm::Id numberOfSamples = COPY_ARRAY_SIZE / 50;
      for (vtkm::Id i = 0; i < numberOfSamples; ++i)
      {
        vtkm::Id randomIndex = distribution(generator);
        T value = output.GetPortalConstControl().Get(randomIndex);
        VTKM_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex)],
                         "Got bad value (CopySubRange 5)");
        value = output.GetPortalConstControl().Get(COPY_ARRAY_SIZE + randomIndex);
        VTKM_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex)],
                         "Got bad value (CopySubRange 5)");
      }
    }

    //6. Verify that whey sub range needs to reallocate the output it
    // properly copies the original data instead of clearing it
    {
      vtkm::cont::ArrayHandle<T> output;
      output.Allocate(COPY_ARRAY_SIZE);
      Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output);
      Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output, COPY_ARRAY_SIZE);
      VTKM_TEST_ASSERT(output.GetNumberOfValues() == (COPY_ARRAY_SIZE * 2),
                       "CopySubRange needs too resize Array");
      std::uniform_int_distribution<vtkm::Id> distribution(0, COPY_ARRAY_SIZE - 1);
      vtkm::Id numberOfSamples = COPY_ARRAY_SIZE / 50;
      for (vtkm::Id i = 0; i < numberOfSamples; ++i)
      {
        vtkm::Id randomIndex = distribution(generator);
        T value = output.GetPortalConstControl().Get(randomIndex);
        VTKM_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex)],
                         "Got bad value (CopySubRange 6)");
        value = output.GetPortalConstControl().Get(COPY_ARRAY_SIZE + randomIndex);
        VTKM_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex)],
                         "Got bad value (CopySubRange 6)");
      }
    }

    // 7. Test that overlapping ranges trigger a failure:
    // 7.1 output starts inside input range:
    {
      const vtkm::Id inBegin = 100;
      const vtkm::Id inEnd = 200;
      const vtkm::Id outBegin = 150;

      const vtkm::Id numVals = inEnd - inBegin;
      bool result = Algorithm::CopySubRange(input, inBegin, numVals, input, outBegin);
      VTKM_TEST_ASSERT(result == false, "Overlapping subrange did not fail.");
    }

    // 7.2 input starts inside output range
    {
      const vtkm::Id inBegin = 100;
      const vtkm::Id inEnd = 200;
      const vtkm::Id outBegin = 50;

      const vtkm::Id numVals = inEnd - inBegin;
      bool result = Algorithm::CopySubRange(input, inBegin, numVals, input, outBegin);
      VTKM_TEST_ASSERT(result == false, "Overlapping subrange did not fail.");
    }

    {
      vtkm::cont::ArrayHandle<T> output;

      //7. Verify negative input index returns false
      bool result = Algorithm::CopySubRange(input, -1, COPY_ARRAY_SIZE, output);
      VTKM_TEST_ASSERT(result == false, "CopySubRange negative index should fail");

      //8. Verify negative input numberOfElementsToCopy returns false
      result = Algorithm::CopySubRange(input, 0, -COPY_ARRAY_SIZE, output);
      VTKM_TEST_ASSERT(result == false, "CopySubRange negative number elements should fail");

      //9. Verify negative output index return false
      result = Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output, -2);
      VTKM_TEST_ASSERT(result == false, "CopySubRange negative output index should fail");
    }

#undef COPY_ARRAY_SIZE
  }

  static VTKM_CONT void TestCopyArraysMany()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing Copy to same array type" << std::endl;
    TestCopyArrays<vtkm::Vec3f_32>();
    TestCopyArrays<vtkm::Vec4ui_8>();
    //
    TestCopyArrays<vtkm::Pair<vtkm::Id, vtkm::Float32>>();
    TestCopyArrays<vtkm::Pair<vtkm::Id, vtkm::Vec3f_32>>();
    //
    TestCopyArrays<vtkm::Float32>();
    TestCopyArrays<vtkm::Float64>();
    //
    TestCopyArrays<vtkm::Int32>();
    TestCopyArrays<vtkm::Int64>();
    //
    TestCopyArrays<vtkm::UInt8>();
    TestCopyArrays<vtkm::UInt16>();
    TestCopyArrays<vtkm::UInt32>();
    TestCopyArrays<vtkm::UInt64>();
  }

  static VTKM_CONT void TestCopyArraysInDiffTypes()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing Copy to a different array type" << std::endl;
    std::vector<vtkm::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<vtkm::Id>(OFFSET + (i % 50));
    }

    IdArrayHandle input = vtkm::cont::make_ArrayHandle(testData);

    //make a deep copy of input and place it into temp
    vtkm::cont::ArrayHandle<vtkm::Float64> temp;
    Algorithm::Copy(input, temp);

    std::vector<vtkm::Id>::const_iterator c = testData.begin();
    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i, ++c)
    {
      vtkm::Float64 value = temp.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(value == static_cast<vtkm::Float64>(*c), "Got bad value (Copy)");
    }
  }

  static VTKM_CONT void TestAtomicArray()
  {
    //we can't use ARRAY_SIZE as that would cause a overflow
    vtkm::Int32 SHORT_ARRAY_SIZE = 10000;

    vtkm::Int32 atomicCount = 0;
    for (vtkm::Int32 i = 0; i < SHORT_ARRAY_SIZE; i++)
    {
      atomicCount += i;
    }
    std::cout << "-------------------------------------------" << std::endl;
    // To test the atomics, SHORT_ARRAY_SIZE number of threads will all increment
    // a single atomic value.
    std::cout << "Testing Atomic Add with vtkm::Int32" << std::endl;
    {
      std::vector<vtkm::Int32> singleElement;
      singleElement.push_back(0);
      vtkm::cont::ArrayHandle<vtkm::Int32> atomicElement =
        vtkm::cont::make_ArrayHandle(singleElement);

      vtkm::cont::AtomicArray<vtkm::Int32> atomic(atomicElement);
      Algorithm::Schedule(AtomicKernel<vtkm::Int32>(atomic), SHORT_ARRAY_SIZE);
      vtkm::Int32 expected = vtkm::Int32(atomicCount);
      vtkm::Int32 actual = atomicElement.GetPortalControl().Get(0);
      VTKM_TEST_ASSERT(expected == actual, "Did not get expected value: Atomic add Int32");
    }

    std::cout << "Testing Atomic Add with vtkm::Int64" << std::endl;
    {
      std::vector<vtkm::Int64> singleElement;
      singleElement.push_back(0);
      vtkm::cont::ArrayHandle<vtkm::Int64> atomicElement =
        vtkm::cont::make_ArrayHandle(singleElement);

      vtkm::cont::AtomicArray<vtkm::Int64> atomic(atomicElement);
      Algorithm::Schedule(AtomicKernel<vtkm::Int64>(atomic), SHORT_ARRAY_SIZE);
      vtkm::Int64 expected = vtkm::Int64(atomicCount);
      vtkm::Int64 actual = atomicElement.GetPortalControl().Get(0);
      VTKM_TEST_ASSERT(expected == actual, "Did not get expected value: Atomic add Int64");
    }

    std::cout << "Testing Atomic CAS with vtkm::Int32" << std::endl;
    {
      std::vector<vtkm::Int32> singleElement;
      singleElement.push_back(0);
      vtkm::cont::ArrayHandle<vtkm::Int32> atomicElement =
        vtkm::cont::make_ArrayHandle(singleElement);

      vtkm::cont::AtomicArray<vtkm::Int32> atomic(atomicElement);
      Algorithm::Schedule(AtomicCASKernel<vtkm::Int32>(atomic), SHORT_ARRAY_SIZE);
      vtkm::Int32 expected = vtkm::Int32(atomicCount);
      vtkm::Int32 actual = atomicElement.GetPortalControl().Get(0);
      VTKM_TEST_ASSERT(expected == actual, "Did not get expected value: Atomic CAS Int32");
    }

    std::cout << "Testing Atomic CAS with vtkm::Int64" << std::endl;
    {
      std::vector<vtkm::Int64> singleElement;
      singleElement.push_back(0);
      vtkm::cont::ArrayHandle<vtkm::Int64> atomicElement =
        vtkm::cont::make_ArrayHandle(singleElement);

      vtkm::cont::AtomicArray<vtkm::Int64> atomic(atomicElement);
      Algorithm::Schedule(AtomicCASKernel<vtkm::Int64>(atomic), SHORT_ARRAY_SIZE);
      vtkm::Int64 expected = vtkm::Int64(atomicCount);
      vtkm::Int64 actual = atomicElement.GetPortalControl().Get(0);
      VTKM_TEST_ASSERT(expected == actual, "Did not get expected value: Atomic CAS Int64");
    }
  }

  static VTKM_CONT void TestBitFieldToUnorderedSet()
  {
    using IndexArray = vtkm::cont::ArrayHandle<vtkm::Id>;
    using WordType = WordTypeDefault;

    // Test that everything works correctly with a partial word at the end.
    static constexpr vtkm::Id BitsPerWord = static_cast<vtkm::Id>(sizeof(WordType) * CHAR_BIT);
    // +5 to get a partial word:
    static constexpr vtkm::Id NumBits = 1024 * BitsPerWord + 5;
    static constexpr vtkm::Id NumWords = (NumBits + BitsPerWord - 1) / BitsPerWord;

    auto testIndexArray = [](const BitField& bits) {
      const vtkm::Id numBits = bits.GetNumberOfBits();
      IndexArray indices;
      Algorithm::BitFieldToUnorderedSet(bits, indices);
      Algorithm::Sort(indices);

      auto bitPortal = bits.GetPortalConstControl();
      auto indexPortal = indices.GetPortalConstControl();

      const vtkm::Id numIndices = indices.GetNumberOfValues();
      vtkm::Id curIndex = 0;
      for (vtkm::Id curBit = 0; curBit < numBits; ++curBit)
      {
        const bool markedSet = curIndex < numIndices ? indexPortal.Get(curIndex) == curBit : false;
        const bool isSet = bitPortal.GetBit(curBit);

        //        std::cout << "curBit: " << curBit
        //                  << " activeIndex: "
        //                  << (curIndex < numIndices ? indexPortal.Get(curIndex) : -1)
        //                  << " isSet: " << isSet << " markedSet: " << markedSet << "\n";

        VTKM_TEST_ASSERT(
          markedSet == isSet, "Bit ", curBit, " is set? ", isSet, " Marked set? ", markedSet);

        if (markedSet)
        {
          curIndex++;
        }
      }

      VTKM_TEST_ASSERT(curIndex == indices.GetNumberOfValues(), "Index array has extra values.");
    };

    auto testRepeatedMask = [&](WordType mask) {
      std::cout << "Testing BitFieldToUnorderedSet with repeated 32-bit word 0x" << std::hex << mask
                << std::dec << std::endl;

      BitField bits;
      {
        bits.Allocate(NumBits);
        auto fillPortal = bits.GetPortalControl();
        for (vtkm::Id i = 0; i < NumWords; ++i)
        {
          fillPortal.SetWord(i, mask);
        }
      }

      testIndexArray(bits);
    };

    auto testRandomMask = [&](WordType seed) {
      std::cout << "Testing BitFieldToUnorderedSet with random sequence seeded with 0x" << std::hex
                << seed << std::dec << std::endl;

      std::mt19937 mt{ seed };
      std::uniform_int_distribution<std::mt19937::result_type> rng;

      BitField bits;
      {
        bits.Allocate(NumBits);
        auto fillPortal = bits.GetPortalControl();
        for (vtkm::Id i = 0; i < NumWords; ++i)
        {
          fillPortal.SetWord(i, static_cast<WordType>(rng(mt)));
        }
      }

      testIndexArray(bits);
    };

    testRepeatedMask(0x00000000);
    testRepeatedMask(0xeeeeeeee);
    testRepeatedMask(0xffffffff);
    testRepeatedMask(0x1c0fd395);
    testRepeatedMask(0xdeadbeef);

    testRandomMask(0x00000000);
    testRandomMask(0xeeeeeeee);
    testRandomMask(0xffffffff);
    testRandomMask(0x1c0fd395);
    testRandomMask(0xdeadbeef);

    // This case was causing issues on CUDA:
    {
      BitField bits;
      Algorithm::Fill(bits, false, 32 * 32);
      auto portal = bits.GetPortalControl();
      portal.SetWord(2, 0x00100000ul);
      portal.SetWord(8, 0x00100010ul);
      portal.SetWord(11, 0x10000000ul);
      testIndexArray(bits);
    }
  }

  static VTKM_CONT void TestCountSetBits()
  {
    using WordType = WordTypeDefault;

    // Test that everything works correctly with a partial word at the end.
    static constexpr vtkm::Id BitsPerWord = static_cast<vtkm::Id>(sizeof(WordType) * CHAR_BIT);
    // +5 to get a partial word:
    static constexpr vtkm::Id NumFullWords = 1024;
    static constexpr vtkm::Id NumBits = NumFullWords * BitsPerWord + 5;
    static constexpr vtkm::Id NumWords = (NumBits + BitsPerWord - 1) / BitsPerWord;

    auto verifyPopCount = [](const BitField& bits) {
      vtkm::Id refPopCount = 0;
      const vtkm::Id numBits = bits.GetNumberOfBits();
      auto portal = bits.GetPortalConstControl();
      for (vtkm::Id idx = 0; idx < numBits; ++idx)
      {
        if (portal.GetBit(idx))
        {
          ++refPopCount;
        }
      }

      const vtkm::Id popCount = Algorithm::CountSetBits(bits);

      VTKM_TEST_ASSERT(
        refPopCount == popCount, "CountSetBits returned ", popCount, ", expected ", refPopCount);
    };

    auto testRepeatedMask = [&](WordType mask) {
      std::cout << "Testing CountSetBits with repeated word 0x" << std::hex << mask << std::dec
                << std::endl;

      BitField bits;
      {
        bits.Allocate(NumBits);
        auto fillPortal = bits.GetPortalControl();
        for (vtkm::Id i = 0; i < NumWords; ++i)
        {
          fillPortal.SetWord(i, mask);
        }
      }

      verifyPopCount(bits);
    };

    auto testRandomMask = [&](WordType seed) {
      std::cout << "Testing CountSetBits with random sequence seeded with 0x" << std::hex << seed
                << std::dec << std::endl;

      std::mt19937 mt{ seed };
      std::uniform_int_distribution<std::mt19937::result_type> rng;

      BitField bits;
      {
        bits.Allocate(NumBits);
        auto fillPortal = bits.GetPortalControl();
        for (vtkm::Id i = 0; i < NumWords; ++i)
        {
          fillPortal.SetWord(i, static_cast<WordType>(rng(mt)));
        }
      }

      verifyPopCount(bits);
    };

    testRepeatedMask(0x00000000);
    testRepeatedMask(0xeeeeeeee);
    testRepeatedMask(0xffffffff);
    testRepeatedMask(0x1c0fd395);
    testRepeatedMask(0xdeadbeef);

    testRandomMask(0x00000000);
    testRandomMask(0xeeeeeeee);
    testRandomMask(0xffffffff);
    testRandomMask(0x1c0fd395);
    testRandomMask(0xdeadbeef);

    // This case was causing issues on CUDA:
    {
      BitField bits;
      Algorithm::Fill(bits, false, 32 * 32);
      auto portal = bits.GetPortalControl();
      portal.SetWord(2, 0x00100000ul);
      portal.SetWord(8, 0x00100010ul);
      portal.SetWord(11, 0x10000000ul);
      verifyPopCount(bits);
    }
  }

  template <typename WordType>
  static VTKM_CONT void TestFillBitFieldMask(WordType mask)
  {
    std::cout << "Testing Fill with " << (sizeof(WordType) * CHAR_BIT) << " bit mask: " << std::hex
              << vtkm::UInt64{ mask } << std::dec << std::endl;

    // Test that everything works correctly with a partial word at the end.
    static constexpr vtkm::Id BitsPerWord = static_cast<vtkm::Id>(sizeof(WordType) * CHAR_BIT);
    // +5 to get a partial word:
    static constexpr vtkm::Id NumFullWords = 1024;
    static constexpr vtkm::Id NumBits = NumFullWords * BitsPerWord + 5;
    static constexpr vtkm::Id NumWords = (NumBits + BitsPerWord - 1) / BitsPerWord;

    vtkm::cont::BitField bits;
    {
      Algorithm::Fill(bits, mask, NumBits);

      vtkm::Id numBits = bits.GetNumberOfBits();
      VTKM_TEST_ASSERT(numBits == NumBits, "Unexpected number of bits.");
      vtkm::Id numWords = bits.GetNumberOfWords<WordType>();
      VTKM_TEST_ASSERT(numWords == NumWords, "Unexpected number of words.");

      auto portal = bits.GetPortalConstControl();
      for (vtkm::Id wordIdx = 0; wordIdx < NumWords; ++wordIdx)
      {
        VTKM_TEST_ASSERT(portal.GetWord<WordType>(wordIdx) == mask,
                         "Incorrect word in result BitField; expected 0x",
                         std::hex,
                         vtkm::UInt64{ mask },
                         ", got 0x",
                         vtkm::UInt64{ portal.GetWord<WordType>(wordIdx) },
                         std::dec,
                         " for word ",
                         wordIdx,
                         "/",
                         NumWords);
      }
    }

    // Now fill the BitField with the reversed mask to test the no-alloc
    // overload:
    {
      WordType invWord = static_cast<WordType>(~mask);
      Algorithm::Fill(bits, invWord);

      vtkm::Id numBits = bits.GetNumberOfBits();
      VTKM_TEST_ASSERT(numBits == NumBits, "Unexpected number of bits.");
      vtkm::Id numWords = bits.GetNumberOfWords<WordType>();
      VTKM_TEST_ASSERT(numWords == NumWords, "Unexpected number of words.");

      auto portal = bits.GetPortalConstControl();
      for (vtkm::Id wordIdx = 0; wordIdx < NumWords; ++wordIdx)
      {
        VTKM_TEST_ASSERT(portal.GetWord<WordType>(wordIdx) == invWord,
                         "Incorrect word in result BitField; expected 0x",
                         std::hex,
                         vtkm::UInt64{ invWord },
                         ", got 0x",
                         vtkm::UInt64{ portal.GetWord<WordType>(wordIdx) },
                         std::dec,
                         " for word ",
                         wordIdx,
                         "/",
                         NumWords);
      }
    }
  }

  static VTKM_CONT void TestFillBitFieldBool(bool value)
  {
    std::cout << "Testing Fill with bool: " << value << std::endl;

    // Test that everything works correctly with a partial word at the end.
    // +5 to get a partial word:
    static constexpr vtkm::Id NumBits = 1024 * 32 + 5;

    vtkm::cont::BitField bits;
    {
      Algorithm::Fill(bits, value, NumBits);

      vtkm::Id numBits = bits.GetNumberOfBits();
      VTKM_TEST_ASSERT(numBits == NumBits, "Unexpected number of bits.");

      auto portal = bits.GetPortalConstControl();
      for (vtkm::Id bitIdx = 0; bitIdx < NumBits; ++bitIdx)
      {
        VTKM_TEST_ASSERT(portal.GetBit(bitIdx) == value, "Incorrect bit in result BitField.");
      }
    }

    // Now fill the BitField with the reversed mask to test the no-alloc
    // overload:
    {
      Algorithm::Fill(bits, !value);

      vtkm::Id numBits = bits.GetNumberOfBits();
      VTKM_TEST_ASSERT(numBits == NumBits, "Unexpected number of bits.");

      auto portal = bits.GetPortalConstControl();
      for (vtkm::Id bitIdx = 0; bitIdx < NumBits; ++bitIdx)
      {
        VTKM_TEST_ASSERT(portal.GetBit(bitIdx) == !value, "Incorrect bit in result BitField.");
      }
    }
  }

  static VTKM_CONT void TestFillBitField()
  {
    TestFillBitFieldBool(true);
    TestFillBitFieldBool(false);
    TestFillBitFieldMask<vtkm::UInt8>(vtkm::UInt8{ 0 });
    TestFillBitFieldMask<vtkm::UInt8>(static_cast<vtkm::UInt8>(~vtkm::UInt8{ 0 }));
    TestFillBitFieldMask<vtkm::UInt8>(vtkm::UInt8{ 0xab });
    TestFillBitFieldMask<vtkm::UInt8>(vtkm::UInt8{ 0x4f });
    TestFillBitFieldMask<vtkm::UInt16>(vtkm::UInt16{ 0 });
    TestFillBitFieldMask<vtkm::UInt16>(static_cast<vtkm::UInt16>(~vtkm::UInt16{ 0 }));
    TestFillBitFieldMask<vtkm::UInt16>(vtkm::UInt16{ 0xfade });
    TestFillBitFieldMask<vtkm::UInt16>(vtkm::UInt16{ 0xbeef });
    TestFillBitFieldMask<vtkm::UInt32>(vtkm::UInt32{ 0 });
    TestFillBitFieldMask<vtkm::UInt32>(static_cast<vtkm::UInt32>(~vtkm::UInt32{ 0 }));
    TestFillBitFieldMask<vtkm::UInt32>(vtkm::UInt32{ 0xfacecafe });
    TestFillBitFieldMask<vtkm::UInt32>(vtkm::UInt32{ 0xbaddecaf });
    TestFillBitFieldMask<vtkm::UInt64>(vtkm::UInt64{ 0 });
    TestFillBitFieldMask<vtkm::UInt64>(static_cast<vtkm::UInt64>(~vtkm::UInt64{ 0 }));
    TestFillBitFieldMask<vtkm::UInt64>(vtkm::UInt64{ 0xbaddefacedfacade });
    TestFillBitFieldMask<vtkm::UInt64>(vtkm::UInt64{ 0xfeeddeadbeef2dad });
  }

  static VTKM_CONT void TestFillArrayHandle()
  {
    vtkm::cont::ArrayHandle<vtkm::Int32> handle;
    Algorithm::Fill(handle, 867, ARRAY_SIZE);

    {
      auto portal = handle.GetPortalConstControl();
      VTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE);
      for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        VTKM_TEST_ASSERT(portal.Get(i) == 867);
      }
    }

    Algorithm::Fill(handle, 5309);
    {
      auto portal = handle.GetPortalConstControl();
      VTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE);
      for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        VTKM_TEST_ASSERT(portal.Get(i) == 5309);
      }
    }
  }

  struct TestAll
  {
    VTKM_CONT void operator()() const
    {
      std::cout << "Doing DeviceAdapter tests" << std::endl;

      TestArrayTransfer();
      TestOutOfMemory();
      TestTimer();
      TestVirtualObjectTransfer();

      TestAlgorithmSchedule();
      TestErrorExecution();

      TestReduce();
      TestReduceWithComparisonObject();
      TestReduceWithFancyArrays();

      TestReduceByKey();
      TestReduceByKeyWithFancyArrays();

      TestScanExclusive();
      TestScanExtended();

      TestScanInclusive();
      TestScanInclusiveWithComparisonObject();

      TestScanInclusiveByKeyOne();
      TestScanInclusiveByKeyTwo();
      TestScanInclusiveByKeyLarge();
      TestScanInclusiveByKey();

      TestScanExclusiveByKeyOne();
      TestScanExclusiveByKeyTwo();
      TestScanExclusiveByKeyLarge();
      TestScanExclusiveByKey();

      TestSort();
      TestSortWithComparisonObject();
      TestSortWithFancyArrays();
      TestSortByKey();

      TestLowerBoundsWithComparisonObject();

      TestUpperBoundsWithComparisonObject();

      TestUniqueWithComparisonObject();

      TestOrderedUniqueValues(); //tests Copy, LowerBounds, Sort, Unique
      TestCopyIf();

      TestCopyArraysMany();
      TestCopyArraysInDiffTypes();

      TestAtomicArray();

      TestBitFieldToUnorderedSet();
      TestCountSetBits();
      TestFillBitField();

      TestFillArrayHandle();
    }
  };

public:
  /// Run a suite of tests to check to see if a DeviceAdapter properly supports
  /// all members and classes required for driving vtkm algorithms. Returns an
  /// error code that can be returned from the main function of a test.
  ///
  static VTKM_CONT int Run(int argc, char* argv[])
  {
    return vtkm::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};

#undef ERROR_MESSAGE
#undef ARRAY_SIZE
#undef OFFSET
#undef DIM
}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_TestingDeviceAdapter_h
