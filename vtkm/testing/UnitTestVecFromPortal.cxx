//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/VecFromPortal.h>

#include <vtkm/testing/Testing.h>

namespace UnitTestVecFromPortalNamespace
{

static constexpr vtkm::IdComponent ARRAY_SIZE = 10;

template <typename T>
void CheckType(T, T)
{
  // Check passes if this function is called correctly.
}

template <typename T>
class TestPortal
{
public:
  using ValueType = T;

  VTKM_EXEC
  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VTKM_EXEC
  ValueType Get(vtkm::Id index) const { return TestValue(index, ValueType()); }
};

struct VecFromPortalTestFunctor
{
  template <typename T>
  void operator()(T) const
  {
    using PortalType = TestPortal<T>;
    using VecType = vtkm::VecFromPortal<PortalType>;
    using TTraits = vtkm::TypeTraits<VecType>;
    using VTraits = vtkm::VecTraits<VecType>;

    std::cout << "Checking VecFromPortal traits" << std::endl;

    // The statements will fail to compile if the traits is not working as
    // expected.
    CheckType(typename TTraits::DimensionalityTag(), vtkm::TypeTraitsVectorTag());
    CheckType(typename VTraits::ComponentType(), T());
    CheckType(typename VTraits::HasMultipleComponents(), vtkm::VecTraitsTagMultipleComponents());
    CheckType(typename VTraits::IsSizeStatic(), vtkm::VecTraitsTagSizeVariable());

    std::cout << "Checking VecFromPortal contents" << std::endl;

    PortalType portal;

    for (vtkm::Id offset = 0; offset < ARRAY_SIZE; offset++)
    {
      for (vtkm::IdComponent length = 0; length < ARRAY_SIZE - offset; length++)
      {
        VecType vec(portal, length, offset);

        VTKM_TEST_ASSERT(vec.GetNumberOfComponents() == length, "Wrong length.");
        VTKM_TEST_ASSERT(VTraits::GetNumberOfComponents(vec) == length, "Wrong length.");

        vtkm::Vec<T, ARRAY_SIZE> copyDirect;
        vec.CopyInto(copyDirect);

        vtkm::Vec<T, ARRAY_SIZE> copyTraits;
        VTraits::CopyInto(vec, copyTraits);

        for (vtkm::IdComponent index = 0; index < length; index++)
        {
          T expected = TestValue(index + offset, T());
          VTKM_TEST_ASSERT(test_equal(vec[index], expected), "Wrong value.");
          VTKM_TEST_ASSERT(test_equal(VTraits::GetComponent(vec, index), expected), "Wrong value.");
          VTKM_TEST_ASSERT(test_equal(copyDirect[index], expected), "Wrong copied value.");
          VTKM_TEST_ASSERT(test_equal(copyTraits[index], expected), "Wrong copied value.");
        }
      }
    }
  }
};

void VecFromPortalTest()
{
  vtkm::testing::Testing::TryTypes(VecFromPortalTestFunctor(), vtkm::TypeListCommon());
}

} // namespace UnitTestVecFromPortalNamespace

int UnitTestVecFromPortal(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(UnitTestVecFromPortalNamespace::VecFromPortalTest, argc, argv);
}
