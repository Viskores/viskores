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

//This sets up the ArrayHandle semantics to allocate pointers and share memory
//between control and execution.
#define VTKM_STORAGE VTKM_STORAGE_BASIC
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/testing/Testing.h>

#include <algorithm>

namespace {
const vtkm::Id ARRAY_SIZE = 10;

vtkm::Scalar TestValue(vtkm::Id index)
{
  return static_cast<vtkm::Scalar>(10.0*index + 0.01*(index+1));
}

template<class IteratorType>
bool CheckIterators(IteratorType begin, IteratorType end)
{
  vtkm::Id index = 0;
  for (IteratorType iter = begin; iter != end; iter++)
  {
    if (!test_equal(*iter, TestValue(index))) { return false; }
    index++;
  }
  return true;
}

template<class PortalType>
bool CheckPortal(const PortalType &portal)
{
  vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
  return CheckIterators(iterators.GetBegin(), iterators.GetEnd());
}

template<typename T>
bool CheckArray(const vtkm::cont::ArrayHandle<T> &handle)
{
  return CheckPortal(handle.GetPortalConstControl());
}

void TestArrayHandle()
{
  std::cout << "Create array handle." << std::endl;
  vtkm::Scalar array[ARRAY_SIZE];
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    array[index] = TestValue(index);
  }

  vtkm::cont::ArrayHandle<vtkm::Scalar>::PortalControl arrayPortal(
        &array[0], &array[ARRAY_SIZE]);

  vtkm::cont::ArrayHandle<vtkm::Scalar>
      arrayHandle(arrayPortal);

  VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                   "ArrayHandle has wrong number of entries.");

  std::cout << "Check basic array." << std::endl;
  VTKM_TEST_ASSERT(CheckArray(arrayHandle),
                   "Array values not set correctly.");

  std::cout << "Check out execution array behavior." << std::endl;
  {
  vtkm::cont::ArrayHandle<vtkm::Scalar>::
      ExecutionTypes<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::PortalConst
      executionPortal;
  executionPortal =
      arrayHandle.PrepareForInput(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    VTKM_TEST_ASSERT(CheckPortal(executionPortal),
                     "Array not copied to execution correctly.");
  }

  {
    bool gotException = false;
    try
    {
      arrayHandle.PrepareForInPlace(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    }
    catch (vtkm::cont::Error &error)
    {
      std::cout << "Got expected error: " << error.GetMessage() << std::endl;
      gotException = true;
    }
    VTKM_TEST_ASSERT(gotException,
                     "PrepareForInPlace did not fail for const array.");
  }

  {
    typedef vtkm::cont::ArrayHandle<vtkm::Scalar>::
      ExecutionTypes<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Portal
        ExecutionPortalType;
    ExecutionPortalType executionPortal =
      arrayHandle.PrepareForOutput(ARRAY_SIZE*2,
                                   VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    for (vtkm::Id index = 0;
         index < executionPortal.GetNumberOfValues();
         index++)
    {
      executionPortal.Set(index, TestValue(index));
    }
  }
  VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE*2,
                   "Array not allocated correctly.");
  VTKM_TEST_ASSERT(CheckArray(arrayHandle),
                   "Array values not retrieved from execution.");

  std::cout << "Try shrinking the array." << std::endl;
  arrayHandle.Shrink(ARRAY_SIZE);
  VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                   "Array size did not shrink correctly.");
  VTKM_TEST_ASSERT(CheckArray(arrayHandle),
                   "Array values not retrieved from execution.");

  std::cout << "Try in place operation." << std::endl;
  {
    typedef vtkm::cont::ArrayHandle<vtkm::Scalar>::
      ExecutionTypes<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Portal
      ExecutionPortalType;
    ExecutionPortalType executionPortal =
      arrayHandle.PrepareForInPlace(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    for (vtkm::Id index = 0;
         index < executionPortal.GetNumberOfValues();
         index++)
    {
      executionPortal.Set(index, executionPortal.Get(index) + 1);
    }
  }
  vtkm::cont::ArrayHandle<vtkm::Scalar>::PortalConstControl controlPortal =
      arrayHandle.GetPortalConstControl();
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    VTKM_TEST_ASSERT(test_equal(controlPortal.Get(index), TestValue(index) + 1),
                     "Did not get result from in place operation.");
  }
}

}


int UnitTestArrayHandle(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandle);
}
