//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// This tests deprecated code until it is deleted.

#include <vtkm/TypeListTag.h>

#include <vtkm/Types.h>

#include <vtkm/testing/Testing.h>

#include <set>
#include <string>

VTKM_DEPRECATED_SUPPRESS_BEGIN

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
  vec2.AddExpected(vtkm::Vec2f_32());
  vec2.AddExpected(vtkm::Vec2f_64());
  TryList(vec2, vtkm::TypeListTagFieldVec2());

  std::cout << "TypeListTagFieldVec3" << std::endl;
  TypeSet vec3;
  vec3.AddExpected(vtkm::Vec3f_32());
  vec3.AddExpected(vtkm::Vec3f_64());
  TryList(vec3, vtkm::TypeListTagFieldVec3());

  std::cout << "TypeListTagFieldVec4" << std::endl;
  TypeSet vec4;
  vec4.AddExpected(vtkm::Vec4f_32());
  vec4.AddExpected(vtkm::Vec4f_64());
  TryList(vec4, vtkm::TypeListTagFieldVec4());

  std::cout << "TypeListTagField" << std::endl;
  TypeSet field;
  field.AddExpected(vtkm::Float32());
  field.AddExpected(vtkm::Float64());
  field.AddExpected(vtkm::Vec2f_32());
  field.AddExpected(vtkm::Vec2f_64());
  field.AddExpected(vtkm::Vec3f_32());
  field.AddExpected(vtkm::Vec3f_64());
  field.AddExpected(vtkm::Vec4f_32());
  field.AddExpected(vtkm::Vec4f_64());
  TryList(field, vtkm::TypeListTagField());

  std::cout << "TypeListTagCommon" << std::endl;
  TypeSet common;
  common.AddExpected(vtkm::Float32());
  common.AddExpected(vtkm::Float64());
  common.AddExpected(vtkm::UInt8());
  common.AddExpected(vtkm::Int32());
  common.AddExpected(vtkm::Int64());
  common.AddExpected(vtkm::Vec3f_32());
  common.AddExpected(vtkm::Vec3f_64());
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
  vecCommon.AddExpected(vtkm::Vec2f_32());
  vecCommon.AddExpected(vtkm::Vec2f_64());
  vecCommon.AddExpected(vtkm::Vec2ui_8());
  vecCommon.AddExpected(vtkm::Vec2i_32());
  vecCommon.AddExpected(vtkm::Vec2i_64());
  vecCommon.AddExpected(vtkm::Vec3f_32());
  vecCommon.AddExpected(vtkm::Vec3f_64());
  vecCommon.AddExpected(vtkm::Vec3ui_8());
  vecCommon.AddExpected(vtkm::Vec3i_32());
  vecCommon.AddExpected(vtkm::Vec3i_64());
  vecCommon.AddExpected(vtkm::Vec4f_32());
  vecCommon.AddExpected(vtkm::Vec4f_64());
  vecCommon.AddExpected(vtkm::Vec4ui_8());
  vecCommon.AddExpected(vtkm::Vec4i_32());
  vecCommon.AddExpected(vtkm::Vec4i_64());
  TryList(vecCommon, vtkm::TypeListTagVecCommon());

  std::cout << "TypeListTagVecAll" << std::endl;
  TypeSet vecAll;
  vecAll.AddExpected(vtkm::Vec2f_32());
  vecAll.AddExpected(vtkm::Vec2f_64());
  vecAll.AddExpected(vtkm::Vec2i_8());
  vecAll.AddExpected(vtkm::Vec2i_16());
  vecAll.AddExpected(vtkm::Vec2i_32());
  vecAll.AddExpected(vtkm::Vec2i_64());
  vecAll.AddExpected(vtkm::Vec2ui_8());
  vecAll.AddExpected(vtkm::Vec2ui_16());
  vecAll.AddExpected(vtkm::Vec2ui_32());
  vecAll.AddExpected(vtkm::Vec2ui_64());
  vecAll.AddExpected(vtkm::Vec3f_32());
  vecAll.AddExpected(vtkm::Vec3f_64());
  vecAll.AddExpected(vtkm::Vec3i_8());
  vecAll.AddExpected(vtkm::Vec3i_16());
  vecAll.AddExpected(vtkm::Vec3i_32());
  vecAll.AddExpected(vtkm::Vec3i_64());
  vecAll.AddExpected(vtkm::Vec3ui_8());
  vecAll.AddExpected(vtkm::Vec3ui_16());
  vecAll.AddExpected(vtkm::Vec3ui_32());
  vecAll.AddExpected(vtkm::Vec3ui_64());
  vecAll.AddExpected(vtkm::Vec4f_32());
  vecAll.AddExpected(vtkm::Vec4f_64());
  vecAll.AddExpected(vtkm::Vec4i_8());
  vecAll.AddExpected(vtkm::Vec4i_16());
  vecAll.AddExpected(vtkm::Vec4i_32());
  vecAll.AddExpected(vtkm::Vec4i_64());
  vecAll.AddExpected(vtkm::Vec4ui_8());
  vecAll.AddExpected(vtkm::Vec4ui_16());
  vecAll.AddExpected(vtkm::Vec4ui_32());
  vecAll.AddExpected(vtkm::Vec4ui_64());
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
  all.AddExpected(vtkm::Vec2f_32());
  all.AddExpected(vtkm::Vec2f_64());
  all.AddExpected(vtkm::Vec2i_8());
  all.AddExpected(vtkm::Vec2i_16());
  all.AddExpected(vtkm::Vec2i_32());
  all.AddExpected(vtkm::Vec2i_64());
  all.AddExpected(vtkm::Vec2ui_8());
  all.AddExpected(vtkm::Vec2ui_16());
  all.AddExpected(vtkm::Vec2ui_32());
  all.AddExpected(vtkm::Vec2ui_64());
  all.AddExpected(vtkm::Vec3f_32());
  all.AddExpected(vtkm::Vec3f_64());
  all.AddExpected(vtkm::Vec3i_8());
  all.AddExpected(vtkm::Vec3i_16());
  all.AddExpected(vtkm::Vec3i_32());
  all.AddExpected(vtkm::Vec3i_64());
  all.AddExpected(vtkm::Vec3ui_8());
  all.AddExpected(vtkm::Vec3ui_16());
  all.AddExpected(vtkm::Vec3ui_32());
  all.AddExpected(vtkm::Vec3ui_64());
  all.AddExpected(vtkm::Vec4f_32());
  all.AddExpected(vtkm::Vec4f_64());
  all.AddExpected(vtkm::Vec4i_8());
  all.AddExpected(vtkm::Vec4i_16());
  all.AddExpected(vtkm::Vec4i_32());
  all.AddExpected(vtkm::Vec4i_64());
  all.AddExpected(vtkm::Vec4ui_8());
  all.AddExpected(vtkm::Vec4ui_16());
  all.AddExpected(vtkm::Vec4ui_32());
  all.AddExpected(vtkm::Vec4ui_64());
  TryList(all, vtkm::TypeListTagAll());
}

} // anonymous namespace

int UnitTestTypeListTag(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestLists, argc, argv);
}

VTKM_DEPRECATED_SUPPRESS_END
