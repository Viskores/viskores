//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ArrayHandleDeprecated_h
#define vtk_m_cont_internal_ArrayHandleDeprecated_h

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/internal/ArrayHandleExecutionManager.h>
#include <vtkm/cont/internal/StorageDeprecated.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// The `ArrayHandle` implementation was recently significantly changed.
/// Implementations still using the old style should use this version.
/// These implementations should be deprecated.
///
template <typename T, typename StorageTag_>
class VTKM_ALWAYS_EXPORT ArrayHandleDeprecated : public internal::ArrayHandleBase
{
private:
  // Basic storage is specialized; this template should not be instantiated
  // for it. Specialization is in ArrayHandleBasicImpl.h
  static_assert(!std::is_same<StorageTag_, StorageTagBasic>::value,
                "StorageTagBasic should not use this implementation.");

  using ExecutionManagerType =
    vtkm::cont::internal::ArrayHandleExecutionManagerBase<T, StorageTag_>;

  using MutexType = std::mutex;
  using LockType = std::unique_lock<MutexType>;

  mutable vtkm::cont::internal::Buffer BufferAsStorageWrapper;

  struct PrepareForInputFunctor;
  struct PrepareForOutputFunctor;
  struct PrepareForInPlaceFunctor;

public:
  using StorageType = vtkm::cont::internal::Storage<T, StorageTag_>;
  using ValueType = T;
  using StorageTag = StorageTag_;
  using WritePortalType = typename StorageType::PortalType;
  using ReadPortalType = typename StorageType::PortalConstType;
  template <typename DeviceAdapterTag>
  struct ExecutionTypes
  {
    using Portal = typename ExecutionManagerType::template ExecutionTypes<DeviceAdapterTag>::Portal;
    using PortalConst =
      typename ExecutionManagerType::template ExecutionTypes<DeviceAdapterTag>::PortalConst;
  };

  using PortalControl VTKM_DEPRECATED(1.6, "Use ArrayHandle::WritePortalType instead.") =
    typename StorageType::PortalType;
  using PortalConstControl VTKM_DEPRECATED(1.6, "Use ArrayHandle::ReadPortalType instead.") =
    typename StorageType::PortalConstType;

  // Handle the fact that the ArrayHandle design has changed.
  VTKM_STATIC_ASSERT_MSG((std::is_same<typename StorageType::HasOldBridge, std::true_type>::value),
                         "ArrayHandle design has changed. To support old-style arrays, have the "
                         "Storage implementation declare VTKM_STORAGE_OLD_STYLE at the bottom "
                         "of its implementation.");
  VTKM_CONT static constexpr vtkm::IdComponent GetNumberOfBuffers() { return 1; }
  VTKM_CONT vtkm::cont::internal::Buffer* GetBuffers() const
  {
    this->BufferAsStorageWrapper.SetMetaData(*this);
    return &this->BufferAsStorageWrapper;
  }

  VTKM_CONT ArrayHandleDeprecated(const vtkm::cont::internal::Buffer* buffers)
  {
    VTKM_ASSERT(buffers[0].MetaDataIsType<ArrayHandleDeprecated>());
    *this = buffers[0].GetMetaData<ArrayHandleDeprecated>();
  }

  /// Constructs an empty ArrayHandleDeprecated. Typically used for output or
  /// intermediate arrays that will be filled by a VTKm algorithm.
  ///
  VTKM_CONT ArrayHandleDeprecated();

  /// Copy constructor.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated copy constructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ArrayHandleDeprecated(const ArrayHandleDeprecated<ValueType, StorageTag>& src);

  /// Move constructor.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated move constructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ArrayHandleDeprecated(ArrayHandleDeprecated<ValueType, StorageTag>&& src) noexcept;

  /// Special constructor for subclass specializations that need to set the
  /// initial state of the control array. When this constructor is used, it
  /// is assumed that the control array is valid.
  ///
  ArrayHandleDeprecated(const StorageType& storage);


  /// Special constructor for subclass specializations that need to set the
  /// initial state of the control array. When this constructor is used, it
  /// is assumed that the control array is valid.
  ///
  ArrayHandleDeprecated(StorageType&& storage) noexcept;

  /// Destructs an empty ArrayHandleDeprecated.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleDeprecated();

  /// \brief Copies an ArrayHandleDeprecated
  ///
  VTKM_CONT
  ArrayHandleDeprecated<ValueType, StorageTag>& operator=(
    const ArrayHandleDeprecated<ValueType, StorageTag>& src);

  /// \brief Move and Assignment of an ArrayHandleDeprecated
  ///
  VTKM_CONT
  ArrayHandleDeprecated<ValueType, StorageTag>& operator=(
    ArrayHandleDeprecated<ValueType, StorageTag>&& src) noexcept;

  /// Like a pointer, two \c ArrayHandles are considered equal if they point
  /// to the same location in memory.
  ///
  VTKM_CONT
  bool operator==(const ArrayHandleDeprecated<ValueType, StorageTag>& rhs) const
  {
    return (this->Internals == rhs.Internals);
  }

  VTKM_CONT
  bool operator!=(const ArrayHandleDeprecated<ValueType, StorageTag>& rhs) const
  {
    return (this->Internals != rhs.Internals);
  }

  VTKM_CONT bool operator==(const vtkm::cont::ArrayHandle<ValueType, StorageTag>& rhs) const
  {
    return *this == static_cast<ArrayHandleDeprecated<ValueType, StorageTag>>(rhs);
  }

  VTKM_CONT bool operator!=(const vtkm::cont::ArrayHandle<ValueType, StorageTag>& rhs) const
  {
    return *this != static_cast<ArrayHandleDeprecated<ValueType, StorageTag>>(rhs);
  }

  template <typename VT, typename ST>
  VTKM_CONT bool operator==(const ArrayHandleDeprecated<VT, ST>&) const
  {
    return false; // different valuetype and/or storage
  }

  template <typename VT, typename ST>
  VTKM_CONT bool operator!=(const ArrayHandleDeprecated<VT, ST>&) const
  {
    return true; // different valuetype and/or storage
  }

  template <typename VT, typename ST>
  VTKM_CONT bool operator==(const vtkm::cont::ArrayHandle<VT, ST>&) const
  {
    return false; // different storage
  }

