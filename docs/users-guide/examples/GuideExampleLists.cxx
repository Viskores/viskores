//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#include <viskores/List.h>
#include <viskores/TypeList.h>

namespace
{

////
//// BEGIN-EXAMPLE CustomTypeLists
////
// A list of 2D vector types.
using Vec2List = viskores::List<viskores::Vec2f_32, viskores::Vec2f_64>;

// An application that uses 2D geometry might commonly encounter this list of
// types.
using MyCommonTypes = viskores::ListAppend<Vec2List, viskores::TypeListCommon>;
////
//// END-EXAMPLE CustomTypeLists
////

VISKORES_STATIC_ASSERT(
  (std::is_same<viskores::ListAt<Vec2List, 0>, viskores::Vec2f_32>::value));
VISKORES_STATIC_ASSERT(
  (std::is_same<viskores::ListAt<Vec2List, 1>, viskores::Vec2f_64>::value));

VISKORES_STATIC_ASSERT((std::is_same<MyCommonTypes,
                                     viskores::List<viskores::Vec2f_32,
                                                    viskores::Vec2f_64,
                                                    viskores::UInt8,
                                                    viskores::Int32,
                                                    viskores::Int64,
                                                    viskores::Float32,
                                                    viskores::Float64,
                                                    viskores::Vec3f_32,
                                                    viskores::Vec3f_64>>::value));

} // anonymous namespace

#include <viskores/VecTraits.h>

#include <viskores/testing/Testing.h>

#include <algorithm>
#include <string>
#include <vector>

////
//// BEGIN-EXAMPLE BaseLists
////
#include <viskores/List.h>
//// PAUSE-EXAMPLE
namespace
{
//// RESUME-EXAMPLE

// Placeholder classes representing things that might be in a template
// metaprogram list.
class Foo;
class Bar;
class Baz;
class Qux;
class Xyzzy;

// The names of the following tags are indicative of the lists they contain.

using FooList = viskores::List<Foo>;

using FooBarList = viskores::List<Foo, Bar>;

using BazQuxXyzzyList = viskores::List<Baz, Qux, Xyzzy>;

using QuxBazBarFooList = viskores::List<Qux, Baz, Bar, Foo>;
////
//// END-EXAMPLE BaseLists
////

class Foo
{
};
class Bar
{
};
class Baz
{
};
class Qux
{
};
class Xyzzy
{
};

struct ListFunctor
{
  std::string FoundTags;

  template<typename T>
  void operator()(T)
  {
    this->FoundTags.append(viskores::testing::TypeName<T>::Name());
  }

