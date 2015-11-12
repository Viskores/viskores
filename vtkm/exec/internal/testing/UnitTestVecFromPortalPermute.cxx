//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
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
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/exec/internal/VecFromPortalPermute.h>

#include <vtkm/VecVariable.h>

#include <vtkm/testing/Testing.h>

namespace UnitTestVecFromPortalPermuteNamespace {

static const vtkm::IdComponent ARRAY_SIZE = 10;

template<typename T>
void CheckType(T, T)
{
  // Check passes if this function is called correctly.
}

template<typename T>
class TestPortal
{
public:
  typedef T ValueType;

  VTKM_EXEC_EXPORT
  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VTKM_EXEC_EXPORT
  ValueType Get(vtkm::Id index) const { return TestValue(index, ValueType()); }
};

struct VecFromPortalPermuteTestFunctor
{
  template<typename T>
  void operator()(T) const
  {
    typedef TestPortal<T> PortalType;
    typedef vtkm::VecVariable<vtkm::Id,ARRAY_SIZE> IndexVecType;
    typedef vtkm::exec::internal::VecFromPortalPermute<IndexVecType,PortalType>
        VecType;
    typedef vtkm::TypeTraits<VecType> TTraits;
    typedef vtkm::VecTraits<VecType> VTraits;

    std::cout << "Checking VecFromPortal traits" << std::endl;

    // The statements will fail to compile if the traits is not working as
    // expected.
    CheckType(typename TTraits::DimensionalityTag(),
              vtkm::TypeTraitsVectorTag());
    CheckType(typename VTraits::ComponentType(), T());
    CheckType(typename VTraits::HasMultipleComponents(),
              vtkm::VecTraitsTagMultipleComponents());
    CheckType(typename VTraits::IsSizeStatic(),
              vtkm::VecTraitsTagSizeVariable());

    std::cout << "Checking VecFromPortal contents" << std::endl;

    PortalType portal;

    for (vtkm::Id offset = 0; offset < ARRAY_SIZE; offset++)
    {
      for (vtkm::IdComponent length = 0;
           2*length + offset < ARRAY_SIZE;
           length++)
      {
        IndexVecType indices;
        for (vtkm::IdComponent index = 0; index < length; index++)
        {
          indices.Append(offset + 2*index);
        }

        VecType vec(&indices, portal);

        VTKM_TEST_ASSERT(vec.GetNumberOfComponents() == length,
                         "Wrong length.");
        VTKM_TEST_ASSERT(VTraits::GetNumberOfComponents(vec) == length,
                         "Wrong length.");

        vtkm::Vec<T,ARRAY_SIZE> copyDirect;
        vec.CopyInto(copyDirect);

        vtkm::Vec<T,ARRAY_SIZE> copyTraits;
        VTraits::CopyInto(vec, copyTraits);

        for (vtkm::IdComponent index = 0; index < length; index++)
        {
          T expected = TestValue(2*index+offset, T());
          VTKM_TEST_ASSERT(test_equal(vec[index], expected), "Wrong value.");
          VTKM_TEST_ASSERT(
                test_equal(VTraits::GetComponent(vec, index), expected),
                "Wrong value.");
          VTKM_TEST_ASSERT(test_equal(copyDirect[index], expected),
                           "Wrong copied value.");
          VTKM_TEST_ASSERT(test_equal(copyTraits[index], expected),
                           "Wrong copied value.");
        }
      }
    }
  }
};

void VecFromPortalPermuteTest()
{
  vtkm::testing::Testing::TryTypes(VecFromPortalPermuteTestFunctor(),
                                   vtkm::TypeListTagCommon());
}

} // namespace UnitTestVecFromPortalPermuteNamespace

int UnitTestVecFromPortalPermute(int, char *[])
{
  return vtkm::testing::Testing::Run(
      UnitTestVecFromPortalPermuteNamespace::VecFromPortalPermuteTest);
}
