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
#include <vtkm/testing/VecTraitsTests.h>

#include <vtkm/testing/Testing.h>

namespace {

static const vtkm::Id MAX_VECTOR_SIZE = 5;
static const vtkm::Id VecInit[MAX_VECTOR_SIZE] = { 42, 54, 67, 12, 78 };

struct TestVecTypeFunctor
{
  template <typename T> void operator()(const T&) const
  {
    typedef vtkm::VecTraits<T> Traits;
    typedef typename Traits::ComponentType ComponentType;
    VTKM_TEST_ASSERT(Traits::NUM_COMPONENTS <= MAX_VECTOR_SIZE,
                     "Need to update test for larger vectors.");
    T vector;
    for (vtkm::IdComponent index = 0; index < Traits::NUM_COMPONENTS; index++)
    {
      Traits::SetComponent(vector, index, ComponentType(VecInit[index]));
    }
    vtkm::testing::TestVecType(vector);
  }
};

void TestVecTraits()
{
  TestVecTypeFunctor test;
  vtkm::testing::Testing::TryAllTypes(test);
  std::cout << "vtkm::Vec<vtkm::Scalar, 5>" << std::endl;
  test(vtkm::Vec<vtkm::Scalar,5>());

  vtkm::testing::TestVecComponentsTag<vtkm::Id3>();
  vtkm::testing::TestVecComponentsTag<vtkm::Vector3>();
  vtkm::testing::TestVecComponentsTag<vtkm::Vector4>();
  vtkm::testing::TestScalarComponentsTag<vtkm::Id>();
  vtkm::testing::TestScalarComponentsTag<vtkm::Scalar>();
}

} // anonymous namespace

int UnitTestVecTraits(int, char *[])
{
  return vtkm::testing::Testing::Run(TestVecTraits);
}
