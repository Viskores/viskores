//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayPortalToIterators_h
#define vtk_m_cont_ArrayPortalToIterators_h

#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/internal/IteratorFromArrayPortal.h>

#include <vtkm/internal/ArrayPortalHelpers.h>

#include <vtkmstd/void_t.h>

namespace vtkm
{
namespace cont
{

template <typename PortalType,
          typename CustomIterators = vtkm::internal::PortalSupportsIterators<PortalType>>
class ArrayPortalToIterators;

/// \brief Convert an \c ArrayPortal to STL iterators.
///
/// \c ArrayPortalToIterators is a class that holds an \c ArrayPortal and
/// builds iterators that access the data in the \c ArrayPortal. The point of
/// this class is to use an \c ArrayPortal with generic functions that expect
/// STL iterators such as STL algorithms or Thrust operations.
///
/// The default template implementation constructs iterators that provide
/// values through the \c ArrayPortal itself. However, if the \c ArrayPortal
/// contains its own iterators (by defining \c GetIteratorBegin and
/// \c GetIteratorEnd), then those iterators are used.
///
template <typename PortalType>
class ArrayPortalToIterators<PortalType, std::false_type>
{
public:
  /// \c ArrayPortaltoIterators should be constructed with an instance of
  /// the array portal.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  explicit ArrayPortalToIterators(const PortalType& portal)
    : Portal(portal)
  {
  }

  // These are the same as the default implementation, but explicitly created to prevent warnings
  // from the CUDA compiler where it tries to compile for the device when the underlying portal
  // only works for the host.
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators(const ArrayPortalToIterators& src)
    : Portal(src.Portal)
  {
  }
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators(ArrayPortalToIterators&& rhs)
    : Portal(std::move(rhs.Portal))
  {
  }
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ~ArrayPortalToIterators() {}
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators& operator=(const ArrayPortalToIterators& src)
  {
    this->Portal = src.Portal;
    return *this;
  }
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators& operator=(ArrayPortalToIterators&& rhs)
  {
    this->Portal = std::move(rhs.Portal);
    return *this;
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

// Specialize for custom iterator types:
template <typename PortalType>
class ArrayPortalToIterators<PortalType, std::true_type>
{
public:
  using IteratorType = decltype(std::declval<PortalType>().GetIteratorBegin());

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  explicit ArrayPortalToIterators(const PortalType& portal)
    : Begin(portal.GetIteratorBegin())
    , End(portal.GetIteratorEnd())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorType GetBegin() const { return this->Begin; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorType GetEnd() const { return this->End; }

  // These are the same as the default implementation, but explicitly created to prevent warnings
  // from the CUDA compiler where it tries to compile for the device when the underlying portal
  // only works for the host.
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators(const ArrayPortalToIterators& src)
    : Begin(src.Begin)
    , End(src.End)
  {
  }
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators(ArrayPortalToIterators&& rhs)
    : Begin(std::move(rhs.Begin))
    , End(std::move(rhs.End))
  {
  }
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ~ArrayPortalToIterators() {}
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators& operator=(const ArrayPortalToIterators& src)
  {
    this->Begin = src.Begin;
    this->End = src.End;
    return *this;
  }
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators& operator=(ArrayPortalToIterators&& rhs)
  {
    this->Begin = std::move(rhs.Begin);
    this->End = std::move(rhs.End);
    return *this;
  }

private:
  IteratorType Begin;
  IteratorType End;
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
