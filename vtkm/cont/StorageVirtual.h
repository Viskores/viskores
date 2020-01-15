//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_StorageVirtual_h
#define vtk_m_cont_StorageVirtual_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/Storage.h>
#include <vtkm/cont/internal/TransferInfo.h>
#include <vtkm/internal/ArrayPortalVirtual.h>

#include <typeinfo>

namespace vtkm
{
namespace cont
{

struct VTKM_ALWAYS_EXPORT StorageTagVirtual
{
};

namespace internal
{

namespace detail
{

class VTKM_CONT_EXPORT StorageVirtual
{
public:
  /// \brief construct storage that VTK-m is responsible for
  StorageVirtual() = default;
  StorageVirtual(const StorageVirtual& src);
  StorageVirtual(StorageVirtual&& src) noexcept;
  StorageVirtual& operator=(const StorageVirtual& src);
  StorageVirtual& operator=(StorageVirtual&& src) noexcept;

  virtual ~StorageVirtual();

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  /// Only needs to overridden by subclasses such as Zip that have member
  /// variables that themselves have execution memory
  virtual void ReleaseResourcesExecution() = 0;

  /// Releases all resources in both the control and execution environments.
  ///
  /// Only needs to overridden by subclasses such as Zip that have member
  /// variables that themselves have execution memory
  virtual void ReleaseResources() = 0;

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
  virtual void Allocate(vtkm::Id numberOfValues) = 0;

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  virtual void Shrink(vtkm::Id numberOfValues) = 0;

  /// Determines if storage types matches the type passed in.
  ///
  template <typename DerivedStorage>
  bool IsType() const
  { //needs optimizations based on platform. !OSX can use typeid
    return nullptr != dynamic_cast<const DerivedStorage*>(this);
  }

  /// \brief Create a new storage of the same type as this storage.
  ///
  /// This method creates a new storage that is the same type as this one and
  /// returns a unique_ptr for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  std::unique_ptr<StorageVirtual> NewInstance() const;

  template <typename DerivedStorage>
  const DerivedStorage* Cast() const
  {
    const DerivedStorage* derived = dynamic_cast<const DerivedStorage*>(this);
    if (!derived)
    {
      VTKM_LOG_CAST_FAIL(*this, DerivedStorage);
      throwFailedDynamicCast("StorageVirtual", vtkm::cont::TypeToString<DerivedStorage>());
    }
    VTKM_LOG_CAST_SUCC(*this, derived);
    return derived;
  }

  const vtkm::internal::PortalVirtualBase* PrepareForInput(vtkm::cont::DeviceAdapterId devId) const;

  const vtkm::internal::PortalVirtualBase* PrepareForOutput(vtkm::Id numberOfValues,
                                                            vtkm::cont::DeviceAdapterId devId);

  const vtkm::internal::PortalVirtualBase* PrepareForInPlace(vtkm::cont::DeviceAdapterId devId);

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


  enum struct OutputMode
  {
    WRITE,
    READ_WRITE
  };

protected:
  /// Drop the reference to the execution portal. The underlying array handle might still be
  /// managing data on the execution side, but our references might be out of date, so drop
  /// them and refresh them later if necessary.
  void DropExecutionPortal();

  /// Drop the reference to all portals. The underlying array handle might still be managing data,
  /// but our references might be out of date, so drop them and refresh them later if necessary.
  void DropAllPortals();

private:
  //Memory management routines
  // virtual void DoAllocate(vtkm::Id numberOfValues) = 0;
  // virtual void DoShrink(vtkm::Id numberOfValues) = 0;

  //RTTI routines
  virtual std::unique_ptr<StorageVirtual> MakeNewInstance() const = 0;

  //Portal routines
  virtual void ControlPortalForInput(vtkm::cont::internal::TransferInfoArray& payload) const = 0;
  virtual void ControlPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload);


  virtual void TransferPortalForInput(vtkm::cont::internal::TransferInfoArray& payload,
                                      vtkm::cont::DeviceAdapterId devId) const = 0;
  virtual void TransferPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload,
                                       OutputMode mode,
                                       vtkm::Id numberOfValues,
                                       vtkm::cont::DeviceAdapterId devId);

