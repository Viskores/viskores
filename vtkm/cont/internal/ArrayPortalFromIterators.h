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
#ifndef vtk_m_cont_internal_ArrayPortalFromIterators_h
#define vtk_m_cont_internal_ArrayPortalFromIterators_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/Assert.h>
#include <vtkm/cont/ErrorControlOutOfMemory.h>

#include <iterator>
#include <limits>

namespace vtkm {
namespace cont {
namespace internal {

/// This templated implementation of an ArrayPortal allows you to adapt a pair
/// of begin/end iterators to an ArrayPortal interface.
///
template<class IteratorT>
class ArrayPortalFromIterators
{
public:
  typedef typename std::iterator_traits<IteratorT>::value_type ValueType;
  typedef IteratorT IteratorType;

  VTKM_CONT_EXPORT ArrayPortalFromIterators() {  }

  VTKM_CONT_EXPORT
  ArrayPortalFromIterators(IteratorT begin, IteratorT end)
    : BeginIterator(begin)
  {
    typename std::iterator_traits<IteratorT>::difference_type numberOfValues =
      std::distance(begin, end);
    VTKM_ASSERT_CONT(numberOfValues >= 0);
#ifndef VTKM_USE_64BIT_IDS
    if (numberOfValues > std::numeric_limits<vtkm::Id>::max())
    {
      throw vtkm::cont::ErrorControlOutOfMemory(
        "Distance of iterators larger than maximum array size. "
        "To support larger arrays, try turning on VTKM_USE_64BIT_IDS.");
    }
#endif // !VTKM_USE_64BIT_IDS
    this->NumberOfValues = static_cast<vtkm::Id>(numberOfValues);
  }

  /// Copy constructor for any other ArrayPortalFromIterators with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<class OtherIteratorT>
  VTKM_CONT_EXPORT
  ArrayPortalFromIterators(const ArrayPortalFromIterators<OtherIteratorT> &src)
    : BeginIterator(src.GetIteratorBegin()), NumberOfValues(src.GetNumberOfValues())
  {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  VTKM_CONT_EXPORT
  ValueType Get(vtkm::Id index) const
  {
    return *this->IteratorAt(index);
  }

  VTKM_CONT_EXPORT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    *this->IteratorAt(index) = value;
  }

  VTKM_CONT_EXPORT
  IteratorT GetIteratorBegin() const {
    return this->BeginIterator;
  }

private:
  IteratorT BeginIterator;
  vtkm::Id NumberOfValues;

  VTKM_CONT_EXPORT
  IteratorT IteratorAt(vtkm::Id index) const
  {
    VTKM_ASSERT_CONT(index >= 0);
    VTKM_ASSERT_CONT(index < this->GetNumberOfValues());

    return this->BeginIterator + index;
  }
};

}
}
} // namespace vtkm::cont::internal

namespace vtkm {
namespace cont {

/// Partial specialization of \c ArrayPortalToIterators for \c
/// ArrayPortalFromIterators. Returns the original array rather than
/// the portal wrapped in an \c IteratorFromArrayPortal.
///
template<typename _IteratorType>
class ArrayPortalToIterators<
    vtkm::cont::internal::ArrayPortalFromIterators<_IteratorType> >
{
  typedef vtkm::cont::internal::ArrayPortalFromIterators<_IteratorType>
      PortalType;
public:
#ifndef VTKM_MSVC
  typedef _IteratorType IteratorType;

  VTKM_CONT_EXPORT
  ArrayPortalToIterators(const PortalType &portal)
    : Iterator(portal.GetIteratorBegin()),
      NumberOfValues(portal.GetNumberOfValues())
  {  }

#else // VTKM_MSVC
  // The MSVC compiler issues warnings if you use raw pointers with some
  // operations. To keep the compiler happy (and add some safety checks),
  // wrap the iterator in a check.
  typedef stdext::checked_array_iterator<_IteratorType> IteratorType;

  VTKM_CONT_EXPORT
  ArrayPortalToIterators(const PortalType &portal)
    : Iterator(portal.GetIteratorBegin(),
	           static_cast<size_t>(portal.GetNumberOfValues())),
      NumberOfValues(portal.GetNumberOfValues())
  {  }

#endif // VTKM_MSVC

  VTKM_CONT_EXPORT
  IteratorType GetBegin() const { return this->Iterator; }

  VTKM_CONT_EXPORT
  IteratorType GetEnd() const {
    IteratorType iterator = this->Iterator;
	typedef typename std::iterator_traits<IteratorType>::difference_type
		difference_type;
    std::advance(iterator, static_cast<difference_type>(this->NumberOfValues));
    return iterator;
  }

private:
  IteratorType Iterator;
  vtkm::Id NumberOfValues;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_internal_ArrayPortalFromIterators_h
