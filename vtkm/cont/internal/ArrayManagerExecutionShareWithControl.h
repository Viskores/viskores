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
#ifndef vtk_m_cont_internal_ArrayManagerExecutionShareWithControl_h
#define vtk_m_cont_internal_ArrayManagerExecutionShareWithControl_h

#include <vtkm/Types.h>

#include <vtkm/cont/Assert.h>
#include <vtkm/cont/Storage.h>

#include <vtkm/cont/internal/ArrayPortalShrink.h>

#include <algorithm>

namespace vtkm {
namespace cont {
namespace internal {

/// \c ArrayManagerExecutionShareWithControl provides an implementation for a
/// \c ArrayManagerExecution class for a device adapter when the execution
/// and control environments share memory. This class basically defers all its
/// calls to a \c Storage class and uses the array allocated there.
///
template<typename T, class StorageTag>
class ArrayManagerExecutionShareWithControl
{
public:
  typedef T ValueType;
  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;
  typedef vtkm::cont::internal::ArrayPortalShrink<
      typename StorageType::PortalType> PortalType;
  typedef vtkm::cont::internal::ArrayPortalShrink<
      typename StorageType::PortalConstType> PortalConstType;

  VTKM_CONT_EXPORT ArrayManagerExecutionShareWithControl()
    : PortalValid(false), ConstPortalValid(false) { }

  /// Returns the size of the saved portal.
  ///
  VTKM_CONT_EXPORT vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT_CONT(this->ConstPortalValid);
    return this->ConstPortal.GetNumberOfValues();
  }

  /// Saves the given iterators to be returned later.
  ///
  VTKM_CONT_EXPORT void LoadDataForInput(const StorageType& storage)
  {
    this->ConstPortal = storage.GetPortalConst();
    this->ConstPortalValid = true;

    // Non-const versions not defined.
    this->PortalValid = false;
  }

  /// Saves the given iterators to be returned later.
  ///
  VTKM_CONT_EXPORT void LoadDataForInPlace(StorageType &storage)
  {
    // This only works if there is a valid cast from non-const to const
    // iterator.
    this->LoadDataForInput(storage);

    this->Portal = storage.GetPortal();
    this->PortalValid = true;
  }

  /// Actually just allocates memory in the given \p controlArray.
  ///
  VTKM_CONT_EXPORT void AllocateArrayForOutput(StorageType &controlArray,
                                               vtkm::Id numberOfValues)
  {
    controlArray.Allocate(numberOfValues);

    this->Portal = controlArray.GetPortal();
    this->PortalValid = true;

    this->ConstPortal = controlArray.GetPortalConst();
    this->ConstPortalValid = true;
  }

  /// This method is a no-op (except for a few checks). Any data written to
  /// this class's portals should already be written to the given \c
  /// controlArray (under correct operation).
  ///
  VTKM_CONT_EXPORT void RetrieveOutputData(StorageType &controlArray) const
  {
    VTKM_ASSERT_CONT(this->ConstPortalValid);
    controlArray.Shrink(this->ConstPortal.GetNumberOfValues());
  }

  /// Adjusts saved end iterators to resize array.
  ///
  VTKM_CONT_EXPORT void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT_CONT(this->ConstPortalValid);
    this->ConstPortal.Shrink(numberOfValues);

    if (this->PortalValid)
    {
      this->Portal.Shrink(numberOfValues);
    }
  }

  /// Returns the portal previously saved from a \c Storage.
  ///
  VTKM_CONT_EXPORT PortalType GetPortal()
  {
    VTKM_ASSERT_CONT(this->PortalValid);
    return this->Portal;
  }

  /// Const version of GetPortal.
  ///
  VTKM_CONT_EXPORT PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT_CONT(this->ConstPortalValid);
    return this->ConstPortal;
  }

  /// A no-op.
  ///
  VTKM_CONT_EXPORT void ReleaseResources() { }

private:
  // Not implemented.
  ArrayManagerExecutionShareWithControl(
      ArrayManagerExecutionShareWithControl<T, StorageTag> &);
  void operator=(
      ArrayManagerExecutionShareWithControl<T, StorageTag> &);

  PortalType Portal;
  bool PortalValid;

  PortalConstType ConstPortal;
  bool ConstPortalValid;
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ArrayManagerExecutionShareWithControl_h
