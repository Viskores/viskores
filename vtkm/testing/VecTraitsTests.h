//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_testing_VecTraitsTest_h
#define vtkm_testing_VecTraitsTest_h

//GCC 4+ when running the test code have false positive warnings
//about uninitialized vtkm::VecC<> when filled by VecTraits<T>::CopyInto.
//The testing code already verifies that CopyInto works by verifying the
//results, so we are going to suppress `-Wmaybe-uninitialized` for this
//file
//This block has to go before we include any vtkm file that brings in
//<vtkm/Types.h> otherwise the warning suppression will not work
#include <vtkm/internal/Configure.h>
#if (defined(VTKM_GCC) && __GNUC__ >= 4)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif // gcc  4+

#include <vtkm/VecTraits.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/TypeTraits.h>

#include <vtkm/testing/Testing.h>

namespace vtkm
{
namespace testing
{

namespace detail
{

inline void CompareDimensionalityTags(vtkm::TypeTraitsScalarTag, vtkm::VecTraitsTagSingleComponent)
{
  // If we are here, everything is fine.
}
inline void CompareDimensionalityTags(vtkm::TypeTraitsVectorTag,
                                      vtkm::VecTraitsTagMultipleComponents)
{
  // If we are here, everything is fine.
}

template <vtkm::IdComponent NUM_COMPONENTS, typename T>
inline void CheckIsStatic(const T&, vtkm::VecTraitsTagSizeStatic)
{
  VTKM_TEST_ASSERT(vtkm::VecTraits<T>::NUM_COMPONENTS == NUM_COMPONENTS,
                   "Traits returns unexpected number of components");
}

template <vtkm::IdComponent NUM_COMPONENTS, typename T>
inline void CheckIsStatic(const T&, vtkm::VecTraitsTagSizeVariable)
{
  // If we are here, everything is fine.
}

template <typename VecType>
struct VecIsWritable
{
  using type = std::true_type;
};

template <typename ComponentType>
struct VecIsWritable<vtkm::VecCConst<ComponentType>>
{
  using type = std::false_type;
};

// Part of TestVecTypeImpl that writes to the Vec type
template <vtkm::IdComponent NUM_COMPONENTS, typename T, typename VecCopyType>
static void TestVecTypeWritableImpl(const T& inVector,
                                    const VecCopyType& vectorCopy,
                                    T& outVector,
                                    std::true_type)
{
  using Traits = vtkm::VecTraits<T>;
  using ComponentType = typename Traits::ComponentType;

  {
    const ComponentType multiplier = 4;
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      Traits::SetComponent(
        outVector, i, ComponentType(multiplier * Traits::GetComponent(inVector, i)));
    }
    vtkm::Vec<ComponentType, NUM_COMPONENTS> resultCopy;
    Traits::CopyInto(outVector, resultCopy);
    VTKM_TEST_ASSERT(test_equal(resultCopy, multiplier * vectorCopy),
                     "Got bad result for scalar multiple");
  }

  {
    const ComponentType multiplier = 7;
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      Traits::GetComponent(outVector, i) =
        ComponentType(multiplier * Traits::GetComponent(inVector, i));
    }
    vtkm::Vec<ComponentType, NUM_COMPONENTS> resultCopy;
    Traits::CopyInto(outVector, resultCopy);
    VTKM_TEST_ASSERT(test_equal(resultCopy, multiplier * vectorCopy),
                     "Got bad result for scalar multiple");
  }
}

template <vtkm::IdComponent NUM_COMPONENTS, typename T, typename VecCopyType>
static void TestVecTypeWritableImpl(const T& vtkmNotUsed(inVector),
                                    const VecCopyType& vtkmNotUsed(vectorCopy),
                                    T& vtkmNotUsed(outVector),
                                    std::false_type)
{
  // Skip writable functionality.
}

/// Compares some manual arithmetic through type traits to arithmetic with
/// the Tuple class.
template <vtkm::IdComponent NUM_COMPONENTS, typename T>
static void TestVecTypeImpl(const typename std::remove_const<T>::type& inVector,
                            typename std::remove_const<T>::type& outVector)
{
  using Traits = vtkm::VecTraits<T>;
  using ComponentType = typename Traits::ComponentType;
  using NonConstT = typename std::remove_const<T>::type;

  CheckIsStatic<NUM_COMPONENTS>(inVector, typename Traits::IsSizeStatic());

  VTKM_TEST_ASSERT(Traits::GetNumberOfComponents(inVector) == NUM_COMPONENTS,
                   "Traits returned wrong number of components.");

  vtkm::Vec<ComponentType, NUM_COMPONENTS> vectorCopy;
  Traits::CopyInto(inVector, vectorCopy);
  VTKM_TEST_ASSERT(test_equal(vectorCopy, inVector), "CopyInto does not work.");

  {
    auto expected = vtkm::Dot(vectorCopy, vectorCopy);
    decltype(expected) result = 0;
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      ComponentType component = Traits::GetComponent(inVector, i);
      result = result + (component * component);
    }
    VTKM_TEST_ASSERT(test_equal(result, expected), "Got bad result for dot product");
  }

