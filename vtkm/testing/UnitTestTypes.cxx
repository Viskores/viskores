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
//  Copyright 2014 Los Alamos National Security.
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
//  Copyright 2014 Los Alamos National Security.
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

void CheckTypeSizes()
{
  VTKM_TEST_ASSERT(sizeof(vtkm::Int8) == 1, "Int8 wrong size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::UInt8) == 1, "UInt8 wrong size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::Int16) == 2, "Int16 wrong size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::UInt16) == 2, "UInt16 wrong size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::Int32) == 4, "Int32 wrong size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::UInt32) == 4, "UInt32 wrong size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::Int64) == 8, "Int64 wrong size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::UInt64) == 8, "UInt64 wrong size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::Float32) == 4, "Float32 wrong size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::Float64) == 8, "Float32 wrong size.");
}

// This part of the test has to be broken out of GeneralVecTypeTest because
// the negate operation is only supported on vectors of signed types.
template<typename ComponentType, vtkm::IdComponent Size>
void DoGeneralVecTypeTestNegate(const vtkm::Vec<ComponentType,Size> &)
{
  typedef vtkm::Vec<ComponentType,Size> VectorType;
  for (vtkm::Id valueIndex = 0; valueIndex < 10; valueIndex++)
  {
    VectorType original = TestValue(valueIndex, VectorType());
    VectorType negative = -original;

    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < Size;
         componentIndex++)
    {
      VTKM_TEST_ASSERT(
            test_equal(-(original[componentIndex]), negative[componentIndex]),
            "Vec did not negate correctly.");
    }

    VTKM_TEST_ASSERT(test_equal(original, -negative),
                     "Double Vec negative is not positive.");
  }
}

template<typename ComponentType, vtkm::IdComponent Size>
void GeneralVecTypeTestNegate(const vtkm::Vec<ComponentType,Size> &)
{
  // Do not test the negate operator unless it is a negatable type.
}

template<vtkm::IdComponent Size>
void GeneralVecTypeTestNegate(const vtkm::Vec<vtkm::Int8,Size> &x)
{
  DoGeneralVecTypeTestNegate(x);
}

template<vtkm::IdComponent Size>
void GeneralVecTypeTestNegate(const vtkm::Vec<vtkm::Int16,Size> &x)
{
  DoGeneralVecTypeTestNegate(x);
}

template<vtkm::IdComponent Size>
void GeneralVecTypeTestNegate(const vtkm::Vec<vtkm::Int32,Size> &x)
{
  DoGeneralVecTypeTestNegate(x);
}

template<vtkm::IdComponent Size>
void GeneralVecTypeTestNegate(const vtkm::Vec<vtkm::Int64,Size> &x)
{
  DoGeneralVecTypeTestNegate(x);
}

template<vtkm::IdComponent Size>
void GeneralVecTypeTestNegate(const vtkm::Vec<vtkm::Float32,Size> &x)
{
  DoGeneralVecTypeTestNegate(x);
}

template<vtkm::IdComponent Size>
void GeneralVecTypeTestNegate(const vtkm::Vec<vtkm::Float64,Size> &x)
{
  DoGeneralVecTypeTestNegate(x);
}

