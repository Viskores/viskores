//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_internal_ArrayHandleBasicImpl_h
#define vtk_m_cont_internal_ArrayHandleBasicImpl_h

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/StorageBasic.h>
#include <vtkm/cont/Token.h>

#include <type_traits>

namespace vtkm
{
namespace cont
{

namespace internal
{

struct ArrayHandleImpl;

/// Type-agnostic container for an execution memory buffer.
struct VTKM_CONT_EXPORT TypelessExecutionArray
{

  TypelessExecutionArray(void*& executionArray,
                         void*& executionArrayEnd,
                         void*& executionArrayCapacity,
                         const StorageBasicBase* controlArray);

  void*& Array;
  void*& ArrayEnd;
  void*& ArrayCapacity;
  // Used by cuda to detect and share managed memory allocations.
  const void* ArrayControl;
  const void* ArrayControlCapacity;
};

/// Factory that generates execution portals for basic storage.
template <typename T, typename DeviceTag>
struct ExecutionPortalFactoryBasic
#ifndef VTKM_DOXYGEN_ONLY
  ;
#else  // VTKM_DOXYGEN_ONLY
{
  /// The portal type.
  using PortalType = SomePortalType;

  /// The cont portal type.
  using ConstPortalType = SomePortalType;

  /// Create a portal to access the execution data from @a start to @a end.
  VTKM_CONT
  static PortalType CreatePortal(ValueType* start, ValueType* end);

  /// Create a const portal to access the execution data from @a start to @a end.
  VTKM_CONT
  static PortalConstType CreatePortalConst(const ValueType* start, const ValueType* end);
};
#endif // VTKM_DOXYGEN_ONLY

/// Typeless interface for interacting with a execution memory buffer when using basic storage.
struct VTKM_CONT_EXPORT ExecutionArrayInterfaceBasicBase
{
  VTKM_CONT explicit ExecutionArrayInterfaceBasicBase(StorageBasicBase& storage);
  VTKM_CONT virtual ~ExecutionArrayInterfaceBasicBase();

  VTKM_CONT
  virtual DeviceAdapterId GetDeviceId() const = 0;

  /// If @a execArray's base pointer is null, allocate a new buffer.
  /// If (capacity - base) < @a numBytes, the buffer will be freed and
  /// reallocated. If (capacity - base) >= numBytes, a new end is marked.
  VTKM_CONT
  virtual void Allocate(TypelessExecutionArray& execArray,
                        vtkm::Id numberOfValues,
                        vtkm::UInt64 sizeOfValue) const = 0;

  /// Release the buffer held by @a execArray and reset all pointer to null.
  VTKM_CONT
  virtual void Free(TypelessExecutionArray& execArray) const = 0;

  /// Copy @a numBytes from @a controlPtr to @a executionPtr.
  VTKM_CONT
  virtual void CopyFromControl(const void* controlPtr,
                               void* executionPtr,
                               vtkm::UInt64 numBytes) const = 0;

  /// Copy @a numBytes from @a executionPtr to @a controlPtr.
  VTKM_CONT
  virtual void CopyToControl(const void* executionPtr,
                             void* controlPtr,
                             vtkm::UInt64 numBytes) const = 0;


  VTKM_CONT virtual void UsingForRead(const void* controlPtr,
                                      const void* executionPtr,
                                      vtkm::UInt64 numBytes) const = 0;
  VTKM_CONT virtual void UsingForWrite(const void* controlPtr,
                                       const void* executionPtr,
                                       vtkm::UInt64 numBytes) const = 0;
  VTKM_CONT virtual void UsingForReadWrite(const void* controlPtr,
                                           const void* executionPtr,
                                           vtkm::UInt64 numBytes) const = 0;

protected:
  StorageBasicBase& ControlStorage;
};

/**
 * Specializations should inherit from and implement the API of
 * ExecutionArrayInterfaceBasicBase.
 */
template <typename DeviceTag>
struct ExecutionArrayInterfaceBasic;

struct VTKM_CONT_EXPORT ArrayHandleImpl
{
  using MutexType = std::mutex;
  using LockType = std::unique_lock<MutexType>;

  VTKM_CONT
  template <typename T>
  explicit ArrayHandleImpl(T t)
    : Internals(new InternalStruct(t))
  {
  }

  VTKM_CONT
  template <typename T>
  explicit ArrayHandleImpl(
    const vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>& storage)
    : Internals(new InternalStruct(storage))
  {
  }

  VTKM_CONT
  template <typename T>
  explicit ArrayHandleImpl(vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>&& storage)
    : Internals(new InternalStruct(std::move(storage)))
  {
  }

  VTKM_CONT ~ArrayHandleImpl() = default;

