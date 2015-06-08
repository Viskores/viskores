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
#ifndef vtk_m_cont_testing_TestingDeviceAdapter_h
#define vtk_m_cont_testing_TestingDeviceAdapter_h

#include <vtkm/TypeTraits.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/ErrorControlOutOfMemory.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/StorageBasic.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/cont/internal/DeviceAdapterError.h>

#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#endif

namespace vtkm {
namespace cont {
namespace testing {

namespace comparison {
struct SortLess
{
  template<typename T, typename U>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a, const U& b) const
  {
    return a < b;
  }

  template<typename T, int N>
  VTKM_EXEC_EXPORT bool operator()(const vtkm::Vec<T,N>& a,
                                   const vtkm::Vec<T,N>& b) const
  {
    const vtkm::IdComponent SIZE = vtkm::VecTraits<T>::NUM_COMPONENTS;
    bool valid = true;
    for(vtkm::IdComponent i=0; (i < SIZE) && valid; ++i)
    {
      valid = a[i] < b[i];
    }
    return valid;
  }
};

struct SortGreater
{
  template<typename T, typename U>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a, const U& b) const
  {
    return a > b;
  }

  template<typename T, int N>
  VTKM_EXEC_EXPORT bool operator()(const vtkm::Vec<T,N>& a,
                                   const vtkm::Vec<T,N>& b) const
  {
    const vtkm::IdComponent SIZE = vtkm::VecTraits<T>::NUM_COMPONENTS;
    bool valid = true;
    for(vtkm::IdComponent i=0; (i < SIZE) && valid; ++i)
    {
      valid = a[i] > b[i];
    }
    return valid;
  }
};

struct MaxValue
{
  template<typename T>
  VTKM_EXEC_CONT_EXPORT T operator()(const T& a,const T& b) const
  {
    return (a > b) ? a : b;
  }
};

}


#define ERROR_MESSAGE "Got an error."
#define ARRAY_SIZE 1000
#define OFFSET 1000
#define DIM_SIZE 128

/// This class has a single static member, Run, that tests the templated
/// DeviceAdapter for conformance.
///
template<class DeviceAdapterTag>
struct TestingDeviceAdapter
{
private:
  typedef vtkm::cont::StorageTagBasic StorageTag;

  typedef vtkm::cont::ArrayHandle<vtkm::Id, StorageTag>
        IdArrayHandle;

  typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault,StorageTag>
      ScalarArrayHandle;

  typedef vtkm::cont::internal::ArrayManagerExecution<
      vtkm::Id, StorageTag, DeviceAdapterTag>
      IdArrayManagerExecution;

  typedef vtkm::cont::internal::Storage<vtkm::Id, StorageTag> IdStorage;

  typedef typename IdArrayHandle::template ExecutionTypes<DeviceAdapterTag>
      ::Portal IdPortalType;
  typedef typename IdArrayHandle::template ExecutionTypes<DeviceAdapterTag>
      ::PortalConst IdPortalConstType;

  typedef vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault,3>,StorageTag>
      Vec3ArrayHandle;

  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>
      Algorithm;

public:
  // Cuda kernels have to be public (in Cuda 4.0).

  struct CopyArrayKernel
  {
    VTKM_CONT_EXPORT
    CopyArrayKernel(const IdPortalConstType &input,
                    const IdPortalType &output)
      : InputArray(input), OutputArray(output) {  }

    VTKM_EXEC_EXPORT void operator()(
        vtkm::Id index,
        const vtkm::exec::internal::ErrorMessageBuffer &) const
    {
      this->OutputArray.Set(index, this->InputArray.Get(index));
    }

    VTKM_CONT_EXPORT void SetErrorMessageBuffer(
        const vtkm::exec::internal::ErrorMessageBuffer &) {  }

    IdPortalConstType InputArray;
    IdPortalType OutputArray;
  };

  struct ClearArrayKernel
  {
    VTKM_CONT_EXPORT
    ClearArrayKernel(const IdPortalType &array) : Array(array) {  }

    VTKM_EXEC_EXPORT void operator()(vtkm::Id index) const
    {
      this->Array.Set(index, OFFSET);
    }

    VTKM_CONT_EXPORT void SetErrorMessageBuffer(
        const vtkm::exec::internal::ErrorMessageBuffer &) {  }

    IdPortalType Array;
  };

  struct ClearArrayMapKernel //: public vtkm::exec::WorkletMapField
  {

    // typedef void ControlSignature(Field(Out));
    // typedef void ExecutionSignature(_1);

    template<typename T>
    VTKM_EXEC_EXPORT void operator()(T& value) const
    {
      value = OFFSET;
    }
  };

  struct AddArrayKernel
  {
    VTKM_CONT_EXPORT
    AddArrayKernel(const IdPortalType &array) : Array(array) {  }

    VTKM_EXEC_EXPORT void operator()(vtkm::Id index) const
    {
      this->Array.Set(index, this->Array.Get(index) + index);
    }

    VTKM_CONT_EXPORT void SetErrorMessageBuffer(
        const vtkm::exec::internal::ErrorMessageBuffer &) {  }

    IdPortalType Array;
  };

  struct OneErrorKernel
  {
    VTKM_EXEC_EXPORT void operator()(vtkm::Id index) const
    {
      if (index == ARRAY_SIZE/2)
      {
        this->ErrorMessage.RaiseError(ERROR_MESSAGE);
      }
    }

    VTKM_CONT_EXPORT void SetErrorMessageBuffer(
        const vtkm::exec::internal::ErrorMessageBuffer &errorMessage)
    {
      this->ErrorMessage = errorMessage;
    }

    vtkm::exec::internal::ErrorMessageBuffer ErrorMessage;
  };

  struct AllErrorKernel
  {
    VTKM_EXEC_EXPORT void operator()(vtkm::Id vtkmNotUsed(index)) const
    {
      this->ErrorMessage.RaiseError(ERROR_MESSAGE);
    }

