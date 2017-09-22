//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/BinaryOperators.h>

#include <vtkm/testing/Testing.h>

namespace
{

//general pair test
template <typename T>
void BinaryOperatorTest(){

  //Not using TestValue method as it causes roll-over to occur with
  //uint8 and int8 leading to unexpected comparisons.

  //test Sum
  { vtkm::Sum sum;
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
}
;

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

  //test BitwiseAnd
  {
    vtkm::BitwiseAnd bitwise_and;
    VTKM_TEST_ASSERT(bitwise_and(true, true) == true, "bitwise_and true wrong.");
    VTKM_TEST_ASSERT(bitwise_and(true, false) == false, "bitwise_and true wrong.");
    VTKM_TEST_ASSERT(bitwise_and(false, true) == false, "bitwise_and true wrong.");
    VTKM_TEST_ASSERT(bitwise_and(false, false) == false, "bitwise_and true wrong.");
  }

  //test BitwiseOr
  {
    vtkm::BitwiseOr bitwise_or;
    VTKM_TEST_ASSERT(bitwise_or(true, true) == true, "bitwise_or true wrong.");
    VTKM_TEST_ASSERT(bitwise_or(true, false) == true, "bitwise_or true wrong.");
    VTKM_TEST_ASSERT(bitwise_or(false, true) == true, "bitwise_or true wrong.");
    VTKM_TEST_ASSERT(bitwise_or(false, false) == false, "bitwise_or true wrong.");
  }

  //test BitwiseXor
  {
    vtkm::BitwiseXor bitwise_xor;
    VTKM_TEST_ASSERT(bitwise_xor(true, true) == false, "bitwise_xor true wrong.");
    VTKM_TEST_ASSERT(bitwise_xor(true, false) == true, "bitwise_xor true wrong.");
    VTKM_TEST_ASSERT(bitwise_xor(false, true) == true, "bitwise_xor true wrong.");
    VTKM_TEST_ASSERT(bitwise_xor(false, false) == false, "bitwise_xor true wrong.");
  }
}

} // anonymous namespace

int UnitTestBinaryOperators(int, char* [])
{
  return vtkm::testing::Testing::Run(TestBinaryOperators);
}
