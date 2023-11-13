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

#include <vtkm/List.h>
#include <vtkm/TypeList.h>

namespace
{

////
//// BEGIN-EXAMPLE CustomTypeLists
////
// A list of 2D vector types.
using Vec2List = vtkm::List<vtkm::Vec2f_32, vtkm::Vec2f_64>;

// An application that uses 2D geometry might commonly encounter this list of
// types.
using MyCommonTypes = vtkm::ListAppend<Vec2List, vtkm::TypeListCommon>;
////
//// END-EXAMPLE CustomTypeLists
////

VTKM_STATIC_ASSERT((std::is_same<vtkm::ListAt<Vec2List, 0>, vtkm::Vec2f_32>::value));
VTKM_STATIC_ASSERT((std::is_same<vtkm::ListAt<Vec2List, 1>, vtkm::Vec2f_64>::value));

VTKM_STATIC_ASSERT((std::is_same<MyCommonTypes,
                                 vtkm::List<vtkm::Vec2f_32,
                                            vtkm::Vec2f_64,
                                            vtkm::UInt8,
                                            vtkm::Int32,
                                            vtkm::Int64,
                                            vtkm::Float32,
                                            vtkm::Float64,
                                            vtkm::Vec3f_32,
                                            vtkm::Vec3f_64>>::value));

} // anonymous namespace

#include <vtkm/VecTraits.h>

#include <vtkm/testing/Testing.h>

#include <algorithm>
#include <string>
#include <vector>

////
//// BEGIN-EXAMPLE BaseLists
////
#include <vtkm/List.h>
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

using FooList = vtkm::List<Foo>;

using FooBarList = vtkm::List<Foo, Bar>;

using BazQuxXyzzyList = vtkm::List<Baz, Qux, Xyzzy>;

using QuxBazBarFooList = vtkm::List<Qux, Baz, Bar, Foo>;
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
    this->FoundTags.append(vtkm::testing::TypeName<T>::Name());
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
  vtkm::ListForEach(checkFunctor, List());
  std::cout << std::endl
            << "Expected " << expectedString << std::endl
            << "Found    " << checkFunctor.FoundTags << std::endl;
  VTKM_TEST_ASSERT(checkFunctor.FoundTags == expectedString, "List wrong");
}

void TestBaseLists()
{
  TryList(FooList(), "Foo");
  TryList(FooBarList(), "FooBar");
  TryList(BazQuxXyzzyList(), "BazQuxXyzzy");
  TryList(QuxBazBarFooList(), "QuxBazBarFoo");
}

////
//// BEGIN-EXAMPLE VTKM_IS_LIST
////
template<typename List>
class MyImportantClass
{
  VTKM_IS_LIST(List);
  // Implementation...
};

void DoImportantStuff()
{
  MyImportantClass<vtkm::List<vtkm::Id>> important1; // This compiles fine
  //// PAUSE-EXAMPLE
#if 0
  //// RESUME-EXAMPLE
  MyImportantClass<vtkm::Id> important2;  // COMPILE ERROR: vtkm::Id is not a list
  ////
  //// END-EXAMPLE VTKM_IS_LIST
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
  using MyList = vtkm::List<vtkm::Int8, vtkm::Int32, vtkm::Int64>;

  constexpr vtkm::IdComponent myListSize = vtkm::ListSize<MyList>::value;
  // myListSize is 3
  ////
  //// END-EXAMPLE ListSize
  ////
  VTKM_STATIC_ASSERT(myListSize == 3);
}

void TestListHas()
{
  ////
  //// BEGIN-EXAMPLE ListHas
  ////
  using MyList = vtkm::List<vtkm::Int8, vtkm::Int16, vtkm::Int32, vtkm::Int64>;

  constexpr bool hasInt = vtkm::ListHas<MyList, int>::value;
  // hasInt is true

  constexpr bool hasFloat = vtkm::ListHas<MyList, float>::value;
  // hasFloat is false
  ////
  //// END-EXAMPLE ListHas
  ////
  VTKM_STATIC_ASSERT(hasInt);
  VTKM_STATIC_ASSERT(!hasFloat);
}