  void operator()(Foo) { this->FoundTags.append("Foo"); }
  void operator()(Bar) { this->FoundTags.append("Bar"); }
  void operator()(Baz) { this->FoundTags.append("Baz"); }
  void operator()(Qux) { this->FoundTags.append("Qux"); }
  void operator()(Xyzzy) { this->FoundTags.append("Xyzzy"); }
};

template<typename List>
void TryList(List, const char* expectedString)
{
  ListFunctor checkFunctor;
  viskores::ListForEach(checkFunctor, List());
  std::cout << std::endl
            << "Expected " << expectedString << std::endl
            << "Found    " << checkFunctor.FoundTags << std::endl;
  VISKORES_TEST_ASSERT(checkFunctor.FoundTags == expectedString, "List wrong");
}

void TestBaseLists()
{
  TryList(FooList(), "Foo");
  TryList(FooBarList(), "FooBar");
  TryList(BazQuxXyzzyList(), "BazQuxXyzzy");
  TryList(QuxBazBarFooList(), "QuxBazBarFoo");
}

////
//// BEGIN-EXAMPLE VISKORES_IS_LIST
////
template<typename List>
class MyImportantClass
{
  VISKORES_IS_LIST(List);
  // Implementation...
};

void DoImportantStuff()
{
  MyImportantClass<viskores::List<viskores::Id>> important1; // This compiles fine
  //// PAUSE-EXAMPLE
#if 0
  //// RESUME-EXAMPLE
  MyImportantClass<viskores::Id> important2;  // COMPILE ERROR: viskores::Id is not a list
  ////
  //// END-EXAMPLE VISKORES_IS_LIST
  ////
  //// PAUSE-EXAMPLE
#endif

  (void)important1; // Quiet compiler
  //// RESUME-EXAMPLE
}

void TestCheckListType()
{
  DoImportantStuff();
}

void TestListSize()
{
  ////
  //// BEGIN-EXAMPLE ListSize
  ////
  using MyList = viskores::List<viskores::Int8, viskores::Int32, viskores::Int64>;

  constexpr viskores::IdComponent myListSize = viskores::ListSize<MyList>::value;
  // myListSize is 3
  ////
  //// END-EXAMPLE ListSize
  ////
  VISKORES_STATIC_ASSERT(myListSize == 3);
}

void TestListHas()
{
  ////
  //// BEGIN-EXAMPLE ListHas
  ////
  using MyList =
    viskores::List<viskores::Int8, viskores::Int16, viskores::Int32, viskores::Int64>;

  constexpr bool hasInt = viskores::ListHas<MyList, int>::value;
  // hasInt is true

  constexpr bool hasFloat = viskores::ListHas<MyList, float>::value;
  // hasFloat is false
  ////
  //// END-EXAMPLE ListHas
  ////
  VISKORES_STATIC_ASSERT(hasInt);
  VISKORES_STATIC_ASSERT(!hasFloat);
}

void TestListIndices()
{
  ////
  //// BEGIN-EXAMPLE ListIndices
  ////
  using MyList = viskores::List<viskores::Int8, viskores::Int32, viskores::Int64>;

  constexpr viskores::IdComponent indexOfInt8 =
    viskores::ListIndexOf<MyList, viskores::Int8>::value;
  // indexOfInt8 is 0
  constexpr viskores::IdComponent indexOfInt32 =
    viskores::ListIndexOf<MyList, viskores::Int32>::value;
  // indexOfInt32 is 1
  constexpr viskores::IdComponent indexOfInt64 =
    viskores::ListIndexOf<MyList, viskores::Int64>::value;
  // indexOfInt64 is 2
  constexpr viskores::IdComponent indexOfFloat32 =
    viskores::ListIndexOf<MyList, viskores::Float32>::value;
  // indexOfFloat32 is -1 (not in list)

  using T0 = viskores::ListAt<MyList, 0>; // T0 is viskores::Int8
  using T1 = viskores::ListAt<MyList, 1>; // T1 is viskores::Int32
  using T2 = viskores::ListAt<MyList, 2>; // T2 is viskores::Int64
  ////
  //// END-EXAMPLE ListIndices
  ////
  VISKORES_TEST_ASSERT(indexOfInt8 == 0);
  VISKORES_TEST_ASSERT(indexOfInt32 == 1);
  VISKORES_TEST_ASSERT(indexOfInt64 == 2);
  VISKORES_TEST_ASSERT(indexOfFloat32 == -1);

  VISKORES_STATIC_ASSERT((std::is_same<T0, viskores::Int8>::value));
  VISKORES_STATIC_ASSERT((std::is_same<T1, viskores::Int32>::value));
  VISKORES_STATIC_ASSERT((std::is_same<T2, viskores::Int64>::value));
}

namespace TestListAppend
{
////
//// BEGIN-EXAMPLE ListAppend
////
using BigTypes = viskores::List<viskores::Int64, viskores::Float64>;
using MediumTypes = viskores::List<viskores::Int32, viskores::Float32>;
using SmallTypes = viskores::List<viskores::Int8>;

using SmallAndBigTypes = viskores::ListAppend<SmallTypes, BigTypes>;
// SmallAndBigTypes is viskores::List<viskores::Int8, viskores::Int64, viskores::Float64>

using AllMyTypes = viskores::ListAppend<BigTypes, MediumTypes, SmallTypes>;
// AllMyTypes is
// viskores::List<viskores::Int64, viskores::Float64, viskores::Int32, viskores::Float32, viskores::Int8>
////
//// END-EXAMPLE ListAppend
////
VISKORES_STATIC_ASSERT(
  (std::is_same<
    SmallAndBigTypes,
    viskores::List<viskores::Int8, viskores::Int64, viskores::Float64>>::value));
VISKORES_STATIC_ASSERT((std::is_same<AllMyTypes,
                                     viskores::List<viskores::Int64,
                                                    viskores::Float64,
                                                    viskores::Int32,
                                                    viskores::Float32,
                                                    viskores::Int8>>::value));
}

namespace TestListIntersect
{
////
//// BEGIN-EXAMPLE ListIntersect
////
using SignedInts =
  viskores::List<viskores::Int8, viskores::Int16, viskores::Int32, viskores::Int64>;
using WordTypes =
  viskores::List<viskores::Int32, viskores::UInt32, viskores::Int64, viskores::UInt64>;

using SignedWords = viskores::ListIntersect<SignedInts, WordTypes>;
// SignedWords is viskores::List<viskores::Int32, viskores::Int64>
////
//// END-EXAMPLE ListIntersect
////
VISKORES_STATIC_ASSERT(
  (std::is_same<SignedWords, viskores::List<viskores::Int32, viskores::Int64>>::value));
}

namespace TestListApply
{
////
//// BEGIN-EXAMPLE ListApply
////
using MyList = viskores::List<viskores::Id, viskores::Id3, viskores::Vec3f>;

using MyTuple = viskores::ListApply<MyList, std::tuple>;
// MyTuple is std::tuple<viskores::Id, viskores::Id3, viskores::Vec3f>
////
//// END-EXAMPLE ListApply
////
VISKORES_STATIC_ASSERT(
  (std::is_same<MyTuple,
                std::tuple<viskores::Id, viskores::Id3, viskores::Vec3f>>::value));
}

namespace TestListTransform
{
////
//// BEGIN-EXAMPLE ListTransform
////
using MyList = viskores::List<viskores::Int32, viskores::Float32>;

template<typename T>
using MakeVec = viskores::Vec<T, 3>;

using MyVecList = viskores::ListTransform<MyList, MakeVec>;
// MyVecList is viskores::List<viskores::Vec<viskores::Int32, 3>, viskores::Vec<viskores::Float32, 3>>
////
//// END-EXAMPLE ListTransform
////
VISKORES_STATIC_ASSERT(
  (std::is_same<MyVecList,
                viskores::List<viskores::Vec<viskores::Int32, 3>,
                               viskores::Vec<viskores::Float32, 3>>>::value));
}

namespace TestListRemoveIf
{
////
//// BEGIN-EXAMPLE ListRemoveIf
////
using MyList = viskores::List<viskores::Int64,
                              viskores::Float64,
                              viskores::Int32,
                              viskores::Float32,
                              viskores::Int8>;

using FilteredList = viskores::ListRemoveIf<MyList, std::is_integral>;
// FilteredList is viskores::List<viskores::Float64, viskores::Float32>
////
//// END-EXAMPLE ListRemoveIf
////
VISKORES_STATIC_ASSERT(
  (std::is_same<FilteredList,
                viskores::List<viskores::Float64, viskores::Float32>>::value));
}

namespace TestListCross
{
////
//// BEGIN-EXAMPLE ListCross
////
using BaseTypes = viskores::List<viskores::Int8, viskores::Int32, viskores::Int64>;
using BoolCases = viskores::List<std::false_type, std::true_type>;

using CrossTypes = viskores::ListCross<BaseTypes, BoolCases>;
// CrossTypes is
//   viskores::List<viskores::List<viskores::Int8, std::false_type>,
//              viskores::List<viskores::Int8, std::true_type>,
//              viskores::List<viskores::Int32, std::false_type>,
//              viskores::List<viskores::Int32, std::true_type>,
//              viskores::List<viskores::Int64, std::false_type>,
//              viskores::List<viskores::Int64, std::true_type>>

template<typename TypeAndIsVec>
using ListPairToType =
  typename std::conditional<viskores::ListAt<TypeAndIsVec, 1>::value,
                            viskores::Vec<viskores::ListAt<TypeAndIsVec, 0>, 3>,
                            viskores::ListAt<TypeAndIsVec, 0>>::type;

using AllTypes = viskores::ListTransform<CrossTypes, ListPairToType>;
// AllTypes is
//   viskores::List<viskores::Int8,
//              viskores::Vec<viskores::Int8, 3>,
//              viskores::Int32,
//              viskores::Vec<viskores::Int32, 3>,
//              viskores::Int64,
//              viskores::Vec<viskores::Int64, 3>>
////
//// END-EXAMPLE ListCross
////
VISKORES_STATIC_ASSERT((
  std::is_same<CrossTypes,
               viskores::List<viskores::List<viskores::Int8, std::false_type>,
                              viskores::List<viskores::Int8, std::true_type>,
                              viskores::List<viskores::Int32, std::false_type>,
                              viskores::List<viskores::Int32, std::true_type>,
                              viskores::List<viskores::Int64, std::false_type>,
                              viskores::List<viskores::Int64, std::true_type>>>::value));

VISKORES_STATIC_ASSERT(
  (std::is_same<AllTypes,
                viskores::List<viskores::Int8,
                               viskores::Vec<viskores::Int8, 3>,
                               viskores::Int32,
                               viskores::Vec<viskores::Int32, 3>,
                               viskores::Int64,
                               viskores::Vec<viskores::Int64, 3>>>::value));
}

////
//// BEGIN-EXAMPLE ListForEach
////
struct MyArrayBase
{
  // A virtual destructor makes sure C++ RTTI will be generated. It also helps
  // ensure subclass destructors are called.
  virtual ~MyArrayBase() {}
};

template<typename T>
struct MyArrayImpl : public MyArrayBase
{
  std::vector<T> Array;
};

template<typename T>
void PrefixSum(std::vector<T>& array)
{
  T sum(typename viskores::VecTraits<T>::ComponentType(0));
  for (typename std::vector<T>::iterator iter = array.begin(); iter != array.end();
       iter++)
  {
    sum = sum + *iter;
    *iter = sum;
  }
}

struct PrefixSumFunctor
{
  MyArrayBase* ArrayPointer;

