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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ArrayPortalToIterators_h
#define vtk_m_cont_ArrayPortalToIterators_h

#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/internal/IteratorFromArrayPortal.h>

namespace vtkm {
namespace cont {

/// \brief Convert an \c ArrayPortal to STL iterators.
///
/// \c ArrayPortalToIterators is a class that holds an \c ArrayPortal and
/// builds iterators that access the data in the \c ArrayPortal. The point of
/// this class is to use an \c ArrayPortal with generic functions that expect
/// STL iterators such as STL algorithms or Thrust operations.
///
/// The default template implementation constructs iterators that provide
/// values through the \c ArrayPortal itself. This class can be specialized to
/// provide iterators that more directly access the data. For example, \c
/// ArrayPortalFromIterator has a specialization to return the original
/// iterators.
///
template<typename PortalType>
class ArrayPortalToIterators
{
public:
  /// \c ArrayPortaltoIterators should be constructed with an instance of
  /// the array portal.
  ///
  VTKM_CONT_EXPORT
  ArrayPortalToIterators(const PortalType &portal) : Portal(portal) {  }

  /// The type of the iterator.
  ///
  typedef vtkm::cont::internal::IteratorFromArrayPortal<PortalType>
      IteratorType;

  /// Returns an iterator pointing to the beginning of the ArrayPortal.
  ///
  VTKM_CONT_EXPORT
  IteratorType GetBegin() const {
    return vtkm::cont::internal::make_IteratorBegin(this->Portal);
  }

  /// Returns an iterator pointing to one past the end of the ArrayPortal.
  ///
  VTKM_CONT_EXPORT
  IteratorType GetEnd() const {
    return vtkm::cont::internal::make_IteratorEnd(this->Portal);
  }

private:
  PortalType Portal;
};

/// Convienience function for converting an ArrayPortal to a begin iterator.
///
template<typename PortalType>
VTKM_CONT_EXPORT
typename vtkm::cont::ArrayPortalToIterators<PortalType>::IteratorType
ArrayPortalToIteratorBegin(const PortalType &portal)
{
  vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
  return iterators.GetBegin();
}

/// Convienience function for converting an ArrayPortal to an end iterator.
///
template<typename PortalType>
VTKM_CONT_EXPORT
typename vtkm::cont::ArrayPortalToIterators<PortalType>::IteratorType
ArrayPortalToIteratorEnd(const PortalType &portal)
{
  vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
  return iterators.GetEnd();
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayPortalToIterators_h
