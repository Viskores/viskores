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
#ifndef vtk_m_cont_internal_ArrayPortalShrink_h
#define vtk_m_cont_internal_ArrayPortalShrink_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/Assert.h>

namespace vtkm {
namespace cont {
namespace internal {

/// This ArrayPortal adapter is a utility that allows you to shrink the
/// (reported) array size without actually modifying the underlying allocation.
///
template<class PortalT>
class ArrayPortalShrink
{
public:
  typedef PortalT DelegatePortalType;

  typedef typename DelegatePortalType::ValueType ValueType;
  typedef typename DelegatePortalType::IteratorType IteratorType;

  VTKM_CONT_EXPORT ArrayPortalShrink() : NumberOfValues(0) {  }

  VTKM_CONT_EXPORT ArrayPortalShrink(const DelegatePortalType &delegatePortal)
    : DelegatePortal(delegatePortal),
      NumberOfValues(delegatePortal.GetNumberOfValues())
  {  }

  VTKM_CONT_EXPORT ArrayPortalShrink(const DelegatePortalType &delegatePortal,
                                     vtkm::Id numberOfValues)
    : DelegatePortal(delegatePortal), NumberOfValues(numberOfValues)
  {
    VTKM_ASSERT_CONT(numberOfValues <= delegatePortal.GetNumberOfValues());
  }

  /// Copy constructor for any other ArrayPortalShrink with a delegate type
  /// that can be copied to this type. This allows us to do any type casting
  /// the delegates can do (like the non-const to const cast).
  ///
  template<class OtherDelegateType>
  VTKM_CONT_EXPORT
  ArrayPortalShrink(const ArrayPortalShrink<OtherDelegateType> &src)
    : DelegatePortal(src.GetDelegatePortal()),
      NumberOfValues(src.GetNumberOfValues())
  {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_CONT_EXPORT
  ValueType Get(vtkm::Id index) const
  {
    VTKM_ASSERT_CONT(index >= 0);
    VTKM_ASSERT_CONT(index < this->GetNumberOfValues());
    return this->DelegatePortal.Get(index);
  }

  VTKM_CONT_EXPORT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    VTKM_ASSERT_CONT(index >= 0);
    VTKM_ASSERT_CONT(index < this->GetNumberOfValues());
    this->DelegatePortal.Set(index, value);
  }

  VTKM_CONT_EXPORT
  IteratorType GetIteratorBegin() const
  {
    return this->DelegatePortal.GetIteratorBegin();
  }

  VTKM_CONT_EXPORT
  IteratorType GetIteratorEnd() const
  {
    IteratorType iterator = this->DelegatePortal.GetIteratorBegin();
    std::advance(iterator, this->GetNumberOfValues());
    return iterator;
  }

  /// Special method in this ArrayPortal that allows you to shrink the
  /// (exposed) array.
  ///
  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT_CONT(numberOfValues < this->GetNumberOfValues());
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

#endif //vtk_m_cont_internal_ArrayPortalShrink_h
