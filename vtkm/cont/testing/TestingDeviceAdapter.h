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

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
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
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a,const T& b) const
  {
    typedef typename vtkm::TypeTraits<T>::DimensionalityTag Dimensionality;
    return this->compare(a,b,Dimensionality());
  }
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool compare(const T& a,const T& b,
                                     vtkm::TypeTraitsScalarTag) const
  {
    return a < b;
  }
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool compare(const T& a,const T& b,
                                     vtkm::TypeTraitsVectorTag) const
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
  template<typename T>
  VTKM_EXEC_CONT_EXPORT bool operator()(const T& a,const T& b) const
  {
    typedef typename vtkm::TypeTraits<T>::DimensionalityTag Dimensionality;
    return this->compare(a,b,Dimensionality());
  }
  template<typename T>
  VTKM_EXEC_EXPORT bool compare(const T& a,const T& b,
                                     vtkm::TypeTraitsScalarTag) const
  {
    return a > b;
  }
  template<typename T>
  VTKM_EXEC_EXPORT bool compare(const T& a,const T& b,
                                     vtkm::TypeTraitsVectorTag) const
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
#define ARRAY_SIZE 500
#define OFFSET 1000
#define DIM 64

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

  struct NGMult //: public vtkm::exec::WorkletMapField
  {
    // typedef void ControlSignature(Field(In), Field(In), Field(Out));
    // typedef _3 ExecutionSignature(_1, _2);

    template<typename T>
    VTKM_EXEC_EXPORT T operator()(T a, T b) const
    {
      return a * b;
    }
  };

  struct NGNoOp //: public vtkm::exec::WorkletMapField
  {
    // typedef void ControlSignature(Field(In), Field(Out));
    // typedef _2 ExecutionSignature(_1);

    template<typename T>
    VTKM_EXEC_EXPORT T operator()(T a) const
    {
      return a;
    }
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
      vtkm::Id DIM_SIZE = vtkm::Id(std::pow(ARRAY_SIZE, 1/3.0f));
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

  // static VTKM_CONT_EXPORT void TestDispatcher()
  // {
  //   std::cout << "-------------------------------------------" << std::endl;
  //   std::cout << "Testing vtkm::cont::Dispatcher* classes" << std::endl;

  //   std::cout << "Testing vtkm::cont::Dispatcher with array of size 1" << std::endl;

  //   std::vector<vtkm::Id> singleElement; singleElement.push_back(1234);
  //   IdArrayHandle hSingleElement = MakeArrayHandle(singleElement);
  //   IdArrayHandle hResult;

  //   vtkm::cont::DispatcherMapField< NGNoOp, DeviceAdapterTag > dispatcherNoOp;
  //   dispatcherNoOp.Invoke( hSingleElement, hResult );

  //   // output
  //   std::cout << "hResult.GetNumberOfValues(): " << hResult.GetNumberOfValues() << std::endl;
  //   for (vtkm::Id i = 0; i < hResult.GetNumberOfValues(); ++i)
  //     {
  //     std::cout << hResult.GetPortalConstControl().Get(i) << ",";
  //     }
  //   std::cout << std::endl;

  //   // assert
  //   VTKM_TEST_ASSERT(
  //           hSingleElement.GetNumberOfValues() == hResult.GetNumberOfValues(),
  //           "out handle of single scheduling is wrong size");
  //   VTKM_TEST_ASSERT(singleElement[0] == 1234,
  //                   "output of single scheduling is incorrect");

  //   std::vector<vtkm::FloatDefault> field(ARRAY_SIZE);
  //   for (vtkm::Id i = 0; i < ARRAY_SIZE; i++)
  //     {
  //     field[i]=i;
  //     }
  //   ScalarArrayHandle fieldHandle = MakeArrayHandle(field);
  //   ScalarArrayHandle multHandle;

  //   std::cout << "Running NG Multiply worklet with two handles" << std::endl;

  //   vtkm::cont::DispatcherMapField< NGMult, DeviceAdapterTag > dispatcherMult;
  //   dispatcherMult.Invoke( fieldHandle, fieldHandle, multHandle );

  //   typename ScalarArrayHandle::PortalConstControl multPortal =
  //       multHandle.GetPortalConstControl();

  //   for (vtkm::Id i = 0; i < ARRAY_SIZE; i++)
  //     {
  //     vtkm::FloatDefault squareValue = multPortal.Get(i);
  //     vtkm::FloatDefault squareTrue = field[i]*field[i];
  //     VTKM_TEST_ASSERT(test_equal(squareValue, squareTrue),
  //                     "Got bad multiply result");
  //     }

  //   std::cout << "Running NG Multiply worklet with handle and constant" << std::endl;
  //   dispatcherMult.Invoke(4.0f,fieldHandle, multHandle);
  //   multPortal = multHandle.GetPortalConstControl();

  //   for (vtkm::Id i = 0; i < ARRAY_SIZE; i++)
  //     {
  //     vtkm::FloatDefault squareValue = multPortal.Get(i);
  //     vtkm::FloatDefault squareTrue = field[i]*4.0f;
  //     VTKM_TEST_ASSERT(test_equal(squareValue, squareTrue),
  //                     "Got bad multiply result");
  //     }


  //   std::cout << "Testing Schedule on Subset" << std::endl;
  //   std::vector<vtkm::FloatDefault> fullField(ARRAY_SIZE);
  //   std::vector<vtkm::Id> subSetLookup(ARRAY_SIZE/2);
  //   for (vtkm::Id i = 0; i < ARRAY_SIZE; i++)
  //     {
  //     field[i]=i;
  //     if(i%2==0)
  //       {
  //       subSetLookup[i/2]=i;
  //       }
  //     }

  //   IdArrayHandle subSetLookupHandle = MakeArrayHandle(subSetLookup);
  //   ScalarArrayHandle fullFieldHandle = MakeArrayHandle(fullField);

  //   std::cout << "Running clear on subset." << std::endl;
  //   vtkm::cont::DispatcherMapField< ClearArrayMapKernel,
  //                                  DeviceAdapterTag > dispatcherClear;
  //   dispatcherClear.Invoke(
  //         make_Permutation(subSetLookupHandle,fullFieldHandle,ARRAY_SIZE));

  //   for (vtkm::Id index = 0; index < ARRAY_SIZE; index+=2)
  //     {
  //     vtkm::Id value = fullFieldHandle.GetPortalConstControl().Get(index);
  //     VTKM_TEST_ASSERT(value == OFFSET,
  //                     "Got bad value for subset scheduled kernel.");
  //     }
  // }

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

    IdArrayHandle handle;
    IdArrayHandle handle1;
    IdArrayHandle temp;
    Algorithm::Copy(input,temp);
    Algorithm::Sort(temp);
    Algorithm::Unique(temp);

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
    IdArrayHandle input = MakeArrayHandle(testData, ARRAY_SIZE);

    IdArrayHandle sorted;

    Algorithm::Copy(input,sorted);

    //Validate the standard sort is correct
    Algorithm::Sort(sorted);

    for (vtkm::Id i = 0; i < ARRAY_SIZE-1; ++i)
    {
      vtkm::Id sorted1 = sorted.GetPortalConstControl().Get(i);
      vtkm::Id sorted2 = sorted.GetPortalConstControl().Get(i+1);
      //      std::cout << sorted1 << " <= " << sorted2 << std::endl;
      VTKM_TEST_ASSERT(sorted1 <= sorted2, "Values not properly sorted.");
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
    IdArrayHandle input = MakeArrayHandle(testData, ARRAY_SIZE);

    IdArrayHandle sorted;
    IdArrayHandle comp_sorted;

    Algorithm::Copy(input,sorted);
    Algorithm::Copy(input,comp_sorted);

    //Validate the standard sort is correct
    Algorithm::Sort(sorted);

    //Validate the sort, and SortGreater are inverse
    Algorithm::Sort(comp_sorted,comparison::SortGreater());

    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
    {
      vtkm::Id sorted1 = sorted.GetPortalConstControl().Get(i);
      vtkm::Id sorted2 = comp_sorted.GetPortalConstControl().Get(ARRAY_SIZE - (i + 1));
      //      std::cout << sorted1 << "==" << sorted2 << std::endl;
      VTKM_TEST_ASSERT(sorted1 == sorted2,
                       "Got bad sort values when using SortGreater");
    }

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

    IdArrayHandle sorted_keys;
    Vec3ArrayHandle sorted_values;

    Algorithm::Copy(keys,sorted_keys);
    Algorithm::Copy(values,sorted_values);

    Algorithm::SortByKey(sorted_keys,sorted_values);
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
      {
      //keys should be sorted from 1 to ARRAY_SIZE
      //values should be sorted from (ARRAY_SIZE-1) to 0
      Vec3 sorted_value = sorted_values.GetPortalConstControl().Get(i);
      vtkm::Id sorted_key = sorted_keys.GetPortalConstControl().Get(i);

      VTKM_TEST_ASSERT( (sorted_key == (i+1)) , "Got bad SortByKeys key");
      VTKM_TEST_ASSERT( test_equal(sorted_value, TestValue(ARRAY_SIZE-1-i, Vec3())),
                                      "Got bad SortByKeys value");
      }

    // this will return everything back to what it was before sorting
    Algorithm::SortByKey(sorted_keys,sorted_values,comparison::SortGreater());
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
      {
      //keys should be sorted from ARRAY_SIZE to 1
      //values should be sorted from 0 to (ARRAY_SIZE-1)
      Vec3 sorted_value = sorted_values.GetPortalConstControl().Get(i);
      vtkm::Id sorted_key = sorted_keys.GetPortalConstControl().Get(i);

      VTKM_TEST_ASSERT( (sorted_key == (ARRAY_SIZE-i)),
                                      "Got bad SortByKeys key");
      VTKM_TEST_ASSERT( test_equal(sorted_value, TestValue(i, Vec3())),
                                      "Got bad SortByKeys value");
      }

    //this is here to verify we can sort by vtkm::Vec
    Algorithm::SortByKey(sorted_values,sorted_keys);
    for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
      {
      //keys should be sorted from ARRAY_SIZE to 1
      //values should be sorted from 0 to (ARRAY_SIZE-1)
      Vec3 sorted_value = sorted_values.GetPortalConstControl().Get(i);
      vtkm::Id sorted_key = sorted_keys.GetPortalConstControl().Get(i);

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

    IdArrayHandle temp;
    Algorithm::Copy(input,temp);
    Algorithm::Sort(temp);
    Algorithm::Unique(temp, FuseAll());

    // Check to make sure that temp was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    temp.GetPortalConstControl();  // Forces copy back to control.
    temp.ReleaseResourcesExecution(); // Make sure not counting on execution.
    std::cout << "temp size: " << temp.GetNumberOfValues() << std::endl;
    VTKM_TEST_ASSERT(
          temp.GetNumberOfValues() == 1,
          "Unique did not resize array (or size did not copy to control).");

    vtkm::Id value = temp.GetPortalConstControl().Get(0);
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

  // template<typename GridType>
  // static VTKM_CONT_EXPORT void TestWorkletMapField()
  // {
  //   std::cout << "-------------------------------------------" << std::endl;
  //   std::cout << "Testing basic map field worklet" << std::endl;

  //   //use a scoped pointer that constructs and fills a grid of the
  //   //right type
  //   vtkm::cont::testing::TestGrid<GridType,StorageTagBasic>
  //       grid(DIM);

  //   vtkm::Vector3 trueGradient = vtkm::make_Vector3(1.0, 1.0, 1.0);

  //   std::vector<vtkm::FloatDefault> field(grid->GetNumberOfPoints());
  //   std::cout << "Number of Points in the grid: "
  //             <<  grid->GetNumberOfPoints()
  //             << std::endl;
  //   for (vtkm::Id pointIndex = 0;
  //        pointIndex < grid->GetNumberOfPoints();
  //        pointIndex++)
  //     {
  //     vtkm::Vector3 coordinates = grid.GetPointCoordinates(pointIndex);
  //     field[pointIndex] = vtkm::dot(coordinates, trueGradient);
  //     }
  //   ScalarArrayHandle fieldHandle = MakeArrayHandle(field);

  //   ScalarArrayHandle squareHandle;

  //   std::cout << "Running Square worklet" << std::endl;
  //   vtkm::cont::DispatcherMapField<vtkm::worklet::Square,
  //                                 DeviceAdapterTag> dispatcher;
  //   dispatcher.Invoke(fieldHandle, squareHandle);

  //   typename ScalarArrayHandle::PortalConstControl squarePortal =
  //       squareHandle.GetPortalConstControl();

  //   std::cout << "Checking result" << std::endl;
  //   for (vtkm::Id pointIndex = 0;
  //        pointIndex < grid->GetNumberOfPoints();
  //        pointIndex++)
  //     {
  //     vtkm::FloatDefault squareValue = squarePortal.Get(pointIndex);
  //     vtkm::FloatDefault squareTrue = field[pointIndex]*field[pointIndex];
  //     VTKM_TEST_ASSERT(test_equal(squareValue, squareTrue),
  //                     "Got bad square");
  //     }
  // }

  // template<typename GridType>
  // static VTKM_CONT_EXPORT void TestWorkletFieldMapError()
  // {
  //   std::cout << "-------------------------------------------" << std::endl;
  //   std::cout << "Testing map field worklet error" << std::endl;

  //   vtkm::cont::testing::TestGrid<GridType,StorageTagBasic>
  //       grid(DIM);

  //   std::cout << "Running field map worklet that errors" << std::endl;
  //   bool gotError = false;
  //   try
  //     {
  //     vtkm::cont::DispatcherMapField< vtkm::worklet::testing::FieldMapError,
  //                                 DeviceAdapterTag> dispatcher;
  //     dispatcher.Invoke( grid.GetRealGrid().GetPointCoordinates() );
  //     }
  //   catch (vtkm::cont::ErrorExecution error)
  //     {
  //     std::cout << "Got expected ErrorExecution object." << std::endl;
  //     std::cout << error.GetMessage() << std::endl;
  //     gotError = true;
  //     }

  //   VTKM_TEST_ASSERT(gotError, "Never got the error thrown.");
  // }

  // template<typename GridType>
  // static VTKM_CONT_EXPORT void TestWorkletMapCell()
  // {
  //   std::cout << "-------------------------------------------" << std::endl;
  //   std::cout << "Testing basic map cell worklet" << std::endl;

  //   if (vtkm::CellTraits<typename GridType::CellTag>::TOPOLOGICAL_DIMENSIONS < 3)
  //     {
  //     std::cout << "Skipping.  Too hard to check gradient "
  //               << "on cells with topological dimension < 3" << std::endl;
  //     }
  //   else
  //     {
  //     // Calling a separate Impl function because the CUDA compiler is good
  //     // enough to optimize the if statement as a constant expression and
  //     // then complains about unreachable statements after a return.
  //     TestWorkletMapCellImpl<GridType>();
  //     }
  // }

  // template<typename GridType>
  // static VTKM_CONT_EXPORT void TestWorkletMapCellImpl()
  // {
  //   vtkm::cont::testing::TestGrid<GridType,StorageTagBasic>
  //       grid(DIM);

  //   vtkm::Vector3 trueGradient = vtkm::make_Vector3(1.0, 1.0, 1.0);

  //   std::vector<vtkm::FloatDefault> field(grid->GetNumberOfPoints());
  //   for (vtkm::Id pointIndex = 0;
  //        pointIndex < grid->GetNumberOfPoints();
  //        pointIndex++)
  //     {
  //     vtkm::Vector3 coordinates = grid.GetPointCoordinates(pointIndex);
  //     field[pointIndex] = vtkm::dot(coordinates, trueGradient);
  //     }
  //   ScalarArrayHandle fieldHandle = MakeArrayHandle(field);

  //   Vec3ArrayHandle gradientHandle;

  //   std::cout << "Running CellGradient worklet" << std::endl;

  //   vtkm::cont::DispatcherMapCell< vtkm::worklet::CellGradient,
  //                                  DeviceAdapterTag> dispatcher;
  //   dispatcher.Invoke(grid.GetRealGrid(),
  //                   grid->GetPointCoordinates(),
  //                   fieldHandle,
  //                   gradientHandle);

  //   typename Vec3ArrayHandle::PortalConstControl gradientPortal =
  //       gradientHandle.GetPortalConstControl();

  //   std::cout << "Checking result" << std::endl;
  //   for (vtkm::Id cellIndex = 0;
  //        cellIndex < grid->GetNumberOfCells();
  //        cellIndex++)
  //     {
  //     vtkm::Vector3 gradientValue = gradientPortal.Get(cellIndex);
  //     VTKM_TEST_ASSERT(test_equal(gradientValue, trueGradient),
  //                     "Got bad gradient");
  //     }
  // }

  // template<typename GridType>
  // static VTKM_CONT_EXPORT void TestWorkletCellMapError()
  // {
  //   std::cout << "-------------------------------------------" << std::endl;
  //   std::cout << "Testing map cell worklet error" << std::endl;

  //   vtkm::cont::testing::TestGrid<GridType,StorageTagBasic>
  //       grid(DIM);

  //   std::cout << "Running cell map worklet that errors" << std::endl;
  //   bool gotError = false;
  //   try
  //     {
  //     vtkm::cont::DispatcherMapCell< vtkm::worklet::testing::CellMapError,
  //                                    DeviceAdapterTag> dispatcher;
  //     dispatcher.Invoke( grid.GetRealGrid() );
  //     }
  //   catch (vtkm::cont::ErrorExecution error)
  //     {
  //     std::cout << "Got expected ErrorExecution object." << std::endl;
  //     std::cout << error.GetMessage() << std::endl;
  //     gotError = true;
  //     }

  //   VTKM_TEST_ASSERT(gotError, "Never got the error thrown.");
  // }

  // struct TestWorklets
  // {
  //   template<typename GridType>
  //   VTKM_CONT_EXPORT void operator()(const GridType&) const
  //     {
  //     TestWorkletMapField<GridType>();
  //     TestWorkletFieldMapError<GridType>();
  //     TestWorkletMapCell<GridType>();
  //     TestWorkletCellMapError<GridType>();
  //     }
  // };

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
      // TestDispatcher();
      TestStreamCompactWithStencil();
      TestStreamCompact();


      // std::cout << "Doing Worklet tests with all grid type" << std::endl;
      // vtkm::cont::testing::GridTesting::TryAllGridTypes(
      //       TestWorklets(), StorageTagBasic());
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