//general type test
template<typename ComponentType, vtkm::IdComponent Size>
void GeneralVecTypeTest(const vtkm::Vec<ComponentType,Size> &)
{
  typedef vtkm::Vec<ComponentType,Size> T;
 
  //grab the number of elements of T
  T a, b, c;
  ComponentType s(5);

  for(vtkm::IdComponent i=0; i < T::NUM_COMPONENTS; ++i)
  {
    a[i]=ComponentType((i+1)*2);
    b[i]=ComponentType(i+1);
    c[i]=ComponentType((i+1)*2);
  }

  //verify prefix and postfix increment and decrement
  ++c[T::NUM_COMPONENTS-1];
  c[T::NUM_COMPONENTS-1]++;
  --c[T::NUM_COMPONENTS-1];
  c[T::NUM_COMPONENTS-1]--;

  //make c nearly like a to verify == and != are correct.
  c[T::NUM_COMPONENTS-1]=ComponentType(c[T::NUM_COMPONENTS-1]-1);

  T plus = a + b;
  T correct_plus;
  for(vtkm::IdComponent i=0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_plus[i] = ComponentType(a[i] + b[i]);
  }
  VTKM_TEST_ASSERT(test_equal(plus, correct_plus),"Tuples not added correctly.");

  T minus = a - b;
  T correct_minus;
  for(vtkm::IdComponent i=0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_minus[i] = ComponentType(a[i] - b[i]);
  }
  VTKM_TEST_ASSERT(test_equal(minus, correct_minus),"Tuples not subtracted correctly.");


  T mult = a * b;
  T correct_mult;
  for(vtkm::IdComponent i=0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_mult[i] = ComponentType(a[i] * b[i]);
  }
  VTKM_TEST_ASSERT(test_equal(mult, correct_mult),"Tuples not multiplied correctly.");

  T div = a / b;
  T correct_div;
  for(vtkm::IdComponent i=0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_div[i] = ComponentType(a[i] / b[i]);
  }
  VTKM_TEST_ASSERT(test_equal(div,correct_div),"Tuples not divided correctly.");

  mult = s * a;
  for(vtkm::IdComponent i=0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_mult[i] = ComponentType(s * a[i]);
  }
  VTKM_TEST_ASSERT(test_equal(mult, correct_mult),
                   "Scalar and Tuple did not multiply correctly.");

  mult = a * s;
  VTKM_TEST_ASSERT(test_equal(mult, correct_mult),
                   "Tuple and Scalar to not multiply correctly.");

  ComponentType d = vtkm::dot(a, b);
  ComponentType correct_d = 0;
  for(vtkm::IdComponent i=0; i < T::NUM_COMPONENTS; ++i)
  {
    correct_d = ComponentType(correct_d + a[i] * b[i]);
  }
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

  GeneralVecTypeTestNegate(T());
}

template<typename ComponentType, vtkm::IdComponent Size>
void TypeTest(const vtkm::Vec<ComponentType,Size> &)
{
  GeneralVecTypeTest(vtkm::Vec<ComponentType,Size>());
}

template<typename Scalar>
void TypeTest(const vtkm::Vec<Scalar,2> &)
{
  typedef vtkm::Vec<Scalar,2> Vector;

  GeneralVecTypeTest(Vector());

  Vector a(2, 4);
  Vector b(1, 2);
  Scalar s = 5;

  Vector plus = a + b;
  VTKM_TEST_ASSERT(test_equal(plus, vtkm::make_Vec(3, 6)),
                   "Vectors do not add correctly.");

  Vector minus = a - b;
  VTKM_TEST_ASSERT(test_equal(minus, vtkm::make_Vec(1, 2)),
                   "Vectors to not subtract correctly.");

  Vector mult = a * b;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vec(2, 8)),
                   "Vectors to not multiply correctly.");

  Vector div = a / b;
  VTKM_TEST_ASSERT(test_equal(div, vtkm::make_Vec(2, 2)),
                   "Vectors to not divide correctly.");

  mult = s * a;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vec(10, 20)),
                   "Vector and scalar to not multiply correctly.");

  mult = a * s;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vec(10, 20)),
                   "Vector and scalar to not multiply correctly.");

  Scalar d = vtkm::dot(a, b);
  VTKM_TEST_ASSERT(test_equal(d, Scalar(10)), "dot(Vector2) wrong");

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
  const Vector c(2, 3);
  VTKM_TEST_ASSERT((c < a),  "operator< wrong");

  VTKM_TEST_ASSERT( !(c == a), "operator == wrong");
  VTKM_TEST_ASSERT( !(a == c), "operator == wrong");

  VTKM_TEST_ASSERT( (c != a), "operator != wrong");
  VTKM_TEST_ASSERT( (a != c), "operator != wrong");
}

