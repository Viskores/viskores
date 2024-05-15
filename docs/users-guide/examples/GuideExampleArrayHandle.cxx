//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ArrayRangeCompute.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{

template<typename T>
vtkm::Float32 TestValue(T index)
{
  return static_cast<vtkm::Float32>(1 + 0.001 * index);
}

void CheckArrayValues(const vtkm::cont::ArrayHandle<vtkm::Float32>& array,
                      vtkm::Float32 factor = 1)
{
  // So far all the examples are using 50 entries. Could change.
  VTKM_TEST_ASSERT(array.GetNumberOfValues() == 50, "Wrong number of values");

  for (vtkm::Id index = 0; index < array.GetNumberOfValues(); index++)
  {
    VTKM_TEST_ASSERT(
      test_equal(array.ReadPortal().Get(index), TestValue(index) * factor),
      "Bad data value.");
  }
}

////
//// BEGIN-EXAMPLE ArrayHandleParameterTemplate
////
template<typename T, typename Storage>
void Foo(const vtkm::cont::ArrayHandle<T, Storage>& array)
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
  VTKM_IS_ARRAY_HANDLE(ArrayType);
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
  vtkm::cont::ArrayHandle<vtkm::Float32> outputArray;
  ////
  //// END-EXAMPLE CreateArrayHandle
  ////

  ////
  //// BEGIN-EXAMPLE ArrayHandleStorageParameter
  ////
  vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic> arrayHandle;
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
  auto fibonacciArray = vtkm::cont::make_ArrayHandle({ 0, 1, 1, 2, 3, 5, 8, 13 });
  ////
  //// END-EXAMPLE ArrayHandleFromInitializerList
  ////

  VTKM_TEST_ASSERT(fibonacciArray.GetNumberOfValues() == 8);
  auto portal = fibonacciArray.ReadPortal();
  VTKM_TEST_ASSERT(test_equal(portal.Get(0), 0));
  VTKM_TEST_ASSERT(test_equal(portal.Get(1), 1));
  VTKM_TEST_ASSERT(test_equal(portal.Get(2), 1));
  VTKM_TEST_ASSERT(test_equal(portal.Get(3), 2));
  VTKM_TEST_ASSERT(test_equal(portal.Get(4), 3));
  VTKM_TEST_ASSERT(test_equal(portal.Get(5), 5));
  VTKM_TEST_ASSERT(test_equal(portal.Get(6), 8));
  VTKM_TEST_ASSERT(test_equal(portal.Get(7), 13));

  ////
  //// BEGIN-EXAMPLE ArrayHandleFromInitializerListTyped
  ////
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> inputArray =
    vtkm::cont::make_ArrayHandle<vtkm::FloatDefault>({ 1.4142f, 2.7183f, 3.1416f });
  ////
  //// END-EXAMPLE ArrayHandleFromInitializerListTyped
  ////

  VTKM_TEST_ASSERT(inputArray.GetNumberOfValues() == 3);
  auto portal2 = inputArray.ReadPortal();
  VTKM_TEST_ASSERT(test_equal(portal2.Get(0), 1.4142));
  VTKM_TEST_ASSERT(test_equal(portal2.Get(1), 2.7183));
  VTKM_TEST_ASSERT(test_equal(portal2.Get(2), 3.1416));
}

void ArrayHandleFromCArray()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleFromCArray
  ////
  vtkm::Float32 dataBuffer[50];
  // Populate dataBuffer with meaningful data. Perhaps read data from a file.
  //// PAUSE-EXAMPLE
  for (vtkm::Id index = 0; index < 50; index++)
  {
    dataBuffer[index] = TestValue(index);
  }
  //// RESUME-EXAMPLE

  vtkm::cont::ArrayHandle<vtkm::Float32> inputArray =
    vtkm::cont::make_ArrayHandle(dataBuffer, 50, vtkm::CopyFlag::On);
  ////
  //// END-EXAMPLE ArrayHandleFromCArray
  ////

  CheckArrayValues(inputArray);
}

