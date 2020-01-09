//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/List.h>

#include <vtkm/testing/Testing.h>

#include <vector>

namespace
{

template <int N>
struct TestClass
{
};

} // anonymous namespace

namespace vtkm
{
namespace testing
{

template <int N>
struct TypeName<TestClass<N>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "TestClass<" << N << ">";
    return stream.str();
  }
};
}
} // namespace vtkm::testing

namespace
{

template <typename T>
struct DoubleTransformLazy;
template <int N>
struct DoubleTransformLazy<TestClass<N>>
{
  using type = TestClass<2 * N>;
};

template <typename T>
using DoubleTransform = typename DoubleTransformLazy<T>::type;

template <typename T>
struct EvenPredicate;
template <int N>
struct EvenPredicate<TestClass<N>> : std::integral_constant<bool, (N % 2) == 0>
{
};

template <typename T1, typename T2>
void CheckSame(T1, T2)
{
  VTKM_STATIC_ASSERT((std::is_same<T1, T2>::value));

  std::cout << "     Got expected type: " << vtkm::testing::TypeName<T1>::Name() << std::endl;
}

template <typename ExpectedList, typename List>
void CheckList(ExpectedList, List)
{
  VTKM_IS_LIST(List);
  CheckSame(ExpectedList{}, List{});
}

template <int N>
int test_number(TestClass<N>)
{
  return N;
}

template <typename T>
struct MutableFunctor
{
  std::vector<T> FoundTypes;