    VTKM_CONT_EXPORT void SetErrorMessageBuffer(
        const vtkm::exec::internal::ErrorMessageBuffer &errorMessage)
    {
      this->ErrorMessage = errorMessage;
    }

    vtkm::exec::internal::ErrorMessageBuffer ErrorMessage;
  };

  struct OffsetPlusIndexKernel
  {
    VTKM_CONT_EXPORT
    OffsetPlusIndexKernel(const IdPortalType &array) : Array(array) {  }

    VTKM_EXEC_EXPORT void operator()(vtkm::Id index) const
    {
      this->Array.Set(index, OFFSET + index);
    }

    VTKM_CONT_EXPORT void SetErrorMessageBuffer(
        const vtkm::exec::internal::ErrorMessageBuffer &) {  }

    IdPortalType Array;
  };

  struct MarkOddNumbersKernel
  {
    VTKM_CONT_EXPORT
    MarkOddNumbersKernel(const IdPortalType &array) : Array(array) {  }

    VTKM_EXEC_EXPORT void operator()(vtkm::Id index) const
    {
      this->Array.Set(index, index%2);
    }

    VTKM_CONT_EXPORT void SetErrorMessageBuffer(
        const vtkm::exec::internal::ErrorMessageBuffer &) {  }

    IdPortalType Array;
  };

  struct FuseAll
  {
    template<typename T>
    VTKM_EXEC_EXPORT bool operator()(const T&, const T&) const
    {
      //binary predicates for unique return true if they are the same
      return true;
    }
  };


private:

  template<typename T>
  static VTKM_CONT_EXPORT
  vtkm::cont::ArrayHandle<T, StorageTagBasic>
  MakeArrayHandle(const T *array, vtkm::Id length)
  {
    return vtkm::cont::make_ArrayHandle(array, length);
  }

  template<typename T>
  static VTKM_CONT_EXPORT
  vtkm::cont::ArrayHandle<T, StorageTagBasic>
  MakeArrayHandle(const std::vector<T>& array)
  {
    return vtkm::cont::make_ArrayHandle(array,
                                        StorageTagBasic());
  }

  static VTKM_CONT_EXPORT void TestDeviceAdapterTag()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing device adapter tag" << std::endl;

    typedef vtkm::cont::internal::DeviceAdapterTraits<DeviceAdapterTag> Traits;
    typedef vtkm::cont::internal::DeviceAdapterTraits<
        vtkm::cont::DeviceAdapterTagError> ErrorTraits;

    VTKM_TEST_ASSERT(Traits::GetId() == Traits::GetId(),
                     "Device adapter Id does not equal itself.");
    VTKM_TEST_ASSERT(Traits::GetId() != ErrorTraits::GetId(),
                     "Device adapter Id not distinguishable from others.");
  }

  // Note: this test does not actually test to make sure the data is available
  // in the execution environment. It tests to make sure data gets to the array
  // and back, but it is possible that the data is not available in the
  // execution environment.
  static VTKM_CONT_EXPORT void TestArrayManagerExecution()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing ArrayManagerExecution" << std::endl;

    typedef vtkm::cont::internal::ArrayManagerExecution<
        vtkm::Id,StorageTagBasic,DeviceAdapterTag>
        ArrayManagerExecution;

    typedef vtkm::cont::internal::Storage<vtkm::Id,StorageTagBasic> StorageType;

    // Create original input array.
    StorageType storage;
    storage.Allocate(ARRAY_SIZE*2);

    StorageType::PortalType portal = storage.GetPortal();
    VTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE*2,
                     "Storage portal has unexpected size.");

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      portal.Set(index, TestValue(index, vtkm::Id()));
    }

    ArrayManagerExecution manager(&storage);

    // Do an operation just so we know the values are placed in the execution
    // environment and they change. We are only calling on half the array
    // because we are about to shrink.
    Algorithm::Schedule(AddArrayKernel(manager.PrepareForInPlace(true)),
                        ARRAY_SIZE);

    // Change size.
    manager.Shrink(ARRAY_SIZE);

    VTKM_TEST_ASSERT(manager.GetNumberOfValues() == ARRAY_SIZE,
                     "Shrink did not set size of array manager correctly.");

    // Get the array back and check its values. We have to get it back into
    // the same storage since some ArrayManagerExecution classes will expect
    // that.
    manager.RetrieveOutputData(&storage);

    VTKM_TEST_ASSERT(storage.GetNumberOfValues() == ARRAY_SIZE,
                     "Storage has wrong number of values after execution "
                     "array shrink.");

    // Check array.
    StorageType::PortalConstType checkPortal = storage.GetPortalConst();
    VTKM_TEST_ASSERT(checkPortal.GetNumberOfValues() == ARRAY_SIZE,
                     "Storage portal wrong size.");

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      VTKM_TEST_ASSERT(
            checkPortal.Get(index) == TestValue(index, vtkm::Id()) + index,
            "Did not get correct values from array.");
    }
  }

  static VTKM_CONT_EXPORT void TestOutOfMemory()
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
      vtkm::cont::internal::ArrayManagerExecution<
          vtkm::Vector4,StorageTagBasic,DeviceAdapterTag>
          bigManager;
      vtkm::cont::internal::Storage<
          vtkm::Vector4, StorageTagBasic> supportArray;
      const vtkm::Id bigSize = 0x7FFFFFFFFFFFFFFFLL;
      bigManager.AllocateArrayForOutput(supportArray, bigSize);
      // It does not seem reasonable to get here.  The previous call should fail.
      VTKM_TEST_FAIL("A ridiculously sized allocation succeeded.  Either there "
                     "was a failure that was not reported but should have been "
                     "or the width of vtkm::Id is not large enough to express all "
                     "array sizes.");
    }
    catch (vtkm::cont::ErrorControlOutOfMemory error)
    {
      std::cout << "Got the expected error: " << error.GetMessage() << std::endl;
    }
#else
    std::cout << "--------- Skiping out of memory test" << std::endl;
