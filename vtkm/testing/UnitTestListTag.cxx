//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/ListTag.h>

#include <vtkm/Types.h>

#include <vtkm/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{

template <int N>
struct TestClass
{
};

struct TestListTag1 : vtkm::ListTagBase<TestClass<11>>
{
};

struct TestListTag2 : vtkm::ListTagBase<TestClass<21>, TestClass<22>>
{
};

struct TestListTag3 : vtkm::ListTagBase<TestClass<31>, TestClass<32>, TestClass<33>>
{
};

struct TestListTag4 : vtkm::ListTagBase<TestClass<41>, TestClass<42>, TestClass<43>, TestClass<44>>
{
};

struct TestListTagJoin : vtkm::ListTagJoin<TestListTag3, TestListTag1>
{
};

struct TestListTagIntersect : vtkm::ListTagIntersect<TestListTag3, TestListTagJoin>
{
};

struct TestListTagCrossProduct : vtkm::ListCrossProduct<TestListTag3, TestListTag1>
{
};

struct TestListTagUniversal : vtkm::ListTagUniversal
{
};

struct TestListTagAppend : vtkm::ListTagAppend<TestListTag3, TestClass<34>>
{
};

struct TestListTagAppendUnique1 : vtkm::ListTagAppendUnique<TestListTag3, TestClass<32>>
{
};

struct TestListTagAppendUnique2 : vtkm::ListTagAppendUnique<TestListTagAppendUnique1, TestClass<34>>
{
};

