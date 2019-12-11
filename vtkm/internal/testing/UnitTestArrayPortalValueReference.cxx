//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/testing/Testing.h>

#include <vtkm/TypeTraits.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/internal/ArrayPortalValueReference.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename ArrayPortalType>
void SetReference(vtkm::Id index, vtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref)
{
  using ValueType = typename ArrayPortalType::ValueType;
  ref = TestValue(index, ValueType());
}

template <typename ArrayPortalType>
void CheckReference(vtkm::Id index, vtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref)
{
  using ValueType = typename ArrayPortalType::ValueType;
  VTKM_TEST_ASSERT(test_equal(ref, TestValue(index, ValueType())), "Got bad value from reference.");
}

template <typename ArrayPortalType>
void TryOperatorsNoVec(vtkm::Id index,
                       vtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref,
                       vtkm::TypeTraitsScalarTag)
{
  using ValueType = typename ArrayPortalType::ValueType;

  ValueType expected = TestValue(index, ValueType());
  VTKM_TEST_ASSERT(ref.Get() == expected, "Reference did not start out as expected.");

  VTKM_TEST_ASSERT(!(ref < ref));
  VTKM_TEST_ASSERT(ref < ValueType(expected + ValueType(1)));
  VTKM_TEST_ASSERT(ValueType(expected - ValueType(1)) < ref);

  VTKM_TEST_ASSERT(!(ref > ref));
  VTKM_TEST_ASSERT(ref > ValueType(expected - ValueType(1)));
  VTKM_TEST_ASSERT(ValueType(expected + ValueType(1)) > ref);

  VTKM_TEST_ASSERT(ref <= ref);
  VTKM_TEST_ASSERT(ref <= ValueType(expected + ValueType(1)));
  VTKM_TEST_ASSERT(ValueType(expected - ValueType(1)) <= ref);

  VTKM_TEST_ASSERT(ref >= ref);
  VTKM_TEST_ASSERT(ref >= ValueType(expected - ValueType(1)));
  VTKM_TEST_ASSERT(ValueType(expected + ValueType(1)) >= ref);
}

template <typename ArrayPortalType>
void TryOperatorsNoVec(vtkm::Id,
                       vtkm::internal::ArrayPortalValueReference<ArrayPortalType>,
                       vtkm::TypeTraitsVectorTag)
{
}

template <typename ArrayPortalType>
void TryOperatorsInt(vtkm::Id index,
                     vtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref,
                     vtkm::TypeTraitsScalarTag,
                     vtkm::TypeTraitsIntegerTag)
{
  using ValueType = typename ArrayPortalType::ValueType;

  const ValueType operand = TestValue(ARRAY_SIZE, ValueType());
  ValueType expected = TestValue(index, ValueType());
  VTKM_TEST_ASSERT(ref.Get() == expected, "Reference did not start out as expected.");

  VTKM_TEST_ASSERT((ref % ref) == (expected % expected));
  VTKM_TEST_ASSERT((ref % expected) == (expected % expected));
  VTKM_TEST_ASSERT((expected % ref) == (expected % expected));

  VTKM_TEST_ASSERT((ref ^ ref) == (expected ^ expected));
  VTKM_TEST_ASSERT((ref ^ expected) == (expected ^ expected));
  VTKM_TEST_ASSERT((expected ^ ref) == (expected ^ expected));

  VTKM_TEST_ASSERT((ref | ref) == (expected | expected));
  VTKM_TEST_ASSERT((ref | expected) == (expected | expected));
  VTKM_TEST_ASSERT((expected | ref) == (expected | expected));

  VTKM_TEST_ASSERT((ref & ref) == (expected & expected));
  VTKM_TEST_ASSERT((ref & expected) == (expected & expected));
  VTKM_TEST_ASSERT((expected & ref) == (expected & expected));

  VTKM_TEST_ASSERT((ref << ref) == (expected << expected));
  VTKM_TEST_ASSERT((ref << expected) == (expected << expected));
  VTKM_TEST_ASSERT((expected << ref) == (expected << expected));

  VTKM_TEST_ASSERT((ref << ref) == (expected << expected));
  VTKM_TEST_ASSERT((ref << expected) == (expected << expected));
  VTKM_TEST_ASSERT((expected << ref) == (expected << expected));

  VTKM_TEST_ASSERT(~ref == ~expected);

  VTKM_TEST_ASSERT(!(!ref));

  VTKM_TEST_ASSERT(ref && ref);
  VTKM_TEST_ASSERT(ref && expected);
  VTKM_TEST_ASSERT(expected && ref);

  VTKM_TEST_ASSERT(ref || ref);
  VTKM_TEST_ASSERT(ref || expected);
  VTKM_TEST_ASSERT(expected || ref);

#if defined(VTKM_CLANG) && __clang_major__ >= 7
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif

  ref &= ref;
  expected &= expected;
  VTKM_TEST_ASSERT(ref == expected);
  ref &= operand;
  expected &= operand;
  VTKM_TEST_ASSERT(ref == expected);

  ref |= ref;
  expected |= expected;
  VTKM_TEST_ASSERT(ref == expected);
  ref |= operand;
  expected |= operand;
  VTKM_TEST_ASSERT(ref == expected);

  ref >>= ref;
  expected >>= expected;
  VTKM_TEST_ASSERT(ref == expected);
  ref >>= operand;
  expected >>= operand;
  VTKM_TEST_ASSERT(ref == expected);

  ref <<= ref;
  expected <<= expected;
  VTKM_TEST_ASSERT(ref == expected);
  ref <<= operand;
  expected <<= operand;
  VTKM_TEST_ASSERT(ref == expected);

  ref ^= ref;
  expected ^= expected;
  VTKM_TEST_ASSERT(ref == expected);
  ref ^= operand;
  expected ^= operand;
  VTKM_TEST_ASSERT(ref == expected);

#if defined(VTKM_CLANG) && __clang_major__ >= 7
#pragma clang diagnostic pop
#endif
}

