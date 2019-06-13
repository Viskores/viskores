//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/MaskSelect.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace
{

constexpr vtkm::Id nullValue = -2;

struct TestMaskArrays
{
  vtkm::cont::ArrayHandle<vtkm::IdComponent> SelectArray;
  vtkm::cont::ArrayHandle<vtkm::Id> ThreadToOutputMap;
};

TestMaskArrays MakeMaskArraysShort()
{
  const vtkm::Id selectArraySize = 18;
  const vtkm::IdComponent selectArray[selectArraySize] = { 1, 1, 0, 0, 0, 0, 1, 0, 0,
                                                           0, 0, 0, 0, 0, 0, 0, 0, 1 };
  const vtkm::Id threadRange = 4;
  const vtkm::Id threadToOutputMap[threadRange] = { 0, 1, 6, 17 };

  TestMaskArrays arrays;

  // Need to copy arrays so that the data does not go out of scope.
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandle(selectArray, selectArraySize),
                        arrays.SelectArray);
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandle(threadToOutputMap, threadRange),
                        arrays.ThreadToOutputMap);

  return arrays;
}

TestMaskArrays MakeMaskArraysLong()
{
  const vtkm::Id selectArraySize = 16;
  const vtkm::IdComponent selectArray[selectArraySize] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
  };
  const vtkm::Id threadRange = 15;
  const vtkm::Id threadToOutputMap[threadRange] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15
  };

  TestMaskArrays arrays;

  // Need to copy arrays so that the data does not go out of scope.
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandle(selectArray, selectArraySize),
                        arrays.SelectArray);
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandle(threadToOutputMap, threadRange),
                        arrays.ThreadToOutputMap);

  return arrays;
}

TestMaskArrays MakeMaskArraysZero()
{
  const vtkm::Id selectArraySize = 6;
  const vtkm::IdComponent selectArray[selectArraySize] = { 0, 0, 0, 0, 0, 0 };

  TestMaskArrays arrays;

  // Need to copy arrays so that the data does not go out of scope.
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandle(selectArray, selectArraySize),
                        arrays.SelectArray);
  arrays.ThreadToOutputMap.Allocate(0);

  return arrays;
}

struct TestMaskSelectWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inputIndices, FieldInOut copyIndices);
  using ExecutionSignature = void(_1, _2);

  using MaskType = vtkm::worklet::MaskSelect;

  VTKM_EXEC
  void operator()(vtkm::Id inputIndex, vtkm::Id& indexCopy) const { indexCopy = inputIndex; }
};

template <typename T, typename SelectArrayType>
void CompareArrays(vtkm::cont::ArrayHandle<T> array1,
                   vtkm::cont::ArrayHandle<T> array2,
                   const SelectArrayType& selectArray)
{
  VTKM_IS_ARRAY_HANDLE(SelectArrayType);

  auto portal1 = array1.GetPortalConstControl();
  auto portal2 = array2.GetPortalConstControl();
  auto selectPortal = selectArray.GetPortalConstControl();

  VTKM_TEST_ASSERT(portal1.GetNumberOfValues() == portal2.GetNumberOfValues());
  VTKM_TEST_ASSERT(portal1.GetNumberOfValues() == selectArray.GetNumberOfValues());

  for (vtkm::Id index = 0; index < portal1.GetNumberOfValues(); index++)
  {
    if (selectPortal.Get(index))
    {
      T value1 = portal1.Get(index);
      T value2 = portal2.Get(index);
      VTKM_TEST_ASSERT(
        value1 == value2, "Array values not equal (", index, ": ", value1, " ", value2, ")");
    }
    else
    {
      T value = portal2.Get(index);
      VTKM_TEST_ASSERT(value == nullValue, "Expected null value, got ", value);
    }
  }
}

template <typename T>
void CompareArrays(vtkm::cont::ArrayHandle<T> array1, vtkm::cont::ArrayHandle<T> array2)
{
  CompareArrays(
    array1, array2, vtkm::cont::make_ArrayHandleConstant<bool>(true, array1.GetNumberOfValues()));
}

// This unit test makes sure the ScatterCounting generates the correct map
// and visit arrays.
void TestMaskArrayGeneration(const TestMaskArrays& arrays)
{
  std::cout << "  Testing array generation" << std::endl;

  vtkm::worklet::MaskSelect mask(arrays.SelectArray, vtkm::cont::DeviceAdapterTagAny());

  vtkm::Id inputSize = arrays.SelectArray.GetNumberOfValues();

  std::cout << "    Checking thread to output map ";
  vtkm::cont::printSummary_ArrayHandle(mask.GetThreadToOutputMap(inputSize), std::cout);
  CompareArrays(arrays.ThreadToOutputMap, mask.GetThreadToOutputMap(inputSize));
}

// This is more of an integration test that makes sure the scatter works with a
// worklet invocation.
void TestMaskWorklet(const TestMaskArrays& arrays)
{
  std::cout << "  Testing mask select in a worklet." << std::endl;

  vtkm::worklet::DispatcherMapField<TestMaskSelectWorklet> dispatcher(
    vtkm::worklet::MaskSelect(arrays.SelectArray));

  vtkm::Id inputSize = arrays.SelectArray.GetNumberOfValues();

  vtkm::cont::ArrayHandle<vtkm::Id> inputIndices;
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(inputSize), inputIndices);

  vtkm::cont::ArrayHandle<vtkm::Id> selectIndexCopy;
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(nullValue, inputSize),
                        selectIndexCopy);

  std::cout << "    Invoke worklet" << std::endl;
  dispatcher.Invoke(inputIndices, selectIndexCopy);

  std::cout << "    Check copied indices." << std::endl;
  CompareArrays(inputIndices, selectIndexCopy, arrays.SelectArray);
}

void TestMaskSelectWithArrays(const TestMaskArrays& arrays)
{
  TestMaskArrayGeneration(arrays);
  TestMaskWorklet(arrays);
}

void TestMaskSelect()
{
  std::cout << "Testing arrays with output smaller than input." << std::endl;
  TestMaskSelectWithArrays(MakeMaskArraysShort());

  std::cout << "Testing arrays with output larger than input." << std::endl;
  TestMaskSelectWithArrays(MakeMaskArraysLong());

  std::cout << "Testing arrays with zero output." << std::endl;
  TestMaskSelectWithArrays(MakeMaskArraysZero());
}

} // anonymous namespace

int UnitTestMaskSelect(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMaskSelect, argc, argv);
}
