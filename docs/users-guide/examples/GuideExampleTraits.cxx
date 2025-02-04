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

#include <viskores/TypeTraits.h>
#include <viskores/VecTraits.h>

#include <viskores/testing/Testing.h>

////
//// BEGIN-EXAMPLE TypeTraits
////
#include <viskores/TypeTraits.h>

#include <viskores/Math.h>
//// PAUSE-EXAMPLE
namespace TraitsExamples
{
//// RESUME-EXAMPLE

template<typename T>
T AnyRemainder(const T& numerator, const T& denominator);

namespace detail
{

template<typename T>
T AnyRemainderImpl(const T& numerator,
                   const T& denominator,
                   viskores::TypeTraitsIntegerTag,
                   viskores::TypeTraitsScalarTag)
{
  return numerator % denominator;
}

template<typename T>
T AnyRemainderImpl(const T& numerator,
                   const T& denominator,
                   viskores::TypeTraitsRealTag,
                   viskores::TypeTraitsScalarTag)
{
  // The Viskores math library contains a Remainder function that operates on
  // floating point numbers.
  return viskores::Remainder(numerator, denominator);
}

template<typename T, typename NumericTag>
T AnyRemainderImpl(const T& numerator,
                   const T& denominator,
                   NumericTag,
                   viskores::TypeTraitsVectorTag)
{
  T result;
  for (int componentIndex = 0; componentIndex < T::NUM_COMPONENTS; componentIndex++)
  {
    result[componentIndex] =
      AnyRemainder(numerator[componentIndex], denominator[componentIndex]);
  }
  return result;
}

} // namespace detail

template<typename T>
T AnyRemainder(const T& numerator, const T& denominator)
{
  return detail::AnyRemainderImpl(numerator,
                                  denominator,
                                  typename viskores::TypeTraits<T>::NumericTag(),
                                  typename viskores::TypeTraits<T>::DimensionalityTag());
}
////
//// END-EXAMPLE TypeTraits
////

void TryRemainder()
{
  viskores::Id m1 = AnyRemainder(7, 3);
  VISKORES_TEST_ASSERT(m1 == 1, "Got bad remainder");

  viskores::Float32 m2 = AnyRemainder(7.0f, 3.0f);
  VISKORES_TEST_ASSERT(test_equal(m2, 1), "Got bad remainder");

  viskores::Id3 m3 = AnyRemainder(viskores::Id3(10, 9, 8), viskores::Id3(7, 6, 5));
  VISKORES_TEST_ASSERT(test_equal(m3, viskores::Id3(3, 3, 3)), "Got bad remainder");

  viskores::Vec3f_32 m4 = AnyRemainder(viskores::make_Vec(10, 9, 8), viskores::make_Vec(7, 6, 5));
  VISKORES_TEST_ASSERT(test_equal(m4, viskores::make_Vec(3, 3, 3)), "Got bad remainder");
}

template<typename T>
struct TypeTraits;

////
//// BEGIN-EXAMPLE TypeTraitsImpl
////
//// PAUSE-EXAMPLE
#if 0
//// RESUME-EXAMPLE
namespace viskores {
//// PAUSE-EXAMPLE
#endif
//// RESUME-EXAMPLE

template<>
struct TypeTraits<viskores::Float32>
{
  using NumericTag = viskores::TypeTraitsRealTag;
  using DimensionalityTag = viskores::TypeTraitsScalarTag;

  VISKORES_EXEC_CONT
  static viskores::Float32 ZeroInitialization() { return viskores::Float32(0); }
};

//// PAUSE-EXAMPLE
#if 0
//// RESUME-EXAMPLE
}
//// PAUSE-EXAMPLE
#endif
//// RESUME-EXAMPLE
////
//// END-EXAMPLE TypeTraitsImpl
////

void TryCustomTypeTraits()
{
  using CustomTraits = TraitsExamples::TypeTraits<viskores::Float32>;
  using OriginalTraits = viskores::TypeTraits<viskores::Float32>;

  VISKORES_STATIC_ASSERT(
    (std::is_same<CustomTraits::NumericTag, OriginalTraits::NumericTag>::value));
  VISKORES_STATIC_ASSERT((std::is_same<CustomTraits::DimensionalityTag,
                                   OriginalTraits::DimensionalityTag>::value));

  VISKORES_TEST_ASSERT(CustomTraits::ZeroInitialization() ==
                     OriginalTraits::ZeroInitialization(),
                   "Bad zero initialization.");
}

} // namespace TraitsExamples