  template <typename VT, typename ST>
  VTKM_CONT bool operator!=(const vtkm::cont::ArrayHandle<VT, ST>&) const
  {
    return false; // different storage
  }

  /// Get the storage.
  ///
  VTKM_CONT StorageType& GetStorage();

  /// Get the storage.
  ///
  VTKM_CONT const StorageType& GetStorage() const;

  /// Get the array portal of the control array.
  /// Since worklet invocations are asynchronous and this routine is a synchronization point,
  /// exceptions maybe thrown for errors from previously executed worklets.
  ///
  /// \deprecated Use `WritePortal` instead. Note that the portal returned from `WritePortal`
  /// will disallow any other reads or writes to the array while it is in scope.
  ///
  VTKM_CONT
  VTKM_DEPRECATED(1.6,
                  "Use ArrayHandle::WritePortal() instead. "
                  "Note that the returned portal will lock the array while it is in scope.")

  /// \cond NOPE
  typename StorageType::PortalType GetPortalControl();
  /// \endcond

  /// Get the array portal of the control array.
  /// Since worklet invocations are asynchronous and this routine is a synchronization point,
  /// exceptions maybe thrown for errors from previously executed worklets.
  ///
  /// \deprecated Use `ReadPortal` instead. Note that the portal returned from `ReadPortal`
  /// will disallow any writes to the array while it is in scope.
  ///
  VTKM_CONT
  VTKM_DEPRECATED(1.6,
                  "Use ArrayHandle::ReadPortal() instead. "
                  "Note that the returned portal will lock the array while it is in scope.")
  /// \cond NOPE
  typename StorageType::PortalConstType GetPortalConstControl() const;
  /// \endcond

  /// \@{
  /// \brief Get an array portal that can be used in the control environment.
  ///
  /// The returned array can be used in the control environment to read values from the array. (It
  /// is not possible to write to the returned portal. That is `Get` will work on the portal, but
  /// `Set` will not.)
  ///
  /// **Note:** The returned portal cannot be used in the execution environment. This is because
  /// the portal will not work on some devices like GPUs. To get a portal that will work in the
  /// execution environment, use `PrepareForInput`.
  ///
  VTKM_CONT ReadPortalType ReadPortal() const;
  /// \@}

  /// \@{
  /// \brief Get an array portal that can be used in the control environment.
  ///
  /// The returned array can be used in the control environment to reand and write values to the
  /// array.
  ///
  ///
  /// **Note:** The returned portal cannot be used in the execution environment. This is because
  /// the portal will not work on some devices like GPUs. To get a portal that will work in the
  /// execution environment, use `PrepareForInput`.
  ///
  VTKM_CONT WritePortalType WritePortal() const;
  /// \@}

  /// Returns the number of entries in the array.
  ///
  VTKM_CONT vtkm::Id GetNumberOfValues() const
  {
    LockType lock = this->GetLock();

    return this->GetNumberOfValues(lock);
  }