vtkm::Float32 GetValueForArray(vtkm::Id index)
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
  vtkm::cont::ArrayHandle<vtkm::Float32> arrayHandle;

  const vtkm::Id ARRAY_SIZE = 50;
  arrayHandle.Allocate(ARRAY_SIZE);
  ////
  //// END-EXAMPLE ArrayHandleAllocate
  ////

  // Usually it is easier to just use the auto keyword.
  using PortalType = vtkm::cont::ArrayHandle<vtkm::Float32>::WritePortalType;
  PortalType portal = arrayHandle.WritePortal();

  for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
  {
    portal.Set(index, GetValueForArray(index));
  }
  ////
  //// END-EXAMPLE ArrayHandlePopulate
  ////

  CheckArrayValues(arrayHandle);

  {
    vtkm::cont::ArrayHandle<vtkm::Float32> srcArray = arrayHandle;
    vtkm::cont::ArrayHandle<vtkm::Float32> destArray;
    ////
    //// BEGIN-EXAMPLE ArrayHandleDeepCopy
    ////
    destArray.DeepCopyFrom(srcArray);
    ////
    //// END-EXAMPLE ArrayHandleDeepCopy
    ////
    VTKM_TEST_ASSERT(srcArray != destArray);
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(srcArray, destArray));
  }

  ////
  //// BEGIN-EXAMPLE ArrayRangeCompute
  ////
  vtkm::cont::ArrayHandle<vtkm::Range> rangeArray =
    vtkm::cont::ArrayRangeCompute(arrayHandle);
  auto rangePortal = rangeArray.ReadPortal();
  for (vtkm::Id index = 0; index < rangePortal.GetNumberOfValues(); ++index)
  {
    vtkm::Range componentRange = rangePortal.Get(index);
    std::cout << "Values for component " << index << " go from " << componentRange.Min
              << " to " << componentRange.Max << std::endl;
  }
  ////
  //// END-EXAMPLE ArrayRangeCompute
  ////

  vtkm::Range range = rangePortal.Get(0);
  VTKM_TEST_ASSERT(test_equal(range.Min, TestValue(0)), "Bad min value.");
  VTKM_TEST_ASSERT(test_equal(range.Max, TestValue(ARRAY_SIZE - 1)), "Bad max value.");

  ////
  //// BEGIN-EXAMPLE ArrayHandleReallocate
  ////
  // Add space for 10 more values at the end of the array.
  arrayHandle.Allocate(arrayHandle.GetNumberOfValues() + 10, vtkm::CopyFlag::On);
  ////
  //// END-EXAMPLE ArrayHandleReallocate
  ////
}

////
//// BEGIN-EXAMPLE ArrayOutOfScope
////
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Float32> BadDataLoad()
{
  std::vector<vtkm::Float32> dataBuffer;
  // Populate dataBuffer with meaningful data. Perhaps read data from a file.
  //// PAUSE-EXAMPLE
  dataBuffer.resize(50);
  for (std::size_t index = 0; index < 50; index++)
  {
    dataBuffer[index] = TestValue(index);
  }
  //// RESUME-EXAMPLE

  vtkm::cont::ArrayHandle<vtkm::Float32> inputArray =
    vtkm::cont::make_ArrayHandle(dataBuffer, vtkm::CopyFlag::Off);
  //// PAUSE-EXAMPLE
  CheckArrayValues(inputArray);
  //// RESUME-EXAMPLE

  return inputArray;
  // THIS IS WRONG! At this point dataBuffer goes out of scope and deletes its
  // memory. However, inputArray has a pointer to that memory, which becomes an
  // invalid pointer in the returned object. Bad things will happen when the
  // ArrayHandle is used.
}

VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Float32> SafeDataLoad1()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleFromVector
  ////
  std::vector<vtkm::Float32> dataBuffer;
  // Populate dataBuffer with meaningful data. Perhaps read data from a file.
  //// PAUSE-EXAMPLE
  dataBuffer.resize(50);
  for (std::size_t index = 0; index < 50; index++)
  {
    dataBuffer[index] = TestValue(index);
  }
  //// RESUME-EXAMPLE

  vtkm::cont::ArrayHandle<vtkm::Float32> inputArray =
    //// LABEL CopyFlagOn
    vtkm::cont::make_ArrayHandle(dataBuffer, vtkm::CopyFlag::On);
  ////
  //// END-EXAMPLE ArrayHandleFromVector
  ////

  return inputArray;
  // This is safe.
}

VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Float32> SafeDataLoad2()
{
  std::vector<vtkm::Float32> dataBuffer;
  // Populate dataBuffer with meaningful data. Perhaps read data from a file.
  //// PAUSE-EXAMPLE
  dataBuffer.resize(50);
  for (std::size_t index = 0; index < 50; index++)
  {
    dataBuffer[index] = TestValue(index);
  }
  //// RESUME-EXAMPLE

  vtkm::cont::ArrayHandle<vtkm::Float32> inputArray =
    //// LABEL MoveVector
    vtkm::cont::make_ArrayHandleMove(std::move(dataBuffer));

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
  vtkm::cont::ArrayHandle<vtkm::Float32> inputArray1 = SafeDataLoad1();
  CheckArrayValues(inputArray1);

  vtkm::cont::ArrayHandle<vtkm::Float32> inputArray2 = SafeDataLoad2();
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
  VTKM_EXEC_CONT
  SimpleScalarArrayPortal(ValueType* array, vtkm::Id numberOfValues)
    : Array(array)
    , NumberOfValues(numberOfValues)
  {
  }

  VTKM_EXEC_CONT
  SimpleScalarArrayPortal()
    : Array(NULL)
    , NumberOfValues(0)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return this->Array[index]; }

  VTKM_EXEC_CONT
  void Set(vtkm::Id index, ValueType value) const { this->Array[index] = value; }

private:
  ValueType* Array;
  vtkm::Id NumberOfValues;
};
////
//// END-EXAMPLE SimpleArrayPortal
////

////
//// BEGIN-EXAMPLE ArrayPortalToIterators
////
template<typename PortalType>
VTKM_CONT std::vector<typename PortalType::ValueType> CopyArrayPortalToVector(
  const PortalType& portal)
{
  using ValueType = typename PortalType::ValueType;
  std::vector<ValueType> result(static_cast<std::size_t>(portal.GetNumberOfValues()));

  vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);

  std::copy(iterators.GetBegin(), iterators.GetEnd(), result.begin());

  return result;
}
////
//// END-EXAMPLE ArrayPortalToIterators
////

void TestArrayPortalVectors()
{
  vtkm::cont::ArrayHandle<vtkm::Float32> inputArray = SafeDataLoad1();
  std::vector<vtkm::Float32> buffer = CopyArrayPortalToVector(inputArray.ReadPortal());

  VTKM_TEST_ASSERT(static_cast<vtkm::Id>(buffer.size()) ==
                     inputArray.GetNumberOfValues(),
                   "Vector was sized wrong.");

  for (vtkm::Id index = 0; index < inputArray.GetNumberOfValues(); index++)
  {
    VTKM_TEST_ASSERT(
      test_equal(buffer[static_cast<std::size_t>(index)], TestValue(index)),
      "Bad data value.");
  }

  SimpleScalarArrayPortal<vtkm::Float32> portal(&buffer.at(0),
                                                static_cast<vtkm::Id>(buffer.size()));

  ////
  //// BEGIN-EXAMPLE ArrayPortalToIteratorBeginEnd
  ////
  std::vector<vtkm::Float32> myContainer(
    static_cast<std::size_t>(portal.GetNumberOfValues()));

  std::copy(vtkm::cont::ArrayPortalToIteratorBegin(portal),
            vtkm::cont::ArrayPortalToIteratorEnd(portal),
            myContainer.begin());
  ////
  //// END-EXAMPLE ArrayPortalToIteratorBeginEnd
  ////

  for (vtkm::Id index = 0; index < inputArray.GetNumberOfValues(); index++)
  {
    VTKM_TEST_ASSERT(
      test_equal(myContainer[static_cast<std::size_t>(index)], TestValue(index)),
      "Bad data value.");
  }
}