template <int N, int M>
std::pair<int, int> test_number(brigand::list<TestClass<N>, TestClass<M>>)
{
  return std::make_pair(N, M);
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

template <typename T, vtkm::IdComponent N>
void CheckSame(const vtkm::Vec<T, N>& expected, const std::vector<T>& found)
{
  VTKM_TEST_ASSERT(static_cast<int>(found.size()) == N, "Got wrong number of items.");

  for (vtkm::IdComponent index = 0; index < N; index++)
  {
    vtkm::UInt32 i = static_cast<vtkm::UInt32>(index);
    VTKM_TEST_ASSERT(expected[index] == found[i], "Got wrong type.");
  }
}

template <int N, typename ListTag>
void CheckContains(TestClass<N>, ListTag, const std::vector<int>& contents)
{
  //Use intersect to verify at compile time that ListTag contains TestClass<N>
  using intersectWith = vtkm::ListTagBase<TestClass<N>>;
  using intersectResult = typename vtkm::ListTagIntersect<intersectWith, ListTag>::list;
  constexpr bool intersectContains = (brigand::size<intersectResult>::value != 0);

  bool listContains = vtkm::ListContains<ListTag, TestClass<N>>::value;
  bool shouldContain = std::find(contents.begin(), contents.end(), N) != contents.end();

  VTKM_TEST_ASSERT(intersectContains == shouldContain, "ListTagIntersect check failed.");
  VTKM_TEST_ASSERT(listContains == shouldContain, "ListContains check failed.");
}

template <int N>
void CheckContains(TestClass<N>, TestListTagUniversal, const std::vector<int>&)
{
  //Use intersect to verify at compile time that ListTag contains TestClass<N>
  using intersectWith = vtkm::ListTagBase<TestClass<N>>;
  using intersectResult =
    typename vtkm::ListTagIntersect<intersectWith, TestListTagUniversal>::list;
  constexpr bool intersectContains = (brigand::size<intersectResult>::value != 0);
  constexpr bool listContains = vtkm::ListContains<TestListTagUniversal, TestClass<N>>::value;

  VTKM_TEST_ASSERT(intersectContains == listContains, "ListTagIntersect check failed.");
}

template <vtkm::IdComponent N, typename ListTag>
void TryList(const vtkm::Vec<int, N>& expected, ListTag)
{
  VTKM_IS_LIST_TAG(ListTag);

  std::cout << "    Try mutable for each" << std::endl;
  MutableFunctor<int> functor;
  vtkm::ListForEach(functor, ListTag());
  CheckSame(expected, functor.FoundTypes);

  std::cout << "    Try constant for each" << std::endl;
  std::vector<int> foundTypes;
  ConstantFunctor<int> cfunc;
  vtkm::ListForEach(cfunc, ListTag(), foundTypes);
  CheckSame(expected, foundTypes);

  std::cout << "    Try checking contents" << std::endl;
  CheckContains(TestClass<11>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<21>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<22>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<31>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<32>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<33>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<41>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<42>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<43>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<44>(), ListTag(), functor.FoundTypes);
}
template <vtkm::IdComponent N, typename ListTag>
void TryList(const vtkm::Vec<std::pair<int, int>, N>& expected, ListTag)
{
  VTKM_IS_LIST_TAG(ListTag);

  std::cout << "    Try mutable for each" << std::endl;
  MutableFunctor<std::pair<int, int>> functor;
  vtkm::ListForEach(functor, ListTag());
  CheckSame(expected, functor.FoundTypes);

  std::cout << "    Try constant for each" << std::endl;
  std::vector<std::pair<int, int>> foundTypes;
  ConstantFunctor<std::pair<int, int>> cfunc;
  vtkm::ListForEach(cfunc, ListTag(), foundTypes);
  CheckSame(expected, foundTypes);
}

template <vtkm::IdComponent N>
void TryList(const vtkm::Vec<int, N>&, TestListTagUniversal tag)
{
  VTKM_IS_LIST_TAG(TestListTagUniversal);

  //TestListTagUniversal can't be used with for_each on purpose

  std::vector<int> found;
  std::cout << "    Try checking contents" << std::endl;
  CheckContains(TestClass<11>(), tag, found);
  CheckContains(TestClass<21>(), tag, found);
  CheckContains(TestClass<22>(), tag, found);
  CheckContains(TestClass<31>(), tag, found);
  CheckContains(TestClass<32>(), tag, found);
  CheckContains(TestClass<33>(), tag, found);
  CheckContains(TestClass<41>(), tag, found);
  CheckContains(TestClass<42>(), tag, found);
  CheckContains(TestClass<43>(), tag, found);
  CheckContains(TestClass<44>(), tag, found);
}

void TestLists()
{
  std::cout << "Valid List Tag Checks" << std::endl;
  VTKM_TEST_ASSERT(vtkm::internal::ListTagCheck<TestListTag1>::value, "Failed list tag check");
  VTKM_TEST_ASSERT(vtkm::internal::ListTagCheck<TestListTagJoin>::value, "Failed list tag check");
  VTKM_TEST_ASSERT(!vtkm::internal::ListTagCheck<TestClass<1>>::value, "Failed list tag check");

  std::cout << "ListTagEmpty" << std::endl;
  TryList(vtkm::Vec<int, 0>(), vtkm::ListTagEmpty());

  std::cout << "ListTagBase" << std::endl;
  TryList(vtkm::Vec<int, 1>(11), TestListTag1());

  std::cout << "ListTagBase2" << std::endl;
  TryList(vtkm::Vec<int, 2>(21, 22), TestListTag2());

  std::cout << "ListTagBase3" << std::endl;
  TryList(vtkm::Vec<int, 3>(31, 32, 33), TestListTag3());

  std::cout << "ListTagBase4" << std::endl;
  TryList(vtkm::Vec<int, 4>(41, 42, 43, 44), TestListTag4());

  std::cout << "ListTagJoin" << std::endl;
  TryList(vtkm::Vec<int, 4>(31, 32, 33, 11), TestListTagJoin());

  std::cout << "ListTagIntersect" << std::endl;
  TryList(vtkm::Vec<int, 3>(31, 32, 33), TestListTagIntersect());

  std::cout << "ListTagCrossProduct" << std::endl;
  TryList(vtkm::Vec<std::pair<int, int>, 3>({ 31, 11 }, { 32, 11 }, { 33, 11 }),
          TestListTagCrossProduct());

  std::cout << "ListTagAppend" << std::endl;
  TryList(vtkm::Vec<int, 4>(31, 32, 33, 34), TestListTagAppend());

  std::cout << "ListTagAppendUnique1" << std::endl;
  TryList(vtkm::Vec<int, 3>(31, 32, 33), TestListTagAppendUnique1());

  std::cout << "ListTagAppendUnique2" << std::endl;
  TryList(vtkm::Vec<int, 4>(31, 32, 33, 34), TestListTagAppendUnique2());



  std::cout << "ListTagUniversal" << std::endl;
  TryList(vtkm::Vec<int, 4>(1, 2, 3, 4), TestListTagUniversal());
}

} // anonymous namespace

int UnitTestListTag(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestLists, argc, argv);
}