  /// \brief Allocates an array large enough to hold the given number of values.
  ///
  /// The allocation may be done on an already existing array, but can wipe out
  /// any data already in the array. This method can throw
  /// ErrorBadAllocation if the array cannot be allocated or
  /// ErrorBadValue if the allocation is not feasible (for example, the
  /// array storage is read-only).
  ///
  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues)
  {
    vtkm::cont::Token token;
    this->Allocate(numberOfValues, token);
  }
  VTKM_CONT void Allocate(vtkm::Id numberOfValues, vtkm::cont::Token& token)
  {
    LockType lock = this->GetLock();
    this->WaitToWrite(lock, token);
    this->ReleaseResourcesExecutionInternal(lock, token);
    this->Internals->GetControlArray(lock)->Allocate(numberOfValues);
    // Set to false and then to true to ensure anything pointing to an array before the allocate
    // is invalidated.
    this->Internals->SetControlArrayValid(lock, false);
    this->Internals->SetControlArrayValid(lock, true);
  }

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  VTKM_CONT void Shrink(vtkm::Id numberOfValues)
  {
    vtkm::cont::Token token;
    this->Shrink(numberOfValues, token);
  }
  VTKM_CONT void Shrink(vtkm::Id numberOfValues, vtkm::cont::Token& token);

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  VTKM_CONT void ReleaseResourcesExecution()
  {
    // A Token should not be declared within the scope of a lock. when the token goes out of scope
    // it will attempt to aquire the lock, which is undefined behavior of the thread already has
    // the lock.
    vtkm::cont::Token token;
    {
      LockType lock = this->GetLock();
      this->WaitToWrite(lock, token);

      // Save any data in the execution environment by making sure it is synced
      // with the control environment.
      this->SyncControlArray(lock, token);

      this->ReleaseResourcesExecutionInternal(lock, token);
    }
  }

  /// Releases all resources in both the control and execution environments.
  ///
  VTKM_CONT void ReleaseResources()
  {
    // A Token should not be declared within the scope of a lock. when the token goes out of scope
    // it will attempt to aquire the lock, which is undefined behavior of the thread already has
    // the lock.
    vtkm::cont::Token token;
    {
      LockType lock = this->GetLock();

      this->ReleaseResourcesExecutionInternal(lock, token);

      if (this->Internals->IsControlArrayValid(lock))
      {
        this->Internals->GetControlArray(lock)->ReleaseResources();
        this->Internals->SetControlArrayValid(lock, false);
      }
    }
  }

  /// Prepares this array to be used as an input to an operation in the
  /// execution environment. If necessary, copies data to the execution
  /// environment. Can throw an exception if this array does not yet contain
  /// any data. Returns a portal that can be used in code running in the
  /// execution environment.
  ///
  /// The `Token` object provided will be attached to this `ArrayHandle`.
  /// The returned portal is guaranteed to be valid while the `Token` is
  /// still attached and in scope. Other operations on this `ArrayHandle`
  /// that would invalidate the returned portal will block until the `Token`
  /// is released. Likewise, this method will block if another `Token` is
  /// already attached. This can potentially lead to deadlocks.
  ///
  template <typename DeviceAdapterTag>
  VTKM_CONT ReadPortalType PrepareForInput(DeviceAdapterTag, vtkm::cont::Token& token) const;
  VTKM_CONT ReadPortalType PrepareForInput(vtkm::cont::DeviceAdapterId device,
                                           vtkm::cont::Token& token) const;

  /// Prepares (allocates) this array to be used as an output from an operation
  /// in the execution environment. The internal state of this class is set to
  /// have valid data in the execution array with the assumption that the array
  /// will be filled soon (i.e. before any other methods of this object are
  /// called). Returns a portal that can be used in code running in the
  /// execution environment.
  ///
  /// The `Token` object provided will be attached to this `ArrayHandle`.
  /// The returned portal is guaranteed to be valid while the `Token` is
  /// still attached and in scope. Other operations on this `ArrayHandle`
  /// that would invalidate the returned portal will block until the `Token`
  /// is released. Likewise, this method will block if another `Token` is
  /// already attached. This can potentially lead to deadlocks.
  ///
  template <typename DeviceAdapterTag>
  VTKM_CONT WritePortalType PrepareForOutput(vtkm::Id numberOfValues,
                                             DeviceAdapterTag,
                                             vtkm::cont::Token& token);
  VTKM_CONT WritePortalType PrepareForOutput(vtkm::Id numberOfValues,
                                             vtkm::cont::DeviceAdapterId device,
                                             vtkm::cont::Token& token);

  /// Prepares this array to be used in an in-place operation (both as input
  /// and output) in the execution environment. If necessary, copies data to
  /// the execution environment. Can throw an exception if this array does not
  /// yet contain any data. Returns a portal that can be used in code running
  /// in the execution environment.
  ///
  /// The `Token` object provided will be attached to this `ArrayHandle`.
  /// The returned portal is guaranteed to be valid while the `Token` is
  /// still attached and in scope. Other operations on this `ArrayHandle`
  /// that would invalidate the returned portal will block until the `Token`
  /// is released. Likewise, this method will block if another `Token` is
  /// already attached. This can potentially lead to deadlocks.
  ///
  template <typename DeviceAdapterTag>
  VTKM_CONT WritePortalType PrepareForInPlace(DeviceAdapterTag, vtkm::cont::Token& token);
  VTKM_CONT WritePortalType PrepareForInPlace(vtkm::cont::DeviceAdapterId device,
                                              vtkm::cont::Token& token);

  template <typename DeviceAdapterTag>
  VTKM_CONT VTKM_DEPRECATED(1.6, "PrepareForInput now requires a vtkm::cont::Token object.")
    typename ExecutionTypes<DeviceAdapterTag>::PortalConst PrepareForInput(DeviceAdapterTag) const
  {
    vtkm::cont::Token token;
    return this->PrepareForInput(DeviceAdapterTag{}, token);
  }
  template <typename DeviceAdapterTag>
  VTKM_CONT VTKM_DEPRECATED(1.6, "PrepareForOutput now requires a vtkm::cont::Token object.")
    typename ExecutionTypes<DeviceAdapterTag>::Portal
    PrepareForOutput(vtkm::Id numberOfValues, DeviceAdapterTag)
  {
    vtkm::cont::Token token;
    return this->PrepareForOutput(numberOfValues, DeviceAdapterTag{}, token);
  }
  template <typename DeviceAdapterTag>
  VTKM_CONT VTKM_DEPRECATED(1.6, "PrepareForInPlace now requires a vtkm::cont::Token object.")
    typename ExecutionTypes<DeviceAdapterTag>::Portal PrepareForInPlace(DeviceAdapterTag)
  {
    vtkm::cont::Token token;
    return this->PrepareForInPlace(DeviceAdapterTag{}, token);
  }

  /// Returns the DeviceAdapterId for the current device. If there is no device
  /// with an up-to-date copy of the data, VTKM_DEVICE_ADAPTER_UNDEFINED is
  /// returned.
  ///
  /// Note that in a multithreaded environment the validity of this result can
  /// change.
  VTKM_CONT
  DeviceAdapterId GetDeviceAdapterId() const
  {
    LockType lock = this->GetLock();
    return this->Internals->IsExecutionArrayValid(lock)
      ? this->Internals->GetExecutionArray(lock)->GetDeviceAdapterId()
      : DeviceAdapterTagUndefined{};
  }

  /// Synchronizes the control array with the execution array. If either the
  /// user array or control array is already valid, this method does nothing
  /// (because the data is already available in the control environment).
  /// Although the internal state of this class can change, the method is
  /// declared const because logically the data does not.
  ///
  VTKM_CONT void SyncControlArray() const
  {
    // A Token should not be declared within the scope of a lock. when the token goes out of scope
    // it will attempt to aquire the lock, which is undefined behavior of the thread already has
    // the lock.
    vtkm::cont::Token token;
    {
      LockType lock = this->GetLock();
      this->SyncControlArray(lock, token);
    }
  }

  /// \brief Enqueue a token for access to this ArrayHandle.
  ///
  /// This method places the given `Token` into the queue of `Token`s waiting for
  /// access to this `ArrayHandle` and then returns immediately. When this token
  /// is later used to get data from this `ArrayHandle` (for example, in a call to
  /// `PrepareForInput`), it will use this place in the queue while waiting for
  /// access.
  ///
  /// This method is to be used to ensure that a set of accesses to an `ArrayHandle`
  /// that happen on multiple threads occur in a specified order. For example, if
  /// you spawn of a job to modify data in an `ArrayHandle` and then spawn off a job
  /// that reads that same data, you need to make sure that the first job gets
  /// access to the `ArrayHandle` before the second. If they both just attempt to call
  /// their respective `Prepare` methods, there is no guarantee which order they
  /// will occur. Having the spawning thread first call this method will ensure the order.
  ///
  /// \warning After calling this method it is required to subsequently
  /// call a method like one of the `Prepare` methods that attaches the token
  /// to this `ArrayHandle`. Otherwise, the enqueued token will block any subsequent
  /// access to the `ArrayHandle`, even if the `Token` is destroyed.
  ///
  VTKM_CONT void Enqueue(const vtkm::cont::Token& token) const;