template <typename ArrayPortalType, typename DimTag, typename NumericTag>
void TryOperatorsInt(vtkm::Id,
                     vtkm::internal::ArrayPortalValueReference<ArrayPortalType>,
                     DimTag,
                     NumericTag)
{
}

template <typename ArrayPortalType>
void TryOperators(vtkm::Id index, vtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref)
{
  using ValueType = typename ArrayPortalType::ValueType;

  const ValueType operand = TestValue(ARRAY_SIZE, ValueType());
  ValueType expected = TestValue(index, ValueType());
  VTKM_TEST_ASSERT(ref.Get() == expected, "Reference did not start out as expected.");

  // Test comparison operators.
  VTKM_TEST_ASSERT(ref == ref);
  VTKM_TEST_ASSERT(ref == expected);
  VTKM_TEST_ASSERT(expected == ref);

  VTKM_TEST_ASSERT(!(ref != ref));
  VTKM_TEST_ASSERT(!(ref != expected));
  VTKM_TEST_ASSERT(!(expected != ref));

  TryOperatorsNoVec(index, ref, typename vtkm::TypeTraits<ValueType>::DimensionalityTag());

  VTKM_TEST_ASSERT((ref + ref) == (expected + expected));
  VTKM_TEST_ASSERT((ref + expected) == (expected + expected));
  VTKM_TEST_ASSERT((expected + ref) == (expected + expected));

  VTKM_TEST_ASSERT((ref - ref) == (expected - expected));
  VTKM_TEST_ASSERT((ref - expected) == (expected - expected));
  VTKM_TEST_ASSERT((expected - ref) == (expected - expected));

  VTKM_TEST_ASSERT((ref * ref) == (expected * expected));
  VTKM_TEST_ASSERT((ref * expected) == (expected * expected));
  VTKM_TEST_ASSERT((expected * ref) == (expected * expected));

  VTKM_TEST_ASSERT((ref / ref) == (expected / expected));
  VTKM_TEST_ASSERT((ref / expected) == (expected / expected));
  VTKM_TEST_ASSERT((expected / ref) == (expected / expected));


#if defined(VTKM_CLANG) && __clang_major__ >= 7
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif

  ref += ref;
  expected += expected;
  VTKM_TEST_ASSERT(ref == expected);
  ref += operand;
  expected += operand;
  VTKM_TEST_ASSERT(ref == expected);

  ref -= ref;
  expected -= expected;
  VTKM_TEST_ASSERT(ref == expected);
  ref -= operand;
  expected -= operand;
  VTKM_TEST_ASSERT(ref == expected);

  ref *= ref;
  expected *= expected;
  VTKM_TEST_ASSERT(ref == expected);
  ref *= operand;
  expected *= operand;
  VTKM_TEST_ASSERT(ref == expected);

  ref /= ref;
  expected /= expected;
  VTKM_TEST_ASSERT(ref == expected);
  ref /= operand;
  expected /= operand;
  VTKM_TEST_ASSERT(ref == expected);

#if defined(VTKM_CLANG) && __clang_major__ >= 7
#pragma clang diagnostic pop
#endif

  // Reset ref
  ref = TestValue(index, ValueType());

  TryOperatorsInt(index,
                  ref,
                  typename vtkm::TypeTraits<ValueType>::DimensionalityTag(),
                  typename vtkm::TypeTraits<ValueType>::NumericTag());
}

struct DoTestForType
{
  template <typename ValueType>
  VTKM_CONT void operator()(const ValueType&) const
  {
    vtkm::cont::ArrayHandle<ValueType> array;
    array.Allocate(ARRAY_SIZE);

    std::cout << "Set array using reference" << std::endl;
    using PortalType = typename vtkm::cont::ArrayHandle<ValueType>::PortalControl;
    PortalType portal = array.GetPortalControl();
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      SetReference(index, vtkm::internal::ArrayPortalValueReference<PortalType>(portal, index));
    }

    std::cout << "Check values" << std::endl;
    CheckPortal(portal);

    std::cout << "Check references in set array." << std::endl;
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      CheckReference(index, vtkm::internal::ArrayPortalValueReference<PortalType>(portal, index));
    }

    std::cout << "Check that operators work." << std::endl;
    // Start at 1 to avoid issues with 0.
    for (vtkm::Id index = 1; index < ARRAY_SIZE; ++index)
    {
      TryOperators(index, vtkm::internal::ArrayPortalValueReference<PortalType>(portal, index));
    }
  }
};

void DoTest()
{
  // We are not testing on the default (exemplar) types because they include unsigned bytes, and
  // simply doing a += (or similar) operation on them automatically creates a conversion warning
  // on some compilers. Since we want to test these operators, just remove the short types from
  // the list to avoid the warning.
  vtkm::testing::Testing::TryTypes(DoTestForType(),
                                   vtkm::List<vtkm::Id, vtkm::FloatDefault, vtkm::Vec3f_64>());
}

} // anonymous namespace

int UnitTestArrayPortalValueReference(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(DoTest, argc, argv);
}
