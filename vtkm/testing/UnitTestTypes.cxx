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

#include <vtkm/Types.h>

#include <vtkm/testing/Testing.h>

namespace {

//general type test
template <typename T> void TypeTest()
{
  //grab the number of elements of T
  T a, b, c;
  typename T::ComponentType s(5);

  for(int i=0; i < T::NUM_COMPONENTS; ++i)
    {
    a[i]=typename T::ComponentType((i+1)*2);
    b[i]=typename T::ComponentType(i+1);
    c[i]=typename T::ComponentType((i+1)*2);
    }

  //verify prefix and postfix increment and decrement
  ++c[T::NUM_COMPONENTS-1];
  c[T::NUM_COMPONENTS-1]++;
  --c[T::NUM_COMPONENTS-1];
  c[T::NUM_COMPONENTS-1]--;

  //make c nearly like a to verify == and != are correct.
  c[T::NUM_COMPONENTS-1]=(c[T::NUM_COMPONENTS-1]-1);

  T plus = a + b;
  T correct_plus;
  for(int i=0; i < T::NUM_COMPONENTS; ++i)
    { correct_plus[i] = a[i] + b[i]; }
  VTKM_TEST_ASSERT(test_equal(plus, correct_plus),"Tuples not added correctly.");

  T minus = a - b;
  T correct_minus;
  for(int i=0; i < T::NUM_COMPONENTS; ++i)
    { correct_minus[i] = a[i] - b[i]; }
  VTKM_TEST_ASSERT(test_equal(minus, correct_minus),"Tuples not subtracted correctly.");


  T mult = a * b;
  T correct_mult;
  for(int i=0; i < T::NUM_COMPONENTS; ++i)
    { correct_mult[i] = a[i] * b[i]; }
  VTKM_TEST_ASSERT(test_equal(mult, correct_mult),"Tuples not multiplied correctly.");

  T div = a / b;
  T correct_div;
  for(int i=0; i < T::NUM_COMPONENTS; ++i)
    { correct_div[i] = a[i] / b[i]; }
  VTKM_TEST_ASSERT(test_equal(div,correct_div),"Tuples not divided correctly.");

  mult = s * a;
  for(int i=0; i < T::NUM_COMPONENTS; ++i)
    { correct_mult[i] = s * a[i]; }
  VTKM_TEST_ASSERT(test_equal(mult, correct_mult),
                  "Scalar and Tuple did not multiply correctly.");

  mult = a * s;
  VTKM_TEST_ASSERT(test_equal(mult, correct_mult),
                  "Tuple and Scalar to not multiply correctly.");

  typename T::ComponentType d = vtkm::dot(a, b);
  typename T::ComponentType correct_d = 0;
  for(int i=0; i < T::NUM_COMPONENTS; ++i)
    {correct_d += a[i] * b[i]; }
  VTKM_TEST_ASSERT(test_equal(d, correct_d), "dot(Tuple) wrong");

  VTKM_TEST_ASSERT(!(a < b), "operator< wrong");
  VTKM_TEST_ASSERT((b < a),  "operator< wrong");
  VTKM_TEST_ASSERT(!(a < a),  "operator< wrong");
  VTKM_TEST_ASSERT((a < plus),  "operator< wrong");
  VTKM_TEST_ASSERT((minus < plus),  "operator< wrong");
  VTKM_TEST_ASSERT((c < a),  "operator< wrong");

  VTKM_TEST_ASSERT(!(a == b), "operator== wrong");
  VTKM_TEST_ASSERT((a == a),  "operator== wrong");

  VTKM_TEST_ASSERT((a != b),  "operator!= wrong");
  VTKM_TEST_ASSERT(!(a != a), "operator!= wrong");

  //test against a tuple that shares some values
  VTKM_TEST_ASSERT( !(c == a), "operator == wrong");
  VTKM_TEST_ASSERT( !(a == c), "operator == wrong");

  VTKM_TEST_ASSERT( (c != a), "operator != wrong");
  VTKM_TEST_ASSERT( (a != c), "operator != wrong");
}

template<> void TypeTest<vtkm::Vector2>()
{
  vtkm::Vector2 a = vtkm::make_Vector2(2, 4);
  vtkm::Vector2 b = vtkm::make_Vector2(1, 2);
  vtkm::Scalar s = 5;

  vtkm::Vector2 plus = a + b;
  VTKM_TEST_ASSERT(test_equal(plus, vtkm::make_Vector2(3, 6)),
                  "Vectors do not add correctly.");

  vtkm::Vector2 minus = a - b;
  VTKM_TEST_ASSERT(test_equal(minus, vtkm::make_Vector2(1, 2)),
                  "Vectors to not subtract correctly.");

  vtkm::Vector2 mult = a * b;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vector2(2, 8)),
                  "Vectors to not multiply correctly.");

  vtkm::Vector2 div = a / b;
  VTKM_TEST_ASSERT(test_equal(div, vtkm::make_Vector2(2, 2)),
                  "Vectors to not divide correctly.");

  mult = s * a;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vector2(10, 20)),
                  "Vector and scalar to not multiply correctly.");

  mult = a * s;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vector2(10, 20)),
                  "Vector and scalar to not multiply correctly.");

  vtkm::Scalar d = vtkm::dot(a, b);
  VTKM_TEST_ASSERT(test_equal(d, vtkm::Scalar(10)), "dot(Vector2) wrong");

  VTKM_TEST_ASSERT(!(a < b), "operator< wrong");
  VTKM_TEST_ASSERT((b < a),  "operator< wrong");
  VTKM_TEST_ASSERT(!(a < a),  "operator< wrong");
  VTKM_TEST_ASSERT((a < plus),  "operator< wrong");
  VTKM_TEST_ASSERT((minus < plus),  "operator< wrong");

  VTKM_TEST_ASSERT(!(a == b), "operator== wrong");
  VTKM_TEST_ASSERT((a == a),  "operator== wrong");

  VTKM_TEST_ASSERT((a != b),  "operator!= wrong");
  VTKM_TEST_ASSERT(!(a != a), "operator!= wrong");

  //test against a tuple that shares some values
  const vtkm::Vector2 c = vtkm::make_Vector2(2,3);
  VTKM_TEST_ASSERT((c < a),  "operator< wrong");

  VTKM_TEST_ASSERT( !(c == a), "operator == wrong");
  VTKM_TEST_ASSERT( !(a == c), "operator == wrong");

  VTKM_TEST_ASSERT( (c != a), "operator != wrong");
  VTKM_TEST_ASSERT( (a != c), "operator != wrong");
}