private:
  /// Acquires a lock on the internals of this `ArrayHandle`. The calling
  /// function should keep the returned lock and let it go out of scope
  /// when the lock is no longer needed.
  ///
  LockType GetLock() const { return LockType(this->Internals->Mutex); }

  /// Returns true if read operations can currently be performed.
  ///
  VTKM_CONT bool CanRead(const LockType& lock, const vtkm::cont::Token& token) const;

  //// Returns true if write operations can currently be performed.
  ///
  VTKM_CONT bool CanWrite(const LockType& lock, const vtkm::cont::Token& token) const;

  //// Will block the current thread until a read can be performed.
  ///
  VTKM_CONT void WaitToRead(LockType& lock, vtkm::cont::Token& token) const;

  //// Will block the current thread until a write can be performed.
  ///
  VTKM_CONT void WaitToWrite(LockType& lock, vtkm::cont::Token& token, bool fakeRead = false) const;

  /// Gets this array handle ready to interact with the given device. If the
  /// array handle has already interacted with this device, then this method
  /// does nothing. Although the internal state of this class can change, the
  /// method is declared const because logically the data does not.
  ///
  template <typename DeviceAdapterTag>
  VTKM_CONT void PrepareForDevice(LockType& lock, vtkm::cont::Token& token, DeviceAdapterTag) const;

  /// Synchronizes the control array with the execution array. If either the
  /// user array or control array is already valid, this method does nothing
  /// (because the data is already available in the control environment).
  /// Although the internal state of this class can change, the method is
  /// declared const because logically the data does not.
  ///
  VTKM_CONT void SyncControlArray(LockType& lock, vtkm::cont::Token& token) const;

  vtkm::Id GetNumberOfValues(LockType& lock) const;

  VTKM_CONT
  void ReleaseResourcesExecutionInternal(LockType& lock, vtkm::cont::Token& token) const
  {
    if (this->Internals->IsExecutionArrayValid(lock))
    {
      this->WaitToWrite(lock, token);
      // Note that it is possible that while waiting someone else deleted the execution array.
      // That is why we check again.
    }
    if (this->Internals->IsExecutionArrayValid(lock))
    {
      this->Internals->GetExecutionArray(lock)->ReleaseResources();
      this->Internals->SetExecutionArrayValid(lock, false);
    }
  }

  VTKM_CONT void Enqueue(const LockType& lock, const vtkm::cont::Token& token) const;

  class VTKM_ALWAYS_EXPORT InternalStruct
  {
    mutable StorageType ControlArray;
    mutable std::shared_ptr<bool> ControlArrayValid;

    mutable std::unique_ptr<ExecutionManagerType> ExecutionArray;
    mutable bool ExecutionArrayValid = false;

    mutable vtkm::cont::Token::ReferenceCount ReadCount = 0;
    mutable vtkm::cont::Token::ReferenceCount WriteCount = 0;

    mutable std::deque<vtkm::cont::Token::Reference> Queue;

    VTKM_CONT void CheckLock(const LockType& lock) const
    {
      VTKM_ASSERT((lock.mutex() == &this->Mutex) && (lock.owns_lock()));
    }

  public:
    MutexType Mutex;
    std::condition_variable ConditionVariable;

    InternalStruct() = default;
    InternalStruct(const StorageType& storage);
    InternalStruct(StorageType&& storage);

    ~InternalStruct()
    {
      // It should not be possible to destroy this array if any tokens are still attached to it.
      LockType lock(this->Mutex);
      VTKM_ASSERT((*this->GetReadCount(lock) == 0) && (*this->GetWriteCount(lock) == 0));
      this->SetControlArrayValid(lock, false);
    }

    // To access any feature in InternalStruct, you must have locked the mutex. You have
    // to prove it by passing in a reference to a std::unique_lock.
    VTKM_CONT bool IsControlArrayValid(const LockType& lock) const
    {
      this->CheckLock(lock);
      if (!this->ControlArrayValid)
      {
        return false;
      }
      else
      {
        return *this->ControlArrayValid;
      }
    }
    VTKM_CONT void SetControlArrayValid(const LockType& lock, bool value)
    {
      this->CheckLock(lock);
      if (IsControlArrayValid(lock) == value)
      {
        return;
      }
      if (value) // ControlArrayValid == false or nullptr
      {
        // If we are changing the valid flag from false to true, then refresh the pointer.
        // There may be array portals that already have a reference to the flag. Those portals
        // will stay in an invalid state whereas new portals will go to a valid state. To
        // handle both conditions, drop the old reference and create a new one.
        this->ControlArrayValid.reset(new bool(true));
      }
      else // value == false and ControlArrayValid == true
      {
        *this->ControlArrayValid = false;
      }
    }
    VTKM_CONT std::shared_ptr<bool> GetControlArrayValidPointer(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ControlArrayValid;
    }
    VTKM_CONT StorageType* GetControlArray(const LockType& lock) const
    {
      this->CheckLock(lock);
      return &this->ControlArray;
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
    VTKM_CONT ExecutionManagerType* GetExecutionArray(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionArray.get();
    }
    VTKM_CONT void DeleteExecutionArray(const LockType& lock)
    {
      this->CheckLock(lock);
      this->ExecutionArray.reset();
      this->ExecutionArrayValid = false;
    }
    template <typename DeviceAdapterTag>
    VTKM_CONT void NewExecutionArray(const LockType& lock, DeviceAdapterTag)
    {
      VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);
      this->CheckLock(lock);
      VTKM_ASSERT(this->ExecutionArray == nullptr);
      VTKM_ASSERT(!this->ExecutionArrayValid);
      this->ExecutionArray.reset(
        new vtkm::cont::internal::ArrayHandleExecutionManager<T, StorageTag, DeviceAdapterTag>(
          &this->ControlArray));
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
    VTKM_CONT std::deque<vtkm::cont::Token::Reference>& GetQueue(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->Queue;
    }
  };

  VTKM_CONT
  ArrayHandleDeprecated(const std::shared_ptr<InternalStruct>& i)
    : Internals(i)
  {
  }

  std::shared_ptr<InternalStruct> Internals;
};

template <typename T, typename S>
ArrayHandleDeprecated<T, S>::InternalStruct::InternalStruct(
  const typename ArrayHandleDeprecated<T, S>::StorageType& storage)
  : ControlArray(storage)
  , ControlArrayValid(new bool(true))
  , ExecutionArrayValid(false)
{
}

template <typename T, typename S>
ArrayHandleDeprecated<T, S>::InternalStruct::InternalStruct(
  typename ArrayHandleDeprecated<T, S>::StorageType&& storage)
  : ControlArray(std::move(storage))
  , ControlArrayValid(new bool(true))
  , ExecutionArrayValid(false)
{
}

