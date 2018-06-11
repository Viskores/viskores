//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_internal_ArrayPortalValueReference_h
#define vtk_m_internal_ArrayPortalValueReference_h

#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace internal
{

/// \brief A value class for returning setable values of an ArrayPortal
///
/// \c ArrayPortal classes have a pair of \c Get and \c Set methods that
/// retrieve and store values in the array. This is to make it easy to
/// implement the \c ArrayPortal even it is not really an array. However, there
/// are some cases where the code structure expects a reference to a value that
/// can be set. For example, the \c IteratorFromArrayPortal class must return
/// something from its * operator that behaves like a reference.
///
/// For cases of this nature \c ArrayPortalValueReference can be used. This
/// class is constructured with an \c ArrayPortal and an index into the array.
/// The object then behaves like a reference to the value in the array. If you
/// set this reference object to a new value, it will call \c Set on the
/// \c ArrayPortal to insert the value into the array.
///
template <typename ArrayPortalType>
struct ArrayPortalValueReference
{
  using ValueType = typename ArrayPortalType::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalValueReference(const ArrayPortalType& portal, vtkm::Id index)
    : Portal(portal)
    , Index(index)
  {
  }

  VTKM_CONT
  void Swap(ArrayPortalValueReference<ArrayPortalType>& rhs) throw()
  {
    //we need use the explicit type not a proxy temp object
    //A proxy temp object would point to the same underlying data structure
    //and would not hold the old value of *this once *this was set to rhs.
    const ValueType aValue = *this;
    *this = rhs;
    rhs = aValue;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalValueReference<ArrayPortalType>& operator=(
    const ArrayPortalValueReference<ArrayPortalType>& rhs)
  {
    this->Portal.Set(this->Index, rhs.Portal.Get(rhs.Index));
    return *this;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType operator=(const ValueType& value)
  {
    this->Portal.Set(this->Index, value);
    return value;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  operator ValueType(void) const { return this->Portal.Get(this->Index); }

  const ArrayPortalType& Portal;
  vtkm::Id Index;
};

//implement a custom swap function, since the std::swap won't work
//since we return RValues instead of Lvalues
template <typename T>
void swap(vtkm::internal::ArrayPortalValueReference<T> a,
          vtkm::internal::ArrayPortalValueReference<T> b)
{
  a.Swap(b);
}

template <typename T>
void swap(vtkm::internal::ArrayPortalValueReference<T> a,
          typename vtkm::internal::ArrayPortalValueReference<T>::ValueType& b)
{
  using ValueType = typename vtkm::internal::ArrayPortalValueReference<T>::ValueType;
  const ValueType tmp = a;
  a = b;
  b = tmp;
}

template <typename T>
void swap(typename vtkm::internal::ArrayPortalValueReference<T>::ValueType& a,
          vtkm::internal::ArrayPortalValueReference<T> b)
{
  using ValueType = typename vtkm::internal::ArrayPortalValueReference<T>::ValueType;
  const ValueType tmp = b;
  b = a;
  a = tmp;
}
}
} // namespace vtkm::internal

namespace vtkm
{

// Make specialization for TypeTraits and VecTraits so that the reference
// behaves the same as the value.

template <typename PortalType>
struct TypeTraits<vtkm::internal::ArrayPortalValueReference<PortalType>>
  : vtkm::TypeTraits<typename vtkm::internal::ArrayPortalValueReference<PortalType>::ValueType>
{
};

template <typename PortalType>
struct VecTraits<vtkm::internal::ArrayPortalValueReference<PortalType>>
  : vtkm::VecTraits<typename vtkm::internal::ArrayPortalValueReference<PortalType>::ValueType>
{
};

} // namespace vtkm

#endif //vtk_m_internal_ArrayPortalValueReference_h
