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
#ifndef vtk_m_cont_exec_ArrayHandleExecutionManager_h
#define vtk_m_cont_exec_ArrayHandleExecutionManager_h

#include <vtkm/cont/ErrorControlInternal.h>
#include <vtkm/cont/Storage.h>

#include <vtkm/cont/internal/ArrayTransfer.h>

namespace vtkm {
namespace cont {
namespace internal {

/// The common base for ArrayHandleExecutionManager. This is the interface
/// used when the type of the device is not known at run time.
///
template<typename T, typename Storage>
class ArrayHandleExecutionManagerBase
{
private:
  typedef vtkm::cont::internal::Storage<T,Storage> StorageType;

public:
  template <typename DeviceAdapter>
  struct ExecutionTypes
  {
  private:
    typedef vtkm::cont::internal::ArrayTransfer<T,Storage,DeviceAdapter>
        ArrayTransferType;
  public:
    typedef typename ArrayTransferType::PortalExecution Portal;
    typedef typename ArrayTransferType::PortalConstExecution PortalConst;
  };

  /// The type of value held in the array (vtkm::FloatDefault, vtkm::Vec, etc.)
  ///
  typedef T ValueType;

  /// An array portal that can be used in the control environment.
  ///
  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;

  VTKM_CONT_EXPORT
  virtual ~ArrayHandleExecutionManagerBase() {  }

  /// Returns the number of values stored in the array.  Results are undefined
  /// if data has not been loaded or allocated.
  ///
  virtual vtkm::Id GetNumberOfValues() const = 0;

  /// Allocates a large enough array in the execution environment and copies
  /// the given data to that array. The allocated array can later be accessed
  /// via the GetPortalConstExecution method. If control and execution share
  /// arrays, then this method may save the iterators to be returned in the \c
  /// GetPortalConst methods.
  ///
  virtual void LoadDataForInput(const StorageType &storage) = 0;

  /// Allocates a large enough array in the execution environment and copies
  /// the given data to that array. The allocated array can later be accessed
  /// via the GetPortalExection method. If control and execution share arrays,
  /// then this method may save the iterators of the storage to be returned
  /// in the \c GetPortal* methods.
  ///
  virtual void LoadDataForInPlace(StorageType &storage) = 0;

  /// Allocates an array in the execution environment of the specified size. If
  /// control and execution share arrays, then this class can allocate data
  /// using the given Storage and remember its iterators so that it can be used
  /// directly in the execution environment.
  ///
  virtual void AllocateArrayForOutput(StorageType &storage,
                                      vtkm::Id numberOfValues) = 0;

  /// Allocates data in the given Storage and copies data held
  /// in the execution environment (managed by this class) into the control
  /// array. If control and execution share arrays, this can be no operation.
  /// This method should only be called after AllocateArrayForOutput is
  /// called.
  ///
  virtual void RetrieveOutputData(StorageType &storage) const = 0;

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  ///
  virtual void Shrink(vtkm::Id numberOfValues) = 0;

  /// Returns an array portal that can be used in the execution environment.
  /// This portal was defined in either LoadDataForInput or
  /// AllocateArrayForOutput. If control and environment share memory space,
  /// this class may return the iterator from the \c controlArray.
  ///
  template<typename DeviceAdapter>
  VTKM_CONT_EXPORT
  typename ExecutionTypes<DeviceAdapter>::Portal
  GetPortalExecution(DeviceAdapter device)
  {
    this->VerifyDeviceAdapter(device);

    typename ExecutionTypes<DeviceAdapter>::Portal portal;
    this->GetPortalExecutionImpl(&portal);
    return portal;
  }

  /// Const version of GetPortal.
  ///
  template<typename DeviceAdapter>
  VTKM_CONT_EXPORT
  typename ExecutionTypes<DeviceAdapter>::PortalConst
  GetPortalConstExecution(DeviceAdapter device) const
  {
    this->VerifyDeviceAdapter(device);

    typename ExecutionTypes<DeviceAdapter>::PortalConst portal;
    this->GetPortalConstExecutionImpl(&portal);
    return portal;
  }