template <typename T, typename S>
ArrayHandleDeprecated<T, S>::ArrayHandleDeprecated()
  : Internals(std::make_shared<InternalStruct>())
{
}

template <typename T, typename S>
ArrayHandleDeprecated<T, S>::ArrayHandleDeprecated(const ArrayHandleDeprecated<T, S>& src)
  : Internals(src.Internals)
{
}

template <typename T, typename S>
ArrayHandleDeprecated<T, S>::ArrayHandleDeprecated(ArrayHandleDeprecated<T, S>&& src) noexcept
  : Internals(std::move(src.Internals))
{
}

template <typename T, typename S>
ArrayHandleDeprecated<T, S>::ArrayHandleDeprecated(
  const typename ArrayHandleDeprecated<T, S>::StorageType& storage)
  : Internals(std::make_shared<InternalStruct>(storage))
{
}

template <typename T, typename S>
ArrayHandleDeprecated<T, S>::ArrayHandleDeprecated(
  typename ArrayHandleDeprecated<T, S>::StorageType&& storage) noexcept
  : Internals(std::make_shared<InternalStruct>(std::move(storage)))
{
}

template <typename T, typename S>
ArrayHandleDeprecated<T, S>::~ArrayHandleDeprecated()
{
}

template <typename T, typename S>
ArrayHandleDeprecated<T, S>& ArrayHandleDeprecated<T, S>::operator=(
  const ArrayHandleDeprecated<T, S>& src)
{
  this->Internals = src.Internals;
  return *this;
}

template <typename T, typename S>
ArrayHandleDeprecated<T, S>& ArrayHandleDeprecated<T, S>::operator=(
  ArrayHandleDeprecated<T, S>&& src) noexcept
{
  this->Internals = std::move(src.Internals);
  return *this;
}

template <typename T, typename S>
typename ArrayHandleDeprecated<T, S>::StorageType& ArrayHandleDeprecated<T, S>::GetStorage()
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();

    this->SyncControlArray(lock, token);
    if (this->Internals->IsControlArrayValid(lock))
    {
      return *this->Internals->GetControlArray(lock);
    }
    else
    {
      throw vtkm::cont::ErrorInternal(
        "ArrayHandle::SyncControlArray did not make control array valid.");
    }
  }
}

template <typename T, typename S>
const typename ArrayHandleDeprecated<T, S>::StorageType& ArrayHandleDeprecated<T, S>::GetStorage()
  const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();

    this->SyncControlArray(lock, token);
    if (this->Internals->IsControlArrayValid(lock))
    {
      return *this->Internals->GetControlArray(lock);
    }
    else
    {
      throw vtkm::cont::ErrorInternal(
        "ArrayHandle::SyncControlArray did not make control array valid.");
    }
  }
}

template <typename T, typename S>
typename ArrayHandleDeprecated<T, S>::StorageType::PortalType
ArrayHandleDeprecated<T, S>::GetPortalControl()
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();

    this->SyncControlArray(lock, token);
    if (this->Internals->IsControlArrayValid(lock))
    {
      // If the user writes into the iterator we return, then the execution
      // array will become invalid. Play it safe and release the execution
      // resources. (Use the const version to preserve the execution array.)
      this->ReleaseResourcesExecutionInternal(lock, token);
      return this->Internals->GetControlArray(lock)->GetPortal();
    }
    else
    {
      throw vtkm::cont::ErrorInternal(
        "ArrayHandle::SyncControlArray did not make control array valid.");
    }
  }
}

template <typename T, typename S>
typename ArrayHandleDeprecated<T, S>::StorageType::PortalConstType
ArrayHandleDeprecated<T, S>::GetPortalConstControl() const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();

    this->SyncControlArray(lock, token);
    if (this->Internals->IsControlArrayValid(lock))
    {
      return this->Internals->GetControlArray(lock)->GetPortalConst();
    }
    else
    {
      throw vtkm::cont::ErrorInternal(
        "ArrayHandle::SyncControlArray did not make control array valid.");
    }
  }
}

template <typename T, typename S>
typename ArrayHandleDeprecated<T, S>::ReadPortalType ArrayHandleDeprecated<T, S>::ReadPortal() const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();
    this->WaitToRead(lock, token);

    this->SyncControlArray(lock, token);
    if (this->Internals->IsControlArrayValid(lock))
    {
      return ReadPortalType(this->Internals->GetControlArray(lock)->GetPortalConst());
    }
    else
    {
      throw vtkm::cont::ErrorInternal(
        "ArrayHandle::SyncControlArray did not make control array valid.");
    }
  }
}

template <typename T, typename S>
typename ArrayHandleDeprecated<T, S>::WritePortalType ArrayHandleDeprecated<T, S>::WritePortal()
  const
{
  // A Token should not be declared within the scope of a lock. when the token goes out of scope
  // it will attempt to aquire the lock, which is undefined behavior of the thread already has
  // the lock.
  vtkm::cont::Token token;
  {
    LockType lock = this->GetLock();
    this->WaitToWrite(lock, token);

    this->SyncControlArray(lock, token);
    if (this->Internals->IsControlArrayValid(lock))
    {
      // If the user writes into the iterator we return, then the execution
      // array will become invalid. Play it safe and release the execution
      // resources. (Use the const version to preserve the execution array.)
      this->ReleaseResourcesExecutionInternal(lock, token);
      return WritePortalType(this->Internals->GetControlArray(lock)->GetPortal());
    }
    else
    {
      throw vtkm::cont::ErrorInternal(
        "ArrayHandle::SyncControlArray did not make control array valid.");
    }
  }
}

template <typename T, typename S>
vtkm::Id ArrayHandleDeprecated<T, S>::GetNumberOfValues(LockType& lock) const
{
  if (this->Internals->IsControlArrayValid(lock))
  {
    return this->Internals->GetControlArray(lock)->GetNumberOfValues();
  }
  else if (this->Internals->IsExecutionArrayValid(lock))
  {
    return this->Internals->GetExecutionArray(lock)->GetNumberOfValues();
  }
  else
  {
    return 0;
  }
}

