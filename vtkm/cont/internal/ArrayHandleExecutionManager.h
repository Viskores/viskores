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
#ifndef vtk_m_cont_exec_ArrayHandleExecutionManager_h
#define vtk_m_cont_exec_ArrayHandleExecutionManager_h

#include <vtkm/cont/ErrorInternal.h>
#include <vtkm/cont/Storage.h>

#include <vtkm/cont/internal/ArrayTransfer.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// The common base for ArrayHandleExecutionManager. This is the interface
/// used when the type of the device is not known at run time.
///
template <typename T, typename Storage>
class ArrayHandleExecutionManagerBase
{
private:
  typedef vtkm::cont::internal::Storage<T, Storage> StorageType;

public:
  template <typename DeviceAdapter>
  struct ExecutionTypes
  {
  private:
    typedef vtkm::cont::internal::ArrayTransfer<T, Storage, DeviceAdapter> ArrayTransferType;

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

  VTKM_CONT
  virtual ~ArrayHandleExecutionManagerBase() {}

  /// Returns the number of values stored in the array.  Results are undefined
  /// if data has not been loaded or allocated.
  ///
  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->GetNumberOfValuesImpl(); }

  /// Prepares the data for use as input in the execution environment. If the
  /// flag \c updateData is true, then data is transferred to the execution
  /// environment. Otherwise, this transfer should be skipped.
  ///
  /// Returns a constant array portal valid in the execution environment.
  ///
  template <typename DeviceAdapter>
  VTKM_CONT typename ExecutionTypes<DeviceAdapter>::PortalConst PrepareForInput(bool updateData,
                                                                                DeviceAdapter)
  {
    this->VerifyDeviceAdapter(DeviceAdapter());

    typename ExecutionTypes<DeviceAdapter>::PortalConst portal;
    this->PrepareForInputImpl(updateData, &portal);
    return portal;
  }

  /// Prepares the data for use as both input and output in the execution
  /// environment. If the flag \c updateData is true, then data is transferred
  /// to the execution environment. Otherwise, this transfer should be skipped.
  ///
  /// Returns a read-write array portal valid in the execution environment.
  ///
  template <typename DeviceAdapter>
  VTKM_CONT typename ExecutionTypes<DeviceAdapter>::Portal PrepareForInPlace(bool updateData,
                                                                             DeviceAdapter)
  {
    this->VerifyDeviceAdapter(DeviceAdapter());

    typename ExecutionTypes<DeviceAdapter>::Portal portal;
    this->PrepareForInPlaceImpl(updateData, &portal);
    return portal;
  }

  /// Allocates an array in the execution environment of the specified size. If
  /// control and execution share arrays, then this class can allocate data
  /// using the given Storage it can be used directly in the execution
  /// environment.
  ///
  /// Returns a writable array portal valid in the execution environment.
  ///
  template <typename DeviceAdapter>
  VTKM_CONT typename ExecutionTypes<DeviceAdapter>::Portal PrepareForOutput(vtkm::Id numberOfValues,
                                                                            DeviceAdapter)
  {
    this->VerifyDeviceAdapter(DeviceAdapter());

    typename ExecutionTypes<DeviceAdapter>::Portal portal;
    this->PrepareForOutputImpl(numberOfValues, &portal);
    return portal;
  }

  /// Allocates data in the given Storage and copies data held in the execution
  /// environment (managed by this class) into the storage object. The
  /// reference to the storage given is the same as that passed to the
  /// constructor. If control and execution share arrays, this can be no
  /// operation. This method should only be called after PrepareForOutput is
  /// called.
  ///
  VTKM_CONT
  void RetrieveOutputData(StorageType* storage) const { this->RetrieveOutputDataImpl(storage); }

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  ///
  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->ShrinkImpl(numberOfValues); }

  /// Frees any resources (i.e. memory) allocated for the exeuction
  /// environment, if any.
  ///
  VTKM_CONT
  void ReleaseResources() { this->ReleaseResourcesImpl(); }

  template <typename DeviceAdapter>
  VTKM_CONT bool IsDeviceAdapter(DeviceAdapter) const
  {
    return this->IsDeviceAdapterImpl(vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetId());
  }