////
//// BEGIN-EXAMPLE VecTraits
////
#include <viskores/VecTraits.h>
//// PAUSE-EXAMPLE
namespace TraitsExamples
{
//// RESUME-EXAMPLE

// This functor provides a total ordering of vectors. Every compared vector
// will be either less, greater, or equal (assuming all the vector components
// also have a total ordering).
template<typename T>
struct LessTotalOrder
{
  VISKORES_EXEC_CONT
  bool operator()(const T& left, const T& right)
  {
    for (int index = 0; index < viskores::VecTraits<T>::NUM_COMPONENTS; index++)
    {
      using ComponentType = typename viskores::VecTraits<T>::ComponentType;
      const ComponentType& leftValue = viskores::VecTraits<T>::GetComponent(left, index);
      const ComponentType& rightValue = viskores::VecTraits<T>::GetComponent(right, index);
      if (leftValue < rightValue)
      {
        return true;
      }
      if (rightValue < leftValue)
      {
        return false;
      }
    }
    // If we are here, the vectors are equal (or at least equivalent).
    return false;
  }
};

// This functor provides a partial ordering of vectors. It returns true if and
// only if all components satisfy the less operation. It is possible for
// vectors to be neither less, greater, nor equal, but the transitive closure
// is still valid.
template<typename T>
struct LessPartialOrder
{
  VISKORES_EXEC_CONT
  bool operator()(const T& left, const T& right)
  {
    for (int index = 0; index < viskores::VecTraits<T>::NUM_COMPONENTS; index++)
    {
      using ComponentType = typename viskores::VecTraits<T>::ComponentType;
      const ComponentType& leftValue = viskores::VecTraits<T>::GetComponent(left, index);
      const ComponentType& rightValue = viskores::VecTraits<T>::GetComponent(right, index);
      if (!(leftValue < rightValue))
      {
        return false;
      }
    }
    // If we are here, all components satisfy less than relation.
    return true;
  }
};
////
//// END-EXAMPLE VecTraits
////

void TryLess()
{
  LessTotalOrder<viskores::Id> totalLess1;
  VISKORES_TEST_ASSERT(totalLess1(1, 2), "Bad less.");
  VISKORES_TEST_ASSERT(!totalLess1(2, 1), "Bad less.");
  VISKORES_TEST_ASSERT(!totalLess1(1, 1), "Bad less.");

  LessPartialOrder<viskores::Id> partialLess1;
  VISKORES_TEST_ASSERT(partialLess1(1, 2), "Bad less.");
  VISKORES_TEST_ASSERT(!partialLess1(2, 1), "Bad less.");
  VISKORES_TEST_ASSERT(!partialLess1(1, 1), "Bad less.");

  LessTotalOrder<viskores::Id3> totalLess3;
  VISKORES_TEST_ASSERT(totalLess3(viskores::Id3(1, 2, 3), viskores::Id3(3, 2, 1)), "Bad less.");
  VISKORES_TEST_ASSERT(!totalLess3(viskores::Id3(3, 2, 1), viskores::Id3(1, 2, 3)), "Bad less.");
  VISKORES_TEST_ASSERT(!totalLess3(viskores::Id3(1, 2, 3), viskores::Id3(1, 2, 3)), "Bad less.");
  VISKORES_TEST_ASSERT(totalLess3(viskores::Id3(1, 2, 3), viskores::Id3(2, 3, 4)), "Bad less.");

  LessPartialOrder<viskores::Id3> partialLess3;
  VISKORES_TEST_ASSERT(!partialLess3(viskores::Id3(1, 2, 3), viskores::Id3(3, 2, 1)), "Bad less.");
  VISKORES_TEST_ASSERT(!partialLess3(viskores::Id3(3, 2, 1), viskores::Id3(1, 2, 3)), "Bad less.");
  VISKORES_TEST_ASSERT(!partialLess3(viskores::Id3(1, 2, 3), viskores::Id3(1, 2, 3)), "Bad less.");
  VISKORES_TEST_ASSERT(partialLess3(viskores::Id3(1, 2, 3), viskores::Id3(2, 3, 4)), "Bad less.");
}

template<typename T>
struct VecTraits;

////
//// BEGIN-EXAMPLE VecTraitsImpl
////
//// PAUSE-EXAMPLE
#if 0
//// RESUME-EXAMPLE
namespace viskores {
//// PAUSE-EXAMPLE
#endif
//// RESUME-EXAMPLE

template<>
struct VecTraits<viskores::Id3>
{
  using ComponentType = viskores::Id;
  using BaseComponentType = viskores::Id;
  static const int NUM_COMPONENTS = 3;
  using IsSizeStatic = viskores::VecTraitsTagSizeStatic;
  using HasMultipleComponents = viskores::VecTraitsTagMultipleComponents;