  /// Frees any resources (i.e. memory) allocated for the exeuction
  /// environment, if any.
  ///
  virtual void ReleaseResources() = 0;

  template<typename DeviceAdapter>
  VTKM_CONT_EXPORT
  bool IsDeviceAdapter(DeviceAdapter) const
  {
    return this->IsDeviceAdapterImpl(
             vtkm::cont::internal::DeviceAdapterTraits<DeviceAdapter>::GetId());
  }

protected:
  virtual void GetPortalExecutionImpl(void *portalExecution) = 0;

  virtual void GetPortalConstExecutionImpl(
    void *portalConstExecution) const = 0;

  virtual bool IsDeviceAdapterImpl(
    const vtkm::cont::internal::DeviceAdapterId &id) const = 0;

private:
  template<typename DeviceAdapter>
  VTKM_CONT_EXPORT
  void VerifyDeviceAdapter(DeviceAdapter device) const
  {
    if (!this->IsDeviceAdapter(device))
    {
      throw vtkm::cont::ErrorControlInternal("Device Adapter Mismatch");
    }
  }
};

/// \brief Used by ArrayHandle to manage execution arrays
///
/// This is an internal class used by ArrayHandle to manage execution arrays.
/// This class uses virtual method polymorphism to allocate and transfer data
/// in the execution environment. This virtual method polymorphism allows the
/// ArrayHandle to change its device at run time.
///
template<typename T,
         typename Storage,
         typename DeviceAdapter>
class ArrayHandleExecutionManager
  : public ArrayHandleExecutionManagerBase<T, Storage>
{
  typedef ArrayHandleExecutionManagerBase<T, Storage> Superclass;
  typedef vtkm::cont::internal::ArrayTransfer<T,Storage,DeviceAdapter>
      ArrayTransferType;
  typedef vtkm::cont::internal::Storage<T,Storage> StorageType;

public:
  typedef typename ArrayTransferType::PortalControl PortalControl;
  typedef typename ArrayTransferType::PortalConstControl PortalConstControl;

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->Transfer.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  void LoadDataForInput(const StorageType &storage)
  {
    this->Transfer.LoadDataForInput(storage);
  }

  VTKM_CONT_EXPORT
  void LoadDataForInPlace(StorageType &storage)
  {
    this->Transfer.LoadDataForInPlace(storage);
  }

  VTKM_CONT_EXPORT
  void AllocateArrayForOutput(StorageType &storage, Id numberOfValues)
  {
    this->Transfer.AllocateArrayForOutput(storage, numberOfValues);
  }

  VTKM_CONT_EXPORT
  void RetrieveOutputData(StorageType &storage) const
  {
    this->Transfer.RetrieveOutputData(storage);
  }

  VTKM_CONT_EXPORT
  void Shrink(Id numberOfValues)
  {
    this->Transfer.Shrink(numberOfValues);
  }

  VTKM_CONT_EXPORT
  void ReleaseResources()
  {
    this->Transfer.ReleaseResources();
  }

protected:
  VTKM_CONT_EXPORT
  void GetPortalExecutionImpl(void *portalExecutionVoid)
  {
    typedef typename ArrayTransferType::PortalExecution PortalType;
    PortalType portalExecution = this->Transfer.GetPortalExecution();
    *reinterpret_cast<PortalType *>(portalExecutionVoid) = portalExecution;
  }

  VTKM_CONT_EXPORT
  void GetPortalConstExecutionImpl(void *portalExecutionVoid) const
  {
    typedef typename ArrayTransferType::PortalConstExecution PortalType;
    PortalType portalExecution = this->Transfer.GetPortalConstExecution();
    *reinterpret_cast<PortalType *>(portalExecutionVoid) = portalExecution;
  }

  VTKM_CONT_EXPORT
  bool IsDeviceAdapterImpl(const DeviceAdapterId &id) const
  {
    return id == vtkm::cont::internal::DeviceAdapterTraits<DeviceAdapter>::GetId();
  }

private:
  ArrayTransferType Transfer;
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_exec_ArrayHandleExecutionManager_h
