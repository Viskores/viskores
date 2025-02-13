//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/cont/DeviceAdapter.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/cont/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{

template<typename T>
viskores::Float32 TestValue(T index)
{
  return static_cast<viskores::Float32>(1 + 0.001 * index);
}

void CheckArrayValues(const viskores::cont::ArrayHandle<viskores::Float32>& array,
                      viskores::Float32 factor = 1)
{
  // So far all the examples are using 50 entries. Could change.
  VISKORES_TEST_ASSERT(array.GetNumberOfValues() == 50, "Wrong number of values");

  for (viskores::Id index = 0; index < array.GetNumberOfValues(); index++)
  {
    VISKORES_TEST_ASSERT(
      test_equal(array.ReadPortal().Get(index), TestValue(index) * factor),
      "Bad data value.");
  }
}

////
//// BEGIN-EXAMPLE ArrayHandleParameterTemplate
////
template<typename T, typename Storage>
void Foo(const viskores::cont::ArrayHandle<T, Storage>& array)
{
  ////
  //// END-EXAMPLE ArrayHandleParameterTemplate
  ////
  (void)array;
}

////
//// BEGIN-EXAMPLE ArrayHandleFullTemplate
////
template<typename ArrayType>
void Bar(const ArrayType& array)
{
  VISKORES_IS_ARRAY_HANDLE(ArrayType);
  ////
  //// END-EXAMPLE ArrayHandleFullTemplate
  ////
  (void)array;
}

void BasicConstruction()
{
  ////
  //// BEGIN-EXAMPLE CreateArrayHandle
  ////
  viskores::cont::ArrayHandle<viskores::Float32> outputArray;
  ////
  //// END-EXAMPLE CreateArrayHandle
  ////

  ////
  //// BEGIN-EXAMPLE ArrayHandleStorageParameter
  ////
  viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>
    arrayHandle;
  ////
  //// END-EXAMPLE ArrayHandleStorageParameter
  ////

  Foo(outputArray);
  Bar(arrayHandle);
}

void ArrayHandleFromInitializerList()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleFromInitializerList
  ////
  auto fibonacciArray = viskores::cont::make_ArrayHandle({ 0, 1, 1, 2, 3, 5, 8, 13 });
  ////
  //// END-EXAMPLE ArrayHandleFromInitializerList
  ////

  VISKORES_TEST_ASSERT(fibonacciArray.GetNumberOfValues() == 8);
  auto portal = fibonacciArray.ReadPortal();
  VISKORES_TEST_ASSERT(test_equal(portal.Get(0), 0));
  VISKORES_TEST_ASSERT(test_equal(portal.Get(1), 1));
  VISKORES_TEST_ASSERT(test_equal(portal.Get(2), 1));
  VISKORES_TEST_ASSERT(test_equal(portal.Get(3), 2));
  VISKORES_TEST_ASSERT(test_equal(portal.Get(4), 3));
  VISKORES_TEST_ASSERT(test_equal(portal.Get(5), 5));
  VISKORES_TEST_ASSERT(test_equal(portal.Get(6), 8));
  VISKORES_TEST_ASSERT(test_equal(portal.Get(7), 13));

  ////
  //// BEGIN-EXAMPLE ArrayHandleFromInitializerListTyped
  ////
  viskores::cont::ArrayHandle<viskores::FloatDefault> inputArray =
    viskores::cont::make_ArrayHandle<viskores::FloatDefault>(
      { 1.4142f, 2.7183f, 3.1416f });
  ////
  //// END-EXAMPLE ArrayHandleFromInitializerListTyped
  ////

  VISKORES_TEST_ASSERT(inputArray.GetNumberOfValues() == 3);
  auto portal2 = inputArray.ReadPortal();
  VISKORES_TEST_ASSERT(test_equal(portal2.Get(0), 1.4142));
  VISKORES_TEST_ASSERT(test_equal(portal2.Get(1), 2.7183));
  VISKORES_TEST_ASSERT(test_equal(portal2.Get(2), 3.1416));
}

