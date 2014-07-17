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
#include <vtkm/cont/Assert.h>

#include <vtkm/cont/testing/Testing.h>

#include <string>

namespace {

const vtkm::Id ARRAY_SIZE = 10;

// An unusual data type that represents a number with a string of a
// particular length. This makes sure that the ArrayHandleCounting
// works correctly with type casts.
class StringInt
{
public:
  StringInt() {}
  StringInt(vtkm::Id v)
  {
    VTKM_ASSERT_CONT(v >= 0);
    for (vtkm::Id i = 0; i < v; i++)
    {
      ++(*this);
    }
  }

  StringInt operator+(const StringInt &rhs) const
  {
    return StringInt(this->Value + rhs.Value);
  }

  bool operator==(const StringInt &other) const
  {
    return this->Value.size() == other.Value.size();
  }

  StringInt &operator++()
  {
    this->Value.append(".");
    return *this;
  }

private:
  StringInt(const std::string &v) : Value(v) { }

  std::string Value;
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
  TestFunctor()(StringInt(0));
  TestFunctor()(StringInt(10));
}


} // annonymous namespace

int UnitTestArrayHandleCounting(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleCounting);
}
