//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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

namespace vtkm
{
namespace cont
{

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
template <typename PortalType>
class ArrayPortalToIterators
{
public:
  /// \c ArrayPortaltoIterators should be constructed with an instance of
  /// the array portal.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators(const PortalType& portal)
    : Portal(portal)
  {
  }

  /// The type of the iterator.
  ///
  using IteratorType = vtkm::cont::internal::IteratorFromArrayPortal<PortalType>;

  /// Returns an iterator pointing to the beginning of the ArrayPortal.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorType GetBegin() const { return vtkm::cont::internal::make_IteratorBegin(this->Portal); }

  /// Returns an iterator pointing to one past the end of the ArrayPortal.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorType GetEnd() const { return vtkm::cont::internal::make_IteratorEnd(this->Portal); }

private:
  PortalType Portal;
};

/// Convenience function for converting an ArrayPortal to a begin iterator.
///
VTKM_SUPPRESS_EXEC_WARNINGS
template <typename PortalType>
VTKM_EXEC_CONT typename vtkm::cont::ArrayPortalToIterators<PortalType>::IteratorType
ArrayPortalToIteratorBegin(const PortalType& portal)
{
  vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
  return iterators.GetBegin();
}

/// Convenience function for converting an ArrayPortal to an end iterator.
///
VTKM_SUPPRESS_EXEC_WARNINGS
template <typename PortalType>
VTKM_EXEC_CONT typename vtkm::cont::ArrayPortalToIterators<PortalType>::IteratorType
ArrayPortalToIteratorEnd(const PortalType& portal)
{
  vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
  return iterators.GetEnd();
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayPortalToIterators_h