void ArrayHandleFromCArray()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleFromCArray
  ////
  viskores::Float32 dataBuffer[50];
  // Populate dataBuffer with meaningful data. Perhaps read data from a file.
  //// PAUSE-EXAMPLE
  for (viskores::Id index = 0; index < 50; index++)
  {
    dataBuffer[index] = TestValue(index);
  }
  //// RESUME-EXAMPLE

  viskores::cont::ArrayHandle<viskores::Float32> inputArray =
    viskores::cont::make_ArrayHandle(dataBuffer, 50, viskores::CopyFlag::On);
  ////
  //// END-EXAMPLE ArrayHandleFromCArray
  ////

  CheckArrayValues(inputArray);
}

viskores::Float32 GetValueForArray(viskores::Id index)
{
  return TestValue(index);
}

void AllocateAndFillArrayHandle()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandlePopulate
  ////
  ////
  //// BEGIN-EXAMPLE ArrayHandleAllocate
  ////
  viskores::cont::ArrayHandle<viskores::Float32> arrayHandle;

  const viskores::Id ARRAY_SIZE = 50;
  arrayHandle.Allocate(ARRAY_SIZE);
  ////
  //// END-EXAMPLE ArrayHandleAllocate
  ////

  // Usually it is easier to just use the auto keyword.
  using PortalType = viskores::cont::ArrayHandle<viskores::Float32>::WritePortalType;
  PortalType portal = arrayHandle.WritePortal();

  for (viskores::Id index = 0; index < portal.GetNumberOfValues(); index++)
  {
    portal.Set(index, GetValueForArray(index));
  }
  ////
  //// END-EXAMPLE ArrayHandlePopulate
  ////

  CheckArrayValues(arrayHandle);

  {
    viskores::cont::ArrayHandle<viskores::Float32> srcArray = arrayHandle;
    viskores::cont::ArrayHandle<viskores::Float32> destArray;
    ////
    //// BEGIN-EXAMPLE ArrayHandleDeepCopy
    ////
    destArray.DeepCopyFrom(srcArray);
    ////
    //// END-EXAMPLE ArrayHandleDeepCopy
    ////
    VISKORES_TEST_ASSERT(srcArray != destArray);
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(srcArray, destArray));
  }

  ////
  //// BEGIN-EXAMPLE ArrayRangeCompute
  ////
  viskores::cont::ArrayHandle<viskores::Range> rangeArray =
    viskores::cont::ArrayRangeCompute(arrayHandle);
  auto rangePortal = rangeArray.ReadPortal();
  for (viskores::Id index = 0; index < rangePortal.GetNumberOfValues(); ++index)
  {
    viskores::Range componentRange = rangePortal.Get(index);
    std::cout << "Values for component " << index << " go from " << componentRange.Min
              << " to " << componentRange.Max << std::endl;
  }
  ////
  //// END-EXAMPLE ArrayRangeCompute
  ////

  viskores::Range range = rangePortal.Get(0);
  VISKORES_TEST_ASSERT(test_equal(range.Min, TestValue(0)), "Bad min value.");
  VISKORES_TEST_ASSERT(test_equal(range.Max, TestValue(ARRAY_SIZE - 1)),
                       "Bad max value.");

  ////
  //// BEGIN-EXAMPLE ArrayHandleReallocate
  ////
  // Add space for 10 more values at the end of the array.
  arrayHandle.Allocate(arrayHandle.GetNumberOfValues() + 10, viskores::CopyFlag::On);
  ////
  //// END-EXAMPLE ArrayHandleReallocate
  ////
}

////
//// BEGIN-EXAMPLE ArrayOutOfScope
////
VISKORES_CONT viskores::cont::ArrayHandle<viskores::Float32> BadDataLoad()
{
  std::vector<viskores::Float32> dataBuffer;
  // Populate dataBuffer with meaningful data. Perhaps read data from a file.
  //// PAUSE-EXAMPLE
  dataBuffer.resize(50);
  for (std::size_t index = 0; index < 50; index++)
  {
    dataBuffer[index] = TestValue(index);
  }
  //// RESUME-EXAMPLE

  viskores::cont::ArrayHandle<viskores::Float32> inputArray =
    viskores::cont::make_ArrayHandle(dataBuffer, viskores::CopyFlag::Off);
  //// PAUSE-EXAMPLE
  CheckArrayValues(inputArray);
  //// RESUME-EXAMPLE

  return inputArray;
  // THIS IS WRONG! At this point dataBuffer goes out of scope and deletes its
  // memory. However, inputArray has a pointer to that memory, which becomes an
  // invalid pointer in the returned object. Bad things will happen when the
  // ArrayHandle is used.
}

