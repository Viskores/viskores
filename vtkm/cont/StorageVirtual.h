//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_StorageVirtual_h
#define vtk_m_cont_StorageVirtual_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/Storage.h>
#include <vtkm/cont/internal/TransferInfo.h>
#include <vtkm/internal/ArrayPortalVirtual.h>

#include <typeinfo>

namespace vtkm
{
namespace cont
{

struct StorageTagVirtual
{
};

namespace internal
{

template <>
class VTKM_CONT_EXPORT Storage<void, vtkm::cont::StorageTagVirtual>
{
public:
  /// \brief construct storage that VTK-m is responsible for
  Storage() = default;
  Storage(const Storage<void, vtkm::cont::StorageTagVirtual>& src);
  Storage(Storage<void, vtkm::cont::StorageTagVirtual>&& src) noexcept;
  Storage& operator=(const Storage<void, vtkm::cont::StorageTagVirtual>& src);
  Storage& operator=(Storage<void, vtkm::cont::StorageTagVirtual>&& src) noexcept;

  virtual ~Storage();

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  /// Only needs to overridden by subclasses such as Zip that have member
  /// variables that themselves have execution memory
  virtual void ReleaseResourcesExecution();

  /// Releases all resources in both the control and execution environments.
  ///
  /// Only needs to overridden by subclasses such as Zip that have member
  /// variables that themselves have execution memory
  virtual void ReleaseResources();

  /// Returns the number of entries in the array.
  ///
  virtual vtkm::Id GetNumberOfValues() const = 0;

  /// \brief Allocates an array large enough to hold the given number of values.
  ///
  /// The allocation may be done on an already existing array, but can wipe out
  /// any data already in the array. This method can throw
  /// ErrorBadAllocation if the array cannot be allocated or
  /// ErrorBadValue if the allocation is not feasible (for example, the
  /// array storage is read-only).
  ///
  void Allocate(vtkm::Id numberOfValues)
  {
    std::cout << "StorageVirtual::Allocate(" << numberOfValues << ") Not Implemented!" << std::endl;
  } //return this->DoAllocate(numberOfValues); }

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  void Shrink(vtkm::Id numberOfValues)
  {
    std::cout << "StorageVirtual::Shrink(" << numberOfValues << ") Not Implemented!." << std::endl;
  } //return this->DoShrink(numberOfValues); }

  /// Determines if storage types matches the type passed in.
  ///
  bool IsType(const std::type_info& other) const { return this->IsSameType(other); }

  /// \brief Create a new storage of the same type as this storage.
  ///
  /// This method creates a new storage that is the same type as this one and
  /// returns a unique_ptr for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  std::unique_ptr<Storage<void, ::vtkm::cont::StorageTagVirtual>> NewInstance() const;

  template <typename DerivedStorage>
  const DerivedStorage* Cast() const
  {
    const DerivedStorage* derived = dynamic_cast<const DerivedStorage*>(this);
    if (!derived)
    {
      VTKM_LOG_CAST_FAIL(*this, DerivedStorage);
      throwFailedDynamicCast("StorageVirtual", vtkm::cont::TypeName<DerivedStorage>());
    }
    VTKM_LOG_CAST_SUCC(*this, derived);
    return derived;
  }

  const vtkm::internal::PortalVirtualBase* PrepareForInput(vtkm::cont::DeviceAdapterId devId) const;

  const vtkm::internal::PortalVirtualBase* PrepareForOutput(vtkm::Id numberOfValues,
                                                            vtkm::cont::DeviceAdapterId devId);

  //This needs to cause a host side sync!
  //This needs to work before we execute on a device
  const vtkm::internal::PortalVirtualBase* GetPortalControl();

  //This needs to cause a host side sync!
  //This needs to work before we execute on a device
  const vtkm::internal::PortalVirtualBase* GetPortalConstControl() const;

  /// Returns the DeviceAdapterId for the current device. If there is no device
  /// with an up-to-date copy of the data, VTKM_DEVICE_ADAPTER_UNDEFINED is
  /// returned.
  DeviceAdapterId GetDeviceAdapterId() const noexcept;

private:
  //Memory management routines
  // virtual void DoAllocate(vtkm::Id numberOfValues) = 0;
  // virtual void DoShrink(vtkm::Id numberOfValues) = 0;

  //RTTI routines
  virtual bool IsSameType(const std::type_info&) const;
  virtual std::unique_ptr<Storage<void, ::vtkm::cont::StorageTagVirtual>> MakeNewInstance()
    const = 0;

  //Portal routines
  virtual void ControlPortalForInput(vtkm::cont::internal::TransferInfoArray& payload) const = 0;
  virtual void ControlPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload);
  virtual void TransferPortalForInput(vtkm::cont::internal::TransferInfoArray& payload,
                                      vtkm::cont::DeviceAdapterId devId) const = 0;
  virtual void TransferPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload,
                                       vtkm::Id numberOfValues,
                                       vtkm::cont::DeviceAdapterId devId);

  //These might need to exist in TransferInfoArray
  mutable bool HostUpToDate = false;
  mutable bool DeviceUpToDate = false;
  std::shared_ptr<vtkm::cont::internal::TransferInfoArray> DeviceTransferState =
    std::make_shared<vtkm::cont::internal::TransferInfoArray>();
};

} // namespace internal

using StorageVirtual = internal::Storage<void, vtkm::cont::StorageTagVirtual>;
}
} // namespace vtkm::cont

#endif