void TestListIndices()
{
  ////
  //// BEGIN-EXAMPLE ListIndices
  ////
  using MyList = vtkm::List<vtkm::Int8, vtkm::Int32, vtkm::Int64>;

  constexpr vtkm::IdComponent indexOfInt8 = vtkm::ListIndexOf<MyList, vtkm::Int8>::value;
  // indexOfInt8 is 0
  constexpr vtkm::IdComponent indexOfInt32 =
    vtkm::ListIndexOf<MyList, vtkm::Int32>::value;
  // indexOfInt32 is 1
  constexpr vtkm::IdComponent indexOfInt64 =
    vtkm::ListIndexOf<MyList, vtkm::Int64>::value;
  // indexOfInt64 is 2
  constexpr vtkm::IdComponent indexOfFloat32 =
    vtkm::ListIndexOf<MyList, vtkm::Float32>::value;
  // indexOfFloat32 is -1 (not in list)

  using T0 = vtkm::ListAt<MyList, 0>; // T0 is vtkm::Int8
  using T1 = vtkm::ListAt<MyList, 1>; // T1 is vtkm::Int32
  using T2 = vtkm::ListAt<MyList, 2>; // T2 is vtkm::Int64
  ////
  //// END-EXAMPLE ListIndices
  ////
  VTKM_TEST_ASSERT(indexOfInt8 == 0);
  VTKM_TEST_ASSERT(indexOfInt32 == 1);
  VTKM_TEST_ASSERT(indexOfInt64 == 2);
  VTKM_TEST_ASSERT(indexOfFloat32 == -1);

  VTKM_STATIC_ASSERT((std::is_same<T0, vtkm::Int8>::value));
  VTKM_STATIC_ASSERT((std::is_same<T1, vtkm::Int32>::value));
  VTKM_STATIC_ASSERT((std::is_same<T2, vtkm::Int64>::value));
}

namespace TestListAppend
{
////
//// BEGIN-EXAMPLE ListAppend
////
using BigTypes = vtkm::List<vtkm::Int64, vtkm::Float64>;
using MediumTypes = vtkm::List<vtkm::Int32, vtkm::Float32>;
using SmallTypes = vtkm::List<vtkm::Int8>;

using SmallAndBigTypes = vtkm::ListAppend<SmallTypes, BigTypes>;
// SmallAndBigTypes is vtkm::List<vtkm::Int8, vtkm::Int64, vtkm::Float64>

using AllMyTypes = vtkm::ListAppend<BigTypes, MediumTypes, SmallTypes>;
// AllMyTypes is
// vtkm::List<vtkm::Int64, vtkm::Float64, vtkm::Int32, vtkm::Float32, vtkm::Int8>
////
//// END-EXAMPLE ListAppend
////
VTKM_STATIC_ASSERT(
  (std::is_same<SmallAndBigTypes,
                vtkm::List<vtkm::Int8, vtkm::Int64, vtkm::Float64>>::value));
VTKM_STATIC_ASSERT(
  (std::is_same<
    AllMyTypes,
    vtkm::List<vtkm::Int64, vtkm::Float64, vtkm::Int32, vtkm::Float32, vtkm::Int8>>::
     value));
}

namespace TestListIntersect
{
////
//// BEGIN-EXAMPLE ListIntersect
////
using SignedInts = vtkm::List<vtkm::Int8, vtkm::Int16, vtkm::Int32, vtkm::Int64>;
using WordTypes = vtkm::List<vtkm::Int32, vtkm::UInt32, vtkm::Int64, vtkm::UInt64>;

using SignedWords = vtkm::ListIntersect<SignedInts, WordTypes>;
// SignedWords is vtkm::List<vtkm::Int32, vtkm::Int64>
////
//// END-EXAMPLE ListIntersect
////
VTKM_STATIC_ASSERT(
  (std::is_same<SignedWords, vtkm::List<vtkm::Int32, vtkm::Int64>>::value));
}

namespace TestListApply
{
////
//// BEGIN-EXAMPLE ListApply
////
using MyList = vtkm::List<vtkm::Id, vtkm::Id3, vtkm::Vec3f>;

using MyTuple = vtkm::ListApply<MyList, std::tuple>;
// MyTuple is std::tuple<vtkm::Id, vtkm::Id3, vtkm::Vec3f>
////
//// END-EXAMPLE ListApply
////
VTKM_STATIC_ASSERT(
  (std::is_same<MyTuple, std::tuple<vtkm::Id, vtkm::Id3, vtkm::Vec3f>>::value));
}