VISKORES_CONT viskores::cont::ArrayHandle<viskores::Float32> SafeDataLoad1()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleFromVector
  ////
  std::vector<viskores::Float32> dataBuffer;
  // Populate dataBuffer with meaningful data. Perhaps read data from a file.
  //// PAUSE-EXAMPLE
  dataBuffer.resize(50);
  for (std::size_t index = 0; index < 50; index++)
  {
    dataBuffer[index] = TestValue(index);
  }
  //// RESUME-EXAMPLE

  viskores::cont::ArrayHandle<viskores::Float32> inputArray =
    //// LABEL CopyFlagOn
    viskores::cont::make_ArrayHandle(dataBuffer, viskores::CopyFlag::On);
  ////
  //// END-EXAMPLE ArrayHandleFromVector
  ////

  return inputArray;
  // This is safe.
}

VISKORES_CONT viskores::cont::ArrayHandle<viskores::Float32> SafeDataLoad2()
{
  std::vector<viskores::Float32> dataBuffer;
  // Populate dataBuffer with meaningful data. Perhaps read data from a file.
  //// PAUSE-EXAMPLE
  dataBuffer.resize(50);
  for (std::size_t index = 0; index < 50; index++)
  {
    dataBuffer[index] = TestValue(index);
  }
  //// RESUME-EXAMPLE

  viskores::cont::ArrayHandle<viskores::Float32> inputArray =
    //// LABEL MoveVector
    viskores::cont::make_ArrayHandleMove(std::move(dataBuffer));

  return inputArray;
  // This is safe.
}
////
//// END-EXAMPLE ArrayOutOfScope
////

void ArrayHandleFromVector()
{
  BadDataLoad();
}

void CheckSafeDataLoad()
{
  viskores::cont::ArrayHandle<viskores::Float32> inputArray1 = SafeDataLoad1();
  CheckArrayValues(inputArray1);

  viskores::cont::ArrayHandle<viskores::Float32> inputArray2 = SafeDataLoad2();
  CheckArrayValues(inputArray2);
}

////
//// BEGIN-EXAMPLE SimpleArrayPortal
////
template<typename T>
class SimpleScalarArrayPortal
{
public:
  using ValueType = T;

  // There is no specification for creating array portals, but they generally
  // need a constructor like this to be practical.
  VISKORES_EXEC_CONT
  SimpleScalarArrayPortal(ValueType* array, viskores::Id numberOfValues)
    : Array(array)
    , NumberOfValues(numberOfValues)
  {
  }

  VISKORES_EXEC_CONT
  SimpleScalarArrayPortal()
    : Array(NULL)
    , NumberOfValues(0)
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const { return this->Array[index]; }

  VISKORES_EXEC_CONT
  void Set(viskores::Id index, ValueType value) const { this->Array[index] = value; }

private:
  ValueType* Array;
  viskores::Id NumberOfValues;
};
////
//// END-EXAMPLE SimpleArrayPortal
////

////
//// BEGIN-EXAMPLE ArrayPortalToIterators
////
template<typename PortalType>
VISKORES_CONT std::vector<typename PortalType::ValueType> CopyArrayPortalToVector(
  const PortalType& portal)
{
  using ValueType = typename PortalType::ValueType;
  std::vector<ValueType> result(static_cast<std::size_t>(portal.GetNumberOfValues()));

  viskores::cont::ArrayPortalToIterators<PortalType> iterators(portal);

  std::copy(iterators.GetBegin(), iterators.GetEnd(), result.begin());

  return result;
}
////
//// END-EXAMPLE ArrayPortalToIterators
////

