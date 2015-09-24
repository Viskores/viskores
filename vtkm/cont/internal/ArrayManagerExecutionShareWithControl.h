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
//  Copyright 2014 Los Alamos National Security.
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
  typedef typename StorageType::PortalType PortalType;
  typedef typename StorageType::PortalConstType PortalConstType;

  VTKM_CONT_EXPORT
  ArrayManagerExecutionShareWithControl(StorageType *storage)
    : Storage(storage) { }

  /// Returns the size of the storage.
  ///
  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->Storage->GetNumberOfValues();
  }

  /// Returns the constant portal from the storage.
  ///
  VTKM_CONT_EXPORT
  PortalConstType PrepareForInput(bool vtkmNotUsed(uploadData)) const
  {
    return this->Storage->GetPortalConst();
  }

  /// Returns the read-write portal from the storage.
  ///
  VTKM_CONT_EXPORT
  PortalType PrepareForInPlace(bool vtkmNotUsed(uploadData))
  {
    return this->Storage->GetPortal();
  }

  /// Allocates data in the storage and return the portal to that.
  ///
  VTKM_CONT_EXPORT
  PortalType PrepareForOutput(vtkm::Id numberOfValues)
  {
    this->Storage->Allocate(numberOfValues);
    return this->Storage->GetPortal();
  }

  /// This method is a no-op (except for a few checks). Any data written to
  /// this class's portals should already be written to the given \c
  /// controlArray (under correct operation).
  ///
  VTKM_CONT_EXPORT
  void RetrieveOutputData(StorageType *storage) const
  {
    (void)storage;
    VTKM_ASSERT_CONT(storage == this->Storage);
  }

  /// This methods copies data from the execution array into the given
  /// iterator.
  ///
  template <class IteratorTypeControl>
  VTKM_CONT_EXPORT void CopyInto(IteratorTypeControl dest) const
  {
    typedef typename StorageType::PortalConstType::IteratorType IteratorType;
    IteratorType beginIterator = 
                    this->Storage->GetPortalConst().GetIteratorBegin();

    std::copy(beginIterator, 
              beginIterator + this->Storage->GetNumberOfValues(), dest);
  }

  /// Shrinks the storage.
  ///
  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id numberOfValues)
  {
    this->Storage->Shrink(numberOfValues);
  }

  /// A no-op.
  ///
  VTKM_CONT_EXPORT
  void ReleaseResources() { }

private:
  // Not implemented.
  ArrayManagerExecutionShareWithControl(
      ArrayManagerExecutionShareWithControl<T, StorageTag> &);
  void operator=(
      ArrayManagerExecutionShareWithControl<T, StorageTag> &);

  StorageType *Storage;
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ArrayManagerExecutionShareWithControl_h
