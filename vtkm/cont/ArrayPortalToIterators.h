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

namespace vtkmstd
{
/// Implementation of std::void_t (C++17):
/// Allows for specialization of class templates based on members of template
/// parameters.
#if defined(VTKM_GCC) && (__GNUC__ < 5)
// Due to a defect in the wording (CWG 1558) unused parameters in alias templates
// were not guaranteed to ensure SFINAE, and therefore would consider everything
// to match the 'true' side. For VTK-m the only known compiler that implemented
// this defect is GCC < 5.
template <class... T>
struct void_pack
{
  using type = void;
};
template <class... T>
using void_t = typename void_pack<T...>::type;
#else
template <typename...>
using void_t = void;
#endif


} // end namespace vtkmstd

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
template <typename PortalType, typename CustomIterSFINAE = void>
class ArrayPortalToIterators
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
class ArrayPortalToIterators<PortalType, vtkmstd::void_t<typename PortalType::IteratorType>>
{
public:
  using IteratorType = typename PortalType::IteratorType;

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