void TestArrayPortalToken()
{
  ////
  //// BEGIN-EXAMPLE ArrayPortalToken
  ////
  viskores::cont::ArrayHandle<viskores::FloatDefault> arrayHandle;
  // Fill array with interesting stuff...
  //// PAUSE-EXAMPLE
  arrayHandle.Allocate(10);
  SetPortal(arrayHandle.WritePortal());
  //// RESUME-EXAMPLE

  std::vector<viskores::FloatDefault> externalData;
  externalData.reserve(static_cast<std::size_t>(arrayHandle.GetNumberOfValues()));
  {
    viskores::cont::Token token;
    auto arrayPortal = arrayHandle.ReadPortal(token);
    // token is attached to arrayHandle. arrayHandle cannot invalidate arrayPortal
    // while token exists.

    for (viskores::Id index = 0; index < arrayPortal.GetNumberOfValues(); ++index)
    {
      externalData.push_back(arrayPortal.Get(index));
    }

    // Error! This will block because of token and therefore cause a deadlock!
    //arrayHandle.ReleaseResources();
  }
  // Token is destroyed. We can delete the array.
  arrayHandle.ReleaseResources();
  ////
  //// END-EXAMPLE ArrayPortalToken
  ////
}

void TestArrayPortalVectors()
{
  viskores::cont::ArrayHandle<viskores::Float32> inputArray = SafeDataLoad1();
  std::vector<viskores::Float32> buffer =
    CopyArrayPortalToVector(inputArray.ReadPortal());

  VISKORES_TEST_ASSERT(static_cast<viskores::Id>(buffer.size()) ==
                         inputArray.GetNumberOfValues(),
                       "Vector was sized wrong.");

  for (viskores::Id index = 0; index < inputArray.GetNumberOfValues(); index++)
  {
    VISKORES_TEST_ASSERT(
      test_equal(buffer[static_cast<std::size_t>(index)], TestValue(index)),
      "Bad data value.");
  }

  SimpleScalarArrayPortal<viskores::Float32> portal(
    &buffer.at(0), static_cast<viskores::Id>(buffer.size()));

  ////
  //// BEGIN-EXAMPLE ArrayPortalToIteratorBeginEnd
  ////
  std::vector<viskores::Float32> myContainer(
    static_cast<std::size_t>(portal.GetNumberOfValues()));

  std::copy(viskores::cont::ArrayPortalToIteratorBegin(portal),
            viskores::cont::ArrayPortalToIteratorEnd(portal),
            myContainer.begin());
  ////
  //// END-EXAMPLE ArrayPortalToIteratorBeginEnd
  ////

  for (viskores::Id index = 0; index < inputArray.GetNumberOfValues(); index++)
  {
    VISKORES_TEST_ASSERT(
      test_equal(myContainer[static_cast<std::size_t>(index)], TestValue(index)),
      "Bad data value.");
  }
}

////
//// BEGIN-EXAMPLE ControlPortals
////
template<typename T, typename Storage>
void SortCheckArrayHandle(viskores::cont::ArrayHandle<T, Storage> arrayHandle)
{
  using WritePortalType =
    typename viskores::cont::ArrayHandle<T, Storage>::WritePortalType;
  using ReadPortalType =
    typename viskores::cont::ArrayHandle<T, Storage>::ReadPortalType;

  WritePortalType readwritePortal = arrayHandle.WritePortal();
  // This is actually pretty dumb. Sorting would be generally faster in
  // parallel in the execution environment using the device adapter algorithms.
  std::sort(viskores::cont::ArrayPortalToIteratorBegin(readwritePortal),
            viskores::cont::ArrayPortalToIteratorEnd(readwritePortal));

  ReadPortalType readPortal = arrayHandle.ReadPortal();
  for (viskores::Id index = 1; index < readPortal.GetNumberOfValues(); index++)
  {
    if (readPortal.Get(index - 1) > readPortal.Get(index))
    {
      //// PAUSE-EXAMPLE
      VISKORES_TEST_FAIL("Sorting is wrong!");
      //// RESUME-EXAMPLE
      std::cout << "Sorting is wrong!" << std::endl;
      break;
    }
  }
}
////
//// END-EXAMPLE ControlPortals
////

void TestControlPortalsExample()
{
  SortCheckArrayHandle(SafeDataLoad2());
}

////
//// BEGIN-EXAMPLE ExecutionPortals
////
template<typename InputPortalType, typename OutputPortalType>
struct DoubleFunctor : public viskores::exec::FunctorBase
{
  InputPortalType InputPortal;
  OutputPortalType OutputPortal;

