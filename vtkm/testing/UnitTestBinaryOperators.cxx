//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/BinaryOperators.h>

#include <vtkm/testing/Testing.h>

namespace
{

//general pair test
template <typename T>
void BinaryOperatorTest()
{

  //Not using TestValue method as it causes roll-over to occur with
  //uint8 and int8 leading to unexpected comparisons.

  //test Sum
  {
    vtkm::Sum sum;
    T result;

    result = sum(vtkm::TypeTraits<T>::ZeroInitialization(), T(1));
    VTKM_TEST_ASSERT(result == T(1), "Sum wrong.");

    result = sum(T(1), T(1));
    VTKM_TEST_ASSERT(result == T(2), "Sum wrong.");
  }

  //test Product
  {
    vtkm::Product product;
    T result;

    result = product(vtkm::TypeTraits<T>::ZeroInitialization(), T(1));
    VTKM_TEST_ASSERT(result == vtkm::TypeTraits<T>::ZeroInitialization(), "Product wrong.");

    result = product(T(1), T(1));
    VTKM_TEST_ASSERT(result == T(1), "Product wrong.");

    result = product(T(2), T(3));
    VTKM_TEST_ASSERT(result == T(6), "Product wrong.");
  }

  //test Maximum
  {
    vtkm::Maximum maximum;
    VTKM_TEST_ASSERT(maximum(T(1), T(2)) == T(2), "Maximum wrong.");
    VTKM_TEST_ASSERT(maximum(T(2), T(2)) == T(2), "Maximum wrong.");
    VTKM_TEST_ASSERT(maximum(T(2), T(1)) == T(2), "Maximum wrong.");
  }

  //test Minimum
  {
    vtkm::Minimum minimum;
    VTKM_TEST_ASSERT(minimum(T(1), T(2)) == T(1), "Minimum wrong.");
    VTKM_TEST_ASSERT(minimum(T(1), T(1)) == T(1), "Minimum wrong.");
    VTKM_TEST_ASSERT(minimum(T(3), T(2)) == T(2), "Minimum wrong.");
  }

  //test MinAndMax
  {
    vtkm::MinAndMax<T> min_and_max;
    vtkm::Vec<T, 2> result;

    // Test1: basic param
    {
      result = min_and_max(T(1));
      VTKM_TEST_ASSERT(test_equal(result, vtkm::Vec<T, 2>(T(1), T(1))), "Test1 MinAndMax wrong");
    }

    // Test2: basic param
    {
      result = min_and_max(vtkm::TypeTraits<T>::ZeroInitialization(), T(1));
      VTKM_TEST_ASSERT(
        test_equal(result, vtkm::Vec<T, 2>(vtkm::TypeTraits<T>::ZeroInitialization(), T(1))),
        "Test2 MinAndMax wrong");

      result = min_and_max(T(2), T(1));
      VTKM_TEST_ASSERT(test_equal(result, vtkm::Vec<T, 2>(T(1), T(2))), "Test2 MinAndMax wrong");
    }

    // Test3: 1st param vector, 2nd param basic
    {
      result = min_and_max(vtkm::Vec<T, 2>(3, 5), T(7));
      VTKM_TEST_ASSERT(test_equal(result, vtkm::Vec<T, 2>(T(3), T(7))), "Test3 MinAndMax Wrong");

      result = min_and_max(vtkm::Vec<T, 2>(3, 5), T(2));
      VTKM_TEST_ASSERT(test_equal(result, vtkm::Vec<T, 2>(T(2), T(5))), "Test3 MinAndMax Wrong");
    }

    // Test4: 1st param basic, 2nd param vector
    {
      result = min_and_max(T(7), vtkm::Vec<T, 2>(3, 5));
      VTKM_TEST_ASSERT(test_equal(result, vtkm::Vec<T, 2>(T(3), T(7))), "Test4 MinAndMax Wrong");

      result = min_and_max(T(2), vtkm::Vec<T, 2>(3, 5));
      VTKM_TEST_ASSERT(test_equal(result, vtkm::Vec<T, 2>(T(2), T(5))), "Test4 MinAndMax Wrong");
    }

    // Test5: 2 vector param
    {
      result = min_and_max(vtkm::Vec<T, 2>(2, 4), vtkm::Vec<T, 2>(3, 5));
      VTKM_TEST_ASSERT(test_equal(result, vtkm::Vec<T, 2>(T(2), T(5))), "Test5 MinAndMax Wrong");

      result = min_and_max(vtkm::Vec<T, 2>(2, 7), vtkm::Vec<T, 2>(3, 5));
      VTKM_TEST_ASSERT(test_equal(result, vtkm::Vec<T, 2>(T(2), T(7))), "Test5 MinAndMax Wrong");

      result = min_and_max(vtkm::Vec<T, 2>(4, 4), vtkm::Vec<T, 2>(1, 8));
      VTKM_TEST_ASSERT(test_equal(result, vtkm::Vec<T, 2>(T(1), T(8))), "Test5 MinAndMax Wrong");

      result = min_and_max(vtkm::Vec<T, 2>(4, 4), vtkm::Vec<T, 2>(3, 3));
      VTKM_TEST_ASSERT(test_equal(result, vtkm::Vec<T, 2>(T(3), T(4))), "Test5 MinAndMax Wrong");
    }
  }
}

struct BinaryOperatorTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    BinaryOperatorTest<T>();
  }
};

void TestBinaryOperators()
{
  vtkm::testing::Testing::TryTypes(BinaryOperatorTestFunctor());

  vtkm::UInt32 v1 = 0xccccccccu;
  vtkm::UInt32 v2 = 0xffffffffu;
  vtkm::UInt32 v3 = 0x0u;

  //test BitwiseAnd
  {
    vtkm::BitwiseAnd bitwise_and;
    VTKM_TEST_ASSERT(bitwise_and(v1, v2) == (v1 & v2), "bitwise_and wrong.");
    VTKM_TEST_ASSERT(bitwise_and(v1, v3) == (v1 & v3), "bitwise_and wrong.");
    VTKM_TEST_ASSERT(bitwise_and(v2, v3) == (v2 & v3), "bitwise_and wrong.");
  }

  //test BitwiseOr
  {
    vtkm::BitwiseOr bitwise_or;
    VTKM_TEST_ASSERT(bitwise_or(v1, v2) == (v1 | v2), "bitwise_or wrong.");
    VTKM_TEST_ASSERT(bitwise_or(v1, v3) == (v1 | v3), "bitwise_or wrong.");
    VTKM_TEST_ASSERT(bitwise_or(v2, v3) == (v2 | v3), "bitwise_or wrong.");
  }

  //test BitwiseXor
  {
    vtkm::BitwiseXor bitwise_xor;
    VTKM_TEST_ASSERT(bitwise_xor(v1, v2) == (v1 ^ v2), "bitwise_xor wrong.");
    VTKM_TEST_ASSERT(bitwise_xor(v1, v3) == (v1 ^ v3), "bitwise_xor wrong.");
    VTKM_TEST_ASSERT(bitwise_xor(v2, v3) == (v2 ^ v3), "bitwise_xor wrong.");
  }
}

} // anonymous namespace

int UnitTestBinaryOperators(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestBinaryOperators, argc, argv);
}
