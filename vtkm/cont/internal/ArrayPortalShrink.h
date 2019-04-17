//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ArrayPortalShrink_h
#define vtk_m_cont_internal_ArrayPortalShrink_h

#include <vtkm/Assert.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/ArrayPortalToIterators.h>

#include <iterator>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// This ArrayPortal adapter is a utility that allows you to shrink the
/// (reported) array size without actually modifying the underlying allocation.
///
template <class PortalT>
class ArrayPortalShrink
{
public:
  using DelegatePortalType = PortalT;

  using ValueType = typename DelegatePortalType::ValueType;

  VTKM_CONT ArrayPortalShrink()
    : NumberOfValues(0)
  {
  }

  VTKM_CONT ArrayPortalShrink(const DelegatePortalType& delegatePortal)
    : DelegatePortal(delegatePortal)
    , NumberOfValues(delegatePortal.GetNumberOfValues())
  {
  }

  VTKM_CONT ArrayPortalShrink(const DelegatePortalType& delegatePortal, vtkm::Id numberOfValues)
    : DelegatePortal(delegatePortal)
    , NumberOfValues(numberOfValues)
  {
    VTKM_ASSERT(numberOfValues <= delegatePortal.GetNumberOfValues());
  }

  /// Copy constructor for any other ArrayPortalShrink with a delegate type
  /// that can be copied to this type. This allows us to do any type casting
  /// the delegates can do (like the non-const to const cast).
  ///
  template <class OtherDelegateType>
  VTKM_CONT ArrayPortalShrink(const ArrayPortalShrink<OtherDelegateType>& src)
    : DelegatePortal(src.GetDelegatePortal())
    , NumberOfValues(src.GetNumberOfValues())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_CONT
  ValueType Get(vtkm::Id index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());
    return this->DelegatePortal.Get(index);
  }

  VTKM_CONT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());
    this->DelegatePortal.Set(index, value);
  }

  /// Special method in this ArrayPortal that allows you to shrink the
  /// (exposed) array.
  ///
  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(numberOfValues < this->GetNumberOfValues());
    this->NumberOfValues = numberOfValues;
  }

  /// Get a copy of the delegate portal. Although safe, this is probably only
  /// useful internally. (It is exposed as public for the templated copy
  /// constructor.)
  ///
  DelegatePortalType GetDelegatePortal() const { return this->DelegatePortal; }

private:
  DelegatePortalType DelegatePortal;
  vtkm::Id NumberOfValues;
};
}
}
} // namespace vtkm::cont::internal

namespace vtkm
{
namespace cont
{

template <typename DelegatePortalType>
class ArrayPortalToIterators<vtkm::cont::internal::ArrayPortalShrink<DelegatePortalType>>
{
  using PortalType = vtkm::cont::internal::ArrayPortalShrink<DelegatePortalType>;
  using DelegateArrayPortalToIterators = vtkm::cont::ArrayPortalToIterators<DelegatePortalType>;

public:
  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalToIterators(const PortalType& portal)
    : DelegateIterators(portal.GetDelegatePortal())
    , NumberOfValues(portal.GetNumberOfValues())
  {
  }

  using IteratorType = typename DelegateArrayPortalToIterators::IteratorType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorType GetBegin() const { return this->DelegateIterators.GetBegin(); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  IteratorType GetEnd() const
  {
    IteratorType iterator = this->GetBegin();
    std::advance(iterator, this->NumberOfValues);
    return iterator;
  }

private:
  DelegateArrayPortalToIterators DelegateIterators;
  vtkm::Id NumberOfValues;
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_internal_ArrayPortalShrink_h