  VISKORES_CONT
  DoubleFunctor(InputPortalType inputPortal, OutputPortalType outputPortal)
    : InputPortal(inputPortal)
    , OutputPortal(outputPortal)
  {
  }

  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    this->OutputPortal.Set(index, 2 * this->InputPortal.Get(index));
  }
};

template<typename T, typename Device>
void DoubleArray(viskores::cont::ArrayHandle<T> inputArray,
                 viskores::cont::ArrayHandle<T> outputArray,
                 Device)
{
  viskores::Id numValues = inputArray.GetNumberOfValues();

  viskores::cont::Token token;
  auto inputPortal = inputArray.PrepareForInput(Device{}, token);
  auto outputPortal = outputArray.PrepareForOutput(numValues, Device{}, token);
  // Token is now attached to inputPortal and outputPortal. Those two portals
  // are guaranteed to be valid until token goes out of scope at the end of
  // this function.

  DoubleFunctor<decltype(inputPortal), decltype(outputPortal)> functor(inputPortal,
                                                                       outputPortal);

  viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(functor, numValues);
}
////
//// END-EXAMPLE ExecutionPortals
////

void TestExecutionPortalsExample()
{
  viskores::cont::ArrayHandle<viskores::Float32> inputArray = SafeDataLoad1();
  CheckArrayValues(inputArray);
  viskores::cont::ArrayHandle<viskores::Float32> outputArray;
  DoubleArray(inputArray, outputArray, viskores::cont::DeviceAdapterTagSerial());
  CheckArrayValues(outputArray, 2);
}

////
//// BEGIN-EXAMPLE GetArrayPointer
////
void LegacyFunction(const int* data);

void UseArrayWithLegacy(const viskores::cont::ArrayHandle<viskores::Int32> array)
{
  viskores::cont::ArrayHandleBasic<viskores::Int32> basicArray = array;
  viskores::cont::Token token; // Token prevents array from changing while in scope.
  const int* cArray = basicArray.GetReadPointer(token);
  LegacyFunction(cArray);
  // When function returns, token goes out of scope and array can be modified.
}
////
//// END-EXAMPLE GetArrayPointer
////

void LegacyFunction(const int* data)
{
  std::cout << "Got data: " << data[0] << std::endl;
}

void TryUseArrayWithLegacy()
{
  viskores::cont::ArrayHandle<viskores::Int32> array;
  array.Allocate(50);
  SetPortal(array.WritePortal());
  UseArrayWithLegacy(array);
}

void ArrayHandleFromComponents()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleSOAFromComponentArrays
  ////
  viskores::cont::ArrayHandle<viskores::FloatDefault> component1;
  viskores::cont::ArrayHandle<viskores::FloatDefault> component2;
  viskores::cont::ArrayHandle<viskores::FloatDefault> component3;
  // Fill component arrays...
  //// PAUSE-EXAMPLE
  component1.AllocateAndFill(50, 0);
  component2.AllocateAndFill(50, 1);
  component3.AllocateAndFill(50, 2);
  //// RESUME-EXAMPLE

  viskores::cont::ArrayHandleSOA<viskores::Vec3f> soaArray =
    viskores::cont::make_ArrayHandleSOA(component1, component2, component3);
  ////
  //// END-EXAMPLE ArrayHandleSOAFromComponentArrays
  ////

  auto portal = soaArray.ReadPortal();
  for (viskores::Id index = 0; index < portal.GetNumberOfValues(); ++index)
  {
    VISKORES_TEST_ASSERT(portal.Get(index) == viskores::Vec3f{ 0, 1, 2 });
  }
}

void Test()
{
  BasicConstruction();
  ArrayHandleFromInitializerList();
  ArrayHandleFromCArray();
  ArrayHandleFromVector();
  AllocateAndFillArrayHandle();
  CheckSafeDataLoad();
  TestArrayPortalToken();
  TestArrayPortalVectors();
  TestControlPortalsExample();
  TestExecutionPortalsExample();
  TryUseArrayWithLegacy();
  ArrayHandleFromComponents();
}

} // anonymous namespace

int GuideExampleArrayHandle(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
