//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayPortal_h
#define vtk_m_cont_ArrayPortal_h

#include <vtkm/Types.h>

namespace vtkm
{
namespace cont
{

#ifdef VTKM_DOXYGEN_ONLY

/// \brief A class that points to and access and array of data.
///
/// The ArrayPortal class itself does not exist; this code is provided for
/// documentation purposes only.
///
/// An ArrayPortal object acts like a pointer to a random-access container
/// (that is, an array) and also lets you set and get values in that array. In
/// many respects an ArrayPortal is similar in concept to that of iterators but
/// with a much simpler interface and no internal concept of position.
/// Otherwise, ArrayPortal objects may be passed and copied around so that
/// multiple entities may be accessing the same array.
///
/// An ArrayPortal differs from an ArrayHandle in that the ArrayPortal is a
/// much lighterweight object and that it does not manage things like
/// allocation and control/execution sharing. An ArrayPortal also differs from
/// a Storage in that it does not actually contain the data but rather
/// points to it. In this way the ArrayPortal can be copied and passed and
/// still point to the same data.
///
/// Most VTKm users generally do not need to do much or anything with
/// ArrayPortal objects. It is mostly an internal mechanism. However, an
/// ArrayPortal can be used to pass constant input data to an ArrayHandle.
///
/// Although portals are defined in the execution environment, they are also
/// used in the control environment for accessing data on the host.
///
/// Since utilities like IsWritableArrayHandle checks for the existence of a Set
/// method on a portal, if the portal is backed by a read-only ArrayHandle, the
/// Set method must not be defined. If the portal may or may not be writable
/// (e.g., ArrayHandleCast may be casting a read-only OR read-write array), the
/// Set method may be conditionally removed using SFINAE.
///
/// The ArrayPortalToIterators utilities wrap ArrayPortals in STL-style
/// iterators. If an ArrayPortal implementation wishes to provide a custom
/// iterator type, it may define an IteratorType type alias along with the
/// methods `IteratorType GetIteratorBegin()` and
/// `IteratorType GetIteratorEnd()`. These are not required members, but if
/// present, will allow additional optimizations for certain portals.
///
template <typename T>
class ArrayPortal
{
public:
  /// The type of each value in the array.
  ///
  using ValueType = T;

  /// The total number of values in the array. They are index from 0 to
  /// GetNumberOfValues()-1.
  ///
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const;

  /// Gets a value from the array.
  ///
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const;

  /// Sets a value in the array. If it is not possible to set a value in the
  /// array, this method must not be defined.
  ///
  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const;
};

#endif // VTKM_DOXYGEN_ONLY
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayPortal_h
