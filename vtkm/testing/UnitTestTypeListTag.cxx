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

#include <vtkm/TypeListTag.h>

#include <vtkm/Types.h>

#include <vtkm/testing/Testing.h>

#include <vector>

namespace {

enum TypeId {
  ID,
  ID2,
  ID3,
  SCALAR,
  VECTOR2,
  VECTOR3,
  VECTOR4
};

TypeId GetTypeId(vtkm::Id) { return ID; }
TypeId GetTypeId(vtkm::Id2) { return ID2; }
TypeId GetTypeId(vtkm::Id3) { return ID3; }
TypeId GetTypeId(vtkm::Scalar) { return SCALAR; }
TypeId GetTypeId(vtkm::Vector2) { return VECTOR2; }
TypeId GetTypeId(vtkm::Vector3) { return VECTOR3; }
TypeId GetTypeId(vtkm::Vector4) { return VECTOR4; }

struct TestFunctor
{
  std::vector<TypeId> FoundTypes;

  template<typename T>
  VTKM_CONT_EXPORT
  void operator()(T) {
    this->FoundTypes.push_back(GetTypeId(T()));
  }
};

template<vtkm::IdComponent N>
void CheckSame(const vtkm::Vec<TypeId,N> &expected,
               const std::vector<TypeId> &found)
{
  VTKM_TEST_ASSERT(static_cast<vtkm::IdComponent>(found.size()) == N,
                   "Got wrong number of items.");

  for (vtkm::IdComponent index = 0; index < N; index++)
  {
    VTKM_TEST_ASSERT(expected[index] == found[index],
                     "Got wrong type.");
  }
}

template<vtkm::IdComponent N, typename ListTag>
void TryList(const vtkm::Vec<TypeId,N> &expected, ListTag)
{
  TestFunctor functor;
  vtkm::ListForEach(functor, ListTag());
  CheckSame(expected, functor.FoundTypes);
}

void TestLists()
{
  std::cout << "TypeListTagId" << std::endl;
  TryList(vtkm::Vec<TypeId,1>(ID), vtkm::TypeListTagId());

  std::cout << "TypeListTagId2" << std::endl;
  TryList(vtkm::Vec<TypeId,1>(ID2), vtkm::TypeListTagId2());

  std::cout << "TypeListTagId3" << std::endl;
  TryList(vtkm::Vec<TypeId,1>(ID3), vtkm::TypeListTagId3());

  std::cout << "TypeListTagScalar" << std::endl;
  TryList(vtkm::Vec<TypeId,1>(SCALAR), vtkm::TypeListTagScalar());

  std::cout << "TypeListTagVector2" << std::endl;
  TryList(vtkm::Vec<TypeId,1>(VECTOR2), vtkm::TypeListTagVector2());

  std::cout << "TypeListTagVector3" << std::endl;
  TryList(vtkm::Vec<TypeId,1>(VECTOR3), vtkm::TypeListTagVector3());

  std::cout << "TypeListTagVector4" << std::endl;
  TryList(vtkm::Vec<TypeId,1>(VECTOR4), vtkm::TypeListTagVector4());

  std::cout << "TypeListTagIndex" << std::endl;
  TryList(vtkm::Vec<TypeId,3>(ID,ID2,ID3), vtkm::TypeListTagIndex());

  std::cout << "TypeListTagReal" << std::endl;
  TryList(vtkm::Vec<TypeId,4>(SCALAR,VECTOR2,VECTOR3,VECTOR4),
          vtkm::TypeListTagReal());

  std::cout << "TypeListTagCommon" << std::endl;
  TryList(vtkm::Vec<TypeId,3>(ID,SCALAR,VECTOR3), vtkm::TypeListTagCommon());

  std::cout << "TypeListTagAll" << std::endl;
  vtkm::Vec<TypeId,7> allTags;
  allTags[0] = ID;
  allTags[1] = ID2;
  allTags[2] = ID3;
  allTags[3] = SCALAR;
  allTags[4] = VECTOR2;
  allTags[5] = VECTOR3;
  allTags[6] = VECTOR4;
  TryList(allTags, vtkm::TypeListTagAll());
}

} // anonymous namespace

int UnitTestTypeListTag(int, char *[])
{
  return vtkm::testing::Testing::Run(TestLists);
}
