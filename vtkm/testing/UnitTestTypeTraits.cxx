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

#include <vtkm/TypeTraits.h>

#include <vtkm/VectorTraits.h>

#include <vtkm/testing/Testing.h>

namespace {

struct TypeTraitTest
{
  template <typename T> void operator()(T t) {
    // If you get compiler errors here, it could be a TypeTraits instance
    // has missing or malformed tags.
    this->TestDimensionality(t, typename vtkm::TypeTraits<T>::DimensionalityTag());
    this->TestNumeric(t, typename vtkm::TypeTraits<T>::NumericTag());
  }
private:

  template <typename T>
  void TestDimensionality(T, vtkm::TypeTraitsScalarTag) {
    std::cout << "  scalar" << std::endl;
    VTKM_TEST_ASSERT(vtkm::VectorTraits<T>::NUM_COMPONENTS == 1,
                    "Scalar type does not have one component.");
  }
  template <typename T>
  void TestDimensionality(T, vtkm::TypeTraitsVectorTag) {
    std::cout << "  vector" << std::endl;
    VTKM_TEST_ASSERT(vtkm::VectorTraits<T>::NUM_COMPONENTS > 1,
                    "Vector type does not have multiple components.");
  }

  template <typename T>
  void TestNumeric(T, vtkm::TypeTraitsIntegerTag) {
    std::cout << "  integer" << std::endl;
    typedef typename vtkm::VectorTraits<T>::ComponentType VT;
    VT value = VT(2.001);
    VTKM_TEST_ASSERT(value == 2, "Integer does not round to integer.");
  }
  template <typename T>
  void TestNumeric(T, vtkm::TypeTraitsRealTag) {
    std::cout << "  real" << std::endl;
    typedef typename vtkm::VectorTraits<T>::ComponentType VT;
    VT value = VT(2.001);
    VTKM_TEST_ASSERT(test_equal(float(value), float(2.001)),
                    "Real does not hold floaing point number.");
  }
};

static void TestTypeTraits()
{
  TypeTraitTest test;
  vtkm::testing::Testing::TryAllTypes(test);
  std::cout << "vtkm::Tuple<vtkm::Scalar, 5>" << std::endl;
  test(vtkm::Tuple<vtkm::Scalar, 5>());
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestTypeTraits(int, char *[])
{
  return vtkm::testing::Testing::Run(TestTypeTraits);
}