template<> void TypeTest<vtkm::Vector3>()
{
  vtkm::Vector3 a = vtkm::make_Vector3(2, 4, 6);
  vtkm::Vector3 b = vtkm::make_Vector3(1, 2, 3);
  vtkm::Scalar s = 5;

  vtkm::Vector3 plus = a + b;
  VTKM_TEST_ASSERT(test_equal(plus, vtkm::make_Vector3(3, 6, 9)),
                  "Vectors do not add correctly.");

  vtkm::Vector3 minus = a - b;
  VTKM_TEST_ASSERT(test_equal(minus, vtkm::make_Vector3(1, 2, 3)),
                  "Vectors to not subtract correctly.");

  vtkm::Vector3 mult = a * b;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vector3(2, 8, 18)),
                  "Vectors to not multiply correctly.");

  vtkm::Vector3 div = a / b;
  VTKM_TEST_ASSERT(test_equal(div, vtkm::make_Vector3(2, 2, 2)),
                  "Vectors to not divide correctly.");

  mult = s * a;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vector3(10, 20, 30)),
                  "Vector and scalar to not multiply correctly.");

  mult = a * s;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vector3(10, 20, 30)),
                  "Vector and scalar to not multiply correctly.");

  vtkm::Scalar d = vtkm::dot(a, b);
  VTKM_TEST_ASSERT(test_equal(d, vtkm::Scalar(28)), "dot(Vector3) wrong");

  VTKM_TEST_ASSERT(!(a < b), "operator< wrong");
  VTKM_TEST_ASSERT((b < a),  "operator< wrong");
  VTKM_TEST_ASSERT(!(a < a),  "operator< wrong");
  VTKM_TEST_ASSERT((a < plus),  "operator< wrong");
  VTKM_TEST_ASSERT((minus < plus),  "operator< wrong");

  VTKM_TEST_ASSERT(!(a == b), "operator== wrong");
  VTKM_TEST_ASSERT((a == a),  "operator== wrong");

  VTKM_TEST_ASSERT((a != b),  "operator!= wrong");
  VTKM_TEST_ASSERT(!(a != a), "operator!= wrong");

  //test against a tuple that shares some values
  const vtkm::Vector3 c = vtkm::make_Vector3(2,4,5);
  VTKM_TEST_ASSERT((c < a),  "operator< wrong");

  VTKM_TEST_ASSERT( !(c == a), "operator == wrong");
  VTKM_TEST_ASSERT( !(a == c), "operator == wrong");

  VTKM_TEST_ASSERT( (c != a), "operator != wrong");
  VTKM_TEST_ASSERT( (a != c), "operator != wrong");
}

