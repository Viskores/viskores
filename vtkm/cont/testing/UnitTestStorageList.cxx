//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/StorageList.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace
{

enum TypeId
{
  BASIC
};

TypeId GetTypeId(vtkm::cont::StorageTagBasic)
{
  return BASIC;
}

struct TestFunctor
{
  std::vector<TypeId> FoundTypes;

  template <typename T>
  VTKM_CONT void operator()(T)
  {
    this->FoundTypes.push_back(GetTypeId(T()));
  }
};

template <vtkm::IdComponent N>
void CheckSame(const vtkm::Vec<TypeId, N>& expected, const std::vector<TypeId>& found)
{
  VTKM_TEST_ASSERT(static_cast<vtkm::IdComponent>(found.size()) == N, "Got wrong number of items.");

  for (vtkm::IdComponent index = 0; index < N; index++)
  {
    vtkm::UInt32 i = static_cast<vtkm::UInt32>(index);
    VTKM_TEST_ASSERT(expected[index] == found[i], "Got wrong type.");
  }
}

template <vtkm::IdComponent N, typename List>
void TryList(const vtkm::Vec<TypeId, N>& expected, List)
{
  TestFunctor functor;
  vtkm::ListForEach(functor, List());
  CheckSame(expected, functor.FoundTypes);
}

void TestLists()
{
  std::cout << "StorageListBasic" << std::endl;
  TryList(vtkm::Vec<TypeId, 1>(BASIC), vtkm::cont::StorageListBasic());
}

} // anonymous namespace

int UnitTestStorageList(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestLists, argc, argv);
}
