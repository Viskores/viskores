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
#ifndef vtk_m_cont_internal_IteratorFromArrayPortal_h
#define vtk_m_cont_internal_IteratorFromArrayPortal_h

#include <vtkm/Assert.h>

#include <vtkm/cont/ArrayPortal.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/iterator/iterator_facade.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace cont {
namespace internal {
namespace detail {

template<class ArrayPortalType>
struct IteratorFromArrayPortalValue
{
  typedef typename ArrayPortalType::ValueType ValueType;

  VTKM_CONT_EXPORT
  IteratorFromArrayPortalValue(const ArrayPortalType &portal, vtkm::Id index)
    : Portal(portal), Index(index) {  }

  VTKM_CONT_EXPORT
  void Swap( IteratorFromArrayPortalValue<ArrayPortalType> &rhs ) throw()
  {
    //we need use the explicit type not a proxy temp object
    //A proxy temp object would point to the same underlying data structure
    //and would not hold the old value of *this once *this was set to rhs.
    const ValueType aValue = *this;
    *this = rhs;
    rhs = aValue;
  }

  VTKM_CONT_EXPORT
  IteratorFromArrayPortalValue<ArrayPortalType> &operator=(
    const IteratorFromArrayPortalValue<ArrayPortalType> &rhs)
  {
    this->Portal.Set(this->Index, rhs.Portal.Get(rhs.Index));
    return *this;
  }

  VTKM_CONT_EXPORT
  ValueType operator=(const ValueType& value)
  {
    this->Portal.Set(this->Index, value);
    return value;
  }

  VTKM_CONT_EXPORT
  operator ValueType(void) const
  {
    return this->Portal.Get(this->Index);
  }

  const ArrayPortalType& Portal;
  vtkm::Id Index;
};

} // namespace detail

template<class ArrayPortalType>
class IteratorFromArrayPortal : public
    boost::iterator_facade<
      IteratorFromArrayPortal<ArrayPortalType>,
      typename ArrayPortalType::ValueType,
      boost::random_access_traversal_tag,
      detail::IteratorFromArrayPortalValue<ArrayPortalType> >
{
  typedef boost::iterator_facade<
      IteratorFromArrayPortal<ArrayPortalType>,
      typename ArrayPortalType::ValueType,
      boost::random_access_traversal_tag,
      detail::IteratorFromArrayPortalValue<ArrayPortalType> > Superclass;

public:

  VTKM_CONT_EXPORT
  IteratorFromArrayPortal()
    : Portal(), Index(0) { }

  VTKM_CONT_EXPORT
  explicit IteratorFromArrayPortal(const ArrayPortalType &portal,
                                   vtkm::Id index = 0)
    : Portal(portal), Index(index) {  }

  VTKM_CONT_EXPORT
  detail::IteratorFromArrayPortalValue<ArrayPortalType>
  operator[](std::ptrdiff_t idx) const //NEEDS to be signed
  {
    return detail::IteratorFromArrayPortalValue<ArrayPortalType>(this->Portal,
           this->Index + static_cast<vtkm::Id>(idx) );
  }

private:
  ArrayPortalType Portal;
  vtkm::Id Index;

  // Implementation for boost iterator_facade
  friend class boost::iterator_core_access;

  VTKM_CONT_EXPORT
  detail::IteratorFromArrayPortalValue<ArrayPortalType> dereference() const
  {
    return detail::IteratorFromArrayPortalValue<ArrayPortalType>(this->Portal,
           this->Index);
  }

  VTKM_CONT_EXPORT
  bool equal(const IteratorFromArrayPortal<ArrayPortalType> &other) const
  {
    // Technically, we should probably check that the portals are the same,
    // but the portal interface does not specify an equal operator.  It is
    // by its nature undefined what happens when comparing iterators from
    // different portals anyway.
    return (this->Index == other.Index);
  }

  VTKM_CONT_EXPORT
  void increment()
  {
    this->Index++;
    VTKM_ASSERT(this->Index >= 0);
    VTKM_ASSERT(this->Index <= this->Portal.GetNumberOfValues());
  }

  VTKM_CONT_EXPORT
  void decrement()
  {
    this->Index--;
    VTKM_ASSERT(this->Index >= 0);
    VTKM_ASSERT(this->Index <= this->Portal.GetNumberOfValues());
  }

  VTKM_CONT_EXPORT
  void advance(typename Superclass::difference_type delta)
  {
    this->Index += static_cast<vtkm::Id>(delta);
    VTKM_ASSERT(this->Index >= 0);
    VTKM_ASSERT(this->Index <= this->Portal.GetNumberOfValues());
  }

  VTKM_CONT_EXPORT
  typename Superclass::difference_type
  distance_to(const IteratorFromArrayPortal<ArrayPortalType> &other) const
  {
    // Technically, we should probably check that the portals are the same,
    // but the portal interface does not specify an equal operator.  It is
    // by its nature undefined what happens when comparing iterators from
    // different portals anyway.
    return static_cast<typename IteratorFromArrayPortal<ArrayPortalType>::difference_type>(
        other.Index - this->Index);
  }
};

template<class ArrayPortalType>
IteratorFromArrayPortal<ArrayPortalType> make_IteratorBegin(
  const ArrayPortalType &portal)
{
  return IteratorFromArrayPortal<ArrayPortalType>(portal, 0);
}

template<class ArrayPortalType>
IteratorFromArrayPortal<ArrayPortalType> make_IteratorEnd(
  const ArrayPortalType &portal)
{
  return IteratorFromArrayPortal<ArrayPortalType>(portal,
         portal.GetNumberOfValues());
}


//implementat a custom swap function, since the std::swap won't work
//since we return RValues instead of Lvalues
template<typename T>
void swap( vtkm::cont::internal::detail::IteratorFromArrayPortalValue<T> a,
           vtkm::cont::internal::detail::IteratorFromArrayPortalValue<T> b)
{
  a.Swap(b);
}

}
}
} // namespace vtkm::cont::internal

namespace boost {

/// The boost::iterator_facade lets you redefine the reference type, which is
/// good since you cannot set an array portal from a reference in general.
/// However, the iterator_facade then checks to see if the reference type is an
/// actual reference, and if it is not it can set up some rather restrictive
/// traits that we do not want. To get around this, specialize the
/// boost::is_reference type check to declare our value class as a reference
/// type. Even though it is not a true reference type, its operators make it
/// behave like one.
///
template<typename T>
struct is_reference<
    vtkm::cont::internal::detail::IteratorFromArrayPortalValue<T> >
  : public boost::true_type {  };

} // namespace boost

#endif //vtk_m_cont_internal_IteratorFromArrayPortal_h