#endif
  }

  VTKM_CONT_EXPORT
  static void TestTimer()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Timer" << std::endl;

    vtkm::cont::Timer<DeviceAdapterTag> timer;

#ifndef _WIN32
    sleep(1);
#else
    Sleep(1000);
#endif

    vtkm::Float64 elapsedTime = timer.GetElapsedTime();

    std::cout << "Elapsed time: " << elapsedTime << std::endl;

    VTKM_TEST_ASSERT(elapsedTime > 0.999,
                     "Timer did not capture full second wait.");
    VTKM_TEST_ASSERT(elapsedTime < 2.0,
                     "Timer counted too far or system really busy.");
  }

  static VTKM_CONT_EXPORT void TestAlgorithmSchedule()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing single value Scheduling with vtkm::Id" << std::endl;

    {
      std::cout << "Allocating execution array" << std::endl;
      IdStorage storage;
      IdArrayManagerExecution manager(&storage);

      std::cout << "Running clear." << std::endl;
      Algorithm::Schedule(ClearArrayKernel(manager.PrepareForOutput(1)), 1);

      std::cout << "Running add." << std::endl;
      Algorithm::Schedule(AddArrayKernel(manager.PrepareForInPlace(false)), 1);

      std::cout << "Checking results." << std::endl;
      manager.RetrieveOutputData(&storage);

      for (vtkm::Id index = 0; index < 1; index++)
      {
        vtkm::Id value = storage.GetPortalConst().Get(index);
        VTKM_TEST_ASSERT(value == index + OFFSET,
                         "Got bad value for single value scheduled kernel.");
      }
    } //release memory

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule with vtkm::Id" << std::endl;

    {
      std::cout << "Allocating execution array" << std::endl;
      IdStorage storage;
      IdArrayManagerExecution manager(&storage);

      std::cout << "Running clear." << std::endl;
      Algorithm::Schedule(ClearArrayKernel(manager.PrepareForOutput(ARRAY_SIZE)),
                          ARRAY_SIZE);

      std::cout << "Running add." << std::endl;
      Algorithm::Schedule(AddArrayKernel(manager.PrepareForInPlace(false)),
                          ARRAY_SIZE);

      std::cout << "Checking results." << std::endl;
      manager.RetrieveOutputData(&storage);

      for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
      {
        vtkm::Id value = storage.GetPortalConst().Get(index);
        VTKM_TEST_ASSERT(value == index + OFFSET,
                         "Got bad value for scheduled kernels.");
      }
    } //release memory

    //verify that the schedule call works with id3
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule with vtkm::Id3" << std::endl;

    {
      std::cout << "Allocating execution array" << std::endl;
      IdStorage storage;
      IdArrayManagerExecution manager(&storage);
      vtkm::Id3 maxRange(DIM_SIZE);

      std::cout << "Running clear." << std::endl;
      Algorithm::Schedule(
            ClearArrayKernel(manager.PrepareForOutput(
                               DIM_SIZE * DIM_SIZE * DIM_SIZE)),
            maxRange);

      std::cout << "Running add." << std::endl;
      Algorithm::Schedule(AddArrayKernel(manager.PrepareForInPlace(false)),
                          maxRange);

      std::cout << "Checking results." << std::endl;
      manager.RetrieveOutputData(&storage);

      const vtkm::Id maxId = DIM_SIZE * DIM_SIZE * DIM_SIZE;
      for (vtkm::Id index = 0; index < maxId; index++)
      {
        vtkm::Id value = storage.GetPortalConst().Get(index);
        VTKM_TEST_ASSERT(value == index + OFFSET,
                         "Got bad value for scheduled vtkm::Id3 kernels.");
      }
    } //release memory
  }

  static VTKM_CONT_EXPORT void TestStreamCompact()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Stream Compact" << std::endl;

    //test the version of compact that takes in input and uses it as a stencil
    //and uses the index of each item as the value to place in the result vector
    IdArrayHandle array;
    IdArrayHandle result;

    //construct the index array

    Algorithm::Schedule(
          MarkOddNumbersKernel(array.PrepareForOutput(ARRAY_SIZE,
                                                      DeviceAdapterTag())),
          ARRAY_SIZE);

    Algorithm::StreamCompact(array, result);
    VTKM_TEST_ASSERT(result.GetNumberOfValues() == array.GetNumberOfValues()/2,
                     "result of compacation has an incorrect size");

    for (vtkm::Id index = 0; index < result.GetNumberOfValues(); index++)
    {
      const vtkm::Id value = result.GetPortalConstControl().Get(index);
      VTKM_TEST_ASSERT(value == (index*2)+1,
                       "Incorrect value in compaction results.");
    }
  }

  static VTKM_CONT_EXPORT void TestStreamCompactWithStencil()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Stream Compact with stencil" << std::endl;

    IdArrayHandle array;
    IdArrayHandle stencil;
    IdArrayHandle result;

    //construct the index array
    Algorithm::Schedule(
          OffsetPlusIndexKernel(array.PrepareForOutput(ARRAY_SIZE,
                                                       DeviceAdapterTag())),
          ARRAY_SIZE);
    Algorithm::Schedule(
          MarkOddNumbersKernel(stencil.PrepareForOutput(ARRAY_SIZE,
                                                        DeviceAdapterTag())),
          ARRAY_SIZE);

    Algorithm::StreamCompact(array,stencil,result);
    VTKM_TEST_ASSERT(result.GetNumberOfValues() == array.GetNumberOfValues()/2,
                     "result of compacation has an incorrect size");

    for (vtkm::Id index = 0; index < result.GetNumberOfValues(); index++)
    {
      const vtkm::Id value = result.GetPortalConstControl().Get(index);
      VTKM_TEST_ASSERT(value == (OFFSET + (index*2)+1),
                       "Incorrect value in compaction result.");
    }
  }

  static VTKM_CONT_EXPORT void TestOrderedUniqueValues()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing Sort, Unique, LowerBounds and UpperBounds" << std::endl;
    vtkm::Id testData[ARRAY_SIZE];
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      testData[i]= OFFSET+(i % 50);
    }

    IdArrayHandle input = MakeArrayHandle(testData, ARRAY_SIZE);

    //make a deep copy of input and place it into temp
    IdArrayHandle temp;
    Algorithm::Copy(input,temp);

    Algorithm::Sort(temp);
    Algorithm::Unique(temp);

    IdArrayHandle handle;
    IdArrayHandle handle1;

    //verify lower and upper bounds work
    Algorithm::LowerBounds(temp,input,handle);
    Algorithm::UpperBounds(temp,input,handle1);

    // Check to make sure that temp was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    temp.GetPortalConstControl();  // Forces copy back to control.
    temp.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VTKM_TEST_ASSERT(
          temp.GetNumberOfValues() == 50,
          "Unique did not resize array (or size did not copy to control).");

    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
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
    randomData[0]=500;  // 2 (lower), 3 (upper)
    randomData[1]=955;  // 3 (lower), 4 (upper)
    randomData[2]=955;  // 3 (lower), 4 (upper)
    randomData[3]=120;  // 0 (lower), 1 (upper)
    randomData[4]=320;  // 1 (lower), 2 (upper)
    randomData[5]=955;  // 3 (lower), 4 (upper)

    //change the control structure under the handle
    input = MakeArrayHandle(randomData, RANDOMDATA_SIZE);
    Algorithm::Copy(input,handle);
    VTKM_TEST_ASSERT(handle.GetNumberOfValues() == RANDOMDATA_SIZE,
                     "Handle incorrect size after setting new control data");

    Algorithm::Copy(input,handle1);
    VTKM_TEST_ASSERT(handle.GetNumberOfValues() == RANDOMDATA_SIZE,
                     "Handle incorrect size after setting new control data");

    Algorithm::Copy(handle,temp);
    VTKM_TEST_ASSERT(temp.GetNumberOfValues() == RANDOMDATA_SIZE,
                     "Copy failed");
    Algorithm::Sort(temp);
    Algorithm::Unique(temp);
    Algorithm::LowerBounds(temp,handle);
    Algorithm::UpperBounds(temp,handle1);

    VTKM_TEST_ASSERT(handle.GetNumberOfValues() == RANDOMDATA_SIZE,
                     "LowerBounds returned incorrect size");

    std::copy(
          vtkm::cont::ArrayPortalToIteratorBegin(handle.GetPortalConstControl()),
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

    std::copy(
          vtkm::cont::ArrayPortalToIteratorBegin(handle1.GetPortalConstControl()),
          vtkm::cont::ArrayPortalToIteratorEnd(handle1.GetPortalConstControl()),
          randomData);
    VTKM_TEST_ASSERT(randomData[0] == 3, "Got bad value - UpperBound");
    VTKM_TEST_ASSERT(randomData[1] == 4, "Got bad value - UpperBound");
    VTKM_TEST_ASSERT(randomData[2] == 4, "Got bad value - UpperBound");
    VTKM_TEST_ASSERT(randomData[3] == 1, "Got bad value - UpperBound");
    VTKM_TEST_ASSERT(randomData[4] == 2, "Got bad value - UpperBound");
    VTKM_TEST_ASSERT(randomData[5] == 4, "Got bad value - UpperBound");
  }

  static VTKM_CONT_EXPORT void TestSort()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort" << std::endl;
    vtkm::Id testData[ARRAY_SIZE];
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      testData[i]= OFFSET+((ARRAY_SIZE-i) % 50);
    }

    IdArrayHandle unsorted = MakeArrayHandle(testData, ARRAY_SIZE);
    IdArrayHandle sorted;
    Algorithm::Copy(unsorted, sorted);

    //Validate the standard inplace sort is correct
    Algorithm::Sort(sorted);

    for (vtkm::Id i = 0; i < ARRAY_SIZE-1; ++i)
    {
      vtkm::Id sorted1 = sorted.GetPortalConstControl().Get(i);
      vtkm::Id sorted2 = sorted.GetPortalConstControl().Get(i+1);
      VTKM_TEST_ASSERT(sorted1 <= sorted2, "Values not properly sorted.");
    }

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort of a ArrayHandleZip" << std::endl;

    //verify that we can use ArrayHandleZip inplace
    vtkm::cont::ArrayHandleZip< IdArrayHandle, IdArrayHandle> zipped(unsorted, sorted);

    //verify we can use the default an custom operator sort with zip handle
    Algorithm::Sort(zipped, comparison::SortGreater());
    Algorithm::Sort(zipped);

    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Pair<vtkm::Id,vtkm::Id> kv_sorted = zipped.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(( OFFSET +  ( i / (ARRAY_SIZE/50)) ) == kv_sorted.first,
                       "ArrayZipHandle improperly sorted");
    }
  }

  static VTKM_CONT_EXPORT void TestSortWithComparisonObject()
    {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort with comparison object" << std::endl;
    vtkm::Id testData[ARRAY_SIZE];
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      testData[i]= OFFSET+((ARRAY_SIZE-i) % 50);
    }

    //sort the users memory in-place
    IdArrayHandle sorted = MakeArrayHandle(testData, ARRAY_SIZE);
    Algorithm::Sort(sorted);

    //copy the sorted array into our own memory, if use the same user ptr
    //we would also sort the 'sorted' handle
    IdArrayHandle comp_sorted;
    Algorithm::Copy(sorted, comp_sorted);
    Algorithm::Sort(comp_sorted,comparison::SortGreater());

    //Validate that sorted and comp_sorted are sorted in the opposite directions
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id sorted1 = sorted.GetPortalConstControl().Get(i);
      vtkm::Id sorted2 = comp_sorted.GetPortalConstControl().Get(ARRAY_SIZE - (i + 1));
      VTKM_TEST_ASSERT(sorted1 == sorted2,
                       "Got bad sort values when using SortGreater");
    }

    //validate that sorted and comp_sorted are now equal
    Algorithm::Sort(comp_sorted,comparison::SortLess());
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id sorted1 = sorted.GetPortalConstControl().Get(i);
      vtkm::Id sorted2 = comp_sorted.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(sorted1 == sorted2,
                       "Got bad sort values when using SortLesser");
    }
  }

  static VTKM_CONT_EXPORT void TestSortByKey()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort by keys" << std::endl;

    typedef vtkm::Vec<FloatDefault,3> Vec3;

    vtkm::Id testKeys[ARRAY_SIZE];
    Vec3 testValues[ARRAY_SIZE];

    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
      {
      testKeys[i] = ARRAY_SIZE - i;
      testValues[i] = TestValue(i, Vec3());
      }

    IdArrayHandle keys = MakeArrayHandle(testKeys, ARRAY_SIZE);
    Vec3ArrayHandle values = MakeArrayHandle(testValues, ARRAY_SIZE);

    Algorithm::SortByKey(keys,values);

    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
      {
      //keys should be sorted from 1 to ARRAY_SIZE
      //values should be sorted from (ARRAY_SIZE-1) to 0
      Vec3 sorted_value = values.GetPortalConstControl().Get(i);
      vtkm::Id sorted_key = keys.GetPortalConstControl().Get(i);

      VTKM_TEST_ASSERT( (sorted_key == (i+1)) , "Got bad SortByKeys key");
      VTKM_TEST_ASSERT( test_equal(sorted_value, TestValue(ARRAY_SIZE-1-i, Vec3())),
                                      "Got bad SortByKeys value");
      }

    // this will return everything back to what it was before sorting
    Algorithm::SortByKey(keys,values,comparison::SortGreater());
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
      {
      //keys should be sorted from ARRAY_SIZE to 1
      //values should be sorted from 0 to (ARRAY_SIZE-1)
      Vec3 sorted_value = values.GetPortalConstControl().Get(i);
      vtkm::Id sorted_key = keys.GetPortalConstControl().Get(i);

      VTKM_TEST_ASSERT( (sorted_key == (ARRAY_SIZE-i)),
                                      "Got bad SortByKeys key");
      VTKM_TEST_ASSERT( test_equal(sorted_value, TestValue(i, Vec3())),
                                      "Got bad SortByKeys value");
      }

    //this is here to verify we can sort by vtkm::Vec
    Algorithm::SortByKey(values,keys);
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
      {
      //keys should be sorted from ARRAY_SIZE to 1
      //values should be sorted from 0 to (ARRAY_SIZE-1)
      Vec3 sorted_value = values.GetPortalConstControl().Get(i);
      vtkm::Id sorted_key = keys.GetPortalConstControl().Get(i);

      VTKM_TEST_ASSERT( (sorted_key == (ARRAY_SIZE-i)),
                                      "Got bad SortByKeys key");
      VTKM_TEST_ASSERT( test_equal(sorted_value, TestValue(i, Vec3())),
                                      "Got bad SortByKeys value");
      }
  }

  static VTKM_CONT_EXPORT void TestLowerBoundsWithComparisonObject()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing LowerBounds with comparison object" << std::endl;
    vtkm::Id testData[ARRAY_SIZE];
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      testData[i]= OFFSET+(i % 50);
    }
    IdArrayHandle input = MakeArrayHandle(testData, ARRAY_SIZE);

    //make a deep copy of input and place it into temp
    IdArrayHandle temp;
    Algorithm::Copy(input,temp);

    Algorithm::Sort(temp);
    Algorithm::Unique(temp);

    IdArrayHandle handle;
    //verify lower bounds work
    Algorithm::LowerBounds(temp,input,handle,comparison::SortLess());

    // Check to make sure that temp was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    temp.GetPortalConstControl();  // Forces copy back to control.
    temp.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VTKM_TEST_ASSERT(
          temp.GetNumberOfValues() == 50,
          "Unique did not resize array (or size did not copy to control).");

    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id value = handle.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(value == i % 50, "Got bad LowerBounds value with SortLess");
    }
  }


  static VTKM_CONT_EXPORT void TestUpperBoundsWithComparisonObject()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing UpperBounds with comparison object" << std::endl;
    vtkm::Id testData[ARRAY_SIZE];
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      testData[i]= OFFSET+(i % 50);
    }
    IdArrayHandle input = MakeArrayHandle(testData, ARRAY_SIZE);

    //make a deep copy of input and place it into temp
    IdArrayHandle temp;
    Algorithm::Copy(input,temp);

    Algorithm::Sort(temp);
    Algorithm::Unique(temp);

    IdArrayHandle handle;
    //verify upper bounds work
    Algorithm::UpperBounds(temp,input,handle,comparison::SortLess());

    // Check to make sure that temp was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    temp.GetPortalConstControl();  // Forces copy back to control.
    temp.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VTKM_TEST_ASSERT(
          temp.GetNumberOfValues() == 50,
          "Unique did not resize array (or size did not copy to control).");

    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id value = handle.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(value == (i % 50)+1, "Got bad UpperBounds value with SortLess");
    }
  }

  static VTKM_CONT_EXPORT void TestUniqueWithComparisonObject()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing Unique with comparison object" << std::endl;
    vtkm::Id testData[ARRAY_SIZE];
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      testData[i]= OFFSET+(i % 50);
    }
    IdArrayHandle input = MakeArrayHandle(testData, ARRAY_SIZE);
    Algorithm::Sort(input);
    Algorithm::Unique(input, FuseAll());

    // Check to make sure that input was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    input.GetPortalConstControl();  // Forces copy back to control.
    input.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VTKM_TEST_ASSERT(
          input.GetNumberOfValues() == 1,
          "Unique did not resize array (or size did not copy to control).");

    vtkm::Id value = input.GetPortalConstControl().Get(0);
    VTKM_TEST_ASSERT(value == OFFSET, "Got bad unique value");
  }

  static VTKM_CONT_EXPORT void TestReduce()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce" << std::endl;

    //construct the index array
    IdArrayHandle array;
    Algorithm::Schedule(
      ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE,
                       DeviceAdapterTag())),
      ARRAY_SIZE);

    //the output of reduce and scan inclusive should be the same
    vtkm::Id reduce_sum = Algorithm::Reduce(array, vtkm::Id(0));
    vtkm::Id reduce_sum_with_intial_value = Algorithm::Reduce(array,
                                                          vtkm::Id(ARRAY_SIZE));
    vtkm::Id inclusive_sum = Algorithm::ScanInclusive(array, array);

    VTKM_TEST_ASSERT(reduce_sum == OFFSET * ARRAY_SIZE,
                     "Got bad sum from Reduce");
    VTKM_TEST_ASSERT(reduce_sum_with_intial_value == reduce_sum + ARRAY_SIZE,
                     "Got bad sum from Reduce with initial value");

    VTKM_TEST_ASSERT(reduce_sum == inclusive_sum,
                     "Got different sums from Reduce and ScanInclusive");
  }

  static VTKM_CONT_EXPORT void TestReduceWithComparisonObject()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce with comparison object " << std::endl;

    //construct the index array. Assign an abnormally large value
    //to the middle of the array, that should be what we see as our sum.
    vtkm::Id testData[ARRAY_SIZE];
    const vtkm::Id maxValue = ARRAY_SIZE*2;
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      testData[i]= i;
    }
    testData[ARRAY_SIZE/2] = maxValue;

    IdArrayHandle input = MakeArrayHandle(testData, ARRAY_SIZE);
    vtkm::Id largestValue = Algorithm::Reduce(input,
                                              vtkm::Id(),
                                              comparison::MaxValue());

    VTKM_TEST_ASSERT(largestValue == maxValue,
                    "Got bad value from Reduce with comparison object");
  }

  static VTKM_CONT_EXPORT void TestReduceByKey()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce By Key" << std::endl;

    //first test with very basic integer key / values
    {
    const vtkm::Id inputLength = 12;
    const vtkm::Id expectedLength = 6;
    vtkm::Id inputKeys[inputLength] =    {0, 0, 0,\
                                          1, 1,\
                                          4,\
                                          0,\
                                          2, 2, 2, 2,\
                                          -1}; // input keys
    vtkm::Id inputValues[inputLength] =  {13, -2, -1,\
                                          1, 1,\
                                          0,\
                                          3,\
                                          1, 2, 3, 4, \
                                          -42}; // input keys
    vtkm::Id expectedKeys[expectedLength] =   { 0, 1, 4, 0,  2, -1 };
    vtkm::Id expectedValues[expectedLength] = {10, 2, 0, 3, 10, -42};

    IdArrayHandle keys = MakeArrayHandle(inputKeys, inputLength);
    IdArrayHandle values = MakeArrayHandle(inputValues, inputLength);

    IdArrayHandle keysOut, valuesOut;
    Algorithm::ReduceByKey( keys,
                            values,
                            keysOut,
                            valuesOut,
                            vtkm::internal::Add() );

    VTKM_TEST_ASSERT(keysOut.GetNumberOfValues() == expectedLength,
                    "Got wrong number of output keys");

    VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                    "Got wrong number of output values");

    for(vtkm::Id i=0; i < expectedLength; ++i)
      {
      const vtkm::Id k = keysOut.GetPortalConstControl().Get(i);
      const vtkm::Id v = valuesOut.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT( expectedKeys[i] == k, "Incorrect reduced key");
      VTKM_TEST_ASSERT( expectedValues[i] == v, "Incorrect reduced vale");
      }
    }

    //next test with a single key across the entire set
    {
    const vtkm::Id inputLength = 3;
    const vtkm::Id expectedLength = 1;
    vtkm::Id inputKeys[inputLength] =    {0, 0, 0}; // input keys
    vtkm::Id inputValues[inputLength] =  {13, -2, -1}; // input keys
    vtkm::Id expectedKeys[expectedLength] =   { 0};
    vtkm::Id expectedValues[expectedLength] = {10};

    IdArrayHandle keys = MakeArrayHandle(inputKeys, inputLength);
    IdArrayHandle values = MakeArrayHandle(inputValues, inputLength);

    IdArrayHandle keysOut, valuesOut;
    Algorithm::ReduceByKey( keys,
                            values,
                            keysOut,
                            valuesOut,
                            vtkm::internal::Add() );

    VTKM_TEST_ASSERT(keysOut.GetNumberOfValues() == expectedLength,
                    "Got wrong number of output keys");

    VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                    "Got wrong number of output values");

    for(vtkm::Id i=0; i < expectedLength; ++i)
      {
      const vtkm::Id k = keysOut.GetPortalConstControl().Get(i);
      const vtkm::Id v = valuesOut.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT( expectedKeys[i] == k, "Incorrect reduced key");
      VTKM_TEST_ASSERT( expectedValues[i] == v, "Incorrect reduced vale");
      }
    }


    //next test with values in vec3d
    {
    const vtkm::Id inputLength = 3;
    const vtkm::Id expectedLength = 1;
    vtkm::Id inputKeys[inputLength] =    {0, 0, 0}; // input keys
    vtkm::Vec<vtkm::Float64, 3> inputValues[inputLength];
    inputValues[0] = vtkm::make_Vec(13.1, 13.3, 13.5);
    inputValues[1] = vtkm::make_Vec(-2.1, -2.3, -2.5);
    inputValues[2] = vtkm::make_Vec(-1.0, -1.0, -1.0); // input keys
    vtkm::Id expectedKeys[expectedLength] =   { 0};

    vtkm::Vec<vtkm::Float64, 3> expectedValues[expectedLength];
    expectedValues[0] = vtkm::make_Vec(10., 10., 10.);

    IdArrayHandle keys = MakeArrayHandle(inputKeys, inputLength);
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>, StorageTag> values = MakeArrayHandle(inputValues, inputLength);

    IdArrayHandle keysOut;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>, StorageTag> valuesOut;
    Algorithm::ReduceByKey( keys,
                            values,
                            keysOut,
                            valuesOut,
                            vtkm::internal::Add() );

    VTKM_TEST_ASSERT(keysOut.GetNumberOfValues() == expectedLength,
                    "Got wrong number of output keys");

    VTKM_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                    "Got wrong number of output values");

    for(vtkm::Id i=0; i < expectedLength; ++i)
      {
      const vtkm::Id k = keysOut.GetPortalConstControl().Get(i);
      const vtkm::Vec<vtkm::Float64, 3> v = valuesOut.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT( expectedKeys[i] == k, "Incorrect reduced key");
      VTKM_TEST_ASSERT( expectedValues[i] == v, "Incorrect reduced vale");
      }
    }

    //next test with values in zip
    {
    const vtkm::Id inputLength = 3;
    const vtkm::Id expectedLength = 1;
    typedef vtkm::Float32 ValueType;
    vtkm::Id inputKeys[inputLength] =    {0, 0, 0}; // input keys
    ValueType inputValues1[inputLength] = {13.1f, -2.1f, -1.0f}; // input values array1
    ValueType inputValues2[inputLength] = {13.3f, -2.3f, -1.0f}; // input values array2
    vtkm::Id expectedKeys[expectedLength] =   { 0};

    ValueType expectedValues1[expectedLength] = {10.f}; // output values 1
    ValueType expectedValues2[expectedLength] = {10.f}; // output values 2

    IdArrayHandle keys = MakeArrayHandle(inputKeys, inputLength);
    typedef vtkm::cont::ArrayHandle<ValueType, StorageTag> ValueArrayType;
    ValueArrayType values1 = MakeArrayHandle(inputValues1, inputLength);
    ValueArrayType values2 = MakeArrayHandle(inputValues2, inputLength);

    vtkm::cont::ArrayHandleZip<ValueArrayType, ValueArrayType> valuesZip;
    valuesZip = make_ArrayHandleZip(values1, values2); // values in zip

    IdArrayHandle keysOut;
    ValueArrayType valuesOut1;
    ValueArrayType valuesOut2;
    vtkm::cont::ArrayHandleZip<ValueArrayType, ValueArrayType> valuesOutZip(valuesOut1, valuesOut2);

    Algorithm::ReduceByKey( keys,
                            valuesZip,
                            keysOut,
                            valuesOutZip,
                            vtkm::internal::Add() );

    VTKM_TEST_ASSERT(keysOut.GetNumberOfValues() == expectedLength,
                    "Got wrong number of output keys");

    VTKM_TEST_ASSERT(valuesOutZip.GetNumberOfValues() == expectedLength,
                    "Got wrong number of output values");

    for(vtkm::Id i=0; i < expectedLength; ++i)
      {
      const vtkm::Id k = keysOut.GetPortalConstControl().Get(i);
      const vtkm::Pair<ValueType, ValueType> v = valuesOutZip.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT( expectedKeys[i] == k, "Incorrect reduced key");
      VTKM_TEST_ASSERT( expectedValues1[i] == v.first, "Incorrect reduced vale");
      VTKM_TEST_ASSERT( expectedValues2[i] == v.second, "Incorrect reduced vale");
      }
    }

    //next test with values in heterogeneous zip
    {
    const vtkm::Id inputLength = 3;
    const vtkm::Id expectedLength = 1;
    typedef vtkm::Float32 ValueType;
    vtkm::Id inputKeys[inputLength] =    {0, 0, 0}; // input keys
    ValueType inputValues1[inputLength] = {13.1f, -2.1f, -1.0f}; // input values array1
    vtkm::Id expectedKeys[expectedLength] =   { 0};

    ValueType expectedValues1[expectedLength] = {10.f}; // output values 1
    ValueType expectedValues2[expectedLength] = {3.f}; // output values 2

    IdArrayHandle keys = MakeArrayHandle(inputKeys, inputLength);
    typedef vtkm::cont::ArrayHandle<ValueType, StorageTag> ValueArrayType;
    ValueArrayType values1 = MakeArrayHandle(inputValues1, inputLength);
    typedef vtkm::cont::ArrayHandleConstant<ValueType> ConstValueArrayType;
    ConstValueArrayType constOneArray(1.f, inputLength);

    vtkm::cont::ArrayHandleZip<ValueArrayType, ConstValueArrayType> valuesZip;
    valuesZip = make_ArrayHandleZip(values1, constOneArray); // values in zip

    IdArrayHandle keysOut;
    ValueArrayType valuesOut1;
    ValueArrayType valuesOut2;
    vtkm::cont::ArrayHandleZip<ValueArrayType, ValueArrayType> valuesOutZip(valuesOut1, valuesOut2);

    Algorithm::ReduceByKey( keys,
                        valuesZip,
                        keysOut,
                        valuesOutZip,
                        vtkm::internal::Add() );

    VTKM_TEST_ASSERT(keysOut.GetNumberOfValues() == expectedLength,
                "Got wrong number of output keys");

    VTKM_TEST_ASSERT(valuesOutZip.GetNumberOfValues() == expectedLength,
                "Got wrong number of output values");

    for(vtkm::Id i=0; i < expectedLength; ++i)
    {
      const vtkm::Id k = keysOut.GetPortalConstControl().Get(i);
      const vtkm::Pair<ValueType, ValueType> v = valuesOutZip.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT( expectedKeys[i] == k, "Incorrect reduced key");
      VTKM_TEST_ASSERT( expectedValues1[i] == v.first, "Incorrect reduced vale");
      VTKM_TEST_ASSERT( expectedValues2[i] == v.second, "Incorrect reduced vale");
    }
    }

  }

  static VTKM_CONT_EXPORT void TestScanInclusive()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Inclusive Scan" << std::endl;
    //construct the index array
    IdArrayHandle array;
    Algorithm::Schedule(
      ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE,
                       DeviceAdapterTag())),
      ARRAY_SIZE);

    //we know have an array whose sum is equal to OFFSET * ARRAY_SIZE,
    //let's validate that
    vtkm::Id sum = Algorithm::ScanInclusive(array, array);
    VTKM_TEST_ASSERT(sum == OFFSET * ARRAY_SIZE,
                     "Got bad sum from Inclusive Scan");

    //each value should be equal to the Triangle Number of that index
    //ie 1, 3, 6, 10, 15, 21 ...
    vtkm::Id partialSum = 0;
    vtkm::Id triangleNumber = 0;
    for(vtkm::Id i=0, pos=1; i < ARRAY_SIZE; ++i, ++pos)
    {
      const vtkm::Id value = array.GetPortalConstControl().Get(i);
      partialSum += value;
      triangleNumber = ((pos*(pos+1))/2);
      VTKM_TEST_ASSERT(partialSum == triangleNumber * OFFSET,
                       "Incorrect partial sum");
    }
  }

  static VTKM_CONT_EXPORT void TestScanInclusiveWithComparisonObject()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Inclusive Scan with comparison object " << std::endl;

    //construct the index array
    IdArrayHandle array;
    Algorithm::Schedule(
      ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE,
                       DeviceAdapterTag())),
      ARRAY_SIZE);

    Algorithm::Schedule(
      AddArrayKernel(array.PrepareForOutput(ARRAY_SIZE,
                     DeviceAdapterTag())),
      ARRAY_SIZE);
    //we know have an array whose sum is equal to OFFSET * ARRAY_SIZE,
    //let's validate that
    IdArrayHandle result;
    vtkm::Id sum = Algorithm::ScanInclusive(array,
                                            result,
                                            comparison::MaxValue());
    VTKM_TEST_ASSERT(sum == OFFSET + (ARRAY_SIZE-1),
                     "Got bad sum from Inclusive Scan with comparison object");

    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      const vtkm::Id input_value = array.GetPortalConstControl().Get(i);
      const vtkm::Id result_value = result.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(input_value == result_value, "Incorrect partial sum");
    }

    //now try it inline
    sum = Algorithm::ScanInclusive(array,
                                   array,
                                   comparison::MaxValue());
    VTKM_TEST_ASSERT(sum == OFFSET + (ARRAY_SIZE-1),
                     "Got bad sum from Inclusive Scan with comparison object");

    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      const vtkm::Id input_value = array.GetPortalConstControl().Get(i);
      const vtkm::Id result_value = result.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(input_value == result_value, "Incorrect partial sum");
    }

  }

  static VTKM_CONT_EXPORT void TestScanExclusive()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Exclusive Scan" << std::endl;

    //construct the index array
    IdArrayHandle array;
    Algorithm::Schedule(
      ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE,
                       DeviceAdapterTag())),
      ARRAY_SIZE);

    // we know have an array whose sum = (OFFSET * ARRAY_SIZE),
    // let's validate that
    vtkm::Id sum = Algorithm::ScanExclusive(array, array);

    VTKM_TEST_ASSERT(sum == (OFFSET * ARRAY_SIZE),
                     "Got bad sum from Exclusive Scan");

    //each value should be equal to the Triangle Number of that index
    //ie 0, 1, 3, 6, 10, 15, 21 ...
    vtkm::Id partialSum = 0;
    vtkm::Id triangleNumber = 0;
    for(vtkm::Id i=0, pos=0; i < ARRAY_SIZE; ++i, ++pos)
    {
      const vtkm::Id value = array.GetPortalConstControl().Get(i);
      partialSum += value;
      triangleNumber = ((pos*(pos+1))/2);
      VTKM_TEST_ASSERT(partialSum == triangleNumber * OFFSET,
                       "Incorrect partial sum");
    }
  }

  static VTKM_CONT_EXPORT void TestErrorExecution()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Exceptions in Execution Environment" << std::endl;

    std::cout << "Generating one error." << std::endl;
    std::string message;
    try
    {
      Algorithm::Schedule(OneErrorKernel(), ARRAY_SIZE);
    }
    catch (vtkm::cont::ErrorExecution error)
    {
      std::cout << "Got expected error: " << error.GetMessage() << std::endl;
      message = error.GetMessage();
    }
    VTKM_TEST_ASSERT(message == ERROR_MESSAGE,
                     "Did not get expected error message.");

    std::cout << "Generating lots of errors." << std::endl;
    message = "";
    try
    {
      Algorithm::Schedule(AllErrorKernel(), ARRAY_SIZE);
    }
    catch (vtkm::cont::ErrorExecution error)
    {
      std::cout << "Got expected error: " << error.GetMessage() << std::endl;
      message = error.GetMessage();
    }
    VTKM_TEST_ASSERT(message == ERROR_MESSAGE,
                     "Did not get expected error message.");
  }

  struct TestAll
  {
    VTKM_CONT_EXPORT void operator()() const
    {
      std::cout << "Doing DeviceAdapter tests" << std::endl;
      TestArrayManagerExecution();
      TestOutOfMemory();
      TestTimer();

      TestAlgorithmSchedule();
      TestErrorExecution();

      TestReduce();
      TestReduceWithComparisonObject();

      TestReduceByKey();

      TestScanInclusive();
      TestScanInclusiveWithComparisonObject();

      TestScanExclusive();

      TestSort();
      TestSortWithComparisonObject();
      TestSortByKey();

      TestLowerBoundsWithComparisonObject();

      TestUpperBoundsWithComparisonObject();

      TestUniqueWithComparisonObject();

      TestOrderedUniqueValues(); //tests Copy, LowerBounds, Sort, Unique
      TestStreamCompactWithStencil();
      TestStreamCompact();
    }
  };

public:

  /// Run a suite of tests to check to see if a DeviceAdapter properly supports
  /// all members and classes required for driving vtkm algorithms. Returns an
  /// error code that can be returned from the main function of a test.
  ///
  static VTKM_CONT_EXPORT int Run()
  {
    return vtkm::cont::testing::Testing::Run(TestAll());
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