  //These might need to exist in TransferInfoArray
  mutable bool HostUpToDate = false;
  mutable bool DeviceUpToDate = false;
  std::shared_ptr<vtkm::cont::internal::TransferInfoArray> DeviceTransferState =
    std::make_shared<vtkm::cont::internal::TransferInfoArray>();
};

template <typename T, typename S>
class VTKM_ALWAYS_EXPORT StorageVirtualImpl final
  : public vtkm::cont::internal::detail::StorageVirtual
{
public:
  VTKM_CONT
  explicit StorageVirtualImpl(const vtkm::cont::ArrayHandle<T, S>& ah);

  explicit StorageVirtualImpl(vtkm::cont::ArrayHandle<T, S>&& ah) noexcept;

  VTKM_CONT
  ~StorageVirtualImpl() override = default;

  const vtkm::cont::ArrayHandle<T, S>& GetHandle() const { return this->Handle; }

  vtkm::Id GetNumberOfValues() const override { return this->Handle.GetNumberOfValues(); }

  void ReleaseResourcesExecution() override;
  void ReleaseResources() override;

  void Allocate(vtkm::Id numberOfValues) override;
  void Shrink(vtkm::Id numberOfValues) override;

private:
  std::unique_ptr<vtkm::cont::internal::detail::StorageVirtual> MakeNewInstance() const override
  {
    return std::unique_ptr<vtkm::cont::internal::detail::StorageVirtual>(
      new StorageVirtualImpl<T, S>{ vtkm::cont::ArrayHandle<T, S>{} });
  }


  void ControlPortalForInput(vtkm::cont::internal::TransferInfoArray& payload) const override;
  void ControlPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload) override;

  void TransferPortalForInput(vtkm::cont::internal::TransferInfoArray& payload,
                              vtkm::cont::DeviceAdapterId devId) const override;

  void TransferPortalForOutput(vtkm::cont::internal::TransferInfoArray& payload,
                               OutputMode mode,
                               vtkm::Id numberOfValues,
                               vtkm::cont::DeviceAdapterId devId) override;

  vtkm::cont::ArrayHandle<T, S> Handle;
};

} // namespace detail

template <typename T>
class VTKM_ALWAYS_EXPORT Storage<T, vtkm::cont::StorageTagVirtual>
{
public:
  using ValueType = T;

  using PortalType = vtkm::ArrayPortalRef<T>;
  using PortalConstType = vtkm::ArrayPortalRef<T>;

  Storage() = default;

  // Should never really be used, but just in case.
  Storage(const Storage<T, vtkm::cont::StorageTagVirtual>&) = default;

  template <typename S>
  Storage(const vtkm::cont::ArrayHandle<T, S>& srcArray)
    : VirtualStorage(std::make_shared<detail::StorageVirtualImpl<T, S>>(srcArray))
  {
  }

  ~Storage() = default;

  PortalType GetPortal()
  {
    return make_ArrayPortalRef(
      static_cast<const vtkm::ArrayPortalVirtual<T>*>(this->VirtualStorage->GetPortalControl()),
      this->GetNumberOfValues());
  }

  PortalConstType GetPortalConst() const
  {
    return make_ArrayPortalRef(static_cast<const vtkm::ArrayPortalVirtual<T>*>(
                                 this->VirtualStorage->GetPortalConstControl()),
                               this->GetNumberOfValues());
  }

  vtkm::Id GetNumberOfValues() const { return this->VirtualStorage->GetNumberOfValues(); }

  void Allocate(vtkm::Id numberOfValues);

  void Shrink(vtkm::Id numberOfValues);

  void ReleaseResources();

  Storage<T, vtkm::cont::StorageTagVirtual> NewInstance() const;

  const detail::StorageVirtual* GetStorageVirtual() const { return this->VirtualStorage.get(); }
  detail::StorageVirtual* GetStorageVirtual() { return this->VirtualStorage.get(); }

private:
  std::shared_ptr<detail::StorageVirtual> VirtualStorage;

  Storage(std::shared_ptr<detail::StorageVirtual> virtualStorage)
    : VirtualStorage(virtualStorage)
  {
  }
};

} // namespace internal
}
} // namespace vtkm::cont

#ifndef vtk_m_cont_StorageVirtual_hxx
#include <vtkm/cont/StorageVirtual.hxx>
#endif

#endif
