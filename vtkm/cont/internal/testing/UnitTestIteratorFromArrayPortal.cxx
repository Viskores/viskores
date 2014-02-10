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

#include <vtkm/cont/internal/IteratorFromArrayPortal.h>

#include <vtkm/VectorTraits.h>
#include <vtkm/cont/internal/ArrayPortalFromIterators.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

template<typename T>
struct TemplatedTests
{
  static const vtkm::Id ARRAY_SIZE = 10;

  typedef T ValueType;
  typedef typename vtkm::VectorTraits<ValueType>::ComponentType ComponentType;

  ValueType ExpectedValue(vtkm::Id index, ComponentType value) {
    return ValueType(index + value);
  }

  template<class IteratorType>
  void FillIterator(IteratorType begin, IteratorType end, ComponentType value) {
    vtkm::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
      {
      *iter = ExpectedValue(index, value);
      index++;
      }
  }

  template<class IteratorType>
  bool CheckIterator(IteratorType begin,
                     IteratorType end,
                     ComponentType value) {
    vtkm::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
      {
      if (ValueType(*iter) != ExpectedValue(index, value)) return false;
      index++;
      }
    return true;
  }

  ComponentType ORIGINAL_VALUE() { return 239; }

  template<class ArrayPortalType>
  void TestIteratorRead(ArrayPortalType portal)
  {
    typedef vtkm::cont::internal::IteratorFromArrayPortal<ArrayPortalType> IteratorType;

    IteratorType begin = vtkm::cont::internal::make_IteratorBegin(portal);
    IteratorType end = vtkm::cont::internal::make_IteratorEnd(portal);
    VTKM_TEST_ASSERT(std::distance(begin, end) == ARRAY_SIZE,
                    "Distance between begin and end incorrect.");

    std::cout << "    Check forward iteration." << std::endl;
    VTKM_TEST_ASSERT(CheckIterator(begin, end, ORIGINAL_VALUE()),
                    "Forward iteration wrong");

    std::cout << "    Check backward iteration." << std::endl;
    IteratorType middle = end;
    for (vtkm::Id index = portal.GetNumberOfValues()-1; index >= 0; index--)
      {
      middle--;
      ValueType value = *middle;
      VTKM_TEST_ASSERT(value == ExpectedValue(index, ORIGINAL_VALUE()),
                      "Backward iteration wrong");
      }

    std::cout << "    Check advance" << std::endl;
    middle = begin + ARRAY_SIZE/2;
    VTKM_TEST_ASSERT(std::distance(begin, middle) == ARRAY_SIZE/2,
                    "Bad distance to middle.");
    VTKM_TEST_ASSERT(
          ValueType(*middle) == ExpectedValue(ARRAY_SIZE/2, ORIGINAL_VALUE()),
          "Bad value at middle.");
  }

  template<class ArrayPortalType>
  void TestIteratorWrite(ArrayPortalType portal)
  {
    typedef vtkm::cont::internal::IteratorFromArrayPortal<ArrayPortalType> IteratorType;

    IteratorType begin = vtkm::cont::internal::make_IteratorBegin(portal);
    IteratorType end = vtkm::cont::internal::make_IteratorEnd(portal);

    static const ComponentType WRITE_VALUE = 873;

    std::cout << "    Write values to iterator." << std::endl;
    FillIterator(begin, end, WRITE_VALUE);

    std::cout << "    Check values in portal." << std::endl;
    VTKM_TEST_ASSERT(CheckIterator(portal.GetIteratorBegin(),
                                  portal.GetIteratorEnd(),
                                  WRITE_VALUE),
                    "Did not get correct values when writing to iterator.");
  }

  void operator()()
  {
    ValueType array[ARRAY_SIZE];

    FillIterator(array, array+ARRAY_SIZE, ORIGINAL_VALUE());

    ::vtkm::cont::internal::ArrayPortalFromIterators<ValueType *>
        portal(array, array+ARRAY_SIZE);
    ::vtkm::cont::internal::ArrayPortalFromIterators<const ValueType *>
        const_portal(array, array+ARRAY_SIZE);

    std::cout << "  Test read from iterator." << std::endl;
    TestIteratorRead(portal);

    std::cout << "  Test read from const iterator." << std::endl;
    TestIteratorRead(const_portal);

    std::cout << "  Test write to iterator." << std::endl;
    TestIteratorWrite(portal);
  }
};

struct TestFunctor
{
  template<typename T>
  void operator()(T)
  {
    TemplatedTests<T> tests;
    tests();
  }
};

void TestArrayIteratorFromArrayPortal()
{
  vtkm::testing::Testing::TryAllTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestIteratorFromArrayPortal(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayIteratorFromArrayPortal);
}
