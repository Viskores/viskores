//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/TypeListTag.h>

#include <vtkm/Types.h>

#include <vtkm/testing/Testing.h>

#include <set>
#include <string>

namespace
{

class TypeSet
{
  using NameSetType = std::set<std::string>;
  NameSetType NameSet;

public:
  template <typename T>
  void AddExpected(T)
  {
    this->NameSet.insert(vtkm::testing::TypeName<T>::Name());
  }

  template <typename T>
  void Found(T)
  {
    std::string name = vtkm::testing::TypeName<T>::Name();
    //std::cout << "  found " << name << std::endl;
    NameSetType::iterator typeLocation = this->NameSet.find(name);
    if (typeLocation != this->NameSet.end())
    {
      // This type is expected. Remove it to mark it found.
      this->NameSet.erase(typeLocation);
    }
    else
    {
      std::cout << "**** Did not expect to get type " << name << std::endl;
      VTKM_TEST_FAIL("Got unexpected type.");
    }
  }

  void CheckFound()
  {
    for (NameSetType::iterator typeP = this->NameSet.begin(); typeP != this->NameSet.end(); typeP++)
    {
      std::cout << "**** Failed to find " << *typeP << std::endl;
    }
    VTKM_TEST_ASSERT(this->NameSet.empty(), "List did not call functor on all expected types.");
  }
};

struct TestFunctor
{
  TypeSet ExpectedTypes;

  TestFunctor(const TypeSet& expectedTypes)
    : ExpectedTypes(expectedTypes)
  {
  }

