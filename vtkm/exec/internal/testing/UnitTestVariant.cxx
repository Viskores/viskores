//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/internal/Variant.h>

#include <vtkm/testing/Testing.h>

#include <cstdlib>
#include <vector>

namespace test_variant
{

template <vtkm::IdComponent Index>
struct TypePlaceholder
{
};

// A class that is trivially copiable but not totally trivial.
struct TrivialCopy
{
  vtkm::Id Value = 0;
};

static vtkm::Id g_NonTrivialCount;

// A class that must is not trivial to copy nor construct.
struct NonTrivial
{
  vtkm::Id Value;
  NonTrivial* Self;

  void CheckState() const
  {
    VTKM_TEST_ASSERT(this->Value == 12345);
    VTKM_TEST_ASSERT(this->Self == this);
  }

  NonTrivial()
    : Value(12345)
    , Self(this)
  {
    this->CheckState();
    ++g_NonTrivialCount;
  }

  NonTrivial(const NonTrivial& src)
    : Value(src.Value)
  {
    src.CheckState();
    this->Self = this;
    this->CheckState();
    ++g_NonTrivialCount;
  }

  NonTrivial& operator=(const NonTrivial& src)
  {
    this->CheckState();
    src.CheckState();
    return *this;
  }

  ~NonTrivial()
  {
    if ((this->Value == 12345) && (this->Self == this))
    {
      // Normal destruction
      this->Value = -1;
      this->Self = nullptr;
      --g_NonTrivialCount;
    }
    else
    {
      // Normally we would use VTKM_TEST_ASSERT or VTKM_TEST_FAIL, but it's not good to throw
      // exceptions from destructors (especially since Variant marks these calls as noexcept).
      // Instead, just check and terminate the program.
      std::cout << "ERROR at " << __FILE__ << ":" << __LINE__ << ":\n";
      std::cout << "Destroying a class that was not properly constructed." << std::endl;
      std::exit(1);
    }
  }
};

void TestSize()
{
  std::cout << "Test size" << std::endl;

  using VariantType = vtkm::exec::internal::Variant<float, double, char, short, int, long>;

  constexpr size_t variantSize = sizeof(VariantType);

  VTKM_TEST_ASSERT(variantSize <= 16,
                   "Size of variant should not be larger than biggest type plus and index. ",
                   variantSize);
}

void TestIndexing()
{
  std::cout << "Test indexing" << std::endl;

  using VariantType = vtkm::exec::internal::Variant<TypePlaceholder<0>,
                                                    TypePlaceholder<1>,
                                                    TypePlaceholder<2>,
                                                    TypePlaceholder<3>,
                                                    TypePlaceholder<4>,
                                                    TypePlaceholder<5>,
                                                    TypePlaceholder<6>,
                                                    TypePlaceholder<7>,
                                                    TypePlaceholder<8>,
                                                    TypePlaceholder<9>,
                                                    TypePlaceholder<10>,
                                                    TypePlaceholder<11>,
                                                    TypePlaceholder<12>,
                                                    TypePlaceholder<13>,
                                                    TypePlaceholder<14>,
                                                    TypePlaceholder<15>,
                                                    TypePlaceholder<16>,
                                                    TypePlaceholder<17>,
                                                    TypePlaceholder<18>,
                                                    TypePlaceholder<19>,
                                                    TypePlaceholder<20>,
                                                    TypePlaceholder<21>,
                                                    TypePlaceholder<22>,
                                                    TypePlaceholder<23>,
                                                    TypePlaceholder<24>,
                                                    TypePlaceholder<25>,
                                                    TypePlaceholder<26>,
                                                    TypePlaceholder<27>,
                                                    TypePlaceholder<28>,
                                                    TypePlaceholder<29>>;

  VariantType variant;

  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<0>>::value == 0);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<1>>::value == 1);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<2>>::value == 2);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<3>>::value == 3);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<4>>::value == 4);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<5>>::value == 5);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<6>>::value == 6);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<7>>::value == 7);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<8>>::value == 8);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<9>>::value == 9);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<10>>::value == 10);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<11>>::value == 11);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<12>>::value == 12);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<13>>::value == 13);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<14>>::value == 14);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<15>>::value == 15);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<16>>::value == 16);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<17>>::value == 17);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<18>>::value == 18);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<19>>::value == 19);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<20>>::value == 20);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<21>>::value == 21);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<22>>::value == 22);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<23>>::value == 23);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<24>>::value == 24);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<25>>::value == 25);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<26>>::value == 26);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<27>>::value == 27);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<28>>::value == 28);
  VTKM_TEST_ASSERT(VariantType::IndexOf<TypePlaceholder<29>>::value == 29);

  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<0>>() == 0);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<1>>() == 1);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<2>>() == 2);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<3>>() == 3);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<4>>() == 4);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<5>>() == 5);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<6>>() == 6);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<7>>() == 7);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<8>>() == 8);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<9>>() == 9);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<10>>() == 10);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<11>>() == 11);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<12>>() == 12);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<13>>() == 13);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<14>>() == 14);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<15>>() == 15);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<16>>() == 16);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<17>>() == 17);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<18>>() == 18);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<19>>() == 19);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<20>>() == 20);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<21>>() == 21);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<22>>() == 22);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<23>>() == 23);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<24>>() == 24);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<25>>() == 25);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<26>>() == 26);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<27>>() == 27);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<28>>() == 28);
  VTKM_TEST_ASSERT(variant.GetIndexOf<TypePlaceholder<29>>() == 29);

  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<0>, TypePlaceholder<0>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<1>, TypePlaceholder<1>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<2>, TypePlaceholder<2>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<3>, TypePlaceholder<3>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<4>, TypePlaceholder<4>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<5>, TypePlaceholder<5>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<6>, TypePlaceholder<6>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<7>, TypePlaceholder<7>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<8>, TypePlaceholder<8>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<9>, TypePlaceholder<9>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<10>, TypePlaceholder<10>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<11>, TypePlaceholder<11>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<12>, TypePlaceholder<12>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<13>, TypePlaceholder<13>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<14>, TypePlaceholder<14>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<15>, TypePlaceholder<15>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<16>, TypePlaceholder<16>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<17>, TypePlaceholder<17>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<18>, TypePlaceholder<18>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<19>, TypePlaceholder<19>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<20>, TypePlaceholder<20>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<21>, TypePlaceholder<21>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<22>, TypePlaceholder<22>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<23>, TypePlaceholder<23>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<24>, TypePlaceholder<24>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<25>, TypePlaceholder<25>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<26>, TypePlaceholder<26>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<27>, TypePlaceholder<27>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<28>, TypePlaceholder<28>>::value));
  VTKM_STATIC_ASSERT((std::is_same<VariantType::TypeAt<29>, TypePlaceholder<29>>::value));
}

