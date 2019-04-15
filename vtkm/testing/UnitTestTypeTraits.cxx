//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/TypeTraits.h>

#include <vtkm/VecTraits.h>

#include <vtkm/testing/Testing.h>

namespace
{

struct TypeTraitTest
{
  template <typename T>
  void operator()(T t) const
  {
    // If you get compiler errors here, it could be a TypeTraits instance
    // has missing or malformed tags.
    this->TestDimensionality(t, typename vtkm::TypeTraits<T>::DimensionalityTag());
    this->TestNumeric(t, typename vtkm::TypeTraits<T>::NumericTag());
  }

private:
  template <typename T>
  void TestDimensionality(T, vtkm::TypeTraitsScalarTag) const
  {
    std::cout << "  scalar" << std::endl;
    VTKM_TEST_ASSERT(vtkm::VecTraits<T>::NUM_COMPONENTS == 1,
                     "Scalar type does not have one component.");
  }
  template <typename T>
  void TestDimensionality(T, vtkm::TypeTraitsVectorTag) const
  {
    std::cout << "  vector" << std::endl;
    VTKM_TEST_ASSERT(vtkm::VecTraits<T>::NUM_COMPONENTS > 1,
                     "Vector type does not have multiple components.");
  }

  template <typename T>
  void TestNumeric(T, vtkm::TypeTraitsIntegerTag) const
  {
    std::cout << "  integer" << std::endl;
    using VT = typename vtkm::VecTraits<T>::ComponentType;
    VT value = VT(2.001);
    VTKM_TEST_ASSERT(value == 2, "Integer does not round to integer.");
  }
  template <typename T>
  void TestNumeric(T, vtkm::TypeTraitsRealTag) const
  {
    std::cout << "  real" << std::endl;
    using VT = typename vtkm::VecTraits<T>::ComponentType;
    VT value = VT(2.001);
    VTKM_TEST_ASSERT(test_equal(float(value), float(2.001)),
                     "Real does not hold floaing point number.");
  }
};

static void TestTypeTraits()
{
  TypeTraitTest test;
  vtkm::testing::Testing::TryTypes(test);
  std::cout << "vtkm::Vec<vtkm::FloatDefault, 5>" << std::endl;
  test(vtkm::Vec<vtkm::FloatDefault, 5>());
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestTypeTraits(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestTypeTraits, argc, argv);
}
