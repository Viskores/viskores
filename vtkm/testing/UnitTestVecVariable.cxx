//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/VecVariable.h>

#include <vtkm/testing/Testing.h>

namespace
{

struct VecVariableTestFunctor
{
  // You will get a compile fail if this does not pass
  template <typename NumericTag>
  void CheckNumericTag(NumericTag, NumericTag) const
  {
    std::cout << "NumericTag pass" << std::endl;
  }

  // You will get a compile fail if this does not pass
  void CheckDimensionalityTag(vtkm::TypeTraitsVectorTag) const
  {
    std::cout << "VectorTag pass" << std::endl;
  }

  // You will get a compile fail if this does not pass
  template <typename T>
  void CheckComponentType(T, T) const
  {
    std::cout << "ComponentType pass" << std::endl;
  }

  // You will get a compile fail if this does not pass
  void CheckHasMultipleComponents(vtkm::VecTraitsTagMultipleComponents) const
  {
    std::cout << "MultipleComponents pass" << std::endl;
  }

  // You will get a compile fail if this does not pass
  void CheckVariableSize(vtkm::VecTraitsTagSizeVariable) const
  {
    std::cout << "VariableSize" << std::endl;
  }

  template <typename T>
  void operator()(T) const
  {
    static constexpr vtkm::IdComponent SIZE = 5;
    using VecType = vtkm::Vec<T, SIZE>;
    using VecVariableType = vtkm::VecVariable<T, SIZE>;
    using TTraits = vtkm::TypeTraits<VecVariableType>;
    using VTraits = vtkm::VecTraits<VecVariableType>;

    std::cout << "Check NumericTag." << std::endl;
    this->CheckNumericTag(typename TTraits::NumericTag(),
                          typename vtkm::TypeTraits<T>::NumericTag());

    std::cout << "Check DimensionalityTag." << std::endl;
    this->CheckDimensionalityTag(typename TTraits::DimensionalityTag());

    std::cout << "Check ComponentType." << std::endl;
    this->CheckComponentType(typename VTraits::ComponentType(), T());

    std::cout << "Check MultipleComponents." << std::endl;
    this->CheckHasMultipleComponents(typename VTraits::HasMultipleComponents());

    std::cout << "Check VariableSize." << std::endl;
    this->CheckVariableSize(typename VTraits::IsSizeStatic());

    VecType source = TestValue(0, VecType());

    VecVariableType vec1(source);
    VecType vecCopy;
    vec1.CopyInto(vecCopy);
    VTKM_TEST_ASSERT(test_equal(vec1, vecCopy), "Bad init or copyinto.");

    vtkm::VecVariable<T, SIZE + 1> vec2;
    for (vtkm::IdComponent setIndex = 0; setIndex < SIZE; setIndex++)
    {
      VTKM_TEST_ASSERT(vec2.GetNumberOfComponents() == setIndex,
                       "Report wrong number of components");
      vec2.Append(source[setIndex]);
    }
    VTKM_TEST_ASSERT(test_equal(vec2, vec1), "Bad values from Append.");
  }
};

void TestVecVariable()
{
  vtkm::testing::Testing::TryTypes(VecVariableTestFunctor(), vtkm::TypeListFieldScalar());
}

} // anonymous namespace

int UnitTestVecVariable(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestVecVariable, argc, argv);
}