void TestTriviallyCopyable()
{
#ifdef VTKM_USE_STD_IS_TRIVIAL
  // Make sure base types are behaving as expected
  VTKM_STATIC_ASSERT(vtkmstd::is_trivially_constructible<float>::value);
  VTKM_STATIC_ASSERT(vtkmstd::is_trivially_copyable<float>::value);
  VTKM_STATIC_ASSERT(vtkmstd::is_trivial<float>::value);
  VTKM_STATIC_ASSERT(vtkmstd::is_trivially_constructible<int>::value);
  VTKM_STATIC_ASSERT(vtkmstd::is_trivially_copyable<int>::value);
  VTKM_STATIC_ASSERT(vtkmstd::is_trivial<int>::value);
  VTKM_STATIC_ASSERT(!vtkmstd::is_trivially_constructible<NonTrivial>::value);
  VTKM_STATIC_ASSERT(!vtkmstd::is_trivially_copyable<NonTrivial>::value);
  VTKM_STATIC_ASSERT(!vtkmstd::is_trivial<NonTrivial>::value);
  VTKM_STATIC_ASSERT(!vtkmstd::is_trivially_constructible<TrivialCopy>::value);
  VTKM_STATIC_ASSERT(vtkmstd::is_trivially_copyable<TrivialCopy>::value);
  VTKM_STATIC_ASSERT(!vtkmstd::is_trivial<TrivialCopy>::value);

  // A variant of trivially constructable things should be trivially constructable
  VTKM_STATIC_ASSERT((vtkmstd::is_trivially_constructible<
                      vtkm::exec::internal::detail::VariantUnion<float, int>>::value));
  VTKM_STATIC_ASSERT(
    (vtkmstd::is_trivially_constructible<vtkm::exec::internal::Variant<float, int>>::value));

  // A variant of trivially copyable things should be trivially copyable
  VTKM_STATIC_ASSERT((vtkmstd::is_trivially_copyable<
                      vtkm::exec::internal::detail::VariantUnion<float, int, TrivialCopy>>::value));
  VTKM_STATIC_ASSERT((
    vtkmstd::is_trivially_copyable<vtkm::exec::internal::Variant<float, int, TrivialCopy>>::value));

  // A variant of any non-trivially constructable things is not trivially copyable
  VTKM_STATIC_ASSERT((!vtkmstd::is_trivially_constructible<
                      vtkm::exec::internal::detail::VariantUnion<NonTrivial, float, int>>::value));
  VTKM_STATIC_ASSERT((!vtkmstd::is_trivially_constructible<
                      vtkm::exec::internal::detail::VariantUnion<float, NonTrivial, int>>::value));
  VTKM_STATIC_ASSERT((!vtkmstd::is_trivially_constructible<
                      vtkm::exec::internal::detail::VariantUnion<float, int, NonTrivial>>::value));
  VTKM_STATIC_ASSERT((!vtkmstd::is_trivially_constructible<
                      vtkm::exec::internal::Variant<NonTrivial, float, int>>::value));
  VTKM_STATIC_ASSERT((!vtkmstd::is_trivially_constructible<
                      vtkm::exec::internal::Variant<float, NonTrivial, int>>::value));
  VTKM_STATIC_ASSERT((!vtkmstd::is_trivially_constructible<
                      vtkm::exec::internal::Variant<float, int, NonTrivial>>::value));

  // A variant of any non-trivially copyable things is not trivially copyable
  VTKM_STATIC_ASSERT((!vtkmstd::is_trivially_copyable<
                      vtkm::exec::internal::detail::VariantUnion<NonTrivial, float, int>>::value));
  VTKM_STATIC_ASSERT((!vtkmstd::is_trivially_copyable<
                      vtkm::exec::internal::detail::VariantUnion<float, NonTrivial, int>>::value));
  VTKM_STATIC_ASSERT((!vtkmstd::is_trivially_copyable<
                      vtkm::exec::internal::detail::VariantUnion<float, int, NonTrivial>>::value));
  VTKM_STATIC_ASSERT((
    !vtkmstd::is_trivially_copyable<vtkm::exec::internal::Variant<NonTrivial, float, int>>::value));
  VTKM_STATIC_ASSERT((
    !vtkmstd::is_trivially_copyable<vtkm::exec::internal::Variant<float, NonTrivial, int>>::value));
  VTKM_STATIC_ASSERT((
    !vtkmstd::is_trivially_copyable<vtkm::exec::internal::Variant<float, int, NonTrivial>>::value));

  // A variant of trivial things should be trivial
  VTKM_STATIC_ASSERT((vtkmstd::is_trivial<vtkm::exec::internal::Variant<float, int>>::value));
  VTKM_STATIC_ASSERT(
    (!vtkmstd::is_trivial<vtkm::exec::internal::Variant<float, int, TrivialCopy>>::value));
  VTKM_STATIC_ASSERT(
    (!vtkmstd::is_trivial<vtkm::exec::internal::Variant<float, int, NonTrivial>>::value));
#endif // VTKM_USE_STD_IS_TRIVIAL
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

  using VariantType = vtkm::exec::internal::Variant<TypePlaceholder<0>,
                                                    TypePlaceholder<1>,
                                                    vtkm::Id,
                                                    TypePlaceholder<3>,
                                                    TypePlaceholder<4>,
                                                    TypePlaceholder<5>,
                                                    TypePlaceholder<6>,
                                                    TypePlaceholder<7>,
                                                    TypePlaceholder<8>,
                                                    TypePlaceholder<9>,
                                                    TypePlaceholder<10>,
                                                    TypePlaceholder<11>,
                                                    TypePlaceholder<12>,
                                                    TypePlaceholder<13>,
                                                    TypePlaceholder<14>,
                                                    TypePlaceholder<15>,
                                                    TypePlaceholder<16>,
                                                    TypePlaceholder<17>,
                                                    TypePlaceholder<18>,
                                                    TypePlaceholder<19>,
                                                    TypePlaceholder<20>,
                                                    TypePlaceholder<21>,
                                                    TypePlaceholder<22>,
                                                    TypePlaceholder<23>,
                                                    TypePlaceholder<24>,
                                                    TypePlaceholder<25>,
                                                    TypePlaceholder<26>,
                                                    vtkm::Float32,
                                                    TypePlaceholder<28>,
                                                    TypePlaceholder<29>>;

  {
    const vtkm::Id expectedValue = TestValue(3, vtkm::Id{});

    VariantType variant = expectedValue;
    VTKM_TEST_ASSERT(variant.GetIndex() == 2);

    VTKM_TEST_ASSERT(variant.Get<2>() == expectedValue);

    VTKM_TEST_ASSERT(variant.Get<vtkm::Id>() == expectedValue);
  }

  {
    const vtkm::Float32 expectedValue = TestValue(4, vtkm::Float32{});

    VariantType variant = expectedValue;
    VTKM_TEST_ASSERT(variant.GetIndex() == 27);

    VTKM_TEST_ASSERT(variant.Get<27>() == expectedValue);

    VTKM_TEST_ASSERT(variant.Get<vtkm::Float32>() == expectedValue);
  }
}

