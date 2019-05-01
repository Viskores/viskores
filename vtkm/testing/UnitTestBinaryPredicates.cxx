//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/BinaryPredicates.h>

#include <vtkm/testing/Testing.h>

namespace
{

//general pair test
template <typename T>
void BinaryPredicateTest()
{
  //Not using TestValue method as it causes roll-over to occur with
  //uint8 and int8 leading to unexpected comparisons.

  //test Equal
  {
    vtkm::Equal is_equal;
    VTKM_TEST_ASSERT(is_equal(vtkm::TypeTraits<T>::ZeroInitialization(),
                              vtkm::TypeTraits<T>::ZeroInitialization()),
                     "Equal wrong.");
    VTKM_TEST_ASSERT(is_equal(T(1), T(2)) == false, "Equal wrong.");
  }

  //test NotEqual
  {
    vtkm::NotEqual not_equal;
    VTKM_TEST_ASSERT(not_equal(vtkm::TypeTraits<T>::ZeroInitialization(), T(1)), "NotEqual wrong.");
    VTKM_TEST_ASSERT(not_equal(T(1), T(1)) == false, "NotEqual wrong.");
  }

  //test SortLess
  {
    vtkm::SortLess sort_less;
    VTKM_TEST_ASSERT(sort_less(T(1), T(2)) == true, "SortLess wrong.");
    VTKM_TEST_ASSERT(sort_less(T(2), T(2)) == false, "SortLess wrong.");
    VTKM_TEST_ASSERT(sort_less(T(2), T(1)) == false, "SortLess wrong.");
  }

  //test SortGreater
  {
    vtkm::SortGreater sort_greater;
    VTKM_TEST_ASSERT(sort_greater(T(1), T(2)) == false, "SortGreater wrong.");
    VTKM_TEST_ASSERT(sort_greater(T(1), T(1)) == false, "SortGreater wrong.");
    VTKM_TEST_ASSERT(sort_greater(T(3), T(2)) == true, "SortGreater wrong.");
  }
}

struct BinaryPredicateTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    BinaryPredicateTest<T>();
  }
};

void TestBinaryPredicates()
{
  vtkm::testing::Testing::TryTypes(BinaryPredicateTestFunctor());

  //test LogicalAnd
  {
    vtkm::LogicalAnd logical_and;
    VTKM_TEST_ASSERT(logical_and(true, true) == true, "logical_and true wrong.");
    VTKM_TEST_ASSERT(logical_and(true, false) == false, "logical_and true wrong.");
    VTKM_TEST_ASSERT(logical_and(false, true) == false, "logical_and true wrong.");
    VTKM_TEST_ASSERT(logical_and(false, false) == false, "logical_and true wrong.");
  }

  //test LogicalOr
  {
    vtkm::LogicalOr logical_or;
    VTKM_TEST_ASSERT(logical_or(true, true) == true, "logical_or true wrong.");
    VTKM_TEST_ASSERT(logical_or(true, false) == true, "logical_or true wrong.");
    VTKM_TEST_ASSERT(logical_or(false, true) == true, "logical_or true wrong.");
    VTKM_TEST_ASSERT(logical_or(false, false) == false, "logical_or true wrong.");
  }
}

} // anonymous namespace

int UnitTestBinaryPredicates(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestBinaryPredicates, argc, argv);
}