template<> void TypeTest<vtkm::Vector4>()
{
  vtkm::Vector4 a = vtkm::make_Vector4(2, 4, 6, 8);
  vtkm::Vector4 b = vtkm::make_Vector4(1, 2, 3, 4);
  vtkm::Scalar s = 5;

  vtkm::Vector4 plus = a + b;
  VTKM_TEST_ASSERT(test_equal(plus, vtkm::make_Vector4(3, 6, 9, 12)),
                  "Vectors do not add correctly.");

  vtkm::Vector4 minus = a - b;
  VTKM_TEST_ASSERT(test_equal(minus, vtkm::make_Vector4(1, 2, 3, 4)),
                  "Vectors to not subtract correctly.");

  vtkm::Vector4 mult = a * b;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vector4(2, 8, 18, 32)),
                  "Vectors to not multiply correctly.");

  vtkm::Vector4 div = a / b;
  VTKM_TEST_ASSERT(test_equal(div, vtkm::make_Vector4(2, 2, 2, 2)),
                  "Vectors to not divide correctly.");

  mult = s * a;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vector4(10, 20, 30, 40)),
                  "Vector and scalar to not multiply correctly.");

  mult = a * s;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vector4(10, 20, 30, 40)),
                  "Vector and scalar to not multiply correctly.");

  vtkm::Scalar d = vtkm::dot(a, b);
  VTKM_TEST_ASSERT(test_equal(d, vtkm::Scalar(60)), "dot(Vector4) wrong");


  VTKM_TEST_ASSERT(!(a < b), "operator< wrong");
  VTKM_TEST_ASSERT((b < a),  "operator< wrong");
  VTKM_TEST_ASSERT(!(a < a),  "operator< wrong");
  VTKM_TEST_ASSERT((a < plus),  "operator< wrong");
  VTKM_TEST_ASSERT((minus < plus),  "operator< wrong");

  VTKM_TEST_ASSERT(!(a == b), "operator== wrong");
  VTKM_TEST_ASSERT((a == a),  "operator== wrong");

  VTKM_TEST_ASSERT((a != b),  "operator!= wrong");
  VTKM_TEST_ASSERT(!(a != a), "operator!= wrong");

  //test against a tuple that shares some values
  const vtkm::Vector4 c = vtkm::make_Vector4(2,4,6,7);
  VTKM_TEST_ASSERT((c < a),  "operator< wrong");

  VTKM_TEST_ASSERT( !(c == a), "operator == wrong");
  VTKM_TEST_ASSERT( !(a == c), "operator == wrong");

  VTKM_TEST_ASSERT( (c != a), "operator != wrong");
  VTKM_TEST_ASSERT( (a != c), "operator != wrong");
}

template<> void TypeTest<vtkm::Id3>()
{
  vtkm::Id3 a = vtkm::make_Id3(2, 4, 6);
  vtkm::Id3 b = vtkm::make_Id3(1, 2, 3);
  vtkm::Id s = 5;

  vtkm::Id3 plus = a + b;
  if ((plus[0] != 3) || (plus[1] != 6) || (plus[2] != 9))
    {
    VTKM_TEST_FAIL("Vectors do not add correctly.");
    }

  vtkm::Id3 minus = a - b;
  if ((minus[0] != 1) || (minus[1] != 2) || (minus[2] != 3))
    {
    VTKM_TEST_FAIL("Vectors to not subtract correctly.");
    }

  vtkm::Id3 mult = a * b;
  if ((mult[0] != 2) || (mult[1] != 8) || (mult[2] != 18))
    {
    VTKM_TEST_FAIL("Vectors to not multiply correctly.");
    }

  vtkm::Id3 div = a / b;
  if ((div[0] != 2) || (div[1] != 2) || (div[2] != 2))
    {
    VTKM_TEST_FAIL("Vectors to not divide correctly.");
    }

  mult = s * a;
  if ((mult[0] != 10) || (mult[1] != 20) || (mult[2] != 30))
    {
    VTKM_TEST_FAIL("Vector and scalar to not multiply correctly.");
    }

  mult = a * s;
  if ((mult[0] != 10) || (mult[1] != 20) || (mult[2] != 30))
    {
    VTKM_TEST_FAIL("Vector and scalar to not multiply correctly.");
    }

  if (vtkm::dot(a, b) != 28)
    {
    VTKM_TEST_FAIL("dot(Id3) wrong");
    }

  VTKM_TEST_ASSERT(!(a < b), "operator< wrong");
  VTKM_TEST_ASSERT((b < a),  "operator< wrong");
  VTKM_TEST_ASSERT(!(a < a),  "operator< wrong");
  VTKM_TEST_ASSERT((a < plus),  "operator< wrong");
  VTKM_TEST_ASSERT((minus < plus),  "operator< wrong");

  if (a == b)
    {
    VTKM_TEST_FAIL("operator== wrong");
    }
  if (!(a == a))
    {
    VTKM_TEST_FAIL("operator== wrong");
    }

  if (!(a != b))
    {
    VTKM_TEST_FAIL("operator!= wrong");
    }
  if (a != a)
    {
    VTKM_TEST_FAIL("operator!= wrong");
    }

  //test against a tuple that shares some values
  const vtkm::Id3 c = vtkm::make_Id3(2,4,5);
  VTKM_TEST_ASSERT((c < a),  "operator< wrong");

  if (c == a) { VTKM_TEST_FAIL("operator== wrong"); }
  if (a == c) { VTKM_TEST_FAIL("operator== wrong"); }

  if (!(c != a)) { VTKM_TEST_FAIL("operator!= wrong"); }
  if (!(a != c)) { VTKM_TEST_FAIL("operator!= wrong"); }
}