void TestCastAndCall()
{
  std::cout << "Test CastAndCall" << std::endl;

  using VariantType = vtkm::exec::internal::Variant<TypePlaceholder<0>,
                                                    TypePlaceholder<1>,
                                                    TypePlaceholder<2>,
                                                    TypePlaceholder<3>,
                                                    TypePlaceholder<4>,
                                                    TypePlaceholder<5>,
                                                    TypePlaceholder<6>,
                                                    TypePlaceholder<7>,
                                                    TypePlaceholder<8>,
                                                    TypePlaceholder<9>,
                                                    TypePlaceholder<10>,
                                                    TypePlaceholder<11>,
                                                    TypePlaceholder<12>,
                                                    TypePlaceholder<13>,
                                                    TypePlaceholder<14>,
                                                    TypePlaceholder<15>,
                                                    TypePlaceholder<16>,
                                                    TypePlaceholder<17>,
                                                    TypePlaceholder<18>,
                                                    TypePlaceholder<19>,
                                                    TypePlaceholder<20>,
                                                    TypePlaceholder<21>,
                                                    TypePlaceholder<22>,
                                                    TypePlaceholder<23>,
                                                    TypePlaceholder<24>,
                                                    TypePlaceholder<25>,
                                                    TypePlaceholder<26>,
                                                    TypePlaceholder<27>,
                                                    TypePlaceholder<28>,
                                                    TypePlaceholder<29>>;
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

  VariantType variant26{ TypePlaceholder<26>{} };
  result = variant26.CastAndCall(TestFunctor(), 26);
  VTKM_TEST_ASSERT(test_equal(result, TestValue(26, vtkm::FloatDefault{})));
}

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
  CountConstructDestruct& operator=(const CountConstructDestruct&) = delete;
};