template<typename Scalar>
void TypeTest(const vtkm::Vec<Scalar,3> &)
{
  typedef vtkm::Vec<Scalar,3> Vector;

  GeneralVecTypeTest(Vector());

  Vector a(2, 4, 6);
  Vector b(1, 2, 3);
  Scalar s = 5;

  Vector plus = a + b;
  VTKM_TEST_ASSERT(test_equal(plus, vtkm::make_Vec(3, 6, 9)),
                   "Vectors do not add correctly.");

  Vector minus = a - b;
  VTKM_TEST_ASSERT(test_equal(minus, vtkm::make_Vec(1, 2, 3)),
                   "Vectors to not subtract correctly.");

  Vector mult = a * b;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vec(2, 8, 18)),
                   "Vectors to not multiply correctly.");

  Vector div = a / b;
  VTKM_TEST_ASSERT(test_equal(div, vtkm::make_Vec(2, 2, 2)),
                   "Vectors to not divide correctly.");

  mult = s * a;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vec(10, 20, 30)),
                   "Vector and scalar to not multiply correctly.");

  mult = a * s;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vec(10, 20, 30)),
                   "Vector and scalar to not multiply correctly.");

  Scalar d = vtkm::dot(a, b);
  VTKM_TEST_ASSERT(test_equal(d, Scalar(28)), "dot(Vector3) wrong");

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
  const Vector c(2,4,5);
  VTKM_TEST_ASSERT((c < a),  "operator< wrong");

  VTKM_TEST_ASSERT( !(c == a), "operator == wrong");
  VTKM_TEST_ASSERT( !(a == c), "operator == wrong");

  VTKM_TEST_ASSERT( (c != a), "operator != wrong");
  VTKM_TEST_ASSERT( (a != c), "operator != wrong");
}

template<typename Scalar>
void TypeTest(const vtkm::Vec<Scalar,4> &)
{
  typedef vtkm::Vec<Scalar,4> Vector;

  GeneralVecTypeTest(Vector());

  Vector a(2, 4, 6, 8);
  Vector b(1, 2, 3, 4);
  Scalar s = 5;

  Vector plus = a + b;
  VTKM_TEST_ASSERT(test_equal(plus, vtkm::make_Vec(3, 6, 9, 12)),
                   "Vectors do not add correctly.");

  Vector minus = a - b;
  VTKM_TEST_ASSERT(test_equal(minus, vtkm::make_Vec(1, 2, 3, 4)),
                   "Vectors to not subtract correctly.");

  Vector mult = a * b;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vec(2, 8, 18, 32)),
                   "Vectors to not multiply correctly.");

  Vector div = a / b;
  VTKM_TEST_ASSERT(test_equal(div, vtkm::make_Vec(2, 2, 2, 2)),
                   "Vectors to not divide correctly.");

  mult = s * a;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vec(10, 20, 30, 40)),
                   "Vector and scalar to not multiply correctly.");

  mult = a * s;
  VTKM_TEST_ASSERT(test_equal(mult, vtkm::make_Vec(10, 20, 30, 40)),
                   "Vector and scalar to not multiply correctly.");

  Scalar d = vtkm::dot(a, b);
  VTKM_TEST_ASSERT(test_equal(d, Scalar(60)), "dot(Vector4) wrong");


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
  const Vector c(2,4,6,7);
  VTKM_TEST_ASSERT((c < a),  "operator< wrong");

  VTKM_TEST_ASSERT( !(c == a), "operator == wrong");
  VTKM_TEST_ASSERT( !(a == c), "operator == wrong");

  VTKM_TEST_ASSERT( (c != a), "operator != wrong");
  VTKM_TEST_ASSERT( (a != c), "operator != wrong");
}

template<typename Scalar>
void TypeTest(Scalar)
{
  Scalar a = 4;
  Scalar b = 2;

  Scalar plus = Scalar(a + b);
  if (plus != 6)
  {
    VTKM_TEST_FAIL("Scalars do not add correctly.");
  }

  Scalar minus = Scalar(a - b);
  if (minus != 2)
  {
    VTKM_TEST_FAIL("Scalars to not subtract correctly.");
  }

  Scalar mult = Scalar(a * b);
  if (mult != 8)
  {
    VTKM_TEST_FAIL("Scalars to not multiply correctly.");
  }

  Scalar div = Scalar(a / b);
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

struct TypeTestFunctor
{
  template <typename T> void operator()(const T&) const
  {
    TypeTest(T());
  }
};

void TestTypes()
{
  CheckTypeSizes();

  vtkm::testing::Testing::TryAllTypes(TypeTestFunctor());

  //try with some custom tuple types
  TypeTestFunctor()( vtkm::Vec<vtkm::FloatDefault,6>() );
  TypeTestFunctor()( vtkm::Vec<vtkm::Id,4>() );
  TypeTestFunctor()( vtkm::Vec<unsigned char,4>() );
  TypeTestFunctor()( vtkm::Vec<vtkm::Id,1>() );
  TypeTestFunctor()( vtkm::Vec<vtkm::Float64,1>() );
}

} // anonymous namespace

int UnitTestTypes(int, char *[])
{
  return vtkm::testing::Testing::Run(TestTypes);
}