  template <typename T>
  VTKM_CONT void operator()(T)
  {
    this->ExpectedTypes.Found(T());
  }
};

template <typename ListTag>
void TryList(const TypeSet& expected, ListTag)
{
  TestFunctor functor(expected);
  vtkm::ListForEach(functor, ListTag());
  functor.ExpectedTypes.CheckFound();
}

void TestLists()
{
  std::cout << "TypeListTagId" << std::endl;
  TypeSet id;
  id.AddExpected(vtkm::Id());
  TryList(id, vtkm::TypeListTagId());

  std::cout << "TypeListTagId2" << std::endl;
  TypeSet id2;
  id2.AddExpected(vtkm::Id2());
  TryList(id2, vtkm::TypeListTagId2());

  std::cout << "TypeListTagId3" << std::endl;
  TypeSet id3;
  id3.AddExpected(vtkm::Id3());
  TryList(id3, vtkm::TypeListTagId3());

  std::cout << "TypeListTagIndex" << std::endl;
  TypeSet index;
  index.AddExpected(vtkm::Id());
  index.AddExpected(vtkm::Id2());
  index.AddExpected(vtkm::Id3());
  TryList(index, vtkm::TypeListTagIndex());

  std::cout << "TypeListTagFieldScalar" << std::endl;
  TypeSet scalar;
  scalar.AddExpected(vtkm::Float32());
  scalar.AddExpected(vtkm::Float64());
  TryList(scalar, vtkm::TypeListTagFieldScalar());

  std::cout << "TypeListTagFieldVec2" << std::endl;
  TypeSet vec2;
  vec2.AddExpected(vtkm::Vec<vtkm::Float32, 2>());
  vec2.AddExpected(vtkm::Vec<vtkm::Float64, 2>());
  TryList(vec2, vtkm::TypeListTagFieldVec2());

  std::cout << "TypeListTagFieldVec3" << std::endl;
  TypeSet vec3;
  vec3.AddExpected(vtkm::Vec<vtkm::Float32, 3>());
  vec3.AddExpected(vtkm::Vec<vtkm::Float64, 3>());
  TryList(vec3, vtkm::TypeListTagFieldVec3());

  std::cout << "TypeListTagFieldVec4" << std::endl;
  TypeSet vec4;
  vec4.AddExpected(vtkm::Vec<vtkm::Float32, 4>());
  vec4.AddExpected(vtkm::Vec<vtkm::Float64, 4>());
  TryList(vec4, vtkm::TypeListTagFieldVec4());

  std::cout << "TypeListTagField" << std::endl;
  TypeSet field;
  field.AddExpected(vtkm::Float32());
  field.AddExpected(vtkm::Float64());
  field.AddExpected(vtkm::Vec<vtkm::Float32, 2>());
  field.AddExpected(vtkm::Vec<vtkm::Float64, 2>());
  field.AddExpected(vtkm::Vec<vtkm::Float32, 3>());
  field.AddExpected(vtkm::Vec<vtkm::Float64, 3>());
  field.AddExpected(vtkm::Vec<vtkm::Float32, 4>());
  field.AddExpected(vtkm::Vec<vtkm::Float64, 4>());
  TryList(field, vtkm::TypeListTagField());

  std::cout << "TypeListTagCommon" << std::endl;
  TypeSet common;
  common.AddExpected(vtkm::Float32());
  common.AddExpected(vtkm::Float64());
  common.AddExpected(vtkm::Int32());
  common.AddExpected(vtkm::Int64());
  common.AddExpected(vtkm::Vec<vtkm::Float32, 3>());
  common.AddExpected(vtkm::Vec<vtkm::Float64, 3>());
  TryList(common, vtkm::TypeListTagCommon());

  std::cout << "TypeListTagScalarAll" << std::endl;
  TypeSet scalarsAll;
  scalarsAll.AddExpected(vtkm::Float32());
  scalarsAll.AddExpected(vtkm::Float64());
  scalarsAll.AddExpected(vtkm::Int8());
  scalarsAll.AddExpected(vtkm::UInt8());
  scalarsAll.AddExpected(vtkm::Int16());
  scalarsAll.AddExpected(vtkm::UInt16());
  scalarsAll.AddExpected(vtkm::Int32());
  scalarsAll.AddExpected(vtkm::UInt32());
  scalarsAll.AddExpected(vtkm::Int64());
  scalarsAll.AddExpected(vtkm::UInt64());
  TryList(scalarsAll, vtkm::TypeListTagScalarAll());

  std::cout << "TypeListTagVecCommon" << std::endl;
  TypeSet vecCommon;
  vecCommon.AddExpected(vtkm::Vec<vtkm::Float32, 2>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::Float64, 2>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::UInt8, 2>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::Int32, 2>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::Int64, 2>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::Float32, 3>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::Float64, 3>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::UInt8, 3>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::Int32, 3>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::Int64, 3>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::Float32, 4>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::Float64, 4>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::UInt8, 4>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::Int32, 4>());
  vecCommon.AddExpected(vtkm::Vec<vtkm::Int64, 4>());
  TryList(vecCommon, vtkm::TypeListTagVecCommon());

  std::cout << "TypeListTagVecAll" << std::endl;
  TypeSet vecAll;
  vecAll.AddExpected(vtkm::Vec<vtkm::Float32, 2>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Float64, 2>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int8, 2>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int16, 2>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int32, 2>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int64, 2>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt8, 2>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt16, 2>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt32, 2>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt64, 2>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Float32, 3>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Float64, 3>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int8, 3>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int16, 3>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int32, 3>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int64, 3>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt8, 3>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt16, 3>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt32, 3>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt64, 3>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Float32, 4>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Float64, 4>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int8, 4>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int16, 4>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int32, 4>());
  vecAll.AddExpected(vtkm::Vec<vtkm::Int64, 4>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt8, 4>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt16, 4>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt32, 4>());
  vecAll.AddExpected(vtkm::Vec<vtkm::UInt64, 4>());
  TryList(vecAll, vtkm::TypeListTagVecAll());

  std::cout << "TypeListTagAll" << std::endl;
  TypeSet all;
  all.AddExpected(vtkm::Float32());
  all.AddExpected(vtkm::Float64());
  all.AddExpected(vtkm::Int8());
  all.AddExpected(vtkm::UInt8());
  all.AddExpected(vtkm::Int16());
  all.AddExpected(vtkm::UInt16());
  all.AddExpected(vtkm::Int32());
  all.AddExpected(vtkm::UInt32());
  all.AddExpected(vtkm::Int64());
  all.AddExpected(vtkm::UInt64());
  all.AddExpected(vtkm::Vec<vtkm::Float32, 2>());
  all.AddExpected(vtkm::Vec<vtkm::Float64, 2>());
  all.AddExpected(vtkm::Vec<vtkm::Int8, 2>());
  all.AddExpected(vtkm::Vec<vtkm::Int16, 2>());
  all.AddExpected(vtkm::Vec<vtkm::Int32, 2>());
  all.AddExpected(vtkm::Vec<vtkm::Int64, 2>());
  all.AddExpected(vtkm::Vec<vtkm::UInt8, 2>());
  all.AddExpected(vtkm::Vec<vtkm::UInt16, 2>());
  all.AddExpected(vtkm::Vec<vtkm::UInt32, 2>());
  all.AddExpected(vtkm::Vec<vtkm::UInt64, 2>());
  all.AddExpected(vtkm::Vec<vtkm::Float32, 3>());
  all.AddExpected(vtkm::Vec<vtkm::Float64, 3>());
  all.AddExpected(vtkm::Vec<vtkm::Int8, 3>());
  all.AddExpected(vtkm::Vec<vtkm::Int16, 3>());
  all.AddExpected(vtkm::Vec<vtkm::Int32, 3>());
  all.AddExpected(vtkm::Vec<vtkm::Int64, 3>());
  all.AddExpected(vtkm::Vec<vtkm::UInt8, 3>());
  all.AddExpected(vtkm::Vec<vtkm::UInt16, 3>());
  all.AddExpected(vtkm::Vec<vtkm::UInt32, 3>());
  all.AddExpected(vtkm::Vec<vtkm::UInt64, 3>());
  all.AddExpected(vtkm::Vec<vtkm::Float32, 4>());
  all.AddExpected(vtkm::Vec<vtkm::Float64, 4>());
  all.AddExpected(vtkm::Vec<vtkm::Int8, 4>());
  all.AddExpected(vtkm::Vec<vtkm::Int16, 4>());
  all.AddExpected(vtkm::Vec<vtkm::Int32, 4>());
  all.AddExpected(vtkm::Vec<vtkm::Int64, 4>());
  all.AddExpected(vtkm::Vec<vtkm::UInt8, 4>());
  all.AddExpected(vtkm::Vec<vtkm::UInt16, 4>());
  all.AddExpected(vtkm::Vec<vtkm::UInt32, 4>());
  all.AddExpected(vtkm::Vec<vtkm::UInt64, 4>());
  TryList(all, vtkm::TypeListTagAll());
}

} // anonymous namespace

int UnitTestTypeListTag(int, char* [])
{
  return vtkm::testing::Testing::Run(TestLists);
}
