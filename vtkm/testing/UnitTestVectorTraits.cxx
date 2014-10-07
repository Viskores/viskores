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
#include <vtkm/testing/VectorTraitsTests.h>

#include <vtkm/testing/Testing.h>

namespace {

static const vtkm::Id MAX_VECTOR_SIZE = 5;
static const vtkm::Id VectorInit[MAX_VECTOR_SIZE] = { 42, 54, 67, 12, 78 };

struct TestVectorTypeFunctor
{
  template <typename T> void operator()(const T&) const
  {
    typedef vtkm::VectorTraits<T> Traits;
    typedef typename Traits::ComponentType ComponentType;
    VTKM_TEST_ASSERT(Traits::NUM_COMPONENTS <= MAX_VECTOR_SIZE,
                     "Need to update test for larger vectors.");
    T vector;
    for (vtkm::IdComponent index = 0; index < Traits::NUM_COMPONENTS; index++)
    {
      Traits::SetComponent(vector, index, ComponentType(VectorInit[index]));
    }
    vtkm::testing::TestVectorType(vector);
  }
};

void TestVectorTraits()
{
  TestVectorTypeFunctor test;
  vtkm::testing::Testing::TryAllTypes(test);
  std::cout << "vtkm::Vec<vtkm::Scalar, 5>" << std::endl;
  test(vtkm::Vec<vtkm::Scalar,5>());

  vtkm::testing::TestVectorComponentsTag<vtkm::Id3>();
  vtkm::testing::TestVectorComponentsTag<vtkm::Vector3>();
  vtkm::testing::TestVectorComponentsTag<vtkm::Vector4>();
  vtkm::testing::TestScalarComponentsTag<vtkm::Id>();
  vtkm::testing::TestScalarComponentsTag<vtkm::Scalar>();
}

} // anonymous namespace

int UnitTestVectorTraits(int, char *[])
{
  return vtkm::testing::Testing::Run(TestVectorTraits);
}