void TestCopyDestroy()
{
  std::cout << "Test copy destroy" << std::endl;

  using VariantType = vtkm::exec::internal::Variant<TypePlaceholder<0>,
                                                    TypePlaceholder<1>,
                                                    CountConstructDestruct,
                                                    TypePlaceholder<3>,
                                                    TypePlaceholder<4>>;
#ifdef VTKM_USE_STD_IS_TRIVIAL
  VTKM_STATIC_ASSERT(!vtkmstd::is_trivially_copyable<VariantType>::value);
#endif // VTKM_USE_STD_IS_TRIVIAL
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

  using VariantType = vtkm::exec::internal::Variant<vtkm::Id, vtkm::Id3, std::vector<vtkm::Id>>;

  VariantType variant;
  variant.Emplace<vtkm::Id>(TestValue(0, vtkm::Id{}));
  VTKM_TEST_ASSERT(variant.GetIndex() == 0);
  VTKM_TEST_ASSERT(variant.Get<vtkm::Id>() == TestValue(0, vtkm::Id{}));

  variant.Emplace<1>(TestValue(1, vtkm::Id{}));
  VTKM_TEST_ASSERT(variant.GetIndex() == 1);
  VTKM_TEST_ASSERT(variant.Get<vtkm::Id3>() == vtkm::Id3{ TestValue(1, vtkm::Id{}) });

  variant.Emplace<1>(TestValue(2, vtkm::Id{}), TestValue(3, vtkm::Id{}), TestValue(4, vtkm::Id{}));
  VTKM_TEST_ASSERT(variant.GetIndex() == 1);
  VTKM_TEST_ASSERT(
    variant.Get<vtkm::Id3>() ==
    vtkm::Id3{ TestValue(2, vtkm::Id{}), TestValue(3, vtkm::Id{}), TestValue(4, vtkm::Id{}) });

  variant.Emplace<2>(
    { TestValue(5, vtkm::Id{}), TestValue(6, vtkm::Id{}), TestValue(7, vtkm::Id{}) });
  VTKM_TEST_ASSERT(variant.GetIndex() == 2);
  VTKM_TEST_ASSERT(variant.Get<std::vector<vtkm::Id>>() ==
                   std::vector<vtkm::Id>{ TestValue(5, vtkm::Id{}),
                                          TestValue(6, vtkm::Id{}),
                                          TestValue(7, vtkm::Id{}) });
}

