//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_testing_VecTraitsTest_h
#define vtkm_testing_VecTraitsTest_h

#include <vtkm/VecTraits.h>

#include <vtkm/TypeTraits.h>

#include <vtkm/testing/Testing.h>

namespace vtkm {
namespace testing {

namespace detail {

inline void CompareDimensionalityTags(vtkm::TypeTraitsScalarTag,
                                      vtkm::VecTraitsTagSingleComponent)
{
  // If we are here, everything is fine.
}
inline void CompareDimensionalityTags(vtkm::TypeTraitsVectorTag,
                                      vtkm::VecTraitsTagMultipleComponents)
{
  // If we are here, everything is fine.
}

template<vtkm::IdComponent NUM_COMPONENTS, typename T>
inline void CheckIsStatic(const T &, vtkm::VecTraitsTagSizeStatic)
{
  VTKM_TEST_ASSERT(vtkm::VecTraits<T>::NUM_COMPONENTS == NUM_COMPONENTS,
                   "Traits returns unexpected number of components");
}

template<vtkm::IdComponent NUM_COMPONENTS, typename T>
inline void CheckIsStatic(const T &, vtkm::VecTraitsTagSizeVariable)
{
  // If we are here, everything is fine.
}

/// Compares some manual arithmetic through type traits to arithmetic with
/// the Tuple class.
template <vtkm::IdComponent NUM_COMPONENTS, typename T>
static void TestVecTypeImpl(
  const typename std::remove_const<T>::type &vector)
{
  typedef typename vtkm::VecTraits<T> Traits;
  typedef typename Traits::ComponentType ComponentType;
  typedef typename std::remove_const<T>::type NonConstT;

  CheckIsStatic<NUM_COMPONENTS>(vector, typename Traits::IsSizeStatic());

  VTKM_TEST_ASSERT(Traits::GetNumberOfComponents(vector) == NUM_COMPONENTS,
                   "Traits returned wrong number of components.");

  vtkm::Vec<ComponentType,NUM_COMPONENTS> vectorCopy;
  Traits::CopyInto(vector, vectorCopy);
  VTKM_TEST_ASSERT(test_equal(vectorCopy, vector), "CopyInto does not work.");

  {
    NonConstT result;
    const ComponentType multiplier = 4;
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      Traits::SetComponent(result,
                           i,
                           ComponentType(
                               multiplier*Traits::GetComponent(vector, i)));
    }
    vtkm::Vec<ComponentType,NUM_COMPONENTS> resultCopy;
    Traits::CopyInto(result, resultCopy);
    VTKM_TEST_ASSERT(test_equal(resultCopy, multiplier*vectorCopy),
                     "Got bad result for scalar multiple");
  }

  {
    NonConstT result;
    const ComponentType multiplier = 7;
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      Traits::GetComponent(result, i)
        = ComponentType(multiplier * Traits::GetComponent(vector, i));
    }
    vtkm::Vec<ComponentType,NUM_COMPONENTS> resultCopy;
    Traits::CopyInto(result, resultCopy);
    VTKM_TEST_ASSERT(test_equal(resultCopy, multiplier*vectorCopy),
                     "Got bad result for scalar multiple");
  }

  {
    ComponentType result = 0;
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; i++)
    {
      ComponentType component
        = Traits::GetComponent(vector, i);
      result = ComponentType(result + (component * component));
    }
    VTKM_TEST_ASSERT(
      test_equal(result, vtkm::dot(vectorCopy, vectorCopy)),
      "Got bad result for dot product");
  }

  // This will fail to compile if the tags are wrong.
  detail::CompareDimensionalityTags(
    typename vtkm::TypeTraits<T>::DimensionalityTag(),
    typename vtkm::VecTraits<T>::HasMultipleComponents());
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
template<class T>
inline void TestVecComponentsTag()
{
  // This will fail to compile if the tag is wrong
  // (i.e. not vtkm::VecTraitsTagMultipleComponents)
  detail::CheckVecComponentsTag(
    typename vtkm::VecTraits<T>::HasMultipleComponents());
}

namespace detail {

inline void CheckScalarComponentsTag(vtkm::VecTraitsTagSingleComponent)
{
  // If we are running here, everything is fine.
}

} // namespace detail

/// Compares some manual arithmetic through type traits to arithmetic with
/// the Tuple class.
template <vtkm::IdComponent NUM_COMPONENTS, typename T>
static void TestVecType(const T &vector)
{
  detail::TestVecTypeImpl<NUM_COMPONENTS, T>(vector);
  detail::TestVecTypeImpl<NUM_COMPONENTS, const T>(vector);
}

/// Checks to make sure that the HasMultipleComponents tag is actually for a
/// single component. Should only be called for "vector" classes that actually
/// have only a single component (that is, are really scalars).
///
template<class T>
inline void TestScalarComponentsTag()
{
  // This will fail to compile if the tag is wrong
  // (i.e. not vtkm::VecTraitsTagSingleComponent)
  detail::CheckScalarComponentsTag(
    typename vtkm::VecTraits<T>::HasMultipleComponents());
}

}
} // namespace vtkm::testing

#endif //vtkm_testing_VecTraitsTest_h
