//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#include <vtkm/VectorAnalysis.h>

#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

#include <vtkm/testing/Testing.h>

#include <math.h>

namespace {

namespace internal{

template<typename VectorType>
typename vtkm::VecTraits<VectorType>::ComponentType
MyMag(const VectorType &vt)
{
  typedef vtkm::VecTraits<VectorType> Traits;
  double total = 0.0;
  for (vtkm::IdComponent index = 0; index < Traits::NUM_COMPONENTS; ++index)
  {
    total += Traits::GetComponent(vt,index) * Traits::GetComponent(vt,index);
  }
  return static_cast<typename Traits::ComponentType>(sqrt(total));
}

template<typename VectorType>
VectorType MyNormal(const VectorType& vt)
{
  typedef vtkm::VecTraits<VectorType> Traits;
  typename Traits::ComponentType mag = internal::MyMag(vt);
  VectorType temp = vt;
  for (vtkm::IdComponent index = 0; index < Traits::NUM_COMPONENTS; ++index)
  {
    Traits::SetComponent(temp, index, Traits::GetComponent(vt,index)/mag);
  }
  return temp;
}

template<typename T,typename W>
T MyLerp (const T &a, const T &b, const W &w)
{
  return (W(1)-w)*a + w*b;
}

}

template<typename VectorType>
void TestVector(const VectorType& vector)
{
  typedef typename vtkm::VecTraits<VectorType>::ComponentType ComponentType;

  std::cout << "Testing " << vector << std::endl;

  //to do have to implement a norm and normalized call to verify the math ones
  //against
  std::cout << "  Magnitude" << std::endl;
  ComponentType magnitude = vtkm::Magnitude(vector);
  ComponentType magnitudeCompare = internal::MyMag(vector);
  VTKM_TEST_ASSERT(test_equal(magnitude, magnitudeCompare),
                   "Magnitude failed test.");

  std::cout << "  Magnitude squared" << std::endl;
  ComponentType magnitudeSquared = vtkm::MagnitudeSquared(vector);
  VTKM_TEST_ASSERT(test_equal(magnitude*magnitude, magnitudeSquared),
                   "Magnitude squared test failed.");

  if (magnitudeSquared > 0)
    {
    std::cout << "  Reciprocal magnitude" << std::endl;
    ComponentType rmagnitude = vtkm::RMagnitude(vector);
    VTKM_TEST_ASSERT(test_equal(1/magnitude, rmagnitude),
                     "Reciprical magnitude failed.");

    std::cout << "  Normal" << std::endl;
    VTKM_TEST_ASSERT(test_equal(vtkm::Normal(vector),
                                internal::MyNormal(vector)),
                     "Normalized vector failed test.");

    std::cout << "  Normalize" << std::endl;
    VectorType normalizedVector = vector;
    vtkm::Normalize(normalizedVector);
    VTKM_TEST_ASSERT(test_equal(normalizedVector, internal::MyNormal(vector)),
                     "Inplace Normalized vector failed test.");
    }
}

template <typename VectorType>
void TestLerp(const VectorType& a,
              const VectorType& b,
              const VectorType& w,
              const typename vtkm::VecTraits<VectorType>::ComponentType& wS)
{
  std::cout << "Linear interpolation: " << a << "-" << b << ": " << w
            << std::endl;
  VectorType vtkmLerp = vtkm::Lerp(a, b, w);
  VectorType otherLerp = internal::MyLerp(a, b, w);
  VTKM_TEST_ASSERT(test_equal(vtkmLerp, otherLerp),
                   "Vectors with Vector weight do not lerp() correctly");

  std::cout << "Linear interpolation: " << a << "-" << b << ": " << wS
            << std::endl;
  VectorType lhsS = internal::MyLerp(a,b,wS);
  VectorType rhsS = vtkm::Lerp(a,b,wS);
  VTKM_TEST_ASSERT(test_equal(lhsS, rhsS),
                   "Vectors with Scalar weight do not lerp() correctly");
}

template<typename T>
void TestCross(const vtkm::Vec<T,3> &x, const vtkm::Vec<T,3> &y)
{
  std::cout << "Testing " << x << " x " << y << std::endl;

  typedef vtkm::Vec<T,3> Vec3;
  Vec3 cross = vtkm::Cross(x, y);

  std::cout << "  = " << cross << std::endl;

  std::cout << "  Orthogonality" << std::endl;
  // The cross product result should be perpendicular to input vectors.
  VTKM_TEST_ASSERT(test_equal(vtkm::dot(cross,x), T(0.0)),
                   "Cross product not perpendicular.");
  VTKM_TEST_ASSERT(test_equal(vtkm::dot(cross,y), T(0.0)),
                   "Cross product not perpendicular.");

  std::cout << "  Length" << std::endl;
  // The length of cross product should be the lengths of the input vectors
  // times the sin of the angle between them.
  T sinAngle =
      vtkm::Magnitude(cross) * vtkm::RMagnitude(x) * vtkm::RMagnitude(y);

  // The dot product is likewise the lengths of the input vectors times the
  // cos of the angle between them.
  T cosAngle = vtkm::dot(x,y) * vtkm::RMagnitude(x) * vtkm::RMagnitude(y);

  // Test that these are the actual sin and cos of the same angle with a
  // basic trigonometric identity.
  VTKM_TEST_ASSERT(test_equal(sinAngle*sinAngle + cosAngle*cosAngle, T(1.0)),
                   "Bad cross product length.");

  std::cout << "  Triangle normal" << std::endl;
  // Test finding the normal to a triangle (similar to cross product).
  Vec3 normal = vtkm::TriangleNormal(x, y, Vec3(0, 0, 0));
  VTKM_TEST_ASSERT(test_equal(vtkm::dot(normal, x-y), T(0.0)),
                   "Triangle normal is not really normal.");
}

struct TestLinearFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    typedef vtkm::VecTraits<T> Traits;
    const vtkm::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;
    typedef typename Traits::ComponentType ComponentType;

