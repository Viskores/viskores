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
#ifndef vtk_m_cont_internal_ArrayTransfer_h
#define vtk_m_cont_internal_ArrayTransfer_h

#include <vtkm/cont/Storage.h>
#include <vtkm/cont/internal/ArrayManagerExecution.h>

namespace vtkm {
namespace cont {
namespace internal {

/// \brief Class that manages the transfer of data between control and execution.
///
/// This templated class provides a mechanism (used by the ArrayHandle) to
/// transfer data from the control environment to the execution environment and
/// back. The interface for ArrayTransfer is nearly identical to that of
/// ArrayManagerExecution and the default implementation simply delegates all
/// calls to that class.
///
/// The primary motivation for having a separate class is that the
/// ArrayManagerExecution is meant to be specialized for each device adapter
/// whereas the ArrayTransfer is meant to be specialized for each storage type
/// (or specific combination of storage and device adapter). Thus, transfers
/// for most storage tyeps will be delegated through the ArrayManagerExecution,
/// but some storage types, like implicit storage, will be specialized to
/// transfer through a different path.
///
template<typename T, class StorageTag, class DeviceAdapterTag>
class ArrayTransfer
{
private:
  typedef vtkm::cont::internal::Storage<T,StorageTag> StorageType;
  typedef vtkm::cont::internal::ArrayManagerExecution<
      T,StorageTag,DeviceAdapterTag> ArrayManagerType;

public:
  /// The type of value held in the array (vtkm::FloatDefault, vtkm::Vec, etc.)
  ///
  typedef T ValueType;

  /// An array portal that can be used in the control environment.
  ///
  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;

  /// An array portal that can be used in the execution environment.
  ///
  typedef typename ArrayManagerType::PortalType PortalExecution;
  typedef typename ArrayManagerType::PortalConstType PortalConstExecution;


  /// Returns the number of values stored in the array.  Results are undefined
  /// if data has not been loaded or allocated.
  ///
  VTKM_CONT_EXPORT vtkm::Id GetNumberOfValues() const
  {
    return this->ArrayManager.GetNumberOfValues();
  }

  /// Allocates a large enough array in the execution environment and copies
  /// the given data to that array. The allocated array can later be accessed
  /// via the GetPortalConstExecution method. If control and execution share
  /// arrays, then this method may save the iterators to be returned in the \c
  /// GetPortalConst methods.
  ///
  VTKM_CONT_EXPORT void LoadDataForInput(const PortalConstControl& portal)
  {
    this->ArrayManager.LoadDataForInput(portal);
  }

  /// Allocates a large enough array in the execution environment and copies
  /// the given data to that array. The allocated array can later be accessed
  /// via the GetPortalConstExecution method. If control and execution share
  /// arrays, then this method may save the iterators to be returned in the \c
  /// GetPortalConst methods.
  ///
  VTKM_CONT_EXPORT void LoadDataForInput(const StorageType &controlArray)
  {
    this->ArrayManager.LoadDataForInput(controlArray.GetPortalConst());
  }

  /// Allocates a large enough array in the execution environment and copies
  /// the given data to that array. The allocated array can later be accessed
  /// via the GetPortalExection method. If control and execution share arrays,
  /// then this method may save the iterators of the storage to be returned
  /// in the \c GetPortal* methods.
  ///
  VTKM_CONT_EXPORT void LoadDataForInPlace(StorageType &controlArray)
  {
    this->ArrayManager.LoadDataForInPlace(controlArray.GetPortal());
  }

  /// Allocates an array in the execution environment of the specified size. If
  /// control and execution share arrays, then this class can allocate data
  /// using the given Storage and remember its iterators so that it can be used
  /// directly in the execution environment.
  ///
  VTKM_CONT_EXPORT void AllocateArrayForOutput(StorageType &controlArray,
                                               vtkm::Id numberOfValues)
  {
    this->ArrayManager.AllocateArrayForOutput(controlArray, numberOfValues);
  }

  /// Allocates data in the given Storage and copies data held in the execution
  /// environment (managed by this class) into the control array. If control
  /// and execution share arrays, this can be no operation. This method should
  /// only be called after AllocateArrayForOutput is called.
  ///
  VTKM_CONT_EXPORT void RetrieveOutputData(StorageType &controlArray) const
  {
    this->ArrayManager.RetrieveOutputData(controlArray);
  }

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  ///
  VTKM_CONT_EXPORT void Shrink(vtkm::Id numberOfValues)
  {
    this->ArrayManager.Shrink(numberOfValues);
  }

  /// Returns an array portal that can be used in the execution environment.
  /// This portal was defined in either LoadDataForInput or
  /// AllocateArrayForOutput. If control and environment share memory space,
  /// this class may return the iterator from the \c controlArray.
  ///
  VTKM_CONT_EXPORT PortalExecution GetPortalExecution()
  {
    return this->ArrayManager.GetPortal();
  }

  /// Const version of GetPortal.
  ///
  VTKM_CONT_EXPORT PortalConstExecution GetPortalConstExecution() const
  {
    return this->ArrayManager.GetPortalConst();
  }

  /// Frees any resources (i.e. memory) allocated for the exeuction
  /// environment, if any.
  ///
  VTKM_CONT_EXPORT void ReleaseResources()
  {
    this->ArrayManager.ReleaseResources();
  }

private:
  ArrayManagerType ArrayManager;
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ArrayTransfer_h
