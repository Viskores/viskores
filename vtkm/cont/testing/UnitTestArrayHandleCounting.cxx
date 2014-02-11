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

#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL

#include <vtkm/cont/ArrayHandleCounting.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

const vtkm::Id ARRAY_SIZE = 10;

//increments by two instead of one wrapper
template<typename T>
struct CountByTwo
{
  CountByTwo(): Value() {}
  explicit CountByTwo(T t): Value(t) {}

  bool operator==(const T& other) const
    { return Value == other; }

  bool operator==(const CountByTwo<T>& other) const
    { return Value == other.Value; }

  CountByTwo<T> operator+(vtkm::Id count) const
  { return CountByTwo<T>(Value+(count*2)); }

  CountByTwo<T>& operator++()
    { ++Value; ++Value; return *this; }

  friend std::ostream& operator<< (std::ostream& os, const CountByTwo<T>& obj)
    { os << obj.Value; return os; }
  T Value;
};



template< typename ValueType>
struct TemplatedTests
{
  typedef vtkm::cont::ArrayHandleCounting<ValueType> ArrayHandleType;

  typedef vtkm::cont::ArrayHandle<ValueType,
    typename vtkm::cont::internal::ArrayHandleCountingTraits<ValueType>::Tag>
  ArrayHandleType2;

  typedef typename ArrayHandleType::PortalConstControl PortalType;

  void operator()( const ValueType startingValue )
  {
  ArrayHandleType arrayConst(startingValue, ARRAY_SIZE);

  ArrayHandleType arrayMake = vtkm::cont::make_ArrayHandleCounting(startingValue,ARRAY_SIZE);

  ArrayHandleType2 arrayHandle =
      ArrayHandleType2(PortalType(startingValue, ARRAY_SIZE));

  VTKM_TEST_ASSERT(arrayConst.GetNumberOfValues() == ARRAY_SIZE,
                  "Counting array using constructor has wrong size.");

  VTKM_TEST_ASSERT(arrayMake.GetNumberOfValues() == ARRAY_SIZE,
                  "Counting array using make has wrong size.");

  VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                "Counting array using raw array handle + tag has wrong size.");

  ValueType properValue = startingValue;
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
    VTKM_TEST_ASSERT(arrayConst.GetPortalConstControl().Get(index) == properValue,
                    "Counting array using constructor has unexpected value.");
    VTKM_TEST_ASSERT(arrayMake.GetPortalConstControl().Get(index) == properValue,
                    "Counting array using make has unexpected value.");

    VTKM_TEST_ASSERT(arrayHandle.GetPortalConstControl().Get(index) == properValue,
                  "Counting array using raw array handle + tag has unexpected value.");
    ++properValue;
    }
  }
};

struct TestFunctor
{
  template <typename T>
  void operator()(const T t)
  {
    TemplatedTests<T> tests;
    tests(t);
  }
};

void TestArrayHandleCounting()
{
  TestFunctor()(vtkm::Id(0));
  TestFunctor()(vtkm::Scalar(0));
  TestFunctor()( CountByTwo<vtkm::Id>(12) );
  TestFunctor()( CountByTwo<vtkm::Scalar>(1.2f) );
}


} // annonymous namespace

int UnitTestArrayHandleCounting(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleCounting);
}