  VTKM_CONT ArrayHandleImpl(const ArrayHandleImpl&) = delete;
  VTKM_CONT void operator=(const ArrayHandleImpl&) = delete;

  //Throws ErrorInternal if ControlArrayValid == false
  VTKM_CONT void CheckControlArrayValid(const LockType& lock) noexcept(false);

  VTKM_CONT vtkm::Id GetNumberOfValues(const LockType& lock, vtkm::UInt64 sizeOfT) const;
  VTKM_CONT void Allocate(LockType& lock, vtkm::Id numberOfValues, vtkm::UInt64 sizeOfT);
  VTKM_CONT void Shrink(LockType& lock, vtkm::Id numberOfValues, vtkm::UInt64 sizeOfT);

  VTKM_CONT void SyncControlArray(LockType& lock, vtkm::UInt64 sizeofT) const;
  VTKM_CONT void ReleaseResources(LockType& lock);
  VTKM_CONT void ReleaseResourcesExecutionInternal(LockType& lock);

  VTKM_CONT void PrepareForInput(LockType& lock,
                                 vtkm::UInt64 sizeofT,
                                 vtkm::cont::Token& token) const;
  VTKM_CONT void PrepareForOutput(LockType& lock,
                                  vtkm::Id numVals,
                                  vtkm::UInt64 sizeofT,
                                  vtkm::cont::Token& token);
  VTKM_CONT void PrepareForInPlace(LockType& lock, vtkm::UInt64 sizeofT, vtkm::cont::Token& token);

  // Check if the current device matches the last one. If they don't match
  // this moves all data back from execution environment and deletes the
  // ExecutionInterface instance.
  // Returns true when the caller needs to reallocate ExecutionInterface
  VTKM_CONT bool PrepareForDevice(LockType& lock,
                                  DeviceAdapterId devId,
                                  vtkm::UInt64 sizeofT) const;

  VTKM_CONT DeviceAdapterId GetDeviceAdapterId(const LockType& lock) const;

  /// Returns true if read operations can currently be performed.
  VTKM_CONT bool CanRead(const LockType& lock, const vtkm::cont::Token& token) const;
  //// Returns true if write operations can currently be performed.
  VTKM_CONT bool CanWrite(const LockType& lock, const vtkm::cont::Token& token) const;

  //// Will block the current thread until a read can be performed.
  VTKM_CONT void WaitToRead(LockType& lock, const vtkm::cont::Token& token) const;
  //// Will block the current thread until a write can be performed.
  VTKM_CONT void WaitToWrite(LockType& lock, const vtkm::cont::Token& token) const;

  /// Acquires a lock on the internals of this `ArrayHandle`. The calling
  /// function should keep the returned lock and let it go out of scope
  /// when the lock is no longer needed.
  ///
  LockType GetLock() const { return LockType(this->Internals->Mutex); }

  class VTKM_CONT_EXPORT InternalStruct
  {
    mutable std::shared_ptr<bool> ControlArrayValid;
    StorageBasicBase* ControlArray;

    mutable ExecutionArrayInterfaceBasicBase* ExecutionInterface = nullptr;
    mutable bool ExecutionArrayValid = false;
    mutable void* ExecutionArray = nullptr;
    mutable void* ExecutionArrayEnd = nullptr;
    mutable void* ExecutionArrayCapacity = nullptr;

    mutable vtkm::cont::Token::ReferenceCount ReadCount = 0;
    mutable vtkm::cont::Token::ReferenceCount WriteCount = 0;

    VTKM_CONT void CheckLock(const LockType& lock) const
    {
      VTKM_ASSERT((lock.mutex() == &this->Mutex) && (lock.owns_lock()));
    }

  public:
    MutexType Mutex;
    std::condition_variable ConditionVariable;

    template <typename T>
    VTKM_CONT explicit InternalStruct(T)
      : ControlArrayValid(new bool(false))
      , ControlArray(new vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>())
    {
    }

    template <typename T>
    VTKM_CONT explicit InternalStruct(
      const vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>& storage)
      : ControlArrayValid(new bool(true))
      , ControlArray(new vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>(storage))
    {
    }

    VTKM_CONT
    template <typename T>
    explicit InternalStruct(vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>&& storage)
      : ControlArrayValid(new bool(true))
      , ControlArray(
          new vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>(std::move(storage)))
    {
    }

    ~InternalStruct();