protected:
  virtual vtkm::Id GetNumberOfValuesImpl() const = 0;

  virtual void PrepareForInputImpl(bool updateData, void* portalExecutionVoid) = 0;

  virtual void PrepareForInPlaceImpl(bool updateData, void* portalExecutionVoid) = 0;

  virtual void PrepareForOutputImpl(vtkm::Id numberOfValues, void* portalExecution) = 0;

  virtual void RetrieveOutputDataImpl(StorageType* storage) const = 0;

  virtual void ShrinkImpl(Id numberOfValues) = 0;

  virtual void ReleaseResourcesImpl() = 0;

  virtual bool IsDeviceAdapterImpl(const vtkm::cont::DeviceAdapterId& id) const = 0;

private:
  template <typename DeviceAdapter>
  VTKM_CONT void VerifyDeviceAdapter(DeviceAdapter device) const
  {
    if (!this->IsDeviceAdapter(device))
    {
      throw vtkm::cont::ErrorInternal("Device Adapter Mismatch");
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
template <typename T, typename Storage, typename DeviceAdapter>
class ArrayHandleExecutionManager : public ArrayHandleExecutionManagerBase<T, Storage>
{
  typedef ArrayHandleExecutionManagerBase<T, Storage> Superclass;
  typedef vtkm::cont::internal::ArrayTransfer<T, Storage, DeviceAdapter> ArrayTransferType;
  typedef vtkm::cont::internal::Storage<T, Storage> StorageType;

public:
  typedef typename ArrayTransferType::PortalControl PortalControl;
  typedef typename ArrayTransferType::PortalConstControl PortalConstControl;

  typedef typename ArrayTransferType::PortalExecution PortalExecution;
  typedef typename ArrayTransferType::PortalConstExecution PortalConstExecution;

  VTKM_CONT
  ArrayHandleExecutionManager(StorageType* storage)
    : Transfer(storage)
  {
  }

  template <class IteratorTypeControl>
  VTKM_CONT void CopyInto(IteratorTypeControl dest) const
  {
    this->Transfer.CopyInto(dest);
  }

protected:
  VTKM_CONT
  vtkm::Id GetNumberOfValuesImpl() const { return this->Transfer.GetNumberOfValues(); }

  VTKM_CONT
  void PrepareForInputImpl(bool updateData, void* portalExecutionVoid)
  {
    PortalConstExecution portal = this->Transfer.PrepareForInput(updateData);
    *reinterpret_cast<PortalConstExecution*>(portalExecutionVoid) = portal;
  }

  VTKM_CONT
  void PrepareForInPlaceImpl(bool updateData, void* portalExecutionVoid)
  {
    PortalExecution portal = this->Transfer.PrepareForInPlace(updateData);
    *reinterpret_cast<PortalExecution*>(portalExecutionVoid) = portal;
  }

  VTKM_CONT
  void PrepareForOutputImpl(vtkm::Id numberOfValues, void* portalExecutionVoid)
  {
    PortalExecution portal = this->Transfer.PrepareForOutput(numberOfValues);
    *reinterpret_cast<PortalExecution*>(portalExecutionVoid) = portal;
  }

  VTKM_CONT
  void RetrieveOutputDataImpl(StorageType* storage) const
  {
    this->Transfer.RetrieveOutputData(storage);
  }

  VTKM_CONT
  void ShrinkImpl(Id numberOfValues) { this->Transfer.Shrink(numberOfValues); }

  VTKM_CONT
  void ReleaseResourcesImpl() { this->Transfer.ReleaseResources(); }

  VTKM_CONT
  bool IsDeviceAdapterImpl(const DeviceAdapterId& id) const
  {
    return id == vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetId();
  }

private:
  ArrayTransferType Transfer;
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_exec_ArrayHandleExecutionManager_h