template <typename T, typename S>
void ArrayHandleDeprecated<T, S>::Shrink(vtkm::Id numberOfValues, vtkm::cont::Token& token)
{
  VTKM_ASSERT(numberOfValues >= 0);

  if (numberOfValues > 0)
  {
    LockType lock = this->GetLock();

    vtkm::Id originalNumberOfValues = this->GetNumberOfValues(lock);

    if (numberOfValues < originalNumberOfValues)
    {
      this->WaitToWrite(lock, token);
      if (this->Internals->IsControlArrayValid(lock))
      {
        this->Internals->GetControlArray(lock)->Shrink(numberOfValues);
      }
      if (this->Internals->IsExecutionArrayValid(lock))
      {
        this->Internals->GetExecutionArray(lock)->Shrink(numberOfValues);
      }
    }
    else if (numberOfValues == originalNumberOfValues)
    {
      // Nothing to do.
    }
    else // numberOfValues > originalNumberOfValues
    {
      throw vtkm::cont::ErrorBadValue("ArrayHandle::Shrink cannot be used to grow array.");
    }

    VTKM_ASSERT(this->GetNumberOfValues(lock) == numberOfValues);
  }
  else // numberOfValues == 0
  {
    // If we are shrinking to 0, there is nothing to save and we might as well
    // free up memory. Plus, some storage classes expect that data will be
    // deallocated when the size goes to zero.
    this->Allocate(0, token);
  }
}

template <typename T, typename S>
template <typename DeviceAdapterTag>
typename ArrayHandleDeprecated<T, S>::ReadPortalType ArrayHandleDeprecated<T, S>::PrepareForInput(
  DeviceAdapterTag device,
  vtkm::cont::Token& token) const
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  LockType lock = this->GetLock();
  this->WaitToRead(lock, token);

  if (!this->Internals->IsControlArrayValid(lock) && !this->Internals->IsExecutionArrayValid(lock))
  {
    // Want to use an empty array.
    // Set up ArrayHandle state so this actually works.
    this->Internals->GetControlArray(lock)->Allocate(0);
    this->Internals->SetControlArrayValid(lock, true);
  }

  this->PrepareForDevice(lock, token, device);
  auto portal = this->Internals->GetExecutionArray(lock)->PrepareForInput(
    !this->Internals->IsExecutionArrayValid(lock), device, token);

  this->Internals->SetExecutionArrayValid(lock, true);

  return portal;
}

template <typename T, typename S>
struct ArrayHandleDeprecated<T, S>::PrepareForInputFunctor
{
  template <typename Device>
  bool operator()(Device device,
                  const ArrayHandleDeprecated<T, S>& self,
                  vtkm::cont::Token& token,
                  ReadPortalType& portal) const
  {
    portal = self.PrepareForInput(device, token);
    return true;
  }
};

template <typename T, typename S>
typename ArrayHandleDeprecated<T, S>::ReadPortalType ArrayHandleDeprecated<T, S>::PrepareForInput(
  vtkm::cont::DeviceAdapterId device,
  vtkm::cont::Token& token) const
{
  ReadPortalType portal;
  vtkm::cont::TryExecuteOnDevice(device, PrepareForInputFunctor{}, *this, token, portal);
  return portal;
}

template <typename T, typename S>
template <typename DeviceAdapterTag>
typename ArrayHandleDeprecated<T, S>::WritePortalType ArrayHandleDeprecated<T, S>::PrepareForOutput(
  vtkm::Id numberOfValues,
  DeviceAdapterTag device,
  vtkm::cont::Token& token)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  LockType lock = this->GetLock();
  this->WaitToWrite(lock, token);

  // Invalidate any control arrays.
  // Should the control array resource be released? Probably not a good
  // idea when shared with execution.
  this->Internals->SetControlArrayValid(lock, false);

  this->PrepareForDevice(lock, token, device);
  auto portal =
    this->Internals->GetExecutionArray(lock)->PrepareForOutput(numberOfValues, device, token);

  // We are assuming that the calling code will fill the array using the
  // iterators we are returning, so go ahead and mark the execution array as
  // having valid data. (A previous version of this class had a separate call
  // to mark the array as filled, but that was onerous to call at the the
  // right time and rather pointless since it is basically always the case
  // that the array is going to be filled before anything else. In this
  // implementation the only access to the array is through the iterators
  // returned from this method, so you would have to work to invalidate this
  // assumption anyway.)
  this->Internals->SetExecutionArrayValid(lock, true);

  return portal;
}

template <typename T, typename S>
struct ArrayHandleDeprecated<T, S>::PrepareForOutputFunctor
{
  template <typename Device>
  bool operator()(Device device,
                  ArrayHandleDeprecated<T, S>& self,
                  vtkm::Id numberOfValues,
                  vtkm::cont::Token& token,
                  WritePortalType& portal) const
  {
    portal = self.PrepareForOutput(numberOfValues, device, token);
    return true;
  }
};

template <typename T, typename S>
typename ArrayHandleDeprecated<T, S>::WritePortalType ArrayHandleDeprecated<T, S>::PrepareForOutput(
  vtkm::Id numberOfValues,
  vtkm::cont::DeviceAdapterId device,
  vtkm::cont::Token& token)
{
  WritePortalType portal;
  vtkm::cont::TryExecuteOnDevice(
    device, PrepareForOutputFunctor{}, *this, numberOfValues, token, portal);
  return portal;
}

template <typename T, typename S>
template <typename DeviceAdapterTag>
typename ArrayHandleDeprecated<T, S>::WritePortalType
ArrayHandleDeprecated<T, S>::PrepareForInPlace(DeviceAdapterTag device, vtkm::cont::Token& token)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  LockType lock = this->GetLock();
  this->WaitToWrite(lock, token);

  if (!this->Internals->IsControlArrayValid(lock) && !this->Internals->IsExecutionArrayValid(lock))
  {
    // Want to use an empty array.
    // Set up ArrayHandle state so this actually works.
    this->Internals->GetControlArray(lock)->Allocate(0);
    this->Internals->SetControlArrayValid(lock, true);
  }

  this->PrepareForDevice(lock, token, device);
  auto portal = this->Internals->GetExecutionArray(lock)->PrepareForInPlace(
    !this->Internals->IsExecutionArrayValid(lock), device, token);

  this->Internals->SetExecutionArrayValid(lock, true);

  // Invalidate any control arrays since their data will become invalid when
  // the execution data is overwritten. Don't actually release the control
  // array. It may be shared as the execution array.
  this->Internals->SetControlArrayValid(lock, false);

  return portal;
}