  PrefixSumFunctor(MyArrayBase* arrayPointer)
    : ArrayPointer(arrayPointer)
  {
  }

  template<typename T>
  void operator()(T)
  {
    using ConcreteArrayType = MyArrayImpl<T>;
    ConcreteArrayType* concreteArray =
      dynamic_cast<ConcreteArrayType*>(this->ArrayPointer);
    if (concreteArray != NULL)
    {
      PrefixSum(concreteArray->Array);
    }
  }
};

void DoPrefixSum(MyArrayBase* array)
{
  PrefixSumFunctor functor = PrefixSumFunctor(array);
  viskores::ListForEach(functor, viskores::TypeListCommon());
}
////
//// END-EXAMPLE ListForEach
////

void TestPrefixSum()
{
  MyArrayImpl<viskores::Id> array;
  array.Array.resize(10);
  std::fill(array.Array.begin(), array.Array.end(), 1);
  DoPrefixSum(&array);
  for (viskores::Id index = 0; index < 10; index++)
  {
    VISKORES_TEST_ASSERT(array.Array[(std::size_t)index] == index + 1,
                         "Got bad prefix sum.");
  }
}

void Test()
{
  TestBaseLists();
  TestCheckListType();
  TestListSize();
  TestListHas();
  TestListIndices();
  TestPrefixSum();
}

} // anonymous namespace

int GuideExampleLists(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(Test, argc, argv);
}