  // This will fail to compile if the tags are wrong.
  detail::CompareDimensionalityTags(typename vtkm::TypeTraits<T>::DimensionalityTag(),
                                    typename vtkm::VecTraits<T>::HasMultipleComponents());

  TestVecTypeWritableImpl<NUM_COMPONENTS, NonConstT>(
    inVector, vectorCopy, outVector, typename VecIsWritable<NonConstT>::type());

  // Compiler checks for base component types
  using BaseComponentType = typename vtkm::VecTraits<T>::BaseComponentType;
  VTKM_STATIC_ASSERT((std::is_same<typename vtkm::TypeTraits<BaseComponentType>::DimensionalityTag,
                                   vtkm::TypeTraitsScalarTag>::value));
  VTKM_STATIC_ASSERT((std::is_same<typename vtkm::VecTraits<ComponentType>::BaseComponentType,
                                   BaseComponentType>::value));

  // Compiler checks for replacing component types
  using ReplaceWithVecComponent =
    typename vtkm::VecTraits<T>::template ReplaceComponentType<vtkm::Vec<char, 2>>;
  VTKM_STATIC_ASSERT(
    (std::is_same<typename vtkm::TypeTraits<T>::DimensionalityTag,
                  vtkm::TypeTraitsVectorTag>::value &&
     std::is_same<typename vtkm::VecTraits<ReplaceWithVecComponent>::ComponentType,
                  vtkm::Vec<char, 2>>::value) ||
    (std::is_same<typename vtkm::TypeTraits<T>::DimensionalityTag,
                  vtkm::TypeTraitsScalarTag>::value &&
     std::is_same<typename vtkm::VecTraits<ReplaceWithVecComponent>::ComponentType, char>::value));
  VTKM_STATIC_ASSERT(
    (std::is_same<typename vtkm::VecTraits<ReplaceWithVecComponent>::BaseComponentType,
                  char>::value));
  using ReplaceBaseComponent =
    typename vtkm::VecTraits<ReplaceWithVecComponent>::template ReplaceBaseComponentType<short>;
  VTKM_STATIC_ASSERT(
    (std::is_same<typename vtkm::TypeTraits<T>::DimensionalityTag,
                  vtkm::TypeTraitsVectorTag>::value &&
     std::is_same<typename vtkm::VecTraits<ReplaceBaseComponent>::ComponentType,
                  vtkm::Vec<short, 2>>::value) ||
    (std::is_same<typename vtkm::TypeTraits<T>::DimensionalityTag,
                  vtkm::TypeTraitsScalarTag>::value &&
     std::is_same<typename vtkm::VecTraits<ReplaceBaseComponent>::ComponentType, short>::value));
  VTKM_STATIC_ASSERT((
    std::is_same<typename vtkm::VecTraits<ReplaceBaseComponent>::BaseComponentType, short>::value));
}

inline void CheckVecComponentsTag(vtkm::VecTraitsTagMultipleComponents)
{
  // If we are running here, everything is fine.
}

} // namespace detail

/// Checks to make sure that the HasMultipleComponents tag is actually for
/// multiple components. Should only be called for vector classes that actually
/// have multiple components.
///
template <class T>
inline void TestVecComponentsTag()
{
  // This will fail to compile if the tag is wrong
  // (i.e. not vtkm::VecTraitsTagMultipleComponents)
  detail::CheckVecComponentsTag(typename vtkm::VecTraits<T>::HasMultipleComponents());
}

namespace detail
{

inline void CheckScalarComponentsTag(vtkm::VecTraitsTagSingleComponent)
{
  // If we are running here, everything is fine.
}

} // namespace detail

/// Compares some manual arithmetic through type traits to arithmetic with
/// the Tuple class.
template <vtkm::IdComponent NUM_COMPONENTS, typename T>
static void TestVecType(const T& inVector, T& outVector)
{
  detail::TestVecTypeImpl<NUM_COMPONENTS, T>(inVector, outVector);
  detail::TestVecTypeImpl<NUM_COMPONENTS, const T>(inVector, outVector);
}

/// Checks to make sure that the HasMultipleComponents tag is actually for a
/// single component. Should only be called for "vector" classes that actually
/// have only a single component (that is, are really scalars).
///
template <class T>
inline void TestScalarComponentsTag()
{
  // This will fail to compile if the tag is wrong
  // (i.e. not vtkm::VecTraitsTagSingleComponent)
  detail::CheckScalarComponentsTag(typename vtkm::VecTraits<T>::HasMultipleComponents());
}
}
} // namespace vtkm::testing

#if (defined(VTKM_GCC) && __GNUC__ > 4 && __GNUC__ < 7)
#pragma GCC diagnostic pop
#endif // gcc  5 or 6

#endif //vtkm_testing_VecTraitsTest_h
