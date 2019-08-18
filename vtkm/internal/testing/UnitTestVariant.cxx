//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/internal/Variant.h>

#include <vtkm/testing/Testing.h>

#include <vector>

namespace
{

template <vtkm::IdComponent Index>
struct TypePlaceholder
{
};

void TestSize()
{
  std::cout << "Test size" << std::endl;

  using VariantType = vtkm::internal::Variant<float, double, char, short, int, long>;

  constexpr size_t variantSize = sizeof(VariantType);

  VTKM_TEST_ASSERT(variantSize <= 16,
                   "Size of variant should not be larger than biggest type plus and index. ",
                   variantSize);
}

void TestIndexing()
{
  std::cout << "Test indexing" << std::endl;

  using VariantType = vtkm::internal::
    Variant<TypePlaceholder<0>, TypePlaceholder<1>, TypePlaceholder<2>, TypePlaceholder<3>>;

  VariantType variant;

  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<0>>::value == 0);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<1>>::value == 1);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<2>>::value == 2);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<3>>::value == 3);

  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<0>>() == 0);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<1>>() == 1);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<2>>() == 2);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<3>>() == 3);

  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<0>, TypePlaceholder<0>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<1>, TypePlaceholder<1>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<2>, TypePlaceholder<2>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<3>, TypePlaceholder<3>>::value));
}

struct TestFunctor
{
  template <vtkm::IdComponent Index>
  vtkm::FloatDefault operator()(TypePlaceholder<Index>, vtkm::Id expectedValue)
  {
    VTKM_TEST_ASSERT(Index == expectedValue, "Index = ", Index, ", expected = ", expectedValue);
    return TestValue(expectedValue, vtkm::FloatDefault{});
  }
};

void TestGet()
{
  std::cout << "Test Get" << std::endl;

  using VariantType = vtkm::internal::Variant<TypePlaceholder<0>,
                                              TypePlaceholder<1>,
                                              vtkm::Id,
                                              TypePlaceholder<2>,
                                              TypePlaceholder<3>>;

  const vtkm::Id expectedValue = TestValue(3, vtkm::Id{});

  VariantType variant = expectedValue;
  VTKM_TEST_ASSERT(variant.GetIndex() == 2);

  VTKM_TEST_ASSERT(variant.Get<2>() == expectedValue);
  VTKM_TEST_ASSERT(variant.Get<vtkm::Id>() == expectedValue);
}

void TestCastAndCall()
{
  std::cout << "Test CastAndCall" << std::endl;

  using VariantType = vtkm::internal::
    Variant<TypePlaceholder<0>, TypePlaceholder<1>, TypePlaceholder<2>, TypePlaceholder<3>>;
  vtkm::FloatDefault result;

  VariantType variant0{ TypePlaceholder<0>{} };
  result = variant0.CastAndCall(TestFunctor(), 0);
  VTKM_TEST_ASSERT(test_equal(result, TestValue(0, vtkm::FloatDefault{})));

  VariantType variant1{ TypePlaceholder<1>{} };
  result = variant1.CastAndCall(TestFunctor(), 1);
  VTKM_TEST_ASSERT(test_equal(result, TestValue(1, vtkm::FloatDefault{})));

  const VariantType variant2{ TypePlaceholder<2>{} };
  result = variant2.CastAndCall(TestFunctor(), 2);
  VTKM_TEST_ASSERT(test_equal(result, TestValue(2, vtkm::FloatDefault{})));

  VariantType variant3{ TypePlaceholder<3>{} };
  result = variant3.CastAndCall(TestFunctor(), 3);
  VTKM_TEST_ASSERT(test_equal(result, TestValue(3, vtkm::FloatDefault{})));
}

