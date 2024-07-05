//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template<typename ArrayHandleType>
void CheckArray(ArrayHandleType array)
{
  vtkm::cont::printSummary_ArrayHandle(array, std::cout);
  std::cout << std::endl;
  typename ArrayHandleType::ReadPortalType portal = array.ReadPortal();

  vtkm::Id expectedValue = 0;
  for (vtkm::Id vecIndex = 0; vecIndex < portal.GetNumberOfValues(); ++vecIndex)
  {
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < portal.Get(vecIndex).GetNumberOfComponents();
         componentIndex++)
    {
      VTKM_TEST_ASSERT(portal.Get(vecIndex)[componentIndex] == expectedValue,
                       "Got bad value.");
      ++expectedValue;
    }
  }
}

void ArrayHandleGroupVecBasic()
{
  std::cout << "ArrayHandleGroupVec" << std::endl;

  ////
  //// BEGIN-EXAMPLE ArrayHandleGroupVecBasic
  ////
  // Create an array containing [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]
  using ArrayType = vtkm::cont::ArrayHandleIndex;
  ArrayType sourceArray(12);

  // Create an array containing [(0,1), (2,3), (4,5), (6,7), (8,9), (10,11)]
  vtkm::cont::ArrayHandleGroupVec<ArrayType, 2> vec2Array(sourceArray);

  // Create an array containing [(0,1,2), (3,4,5), (6,7,8), (9,10,11)]
  vtkm::cont::ArrayHandleGroupVec<ArrayType, 3> vec3Array(sourceArray);
  ////
  //// END-EXAMPLE ArrayHandleGroupVecBasic
  ////
  CheckArray(vec2Array);
  vtkm::cont::printSummary_ArrayHandle(vec3Array, std::cout);
  std::cout << std::endl;
  CheckArray(vec3Array);

  CheckArray(
    ////
    //// BEGIN-EXAMPLE MakeArrayHandleGroupVec
    ////
    // Create an array containing [(0,1,2,3), (4,5,6,7), (8,9,10,11)]
    vtkm::cont::make_ArrayHandleGroupVec<4>(sourceArray)
    ////
    //// END-EXAMPLE MakeArrayHandleGroupVec
    ////
  );
}

void ArrayHandleGroupVecVariable()
{
  std::cout << "ArrayHandleGroupVecVariable" << std::endl;

  ////
  //// BEGIN-EXAMPLE ArrayHandleGroupVecVariable
  ////
  // Create an array of counts containing [4, 2, 3, 3]
  vtkm::cont::ArrayHandle<vtkm::IdComponent> countArray =
    vtkm::cont::make_ArrayHandle<vtkm::IdComponent>({ 4, 2, 3, 3 });

  // Convert the count array to an offset array [0, 4, 6, 9, 12]
  // Returns the number of total components: 12
  vtkm::Id sourceArraySize;
  using OffsetArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
  OffsetArrayType offsetArray =
    vtkm::cont::ConvertNumComponentsToOffsets(countArray, sourceArraySize);
  //// PAUSE-EXAMPLE
  vtkm::cont::printSummary_ArrayHandle(offsetArray, std::cout);
  std::cout << std::endl;
  VTKM_TEST_ASSERT(sourceArraySize == 12, "Bad source array size");
  VTKM_TEST_ASSERT(offsetArray.GetNumberOfValues() == 5);
  VTKM_TEST_ASSERT(offsetArray.ReadPortal().Get(0) == 0, "Unexpected offset value");
  VTKM_TEST_ASSERT(offsetArray.ReadPortal().Get(1) == 4, "Unexpected offset value");
  VTKM_TEST_ASSERT(offsetArray.ReadPortal().Get(2) == 6, "Unexpected offset value");
  VTKM_TEST_ASSERT(offsetArray.ReadPortal().Get(3) == 9, "Unexpected offset value");
  VTKM_TEST_ASSERT(offsetArray.ReadPortal().Get(4) == 12, "Unexpected offset value");
  //// RESUME-EXAMPLE

  // Create an array containing [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]
  using SourceArrayType = vtkm::cont::ArrayHandleIndex;
  SourceArrayType sourceArray(sourceArraySize);

  // Create an array containing [(0,1,2,3), (4,5), (6,7,8), (9,10,11)]
  vtkm::cont::ArrayHandleGroupVecVariable<SourceArrayType, OffsetArrayType>
    vecVariableArray(sourceArray, offsetArray);
  ////
  //// END-EXAMPLE ArrayHandleGroupVecVariable
  ////
  CheckArray(vecVariableArray);

  CheckArray(
    ////
    //// BEGIN-EXAMPLE MakeArrayHandleGroupVecVariable
    ////
    // Create an array containing [(0,1,2,3), (4,5), (6,7,8), (9,10,11)]
    vtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetArray)
    ////
    //// END-EXAMPLE MakeArrayHandleGroupVecVariable
    ////
  );
}

void Test()
{
  ArrayHandleGroupVecBasic();
  ArrayHandleGroupVecVariable();
}

} // anonymous namespace

int GuideExampleArrayHandleGroupVec(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