    T zeroVector = T(ComponentType(0));
    T normalizedVector = T(vtkm::RSqrt(ComponentType(NUM_COMPONENTS)));
    T posVec = TestValue(1, T());
    T negVec = -TestValue(2, T());

    TestVector(zeroVector);
    TestVector(normalizedVector);
    TestVector(posVec);
    TestVector(negVec);

    T weight(ComponentType(0.5));
    ComponentType weightS(0.5);
    TestLerp(zeroVector,normalizedVector,weight,weightS);
    TestLerp(zeroVector,posVec,weight,weightS);
    TestLerp(zeroVector,negVec,weight,weightS);

    TestLerp(normalizedVector,zeroVector,weight,weightS);
    TestLerp(normalizedVector,posVec,weight,weightS);
    TestLerp(normalizedVector,negVec,weight,weightS);

    TestLerp(posVec,zeroVector,weight,weightS);
    TestLerp(posVec,normalizedVector,weight,weightS);
    TestLerp(posVec,negVec,weight,weightS);

    TestLerp(negVec,zeroVector,weight,weightS);
    TestLerp(negVec,normalizedVector,weight,weightS);
    TestLerp(negVec,posVec,weight,weightS);
  }
};

struct TestCrossFunctor
{
  template<typename VectorType>
  void operator()(const VectorType &) const
  {
    TestCross(VectorType(1.0f,0.0f,0.0f), VectorType(0.0f,1.0f,0.0f));
    TestCross(VectorType(1.0f,2.0f,3.0f), VectorType(-3.0f,-1.0f,1.0f));
    TestCross(VectorType(0.0f,0.0f,1.0f), VectorType(0.001f,0.01f,2.0f));
  }
};

void TestVectorAnalysis()
{
  vtkm::testing::Testing::TryTypes(TestLinearFunctor(),
                                   vtkm::TypeListTagField());
  vtkm::testing::Testing::TryTypes(TestCrossFunctor(),
                                   vtkm::TypeListTagFieldVec3());
}

} // anonymous namespace

int UnitTestVectorAnalysis(int, char *[])
{
  return vtkm::testing::Testing::Run(TestVectorAnalysis);
}