////
//// BEGIN-EXAMPLE ControlPortals
////
template<typename T, typename Storage>
void SortCheckArrayHandle(vtkm::cont::ArrayHandle<T, Storage> arrayHandle)
{
  using WritePortalType = typename vtkm::cont::ArrayHandle<T, Storage>::WritePortalType;
  using ReadPortalType = typename vtkm::cont::ArrayHandle<T, Storage>::ReadPortalType;

  WritePortalType readwritePortal = arrayHandle.WritePortal();
  // This is actually pretty dumb. Sorting would be generally faster in
  // parallel in the execution environment using the device adapter algorithms.
  std::sort(vtkm::cont::ArrayPortalToIteratorBegin(readwritePortal),
            vtkm::cont::ArrayPortalToIteratorEnd(readwritePortal));

  ReadPortalType readPortal = arrayHandle.ReadPortal();
  for (vtkm::Id index = 1; index < readPortal.GetNumberOfValues(); index++)
  {
    if (readPortal.Get(index - 1) > readPortal.Get(index))
    {
      //// PAUSE-EXAMPLE
      VTKM_TEST_FAIL("Sorting is wrong!");
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
struct DoubleFunctor : public vtkm::exec::FunctorBase
{
  InputPortalType InputPortal;
  OutputPortalType OutputPortal;

  VTKM_CONT
  DoubleFunctor(InputPortalType inputPortal, OutputPortalType outputPortal)
    : InputPortal(inputPortal)
    , OutputPortal(outputPortal)
  {
  }

  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    this->OutputPortal.Set(index, 2 * this->InputPortal.Get(index));
  }
};

template<typename T, typename Device>
void DoubleArray(vtkm::cont::ArrayHandle<T> inputArray,
                 vtkm::cont::ArrayHandle<T> outputArray,
                 Device)
{
  vtkm::Id numValues = inputArray.GetNumberOfValues();

  vtkm::cont::Token token;
  auto inputPortal = inputArray.PrepareForInput(Device{}, token);
  auto outputPortal = outputArray.PrepareForOutput(numValues, Device{}, token);
  // Token is now attached to inputPortal and outputPortal. Those two portals
  // are guaranteed to be valid until token goes out of scope at the end of
  // this function.

  DoubleFunctor<decltype(inputPortal), decltype(outputPortal)> functor(inputPortal,
                                                                       outputPortal);

  vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(functor, numValues);
}
////
//// END-EXAMPLE ExecutionPortals
////

void TestExecutionPortalsExample()
{
  vtkm::cont::ArrayHandle<vtkm::Float32> inputArray = SafeDataLoad1();
  CheckArrayValues(inputArray);
  vtkm::cont::ArrayHandle<vtkm::Float32> outputArray;
  DoubleArray(inputArray, outputArray, vtkm::cont::DeviceAdapterTagSerial());
  CheckArrayValues(outputArray, 2);
}

////
//// BEGIN-EXAMPLE GetArrayPointer
////
void LegacyFunction(const int* data);

void UseArrayWithLegacy(const vtkm::cont::ArrayHandle<vtkm::Int32> array)
{
  vtkm::cont::ArrayHandleBasic<vtkm::Int32> basicArray = array;
  vtkm::cont::Token token; // Token prevents array from changing while in scope.
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
  vtkm::cont::ArrayHandle<vtkm::Int32> array;
  array.Allocate(50);
  SetPortal(array.WritePortal());
  UseArrayWithLegacy(array);
}

void ArrayHandleFromComponents()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleSOAFromComponentArrays
  ////
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> component1;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> component2;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> component3;
  // Fill component arrays...
  //// PAUSE-EXAMPLE
  component1.AllocateAndFill(50, 0);
  component2.AllocateAndFill(50, 1);
  component3.AllocateAndFill(50, 2);
  //// RESUME-EXAMPLE

  vtkm::cont::ArrayHandleSOA<vtkm::Vec3f> soaArray =
    vtkm::cont::make_ArrayHandleSOA(component1, component2, component3);
  ////
  //// END-EXAMPLE ArrayHandleSOAFromComponentArrays
  ////

  auto portal = soaArray.ReadPortal();
  for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); ++index)
  {
    VTKM_TEST_ASSERT(portal.Get(index) == vtkm::Vec3f{ 0, 1, 2 });
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
  TestArrayPortalVectors();
  TestControlPortalsExample();
  TestExecutionPortalsExample();
  TryUseArrayWithLegacy();
  ArrayHandleFromComponents();
}

} // anonymous namespace

int GuideExampleArrayHandle(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
