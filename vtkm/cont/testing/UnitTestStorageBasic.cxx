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

#define VTKM_STORAGE VTKM_STORAGE_ERROR

#include <vtkm/cont/StorageBasic.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/VecTraits.h>

namespace {

const vtkm::Id ARRAY_SIZE = 10;

template <typename T>
struct TemplatedTests
{
  typedef vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>
      StorageType;
  typedef typename StorageType::ValueType ValueType;
  typedef typename StorageType::PortalType PortalType;

  void SetStorage(StorageType &array, const ValueType& value)
  {
    PortalType portal = array.GetPortal();
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      portal.Set(index, value);
    }
  }

  bool CheckStorage(StorageType &array, const ValueType& value)
  {
    PortalType portal = array.GetPortal();
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      if (!test_equal(portal.Get(index), value)) { return false; }
    }
    return true;
  }

  typename vtkm::VecTraits<ValueType>::ComponentType STOLEN_ARRAY_VALUE()
  {
    return 4529;
  }

  /// Returned value should later be passed to StealArray2.  It is best to
  /// put as much between the two test parts to maximize the chance of a
  /// deallocated array being overridden (and thus detected).
  ValueType *StealArray1()
  {
    ValueType *stolenArray;

    ValueType stolenArrayValue = ValueType(STOLEN_ARRAY_VALUE());

    StorageType stealMyArray;
    stealMyArray.Allocate(ARRAY_SIZE);
    SetStorage(stealMyArray, stolenArrayValue);

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
    StorageType arrayStorage;
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == 0,
                     "New array storage not zero sized.");

    arrayStorage.Allocate(ARRAY_SIZE);
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE,
                     "Array not properly allocated.");

    const ValueType BASIC_ALLOC_VALUE = ValueType(548);
    SetStorage(arrayStorage, BASIC_ALLOC_VALUE);
    VTKM_TEST_ASSERT(CheckStorage(arrayStorage, BASIC_ALLOC_VALUE),
                     "Array not holding value.");

    arrayStorage.Allocate(ARRAY_SIZE * 2);
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE * 2,
                     "Array not reallocated correctly.");

    arrayStorage.Shrink(ARRAY_SIZE);
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE,
                     "Array Shrnk failed to resize.");

    arrayStorage.ReleaseResources();
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == 0,
                     "Array not released correctly.");

    try
    {
      arrayStorage.Shrink(ARRAY_SIZE);
      VTKM_TEST_ASSERT(true==false,
                       "Array shrink do a larger size was possible. This can't be allowed.");
    }
    catch(vtkm::cont::ErrorControlBadValue) {}
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

void TestStorageBasic()
{
  vtkm::testing::Testing::TryAllTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestStorageBasic(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestStorageBasic);
}