void TestCopyDestroy()
{
  std::cout << "Test copy destroy" << std::endl;

  struct CountConstructDestruct
  {
    vtkm::Id* Count;
    CountConstructDestruct(vtkm::Id* count)
      : Count(count)
    {
      ++(*this->Count);
    }
    CountConstructDestruct(const CountConstructDestruct& src)
      : Count(src.Count)
    {
      ++(*this->Count);
    }
    ~CountConstructDestruct() { --(*this->Count); }
  };

  using VariantType = vtkm::internal::Variant<TypePlaceholder<0>,
                                              TypePlaceholder<1>,
                                              CountConstructDestruct,
                                              TypePlaceholder<2>,
                                              TypePlaceholder<3>>;
  vtkm::Id count = 0;

  VariantType variant1 = CountConstructDestruct(&count);
  VTKM_TEST_ASSERT(count == 1, count);
  VTKM_TEST_ASSERT(*variant1.Get<2>().Count == 1);

  {
    VariantType variant2{ variant1 };
    VTKM_TEST_ASSERT(count == 2, count);
    VTKM_TEST_ASSERT(*variant1.Get<2>().Count == 2);
    VTKM_TEST_ASSERT(*variant2.Get<2>().Count == 2);
  }
  VTKM_TEST_ASSERT(count == 1, count);
  VTKM_TEST_ASSERT(*variant1.Get<2>().Count == 1);

  {
    VariantType variant3{ VariantType(CountConstructDestruct(&count)) };
    VTKM_TEST_ASSERT(count == 2, count);
    VTKM_TEST_ASSERT(*variant1.Get<2>().Count == 2);
    VTKM_TEST_ASSERT(*variant3.Get<2>().Count == 2);
  }
  VTKM_TEST_ASSERT(count == 1, count);
  VTKM_TEST_ASSERT(*variant1.Get<2>().Count == 1);

  {
    VariantType variant4{ variant1 };
    VTKM_TEST_ASSERT(count == 2, count);
    VTKM_TEST_ASSERT(*variant1.Get<2>().Count == 2);
    VTKM_TEST_ASSERT(*variant4.Get<2>().Count == 2);

    variant4 = TypePlaceholder<0>{};
    VTKM_TEST_ASSERT(count == 1, count);
    VTKM_TEST_ASSERT(*variant1.Get<2>().Count == 1);

    variant4 = VariantType{ TypePlaceholder<1>{} };
    VTKM_TEST_ASSERT(count == 1, count);
    VTKM_TEST_ASSERT(*variant1.Get<2>().Count == 1);

    variant4 = variant1;
    VTKM_TEST_ASSERT(count == 2, count);
    VTKM_TEST_ASSERT(*variant1.Get<2>().Count == 2);
    VTKM_TEST_ASSERT(*variant4.Get<2>().Count == 2);
  }
}

void TestEmplace()
{
  std::cout << "Test Emplace" << std::endl;

  using VariantType = vtkm::internal::Variant<vtkm::Id, vtkm::Id3, std::vector<vtkm::Id>>;

  VariantType variant;
  variant.Emplace<vtkm::Id>(TestValue(0, vtkm::Id{}));
  VTKM_TEST_ASSERT(variant.GetIndex() == 0);
  VTKM_TEST_ASSERT(variant.Get<vtkm::Id>() == TestValue(0, vtkm::Id{}));

  variant.Emplace<1>(TestValue(1, vtkm::Id{}));
  VTKM_TEST_ASSERT(variant.GetIndex() == 1);
  VTKM_TEST_ASSERT(variant.Get<vtkm::Id3>() == vtkm::Id3{ TestValue(1, vtkm::Id{}) });

  variant.Emplace<1>(TestValue(2, vtkm::Id{}), TestValue(3, vtkm::Id{}), TestValue(4, vtkm::Id{}));
  VTKM_TEST_ASSERT(variant.GetIndex() == 1);
  VTKM_TEST_ASSERT(variant.Get<vtkm::Id3>() == vtkm::Id3{ TestValue(2, vtkm::Id{}),
                                                          TestValue(3, vtkm::Id{}),
                                                          TestValue(4, vtkm::Id{}) });

  variant.Emplace<2>(
    { TestValue(5, vtkm::Id{}), TestValue(6, vtkm::Id{}), TestValue(7, vtkm::Id{}) });
  VTKM_TEST_ASSERT(variant.GetIndex() == 2);
  VTKM_TEST_ASSERT(variant.Get<std::vector<vtkm::Id>>() ==
                   std::vector<vtkm::Id>{ TestValue(5, vtkm::Id{}),
                                          TestValue(6, vtkm::Id{}),
                                          TestValue(7, vtkm::Id{}) });
}

void RunTest()
{
  TestSize();
  TestIndexing();
  TestGet();
  TestCastAndCall();
  TestCopyDestroy();
  TestEmplace();
}

} // anonymous namespace

int UnitTestVariant(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(RunTest, argc, argv);
}