    // To access any feature in InternalStruct, you must have locked the mutex. You have
    // to prove it by passing in a reference to a std::unique_lock.
    VTKM_CONT bool IsControlArrayValid(const LockType& lock) const
    {
      this->CheckLock(lock);
      return *this->ControlArrayValid;
    }
    VTKM_CONT void SetControlArrayValid(const LockType& lock, bool value)
    {
      this->CheckLock(lock);
      if (!IsControlArrayValid(lock) && value)
      {
        // If we are changing the valid flag from false to true, then refresh the pointer.
        // There may be array portals that already have a reference to the flag. Those portals
        // will stay in an invalid state whereas new portals will go to a valid state. To
        // handle both conditions, drop the old reference and create a new one.
        this->ControlArrayValid.reset(new bool);
      }
      *this->ControlArrayValid = value;
    }
    VTKM_CONT std::shared_ptr<bool> GetControlArrayValidPointer(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ControlArrayValid;
    }
    VTKM_CONT StorageBasicBase* GetControlArray(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ControlArray;
    }
    VTKM_CONT bool IsExecutionArrayValid(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionArrayValid;
    }
    VTKM_CONT void SetExecutionArrayValid(const LockType& lock, bool value)
    {
      this->CheckLock(lock);
      this->ExecutionArrayValid = value;
    }
    VTKM_CONT ExecutionArrayInterfaceBasicBase* GetExecutionInterface(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionInterface;
    }
    VTKM_CONT void SetExecutionInterface(const LockType& lock,
                                         ExecutionArrayInterfaceBasicBase* executionInterface) const
    {
      this->CheckLock(lock);
      if (this->ExecutionInterface != nullptr)
      {
        delete this->ExecutionInterface;
        this->ExecutionInterface = nullptr;
      }
      this->ExecutionInterface = executionInterface;
    }
    VTKM_CONT void*& GetExecutionArray(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionArray;
    }
    VTKM_CONT void*& GetExecutionArrayEnd(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionArrayEnd;
    }
    VTKM_CONT void*& GetExecutionArrayCapacity(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionArrayCapacity;
    }
    VTKM_CONT vtkm::cont::Token::ReferenceCount* GetReadCount(const LockType& lock) const
    {
      this->CheckLock(lock);
      return &this->ReadCount;
    }
    VTKM_CONT vtkm::cont::Token::ReferenceCount* GetWriteCount(const LockType& lock) const
    {
      this->CheckLock(lock);
      return &this->WriteCount;
    }

    VTKM_CONT TypelessExecutionArray MakeTypelessExecutionArray(const LockType& lock);
  };

  std::shared_ptr<InternalStruct> Internals;
};

} // end namespace internal

/// Specialization of ArrayHandle for Basic storage. The goal here is to reduce
/// the amount of codegen for the common case of Basic storage when we build
/// the common arrays into libvtkm_cont.
template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandle<T, ::vtkm::cont::StorageTagBasic>
  : public ::vtkm::cont::internal::ArrayHandleBase
{
private:
  using Thisclass = ArrayHandle<T, ::vtkm::cont::StorageTagBasic>;

  template <typename DeviceTag>
  using PortalFactory = vtkm::cont::internal::ExecutionPortalFactoryBasic<T, DeviceTag>;

  using MutexType = internal::ArrayHandleImpl::MutexType;
  using LockType = internal::ArrayHandleImpl::LockType;

public:
  using StorageTag = ::vtkm::cont::StorageTagBasic;
  using StorageType = vtkm::cont::internal::Storage<T, StorageTag>;
  using ValueType = T;
  using WritePortalType = vtkm::cont::internal::ArrayPortalCheck<typename StorageType::PortalType>;
  using ReadPortalType =
    vtkm::cont::internal::ArrayPortalCheck<typename StorageType::PortalConstType>;

  using PortalControl VTKM_DEPRECATED(1.6, "Use ArrayHandle::WritePortalType instead.") =
    typename StorageType::PortalType;
  using PortalConstControl VTKM_DEPRECATED(1.6, "Use ArrayHandle::ReadPortalType instead.") =
    typename StorageType::PortalConstType;

  template <typename DeviceTag>
  struct ExecutionTypes
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceTag);
    using Portal = typename PortalFactory<DeviceTag>::PortalType;
    using PortalConst = typename PortalFactory<DeviceTag>::PortalConstType;
  };

  VTKM_CONT ArrayHandle();
  VTKM_CONT ArrayHandle(const Thisclass& src);
  VTKM_CONT ArrayHandle(Thisclass&& src) noexcept;

  VTKM_CONT ArrayHandle(const StorageType& storage) noexcept;
  VTKM_CONT ArrayHandle(StorageType&& storage) noexcept;

  VTKM_CONT ~ArrayHandle();

  VTKM_CONT Thisclass& operator=(const Thisclass& src);
  VTKM_CONT Thisclass& operator=(Thisclass&& src) noexcept;

