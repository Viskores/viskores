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

#include <vtkm/cont/ContainerListTag.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace {

enum TypeId {
  BASIC
};

TypeId GetTypeId(vtkm::cont::ArrayContainerControlTagBasic) { return BASIC; }

struct TestFunctor
{
  std::vector<TypeId> FoundTypes;

  template<typename T>
  VTKM_CONT_EXPORT
  void operator()(T) {
    this->FoundTypes.push_back(GetTypeId(T()));
  }
};

template<int N>
void CheckSame(const vtkm::Tuple<TypeId,N> &expected,
               const std::vector<TypeId> &found)
{
  VTKM_TEST_ASSERT(static_cast<int>(found.size()) == N,
                   "Got wrong number of items.");

  for (int index = 0; index < N; index++)
  {
    VTKM_TEST_ASSERT(expected[index] == found[index],
                     "Got wrong type.");
  }
}

template<int N, typename ListTag>
void TryList(const vtkm::Tuple<TypeId,N> &expected, ListTag)
{
  TestFunctor functor;
  vtkm::ListForEach(functor, ListTag());
  CheckSame(expected, functor.FoundTypes);
}

void TestLists()
{
  std::cout << "ContainerListTagBasic" << std::endl;
  TryList(vtkm::Tuple<TypeId,1>(BASIC), vtkm::cont::ContainerListTagBasic());
}

} // anonymous namespace

int UnitTestContainerListTag(int, char *[])
{
  return vtkm::testing::Testing::Run(TestLists);
}