  VISKORES_EXEC_CONT
  static viskores::IdComponent GetNumberOfComponents(const viskores::Id3&)
  {
    return NUM_COMPONENTS;
  }

  VISKORES_EXEC_CONT
  static const viskores::Id& GetComponent(const viskores::Id3& vector, int component)
  {
    return vector[component];
  }
  VISKORES_EXEC_CONT
  static viskores::Id& GetComponent(viskores::Id3& vector, int component)
  {
    return vector[component];
  }

  VISKORES_EXEC_CONT
  static void SetComponent(viskores::Id3& vector, int component, viskores::Id value)
  {
    vector[component] = value;
  }

  template<typename NewComponentType>
  using ReplaceComponentType = viskores::Vec<NewComponentType, 3>;

  template<typename NewComponentType>
  using ReplaceBaseComponentType = viskores::Vec<NewComponentType, 3>;

  template<viskores::IdComponent DestSize>
  VISKORES_EXEC_CONT static void CopyInto(const viskores::Id3& src,
                                      viskores::Vec<viskores::Id, DestSize>& dest)
  {
    for (viskores::IdComponent index = 0; (index < NUM_COMPONENTS) && (index < DestSize);
         index++)
    {
      dest[index] = src[index];
    }
  }
};

//// PAUSE-EXAMPLE
#if 0
//// RESUME-EXAMPLE
} // namespace viskores
//// PAUSE-EXAMPLE
#endif
//// RESUME-EXAMPLE
////
//// END-EXAMPLE VecTraitsImpl
////

void TryCustomVecTriats()
{
  using CustomTraits = TraitsExamples::VecTraits<viskores::Id3>;
  using OriginalTraits = viskores::VecTraits<viskores::Id3>;

  VISKORES_STATIC_ASSERT(
    (std::is_same<CustomTraits::ComponentType, OriginalTraits::ComponentType>::value));
  VISKORES_STATIC_ASSERT(CustomTraits::NUM_COMPONENTS == OriginalTraits::NUM_COMPONENTS);
  VISKORES_STATIC_ASSERT((std::is_same<CustomTraits::HasMultipleComponents,
                                   OriginalTraits::HasMultipleComponents>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<CustomTraits::IsSizeStatic, OriginalTraits::IsSizeStatic>::value));

  viskores::Id3 value = TestValue(10, viskores::Id3());
  VISKORES_TEST_ASSERT(CustomTraits::GetNumberOfComponents(value) ==
                     OriginalTraits::GetNumberOfComponents(value),
                   "Wrong size.");
  VISKORES_TEST_ASSERT(CustomTraits::GetComponent(value, 1) ==
                     OriginalTraits::GetComponent(value, 1),
                   "Wrong component.");

  CustomTraits::SetComponent(value, 2, 0);
  VISKORES_TEST_ASSERT(value[2] == 0, "Did not set component.");

  viskores::Id2 shortValue;
  CustomTraits::CopyInto(value, shortValue);
  VISKORES_TEST_ASSERT(test_equal(shortValue[0], value[0]));
  VISKORES_TEST_ASSERT(test_equal(shortValue[1], value[1]));
}

void Test()
{
  TryRemainder();
  TryCustomTypeTraits();
  TryLess();
  TryCustomVecTriats();
}

} // namespace TraitsExamples

int GuideExampleTraits(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TraitsExamples::Test, argc, argv);
}