  VTKM_CONT bool operator==(const Thisclass& rhs) const;
  VTKM_CONT bool operator!=(const Thisclass& rhs) const;

  template <typename VT, typename ST>
  VTKM_CONT bool operator==(const ArrayHandle<VT, ST>&) const;
  template <typename VT, typename ST>
  VTKM_CONT bool operator!=(const ArrayHandle<VT, ST>&) const;

  VTKM_CONT StorageType& GetStorage();
  VTKM_CONT const StorageType& GetStorage() const;
  VTKM_CONT vtkm::Id GetNumberOfValues() const;

  VTKM_CONT
  VTKM_DEPRECATED(1.6,
                  "Use ArrayHandle::WritePortal() instead. "
                  "Note that the returned portal will lock the array while it is in scope.")
  /// @cond NONE
  typename StorageType::PortalType GetPortalControl();
  /// @endcond
  VTKM_CONT
  VTKM_DEPRECATED(1.6,
                  "Use ArrayHandle::ReadPortal() instead. "
                  "Note that the returned portal will lock the array while it is in scope.")
  /// @cond NONE
  typename StorageType::PortalConstType GetPortalConstControl() const;
  /// @endcond

  VTKM_CONT ReadPortalType ReadPortal() const;
  VTKM_CONT WritePortalType WritePortal() const;

  VTKM_CONT void Allocate(vtkm::Id numberOfValues);
  VTKM_CONT void Shrink(vtkm::Id numberOfValues);
  VTKM_CONT void ReleaseResourcesExecution();
  VTKM_CONT void ReleaseResources();

  template <typename DeviceAdapterTag>
  VTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::PortalConst PrepareForInput(
    DeviceAdapterTag device,
    vtkm::cont::Token& token) const;

  template <typename DeviceAdapterTag>
  VTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::Portal
  PrepareForOutput(vtkm::Id numVals, DeviceAdapterTag device, vtkm::cont::Token& token);

  template <typename DeviceAdapterTag>
  VTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::Portal PrepareForInPlace(
    DeviceAdapterTag device,
    vtkm::cont::Token& token);

  template <typename DeviceAdapterTag>
  VTKM_CONT VTKM_DEPRECATED(1.6, "PrepareForInput now requires a vtkm::cont::Token object.")
    typename ExecutionTypes<DeviceAdapterTag>::PortalConst
    PrepareForInput(DeviceAdapterTag device) const
  {
    vtkm::cont::Token token;
    return this->PrepareForInput(device, token);
  }
  template <typename DeviceAdapterTag>
  VTKM_CONT VTKM_DEPRECATED(1.6, "PrepareForOutput now requires a vtkm::cont::Token object.")
    typename ExecutionTypes<DeviceAdapterTag>::Portal
    PrepareForOutput(vtkm::Id numVals, DeviceAdapterTag device)
  {
    vtkm::cont::Token token;
    return this->PrepareForOutput(numVals, device, token);
  }
  template <typename DeviceAdapterTag>
  VTKM_CONT VTKM_DEPRECATED(1.6, "PrepareForInPlace now requires a vtkm::cont::Token object.")
    typename ExecutionTypes<DeviceAdapterTag>::Portal PrepareForInPlace(DeviceAdapterTag device)
  {
    vtkm::cont::Token token;
    return this->PrepareForInPlace(device, token);
  }

  template <typename DeviceAdapterTag>
  VTKM_CONT void PrepareForDevice(LockType& lock, DeviceAdapterTag) const;

  VTKM_CONT DeviceAdapterId GetDeviceAdapterId() const;

  VTKM_CONT void SyncControlArray() const;

  std::shared_ptr<internal::ArrayHandleImpl> Internals;

private:
  VTKM_CONT void SyncControlArray(LockType& lock) const;
  VTKM_CONT void ReleaseResourcesExecutionInternal(LockType& lock) const;

  /// Acquires a lock on the internals of this `ArrayHandle`. The calling
  /// function should keep the returned lock and let it go out of scope
  /// when the lock is no longer needed.
  ///
  LockType GetLock() const { return this->Internals->GetLock(); }
};

} // end namespace cont
} // end namespace vtkm

#ifndef vtkm_cont_internal_ArrayHandleImpl_cxx
#ifdef VTKM_MSVC
extern template class VTKM_CONT_TEMPLATE_EXPORT
  std::shared_ptr<vtkm::cont::internal::ArrayHandleImpl>;
#endif
#endif

#ifndef vtk_m_cont_internal_ArrayHandleBasicImpl_hxx
#include <vtkm/cont/internal/ArrayHandleBasicImpl.hxx>
#endif

#endif // vtk_m_cont_internal_ArrayHandleBasicImpl_h
