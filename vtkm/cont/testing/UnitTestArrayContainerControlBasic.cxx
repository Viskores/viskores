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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#define VTKM_ARRAY_CONTAINER_CONTROL VTKM_ARRAY_CONTAINER_CONTROL_ERROR

#include <vtkm/cont/ArrayContainerControlBasic.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/VectorTraits.h>

namespace
{

const vtkm::Id ARRAY_SIZE = 10;

template <typename T>
struct TemplatedTests
{
  typedef vtkm::cont::internal::ArrayContainerControl<
      T, vtkm::cont::ArrayContainerControlTagBasic> ArrayContainerType;
  typedef typename ArrayContainerType::ValueType ValueType;
  typedef typename ArrayContainerType::PortalType PortalType;
  typedef typename PortalType::IteratorType IteratorType;

  void SetContainer(ArrayContainerType &array, ValueType value)
  {
    for (IteratorType iter = array.GetPortal().GetIteratorBegin();
         iter != array.GetPortal().GetIteratorEnd();
         iter ++)
      {
      *iter = value;
      }
  }

  bool CheckContainer(ArrayContainerType &array, ValueType value)
  {
    for (IteratorType iter = array.GetPortal().GetIteratorBegin();
         iter != array.GetPortal().GetIteratorEnd();
         iter ++)
      {
      if (!test_equal(*iter, value)) return false;
      }
    return true;
  }

  typename vtkm::VectorTraits<ValueType>::ComponentType STOLEN_ARRAY_VALUE() {
    return 4529;
  }

  /// Returned value should later be passed to StealArray2.  It is best to
  /// put as much between the two test parts to maximize the chance of a
  /// deallocated array being overridden (and thus detected).
  ValueType *StealArray1()
  {
    ValueType *stolenArray;

    ValueType stolenArrayValue = ValueType(STOLEN_ARRAY_VALUE());

    ArrayContainerType stealMyArray;
    stealMyArray.Allocate(ARRAY_SIZE);
    SetContainer(stealMyArray, stolenArrayValue);

    VTKM_TEST_ASSERT(stealMyArray.GetNumberOfValues() == ARRAY_SIZE,
                    "Array not properly allocated.");
    // This call steals the array and prevents deallocation.
    stolenArray = stealMyArray.StealArray();
    VTKM_TEST_ASSERT(stealMyArray.GetNumberOfValues() == 0,
                    "StealArray did not let go of array.");

    return stolenArray;
  }
  void StealArray2(ValueType *stolenArray)
  {
    ValueType stolenArrayValue = ValueType(STOLEN_ARRAY_VALUE());

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
      {
      VTKM_TEST_ASSERT(test_equal(stolenArray[index], stolenArrayValue),
                      "Stolen array did not retain values.");
      }
    delete[] stolenArray;
  }

  void BasicAllocation()
  {
    ArrayContainerType arrayContainer;
    VTKM_TEST_ASSERT(arrayContainer.GetNumberOfValues() == 0,
                    "New array container not zero sized.");

    arrayContainer.Allocate(ARRAY_SIZE);
    VTKM_TEST_ASSERT(arrayContainer.GetNumberOfValues() == ARRAY_SIZE,
                    "Array not properly allocated.");

    const ValueType BASIC_ALLOC_VALUE = ValueType(548);
    SetContainer(arrayContainer, BASIC_ALLOC_VALUE);
    VTKM_TEST_ASSERT(CheckContainer(arrayContainer, BASIC_ALLOC_VALUE),
                    "Array not holding value.");

    arrayContainer.Allocate(ARRAY_SIZE * 2);
    VTKM_TEST_ASSERT(arrayContainer.GetNumberOfValues() == ARRAY_SIZE * 2,
                    "Array not reallocated correctly.");

    arrayContainer.Shrink(ARRAY_SIZE);
    VTKM_TEST_ASSERT(arrayContainer.GetNumberOfValues() == ARRAY_SIZE,
                    "Array Shrnk failed to resize.");

    arrayContainer.ReleaseResources();
    VTKM_TEST_ASSERT(arrayContainer.GetNumberOfValues() == 0,
                    "Array not released correctly.");

    try
      {
      arrayContainer.Shrink(ARRAY_SIZE);
      VTKM_TEST_ASSERT(true==false,
                      "Array shrink do a larger size was possible. This can't be allowed.");
      }
    catch(vtkm::cont::ErrorControlBadValue){}
  }

  void operator()()
  {
    ValueType *stolenArray = StealArray1();

    BasicAllocation();

    StealArray2(stolenArray);
  }
};

struct TestFunctor
{
  template <typename T>
  void operator()(T)
  {
    TemplatedTests<T> tests;
    tests();

  }
};

void TestArrayContainerControlBasic()
{
  vtkm::testing::Testing::TryAllTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestArrayContainerControlBasic(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayContainerControlBasic);
}