  template <typename U>
  VTKM_CONT void operator()(U u)
  {
    this->FoundTypes.push_back(test_number(u));
  }
};

template <typename T>
struct ConstantFunctor
{
  template <typename U, typename VectorType>
  VTKM_CONT void operator()(U u, VectorType& vector) const
  {
    vector.push_back(test_number(u));
  }
};

void TryForEach()
{
  using TestList =
    vtkm::List<TestClass<1>, TestClass<1>, TestClass<2>, TestClass<3>, TestClass<5>, TestClass<8>>;
  const std::vector<int> expectedList = { 1, 1, 2, 3, 5, 8 };

  std::cout << "Check mutable for each" << std::endl;
  MutableFunctor<int> functor;
  vtkm::ListForEach(functor, TestList{});
  VTKM_TEST_ASSERT(expectedList == functor.FoundTypes);

  std::cout << "Check constant for each" << std::endl;
  std::vector<int> foundTypes;
  vtkm::ListForEach(ConstantFunctor<int>{}, TestList{}, foundTypes);
  VTKM_TEST_ASSERT(expectedList == foundTypes);
}

void TestLists()
{
  using SimpleCount = vtkm::List<TestClass<1>, TestClass<2>, TestClass<3>, TestClass<4>>;
  using EvenList = vtkm::List<TestClass<2>, TestClass<4>, TestClass<6>, TestClass<8>>;
  using LongList = vtkm::List<TestClass<1>,
                              TestClass<2>,
                              TestClass<3>,
                              TestClass<4>,
                              TestClass<5>,
                              TestClass<6>,
                              TestClass<7>,
                              TestClass<8>,
                              TestClass<9>,
                              TestClass<10>,
                              TestClass<11>,
                              TestClass<12>,
                              TestClass<13>,
                              TestClass<14>>;
  using RepeatList = vtkm::List<TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<14>>;

  TryForEach();

  std::cout << "Valid List Tag Checks" << std::endl;
  VTKM_TEST_ASSERT(vtkm::internal::IsList<vtkm::List<TestClass<11>>>::value);
  VTKM_TEST_ASSERT(vtkm::internal::IsList<vtkm::List<TestClass<21>, TestClass<22>>>::value);
  VTKM_TEST_ASSERT(vtkm::internal::IsList<vtkm::ListEmpty>::value);
  VTKM_TEST_ASSERT(vtkm::internal::IsList<vtkm::ListUniversal>::value);

  std::cout << "ListEmpty" << std::endl;
  CheckList(vtkm::List<>{}, vtkm::ListEmpty{});

  std::cout << "ListAppend" << std::endl;
  CheckList(vtkm::List<TestClass<31>,
                       TestClass<32>,
                       TestClass<33>,
                       TestClass<11>,
                       TestClass<21>,
                       TestClass<22>>{},
            vtkm::ListAppend<vtkm::List<TestClass<31>, TestClass<32>, TestClass<33>>,
                             vtkm::List<TestClass<11>>,
                             vtkm::List<TestClass<21>, TestClass<22>>>{});

  std::cout << "ListIntersect" << std::endl;
  CheckList(vtkm::List<TestClass<3>, TestClass<5>>{},
            vtkm::ListIntersect<
              vtkm::List<TestClass<1>, TestClass<2>, TestClass<3>, TestClass<4>, TestClass<5>>,
              vtkm::List<TestClass<3>, TestClass<5>, TestClass<6>>>{});
  CheckList(vtkm::List<TestClass<1>, TestClass<2>>{},
            vtkm::ListIntersect<vtkm::List<TestClass<1>, TestClass<2>>, vtkm::ListUniversal>{});
  CheckList(vtkm::List<TestClass<1>, TestClass<2>>{},
            vtkm::ListIntersect<vtkm::ListUniversal, vtkm::List<TestClass<1>, TestClass<2>>>{});

  std::cout << "ListTransform" << std::endl;
  CheckList(EvenList{}, vtkm::ListTransform<SimpleCount, DoubleTransform>{});

  std::cout << "ListRemoveIf" << std::endl;
  CheckList(vtkm::List<TestClass<1>, TestClass<3>>{},
            vtkm::ListRemoveIf<SimpleCount, EvenPredicate>{});

  std::cout << "ListSize" << std::endl;
  VTKM_TEST_ASSERT(vtkm::ListSize<vtkm::ListEmpty>::value == 0);
  VTKM_TEST_ASSERT(vtkm::ListSize<vtkm::List<TestClass<2>>>::value == 1);
  VTKM_TEST_ASSERT(vtkm::ListSize<vtkm::List<TestClass<2>, TestClass<4>>>::value == 2);

  std::cout << "ListCross" << std::endl;
  CheckList(vtkm::List<vtkm::List<TestClass<31>, TestClass<11>>,
                       vtkm::List<TestClass<32>, TestClass<11>>,
                       vtkm::List<TestClass<33>, TestClass<11>>>{},
            vtkm::ListCross<vtkm::List<TestClass<31>, TestClass<32>, TestClass<33>>,
                            vtkm::List<TestClass<11>>>{});

  std::cout << "ListAt" << std::endl;
  CheckSame(TestClass<2>{}, vtkm::ListAt<EvenList, 0>{});
  CheckSame(TestClass<4>{}, vtkm::ListAt<EvenList, 1>{});
  CheckSame(TestClass<6>{}, vtkm::ListAt<EvenList, 2>{});
  CheckSame(TestClass<8>{}, vtkm::ListAt<EvenList, 3>{});

  std::cout << "ListIndexOf" << std::endl;
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<EvenList, TestClass<2>>::value == 0);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<EvenList, TestClass<4>>::value == 1);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<EvenList, TestClass<6>>::value == 2);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<EvenList, TestClass<8>>::value == 3);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<EvenList, TestClass<1>>::value == -1);

  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<1>>::value == 0);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<2>>::value == 1);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<3>>::value == 2);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<4>>::value == 3);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<5>>::value == 4);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<6>>::value == 5);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<7>>::value == 6);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<8>>::value == 7);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<9>>::value == 8);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<10>>::value == 9);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<11>>::value == 10);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<12>>::value == 11);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<13>>::value == 12);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<14>>::value == 13);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<15>>::value == -1);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<LongList, TestClass<0>>::value == -1);

  VTKM_TEST_ASSERT(vtkm::ListIndexOf<RepeatList, TestClass<0>>::value == -1);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<RepeatList, TestClass<1>>::value == 0);
  VTKM_TEST_ASSERT(vtkm::ListIndexOf<RepeatList, TestClass<14>>::value == 13);

  std::cout << "ListHas" << std::endl;
  VTKM_TEST_ASSERT(vtkm::ListHas<EvenList, TestClass<2>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<EvenList, TestClass<4>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<EvenList, TestClass<6>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<EvenList, TestClass<8>>::value);
  VTKM_TEST_ASSERT(!vtkm::ListHas<EvenList, TestClass<1>>::value);

  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<1>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<2>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<3>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<4>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<5>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<6>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<7>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<7>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<8>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<9>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<10>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<11>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<12>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<13>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<LongList, TestClass<14>>::value);
  VTKM_TEST_ASSERT(!vtkm::ListHas<LongList, TestClass<15>>::value);
  VTKM_TEST_ASSERT(!vtkm::ListHas<LongList, TestClass<0>>::value);

  VTKM_TEST_ASSERT(!vtkm::ListHas<RepeatList, TestClass<0>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<RepeatList, TestClass<1>>::value);
  VTKM_TEST_ASSERT(vtkm::ListHas<RepeatList, TestClass<14>>::value);
}

} // anonymous namespace

int UnitTestList(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestLists, argc, argv);
}