namespace TestListTransform
{
////
//// BEGIN-EXAMPLE ListTransform
////
using MyList = vtkm::List<vtkm::Int32, vtkm::Float32>;

template<typename T>
using MakeVec = vtkm::Vec<T, 3>;

using MyVecList = vtkm::ListTransform<MyList, MakeVec>;
// MyVecList is vtkm::List<vtkm::Vec<vtkm::Int32, 3>, vtkm::Vec<vtkm::Float32, 3>>
////
//// END-EXAMPLE ListTransform
////
VTKM_STATIC_ASSERT((std::is_same<MyVecList,
                                 vtkm::List<vtkm::Vec<vtkm::Int32, 3>,
                                            vtkm::Vec<vtkm::Float32, 3>>>::value));
}

namespace TestListRemoveIf
{
////
//// BEGIN-EXAMPLE ListRemoveIf
////
using MyList =
  vtkm::List<vtkm::Int64, vtkm::Float64, vtkm::Int32, vtkm::Float32, vtkm::Int8>;

using FilteredList = vtkm::ListRemoveIf<MyList, std::is_integral>;
// FilteredList is vtkm::List<vtkm::Float64, vtkm::Float32>
////
//// END-EXAMPLE ListRemoveIf
////
VTKM_STATIC_ASSERT(
  (std::is_same<FilteredList, vtkm::List<vtkm::Float64, vtkm::Float32>>::value));
}

namespace TestListCross
{
////
//// BEGIN-EXAMPLE ListCross
////
using BaseTypes = vtkm::List<vtkm::Int8, vtkm::Int32, vtkm::Int64>;
using BoolCases = vtkm::List<std::false_type, std::true_type>;

using CrossTypes = vtkm::ListCross<BaseTypes, BoolCases>;
// CrossTypes is
//   vtkm::List<vtkm::List<vtkm::Int8, std::false_type>,
//              vtkm::List<vtkm::Int8, std::true_type>,
//              vtkm::List<vtkm::Int32, std::false_type>,
//              vtkm::List<vtkm::Int32, std::true_type>,
//              vtkm::List<vtkm::Int64, std::false_type>,
//              vtkm::List<vtkm::Int64, std::true_type>>

template<typename TypeAndIsVec>
using ListPairToType =
  typename std::conditional<vtkm::ListAt<TypeAndIsVec, 1>::value,
                            vtkm::Vec<vtkm::ListAt<TypeAndIsVec, 0>, 3>,
                            vtkm::ListAt<TypeAndIsVec, 0>>::type;

using AllTypes = vtkm::ListTransform<CrossTypes, ListPairToType>;
// AllTypes is
//   vtkm::List<vtkm::Int8,
//              vtkm::Vec<vtkm::Int8, 3>,
//              vtkm::Int32,
//              vtkm::Vec<vtkm::Int32, 3>,
//              vtkm::Int64,
//              vtkm::Vec<vtkm::Int64, 3>>
////
//// END-EXAMPLE ListCross
////
VTKM_STATIC_ASSERT(
  (std::is_same<CrossTypes,
                vtkm::List<vtkm::List<vtkm::Int8, std::false_type>,
                           vtkm::List<vtkm::Int8, std::true_type>,
                           vtkm::List<vtkm::Int32, std::false_type>,
                           vtkm::List<vtkm::Int32, std::true_type>,
                           vtkm::List<vtkm::Int64, std::false_type>,
                           vtkm::List<vtkm::Int64, std::true_type>>>::value));

VTKM_STATIC_ASSERT((std::is_same<AllTypes,
                                 vtkm::List<vtkm::Int8,
                                            vtkm::Vec<vtkm::Int8, 3>,
                                            vtkm::Int32,
                                            vtkm::Vec<vtkm::Int32, 3>,
                                            vtkm::Int64,
                                            vtkm::Vec<vtkm::Int64, 3>>>::value));
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
  T sum(typename vtkm::VecTraits<T>::ComponentType(0));
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
  vtkm::ListForEach(functor, vtkm::TypeListCommon());
}
////
//// END-EXAMPLE ListForEach
////

void TestPrefixSum()
{
  MyArrayImpl<vtkm::Id> array;
  array.Array.resize(10);
  std::fill(array.Array.begin(), array.Array.end(), 1);
  DoPrefixSum(&array);
  for (vtkm::Id index = 0; index < 10; index++)
  {
    VTKM_TEST_ASSERT(array.Array[(std::size_t)index] == index + 1,
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
  return vtkm::testing::Testing::Run(Test, argc, argv);
}
