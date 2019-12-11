//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Matrix.h>
#include <vtkm/Pair.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

#include <vtkm/testing/Testing.h>

namespace
{

template <typename T, typename U>
void PairTestConstructors()
{
  std::cout << "test that all the constructors work properly" << std::endl;
  vtkm::Pair<T, U> no_params_pair;
  no_params_pair.first = TestValue(12, T());
  no_params_pair.second = TestValue(34, U());
  vtkm::Pair<T, U> copy_constructor_pair(no_params_pair);
  vtkm::Pair<T, U> assignment_pair = no_params_pair;

  VTKM_TEST_ASSERT((no_params_pair == copy_constructor_pair),
                   "copy constructor doesn't match default constructor");
  VTKM_TEST_ASSERT(!(no_params_pair != copy_constructor_pair), "operator != is working properly");

  VTKM_TEST_ASSERT((no_params_pair == assignment_pair),
                   "assignment constructor doesn't match default constructor");
  VTKM_TEST_ASSERT(!(no_params_pair != assignment_pair), "operator != is working properly");
}

template <typename T, typename U>
void PairTestValues()
{
  std::cout << "Check assignment of values" << std::endl;
  T a = TestValue(56, T());
  U b = TestValue(78, U());

  vtkm::Pair<T, U> pair_ab(a, b);
  vtkm::Pair<T, U> copy_constructor_pair(pair_ab);
  vtkm::Pair<T, U> assignment_pair = pair_ab;
  vtkm::Pair<T, U> make_p = vtkm::make_Pair(a, b);

  VTKM_TEST_ASSERT(!(pair_ab != pair_ab), "operator != isn't working properly for vtkm::Pair");
  VTKM_TEST_ASSERT((pair_ab == pair_ab), "operator == isn't working properly for vtkm::Pair");

  VTKM_TEST_ASSERT((pair_ab == copy_constructor_pair),
                   "copy constructor doesn't match pair constructor");
  VTKM_TEST_ASSERT((pair_ab == assignment_pair),
                   "assignment constructor doesn't match pair constructor");

  VTKM_TEST_ASSERT(copy_constructor_pair.first == a, "first field not set right");
  VTKM_TEST_ASSERT(assignment_pair.second == b, "second field not set right");

  VTKM_TEST_ASSERT((pair_ab == make_p), "make_pair function doesn't match pair constructor");
}

template <typename T>
T NextValue(T x)
{
  return x + T(1);
}

template <typename T, typename U>
vtkm::Pair<T, U> NextValue(vtkm::Pair<T, U> x)
{
  return vtkm::make_Pair(NextValue(x.first), NextValue(x.second));
}

template <typename T, typename U>
void PairTestOrdering()
{
  std::cout << "Check that ordering operations work" << std::endl;
  //in all cases pair_ab2 is > pair_ab. these verify that if the second
  //argument of the pair is different we respond properly
  T a = TestValue(67, T());
  U b = TestValue(89, U());

  U b2(b);
  vtkm::VecTraits<U>::SetComponent(b2, 0, NextValue(vtkm::VecTraits<U>::GetComponent(b2, 0)));

  vtkm::Pair<T, U> pair_ab2(a, b2);
  vtkm::Pair<T, U> pair_ab(a, b);

  VTKM_TEST_ASSERT((pair_ab2 >= pair_ab), "operator >= failed");
  VTKM_TEST_ASSERT((pair_ab2 >= pair_ab2), "operator >= failed");

  VTKM_TEST_ASSERT((pair_ab2 > pair_ab), "operator > failed");
  VTKM_TEST_ASSERT(!(pair_ab2 > pair_ab2), "operator > failed");

  VTKM_TEST_ASSERT(!(pair_ab2 < pair_ab), "operator < failed");
  VTKM_TEST_ASSERT(!(pair_ab2 < pair_ab2), "operator < failed");

  VTKM_TEST_ASSERT(!(pair_ab2 <= pair_ab), "operator <= failed");
  VTKM_TEST_ASSERT((pair_ab2 <= pair_ab2), "operator <= failed");

  VTKM_TEST_ASSERT(!(pair_ab2 == pair_ab), "operator == failed");
  VTKM_TEST_ASSERT((pair_ab2 != pair_ab), "operator != failed");

  T a2(a);
  vtkm::VecTraits<T>::SetComponent(a2, 0, NextValue(vtkm::VecTraits<T>::GetComponent(a2, 0)));
  vtkm::Pair<T, U> pair_a2b(a2, b);
  //this way can verify that if the first argument of the pair is different
  //we respond properly
  VTKM_TEST_ASSERT((pair_a2b >= pair_ab), "operator >= failed");
  VTKM_TEST_ASSERT((pair_a2b >= pair_a2b), "operator >= failed");

  VTKM_TEST_ASSERT((pair_a2b > pair_ab), "operator > failed");
  VTKM_TEST_ASSERT(!(pair_a2b > pair_a2b), "operator > failed");

  VTKM_TEST_ASSERT(!(pair_a2b < pair_ab), "operator < failed");
  VTKM_TEST_ASSERT(!(pair_a2b < pair_a2b), "operator < failed");

  VTKM_TEST_ASSERT(!(pair_a2b <= pair_ab), "operator <= failed");
  VTKM_TEST_ASSERT((pair_a2b <= pair_a2b), "operator <= failed");

  VTKM_TEST_ASSERT(!(pair_a2b == pair_ab), "operator == failed");
  VTKM_TEST_ASSERT((pair_a2b != pair_ab), "operator != failed");
}

//general pair test
template <typename T, typename U>
void PairTest()
{
  {
    using P = vtkm::Pair<T, U>;

    // Pair types should preserve the trivial properties of their components.
    // This insures that algorithms like std::copy will optimize fully.
    VTKM_TEST_ASSERT(std::is_trivial<T>::value &&
                       std::is_trivial<U>::value == std::is_trivial<P>::value,
                     "PairType's triviality differs from ComponentTypes.");
  }

  PairTestConstructors<T, U>();

  PairTestValues<T, U>();

  PairTestOrdering<T, U>();
}

using PairTypesToTry = vtkm::List<vtkm::Int8,                             // Integer types
                                  vtkm::FloatDefault,                     // Float types
                                  vtkm::Id3,                              // Vec types
                                  vtkm::Pair<vtkm::Vec3f_32, vtkm::Int64> // Recursive Pairs
                                  >;

template <typename FirstType>
struct DecideSecondType
{
  template <typename SecondType>
  void operator()(const SecondType&) const
  {
    PairTest<FirstType, SecondType>();
  }
};

struct DecideFirstType
{
  template <typename T>
  void operator()(const T&) const
  {
    //T is our first type for vtkm::Pair, now to figure out the second type
    vtkm::testing::Testing::TryTypes(DecideSecondType<T>(), PairTypesToTry());
  }
};

void TestPair()
{
  //we want to test each combination of standard vtkm types in a
  //vtkm::Pair, so to do that we dispatch twice on TryTypes. We could
  //dispatch on all types, but that gets excessively large and takes a
  //long time to compile (although it runs fast). Instead, just select
  //a subset of non-trivial combinations.
  vtkm::testing::Testing::TryTypes(DecideFirstType(), PairTypesToTry());
}

} // anonymous namespace

int UnitTestPair(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestPair, argc, argv);
}