template <typename T, typename S>
struct ArrayHandleDeprecated<T, S>::PrepareForInPlaceFunctor
{
  template <typename Device>
  bool operator()(Device device,
                  ArrayHandleDeprecated<T, S>& self,
                  vtkm::cont::Token& token,
                  ReadPortalType& portal) const
  {
    portal = self.PrepareForInPlace(device, token);
    return true;
  }
};

template <typename T, typename S>
typename ArrayHandleDeprecated<T, S>::WritePortalType
ArrayHandleDeprecated<T, S>::PrepareForInPlace(vtkm::cont::DeviceAdapterId device,
                                               vtkm::cont::Token& token)
{
  WritePortalType portal;
  vtkm::cont::TryExecuteOnDevice(device, PrepareForInPlaceFunctor{}, *this, token, portal);
  return portal;
}

template <typename T, typename S>
template <typename DeviceAdapterTag>
void ArrayHandleDeprecated<T, S>::PrepareForDevice(LockType& lock,
                                                   vtkm::cont::Token& token,
                                                   DeviceAdapterTag device) const
{
  if (this->Internals->GetExecutionArray(lock) != nullptr)
  {
    if (this->Internals->GetExecutionArray(lock)->IsDeviceAdapter(DeviceAdapterTag()))
    {
      // Already have manager for correct device adapter. Nothing to do.
      return;
    }
    else
    {
      // Have the wrong manager. Delete the old one and create a new one
      // of the right type. (TODO: it would be possible for the array handle
      // to hold references to execution arrays on multiple devices. When data
      // are written on one devices, all the other devices should get cleared.)

      // BUG: There is a non-zero chance that while waiting for the write lock, another thread
      // could change the ExecutionInterface, which would cause problems. In the future we should
      // support multiple devices, in which case we would not have to delete one execution array
      // to load another.
      // BUG: The current implementation does not allow the ArrayHandle to be on two devices
      // at the same time. Thus, it is not possible for two simultaneously read from the same
      // ArrayHandle on two different devices. This might cause unexpected deadlocks.
      this->WaitToWrite(lock, token, true); // Make sure no one is reading device array
      this->SyncControlArray(lock, token);
      // Need to change some state that does not change the logical state from
      // an external point of view.
      this->Internals->DeleteExecutionArray(lock);
    }
  }

  // Need to change some state that does not change the logical state from
  // an external point of view.
  this->Internals->NewExecutionArray(lock, device);
}

template <typename T, typename S>
void ArrayHandleDeprecated<T, S>::SyncControlArray(LockType& lock, vtkm::cont::Token& token) const
{
  if (!this->Internals->IsControlArrayValid(lock))
  {
    // It may be the case that `SyncControlArray` is called from a method that has a `Token`.
    // However, if we are here, that `Token` should not already be attached to this array.
    // If it were, then there should be no reason to move data arround (unless the `Token`
    // was used when preparing for multiple devices, which it should not be used like that).
    this->WaitToRead(lock, token);

    // Need to change some state that does not change the logical state from
    // an external point of view.
    if (this->Internals->IsExecutionArrayValid(lock))
    {
      this->Internals->GetExecutionArray(lock)->RetrieveOutputData(
        this->Internals->GetControlArray(lock));
      this->Internals->SetControlArrayValid(lock, true);
    }
    else
    {
      // This array is in the null state (there is nothing allocated), but
      // the calling function wants to do something with the array. Put this
      // class into a valid state by allocating an array of size 0.
      this->Internals->GetControlArray(lock)->Allocate(0);
      this->Internals->SetControlArrayValid(lock, true);
    }
  }
}

template <typename T, typename S>
bool ArrayHandleDeprecated<T, S>::CanRead(const LockType& lock,
                                          const vtkm::cont::Token& token) const
{
  // If the token is already attached to this array, then we allow reading.
  if (token.IsAttached(this->Internals->GetWriteCount(lock)) ||
      token.IsAttached(this->Internals->GetReadCount(lock)))
  {
    return true;
  }

  // If there is anyone else waiting at the top of the queue, we cannot access this array.
  auto& queue = this->Internals->GetQueue(lock);
  if (!queue.empty() && (queue.front() != token))
  {
    return false;
  }

  // No one else is waiting, so we can read the array as long as no one else is writing.
  return (*this->Internals->GetWriteCount(lock) < 1);
}

template <typename T, typename S>
bool ArrayHandleDeprecated<T, S>::CanWrite(const LockType& lock,
                                           const vtkm::cont::Token& token) const
{
  // If the token is already attached to this array, then we allow writing.
  if (token.IsAttached(this->Internals->GetWriteCount(lock)) ||
      token.IsAttached(this->Internals->GetReadCount(lock)))
  {
    return true;
  }

  // If there is anyone else waiting at the top of the queue, we cannot access this array.
  auto& queue = this->Internals->GetQueue(lock);
  if (!queue.empty() && (queue.front() != token))
  {
    return false;
  }

  // No one else is waiting, so we can write the array as long as no one else is reading or writing.
  return ((*this->Internals->GetWriteCount(lock) < 1) &&
          (*this->Internals->GetReadCount(lock) < 1));
}

template <typename T, typename S>
void ArrayHandleDeprecated<T, S>::WaitToRead(LockType& lock, vtkm::cont::Token& token) const
{
  this->Enqueue(lock, token);

  // Note that if you deadlocked here, that means that you are trying to do a read operation on an
  // array where an object is writing to it.
  this->Internals->ConditionVariable.wait(
    lock, [&lock, &token, this] { return this->CanRead(lock, token); });

  token.Attach(this->Internals,
               this->Internals->GetReadCount(lock),
               lock,
               &this->Internals->ConditionVariable);

  // We successfully attached the token. Pop it off the queue.
  auto& queue = this->Internals->GetQueue(lock);
  if (!queue.empty() && queue.front() == token)
  {
    queue.pop_front();
  }
}