template<> void TypeTest<vtkm::Scalar>()
{
  vtkm::Scalar a = 4;
  vtkm::Scalar b = 2;

  vtkm::Scalar plus = a + b;
  if (plus != 6)
    {
    VTKM_TEST_FAIL("Scalars do not add correctly.");
    }

  vtkm::Scalar minus = a - b;
  if (minus != 2)
    {
    VTKM_TEST_FAIL("Scalars to not subtract correctly.");
    }

  vtkm::Scalar mult = a * b;
  if (mult != 8)
    {
    VTKM_TEST_FAIL("Scalars to not multiply correctly.");
    }

  vtkm::Scalar div = a / b;
  if (div != 2)
    {
    VTKM_TEST_FAIL("Scalars to not divide correctly.");
    }

  if (a == b)
    {
    VTKM_TEST_FAIL("operator== wrong");
    }
  if (!(a != b))
    {
    VTKM_TEST_FAIL("operator!= wrong");
    }

  if (vtkm::dot(a, b) != 8)
    {
    VTKM_TEST_FAIL("dot(Scalar) wrong");
    }
}

template<> void TypeTest<vtkm::Id>()
{
  vtkm::Id a = 4;
  vtkm::Id b = 2;

  vtkm::Id plus = a + b;
  if (plus != 6)
    {
    VTKM_TEST_FAIL("Scalars do not add correctly.");
    }

  vtkm::Id minus = a - b;
  if (minus != 2)
    {
    VTKM_TEST_FAIL("Scalars to not subtract correctly.");
    }

  vtkm::Id mult = a * b;
  if (mult != 8)
    {
    VTKM_TEST_FAIL("Scalars to not multiply correctly.");
    }

  vtkm::Id div = a / b;
  if (div != 2)
    {
    VTKM_TEST_FAIL("Scalars to not divide correctly.");
    }

  if (a == b)
    {
    VTKM_TEST_FAIL("operator== wrong");
    }
  if (!(a != b))
    {
    VTKM_TEST_FAIL("operator!= wrong");
    }

  if (vtkm::dot(a, b) != 8)
    {
    VTKM_TEST_FAIL("dot(Id) wrong");
    }
}

struct TypeTestFunctor
{
  template <typename T> void operator()(const T&) const {
    TypeTest<T>();
  }
};

void TestTypes()
{
  vtkm::testing::Testing::TryAllTypes(TypeTestFunctor());

  //try with some custom tuple types
  TypeTestFunctor()( vtkm::Tuple<vtkm::Scalar,6>() );
  TypeTestFunctor()( vtkm::Tuple<vtkm::Id,4>() );
  TypeTestFunctor()( vtkm::Tuple<unsigned char,4>() );
  TypeTestFunctor()( vtkm::Tuple<vtkm::Id,1>() );
  TypeTestFunctor()( vtkm::Tuple<vtkm::Scalar,1>() );
}

} // anonymous namespace

int UnitTestTypes(int, char *[])
{
  return vtkm::testing::Testing::Run(TestTypes);
}
