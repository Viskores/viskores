//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_IteratorFromArrayPortal_h
#define vtk_m_cont_internal_IteratorFromArrayPortal_h

#include <vtkm/Assert.h>
#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/internal/ArrayPortalValueReference.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <class ArrayPortalType>
class IteratorFromArrayPortal
{
public:
  using value_type = typename std::remove_const<typename ArrayPortalType::ValueType>::type;
  using reference = vtkm::internal::ArrayPortalValueReference<ArrayPortalType>;
  using pointer = typename std::add_pointer<value_type>::type;

  using difference_type = std::ptrdiff_t;

  using iterator_category = std::random_access_iterator_tag;

  using iter = IteratorFromArrayPortal<ArrayPortalType>;

  IteratorFromArrayPortal()
    : Portal()
    , Index(0)
  {
  }

  explicit IteratorFromArrayPortal(const ArrayPortalType& portal, vtkm::Id index = 0)
    : Portal(portal)
    , Index(index)
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index <= portal.GetNumberOfValues());
  }

  reference operator*() const { return reference(this->Portal, this->Index); }

  reference operator->() const { return reference(this->Portal, this->Index); }

  reference operator[](difference_type idx) const
  {
    return reference(this->Portal, this->Index + static_cast<vtkm::Id>(idx));
  }

  iter& operator++()
  {
    this->Index++;
    VTKM_ASSERT(this->Index <= this->Portal.GetNumberOfValues());
    return *this;
  }

  iter operator++(int) { return iter(this->Portal, this->Index++); }

  iter& operator--()
  {
    this->Index--;
    VTKM_ASSERT(this->Index >= 0);
    return *this;
  }

  iter operator--(int) { return iter(this->Portal, this->Index--); }

  iter& operator+=(difference_type n)
  {
    this->Index += static_cast<vtkm::Id>(n);
    VTKM_ASSERT(this->Index <= this->Portal.GetNumberOfValues());
    return *this;
  }

  iter& operator-=(difference_type n)
  {
    this->Index += static_cast<vtkm::Id>(n);
    VTKM_ASSERT(this->Index >= 0);
    return *this;
  }

  iter operator-(difference_type n) const
  {
    return iter(this->Portal, this->Index - static_cast<vtkm::Id>(n));
  }

  ArrayPortalType Portal;
  vtkm::Id Index;
};

template <class ArrayPortalType>
IteratorFromArrayPortal<ArrayPortalType> make_IteratorBegin(const ArrayPortalType& portal)
{
  return IteratorFromArrayPortal<ArrayPortalType>(portal, 0);
}

template <class ArrayPortalType>
IteratorFromArrayPortal<ArrayPortalType> make_IteratorEnd(const ArrayPortalType& portal)
{
  return IteratorFromArrayPortal<ArrayPortalType>(portal, portal.GetNumberOfValues());
}

template <typename PortalType>
bool operator==(vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
                vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index == rhs.Index;
}

template <typename PortalType>
bool operator!=(vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
                vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index != rhs.Index;
}

template <typename PortalType>
bool operator<(vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
               vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index < rhs.Index;
}

template <typename PortalType>
bool operator<=(vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
                vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index <= rhs.Index;
}

template <typename PortalType>
bool operator>(vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
               vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index > rhs.Index;
}

template <typename PortalType>
bool operator>=(vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
                vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index >= rhs.Index;
}

template <typename PortalType>
std::ptrdiff_t operator-(vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
                         vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index - rhs.Index;
}

template <typename PortalType>
vtkm::cont::internal::IteratorFromArrayPortal<PortalType> operator+(
  vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& iter,
  std::ptrdiff_t n)
{
  return vtkm::cont::internal::IteratorFromArrayPortal<PortalType>(
    iter.Portal, iter.Index + static_cast<vtkm::Id>(n));
}

template <typename PortalType>
vtkm::cont::internal::IteratorFromArrayPortal<PortalType> operator+(
  std::ptrdiff_t n,
  vtkm::cont::internal::IteratorFromArrayPortal<PortalType> const& iter)
{
  return vtkm::cont::internal::IteratorFromArrayPortal<PortalType>(
    iter.Portal, iter.Index + static_cast<vtkm::Id>(n));
}
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_IteratorFromArrayPortal_h