void TestConstructDestruct()
{
  std::cout << "Make sure constructors and destructors are called correctly" << std::endl;

  g_NonTrivialCount = 0;

  using VariantType = vtkm::exec::internal::Variant<NonTrivial, TrivialCopy>;

  {
    VariantType variant1 = NonTrivial{};
    VariantType variant2 = variant1;
    variant2 = NonTrivial{};
    NonTrivial nonTrivial;
    VariantType variant3 = nonTrivial;
    VariantType variant4;
    variant4.Emplace<NonTrivial>();
    VariantType variant5(VariantType(NonTrivial{}));
  }

  VTKM_TEST_ASSERT(g_NonTrivialCount == 0);
}

void TestCopySelf()
{
  std::cout << "Make sure copying a Variant to itself works" << std::endl;

  using VariantType =
    vtkm::exec::internal::Variant<TypePlaceholder<0>, NonTrivial, TypePlaceholder<2>>;

  VariantType variant{ NonTrivial{} };
  VariantType& variantRef = variant;
  variant = variantRef;
  variant = variant.Get<NonTrivial>();
}

void RunTest()
{
  TestSize();
  TestIndexing();
  TestTriviallyCopyable();
  TestGet();
  TestCastAndCall();
  TestCopyDestroy();
  TestEmplace();
  TestConstructDestruct();
  TestCopySelf();
}

} // namespace test_variant

int UnitTestVariant(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(test_variant::RunTest, argc, argv);
}