template <typename T, typename S>
void ArrayHandleDeprecated<T, S>::WaitToWrite(LockType& lock,
                                              vtkm::cont::Token& token,
                                              bool fakeRead) const
{
  this->Enqueue(lock, token);

  // Note that if you deadlocked here, that means that you are trying to do a write operation on an
  // array where an object is reading or writing to it.
  this->Internals->ConditionVariable.wait(
    lock, [&lock, &token, this] { return this->CanWrite(lock, token); });

  if (!fakeRead)
  {
    token.Attach(this->Internals,
                 this->Internals->GetWriteCount(lock),
                 lock,
                 &this->Internals->ConditionVariable);
  }
  else
  {
    // A current feature limitation of ArrayHandle is that it can only exist on one device at
    // a time. Thus, if a read request comes in for a different device, the prepare has to
    // get satisfy a write lock to boot the array off the existing device. However, we don't
    // want to attach the Token as a write lock because the resulting state is for reading only
    // and others might also want to read. So, we have to pretend that this is a read lock even
    // though we have to make a change to the array.
    //
    // The main point is, this condition is a hack that should go away once ArrayHandle supports
    // multiple devices at once.
    token.Attach(this->Internals,
                 this->Internals->GetReadCount(lock),
                 lock,
                 &this->Internals->ConditionVariable);
  }

  // We successfully attached the token. Pop it off the queue.
  auto& queue = this->Internals->GetQueue(lock);
  if (!queue.empty() && queue.front() == token)
  {
    queue.pop_front();
  }
}

template <typename T, typename S>
void ArrayHandleDeprecated<T, S>::Enqueue(const vtkm::cont::Token& token) const
{
  LockType lock = this->GetLock();
  this->Enqueue(lock, token);
}

template <typename T, typename S>
void ArrayHandleDeprecated<T, S>::Enqueue(const LockType& lock,
                                          const vtkm::cont::Token& token) const
{
  if (token.IsAttached(this->Internals->GetWriteCount(lock)) ||
      token.IsAttached(this->Internals->GetReadCount(lock)))
  {
    // Do not need to enqueue if we are already attached.
    return;
  }

  auto& queue = this->Internals->GetQueue(lock);
  if (std::find(queue.begin(), queue.end(), token.GetReference()) != queue.end())
  {
    // This token is already in the queue.
    return;
  }

  this->Internals->GetQueue(lock).push_back(token.GetReference());
}

// This macro is used to declare an ArrayHandle that uses the old, deprecated style of Storage
// that leverages ArrayTransfer. This macro will go away once all deprecated ArrayHandles
// that use it are replaced with the new style. To use this macro, first have a declaration
// of the template and then put the macro like this:
//
// template <typename T>
// VTKM_ARRAY_HANDLE_DEPRECATED(T, vtkm::cont::StorageTagFoo);
//
// Don't forget to use VTKM_PASS_COMMAS if one of the macro arguments contains
// a template with multiple parameters.
#define VTKM_ARRAY_HANDLE_DEPRECATED(ValueType_, StorageTag_)                            \
  class VTKM_ALWAYS_EXPORT ArrayHandle<ValueType_, StorageTag_>                          \
    : public internal::ArrayHandleDeprecated<ValueType_, StorageTag_>                    \
  {                                                                                      \
    using Superclass = internal::ArrayHandleDeprecated<ValueType_, StorageTag_>;         \
                                                                                         \
  public:                                                                                \
    VTKM_CONT                                                                            \
    ArrayHandle()                                                                        \
      : Superclass()                                                                     \
    {                                                                                    \
    }                                                                                    \
                                                                                         \
    VTKM_CONT                                                                            \
    ArrayHandle(const ArrayHandle<ValueType_, StorageTag_>& src)                         \
      : Superclass(src)                                                                  \
    {                                                                                    \
    }                                                                                    \
                                                                                         \
    VTKM_CONT                                                                            \
    ArrayHandle(ArrayHandle<ValueType_, StorageTag_>&& src) noexcept                     \
      : Superclass(std::move(src))                                                       \
    {                                                                                    \
    }                                                                                    \
                                                                                         \
    VTKM_CONT                                                                            \
    ArrayHandle(const internal::ArrayHandleDeprecated<ValueType_, StorageTag_>& src)     \
      : Superclass(src)                                                                  \
    {                                                                                    \
    }                                                                                    \
                                                                                         \
    VTKM_CONT                                                                            \
    ArrayHandle(internal::ArrayHandleDeprecated<ValueType_, StorageTag_>&& src) noexcept \
      : Superclass(std::move(src))                                                       \
    {                                                                                    \
    }                                                                                    \
                                                                                         \
    VTKM_CONT ArrayHandle(const internal::Storage<ValueType_, StorageTag_>& storage)     \
      : Superclass(storage)                                                              \
    {                                                                                    \
    }                                                                                    \
                                                                                         \
    VTKM_CONT ArrayHandle(internal::Storage<ValueType_, StorageTag_>&& storage)          \
      : Superclass(std::move(storage))                                                   \
    {                                                                                    \
    }                                                                                    \
                                                                                         \
    VTKM_CONT ArrayHandle(const vtkm::cont::internal::Buffer* buffers)                   \
      : Superclass(buffers)                                                              \
    {                                                                                    \
    }                                                                                    \
                                                                                         \
    VTKM_CONT ArrayHandle(const std::vector<vtkm::cont::internal::Buffer>& buffers)      \
      : Superclass(buffers.data())                                                       \
    {                                                                                    \
    }                                                                                    \
                                                                                         \
    VTKM_CONT                                                                            \
    ArrayHandle<ValueType_, StorageTag_>& operator=(                                     \
      const ArrayHandle<ValueType_, StorageTag_>& src)                                   \
    {                                                                                    \
      this->Superclass::operator=(src);                                                  \
      return *this;                                                                      \
    }                                                                                    \
                                                                                         \
    VTKM_CONT                                                                            \
    ArrayHandle<ValueType_, StorageTag_>& operator=(                                     \
      ArrayHandle<ValueType_, StorageTag_>&& src) noexcept                               \
    {                                                                                    \
      this->Superclass::operator=(std::move(src));                                       \
      return *this;                                                                      \
    }                                                                                    \
                                                                                         \
    VTKM_CONT ~ArrayHandle() {}                                                          \
  }

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ArrayHandleDeprecated_h
