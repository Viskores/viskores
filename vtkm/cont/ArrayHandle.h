//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandle_h
#define vtk_m_cont_ArrayHandle_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Assert.h>
#include <vtkm/Deprecated.h>
#include <vtkm/Flags.h>
#include <vtkm/Types.h>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/ErrorInternal.h>
#include <vtkm/cont/SerializableTypeString.h>
#include <vtkm/cont/Serialization.h>
#include <vtkm/cont/Storage.h>
#include <vtkm/cont/StorageBasic.h>
#include <vtkm/cont/Token.h>

#include <vtkm/internal/ArrayPortalHelpers.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <vector>

#include <vtkm/cont/internal/ArrayHandleExecutionManager.h>
#include <vtkm/cont/internal/ArrayPortalCheck.h>
#include <vtkm/cont/internal/ArrayPortalFromIterators.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

/// \brief Base class of all ArrayHandle classes.
///
/// This is an empty class that is used to check if something is an \c
/// ArrayHandle class (or at least something that behaves exactly like one).
/// The \c ArrayHandle template class inherits from this.
///
class VTKM_CONT_EXPORT ArrayHandleBase
{
};

/// Checks to see if the given type and storage forms a valid array handle
/// (some storage objects cannot support all types). This check is compatible
/// with C++11 type_traits.
///
template <typename T, typename StorageTag>
using IsValidArrayHandle =
  std::integral_constant<bool,
                         !(std::is_base_of<vtkm::cont::internal::UndefinedStorage,
                                           vtkm::cont::internal::Storage<T, StorageTag>>::value)>;

/// Checks to see if the given type and storage forms a invalid array handle
/// (some storage objects cannot support all types). This check is compatible
/// with C++11 type_traits.
///
template <typename T, typename StorageTag>
using IsInValidArrayHandle =
  std::integral_constant<bool, !IsValidArrayHandle<T, StorageTag>::value>;

/// Checks to see if the ArrayHandle allows writing, as some ArrayHandles
/// (Implicit) don't support writing. These will be defined as either
/// std::true_type or std::false_type.
///
/// \sa vtkm::internal::PortalSupportsSets
///
template <typename ArrayHandle>
using IsWritableArrayHandle =
  vtkm::internal::PortalSupportsSets<typename std::decay<ArrayHandle>::type::WritePortalType>;
/// @}

/// Checks to see if the given object is an array handle. This check is
/// compatible with C++11 type_traits. It a typedef named \c type that is
/// either std::true_type or std::false_type. Both of these have a typedef
/// named value with the respective boolean value.
///
/// Unlike \c IsValidArrayHandle, if an \c ArrayHandle is used with this
/// class, then it must be created by the compiler and therefore must already
/// be valid. Where \c IsValidArrayHandle is used when you know something is
/// an \c ArrayHandle but you are not sure if the \c StorageTag is valid, this
/// class is used to ensure that a given type is an \c ArrayHandle. It is
/// used internally in the VTKM_IS_ARRAY_HANDLE macro.
///
template <typename T>
struct ArrayHandleCheck
{
  using U = typename std::remove_pointer<T>::type;
  using type = typename std::is_base_of<::vtkm::cont::internal::ArrayHandleBase, U>::type;
};

#define VTKM_IS_ARRAY_HANDLE(T)                                                                    \
  VTKM_STATIC_ASSERT(::vtkm::cont::internal::ArrayHandleCheck<T>::type::value)

} // namespace internal

namespace detail
{

template <typename T>
struct GetTypeInParentheses;
template <typename T>
struct GetTypeInParentheses<void(T)>
{
  using type = T;
};

} // namespace detail

// Implementation for VTKM_ARRAY_HANDLE_SUBCLASS macros
#define VTK_M_ARRAY_HANDLE_SUBCLASS_IMPL(classname, fullclasstype, superclass, typename__)         \
  using Thisclass = typename__ vtkm::cont::detail::GetTypeInParentheses<void fullclasstype>::type; \
  using Superclass = typename__ vtkm::cont::detail::GetTypeInParentheses<void superclass>::type;   \
                                                                                                   \
  VTKM_IS_ARRAY_HANDLE(Superclass);                                                                \
                                                                                                   \
  VTKM_CONT                                                                                        \
  classname()                                                                                      \
    : Superclass()                                                                                 \
  {                                                                                                \
  }                                                                                                \
                                                                                                   \
  VTKM_CONT                                                                                        \
  classname(const Thisclass& src)                                                                  \
    : Superclass(src)                                                                              \
  {                                                                                                \
  }                                                                                                \
                                                                                                   \
  VTKM_CONT                                                                                        \
  classname(Thisclass&& src) noexcept : Superclass(std::move(src)) {}                              \
                                                                                                   \
  VTKM_CONT                                                                                        \
  classname(const vtkm::cont::ArrayHandle<typename__ Superclass::ValueType,                        \
                                          typename__ Superclass::StorageTag>& src)                 \
    : Superclass(src)                                                                              \
  {                                                                                                \
  }                                                                                                \
                                                                                                   \
  VTKM_CONT                                                                                        \
  classname(vtkm::cont::ArrayHandle<typename__ Superclass::ValueType,                              \
                                    typename__ Superclass::StorageTag>&& src) noexcept             \
    : Superclass(std::move(src))                                                                   \
  {                                                                                                \
  }                                                                                                \
                                                                                                   \
  VTKM_CONT                                                                                        \
  Thisclass& operator=(const Thisclass& src)                                                       \
  {                                                                                                \
    this->Superclass::operator=(src);                                                              \
    return *this;                                                                                  \
  }                                                                                                \
                                                                                                   \
  VTKM_CONT                                                                                        \
  Thisclass& operator=(Thisclass&& src) noexcept                                                   \
  {                                                                                                \
    this->Superclass::operator=(std::move(src));                                                   \
    return *this;                                                                                  \
  }                                                                                                \
                                                                                                   \
  using ValueType = typename__ Superclass::ValueType;                                              \
  using StorageTag = typename__ Superclass::StorageTag

/// \brief Macro to make default methods in ArrayHandle subclasses.
///
/// This macro defines the default constructors, destructors and assignment
/// operators for ArrayHandle subclasses that are templates. The ArrayHandle
/// subclasses are assumed to be empty convenience classes. The macro should be
/// defined after a \c public: declaration.
///
/// This macro takes three arguments. The first argument is the classname.
/// The second argument is the full class type. The third argument is the
/// superclass type (either \c ArrayHandle or another sublcass). Because
/// C macros do not handle template parameters very well (the preprocessor
/// thinks the template commas are macro argument commas), the second and
/// third arguments must be wrapped in parentheses.
///
/// This macro also defines a Superclass typedef as well as ValueType and
/// StorageTag.
///
/// Note that this macro only works on ArrayHandle subclasses that are
/// templated. For ArrayHandle sublcasses that are not templates, use
/// VTKM_ARRAY_HANDLE_SUBCLASS_NT.
///
#define VTKM_ARRAY_HANDLE_SUBCLASS(classname, fullclasstype, superclass)                           \
  VTK_M_ARRAY_HANDLE_SUBCLASS_IMPL(classname, fullclasstype, superclass, typename)

/// \brief Macro to make default methods in ArrayHandle subclasses.
///
/// This macro defines the default constructors, destructors and assignment
/// operators for ArrayHandle subclasses that are not templates. The
/// ArrayHandle subclasses are assumed to be empty convenience classes. The
/// macro should be defined after a \c public: declaration.
///
/// This macro takes two arguments. The first argument is the classname. The
/// second argument is the superclass type (either \c ArrayHandle or another
/// sublcass). Because C macros do not handle template parameters very well
/// (the preprocessor thinks the template commas are macro argument commas),
/// the second argument must be wrapped in parentheses.
///
/// This macro also defines a Superclass typedef as well as ValueType and
/// StorageTag.
///
/// Note that this macro only works on ArrayHandle subclasses that are not
/// templated. For ArrayHandle sublcasses that are templates, use
/// VTKM_ARRAY_HANDLE_SUBCLASS.
///
#define VTKM_ARRAY_HANDLE_SUBCLASS_NT(classname, superclass)                                       \
  VTK_M_ARRAY_HANDLE_SUBCLASS_IMPL(classname, (classname), superclass, )

/// \brief Manages an array-worth of data.
///
/// \c ArrayHandle manages as array of data that can be manipulated by VTKm
/// algorithms. The \c ArrayHandle may have up to two copies of the array, one
/// for the control environment and one for the execution environment, although
/// depending on the device and how the array is being used, the \c ArrayHandle
/// will only have one copy when possible.
///
/// An ArrayHandle can be constructed one of two ways. Its default construction
/// creates an empty, unallocated array that can later be allocated and filled
/// either by the user or a VTKm algorithm. The \c ArrayHandle can also be
/// constructed with iterators to a user's array. In this case the \c
/// ArrayHandle will keep a reference to this array but will throw an exception
/// if asked to re-allocate to a larger size.
///
/// \c ArrayHandle behaves like a shared smart pointer in that when it is copied
/// each copy holds a reference to the same array.  These copies are reference
/// counted so that when all copies of the \c ArrayHandle are destroyed, any
/// allocated memory is released.
///
///
template <typename T, typename StorageTag_ = VTKM_DEFAULT_STORAGE_TAG>
class VTKM_ALWAYS_EXPORT ArrayHandle : public internal::ArrayHandleBase
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

public:
  using StorageType = vtkm::cont::internal::Storage<T, StorageTag_>;
  using ValueType = T;
  using StorageTag = StorageTag_;
  using WritePortalType = vtkm::cont::internal::ArrayPortalCheck<typename StorageType::PortalType>;
  using ReadPortalType =
    vtkm::cont::internal::ArrayPortalCheck<typename StorageType::PortalConstType>;
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

  /// Constructs an empty ArrayHandle. Typically used for output or
  /// intermediate arrays that will be filled by a VTKm algorithm.
  ///
  VTKM_CONT ArrayHandle();

  /// Copy constructor.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated copy constructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ArrayHandle(const vtkm::cont::ArrayHandle<ValueType, StorageTag>& src);

  /// Move constructor.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated move constructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ArrayHandle(vtkm::cont::ArrayHandle<ValueType, StorageTag>&& src) noexcept;

  /// Special constructor for subclass specializations that need to set the
  /// initial state of the control array. When this constructor is used, it
  /// is assumed that the control array is valid.
  ///
  ArrayHandle(const StorageType& storage);


  /// Special constructor for subclass specializations that need to set the
  /// initial state of the control array. When this constructor is used, it
  /// is assumed that the control array is valid.
  ///
  ArrayHandle(StorageType&& storage) noexcept;

  /// Destructs an empty ArrayHandle.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandle();

  /// \brief Copies an ArrayHandle
  ///
  VTKM_CONT
  vtkm::cont::ArrayHandle<ValueType, StorageTag>& operator=(
    const vtkm::cont::ArrayHandle<ValueType, StorageTag>& src);

  /// \brief Move and Assignment of an ArrayHandle
  ///
  VTKM_CONT
  vtkm::cont::ArrayHandle<ValueType, StorageTag>& operator=(
    vtkm::cont::ArrayHandle<ValueType, StorageTag>&& src) noexcept;

  /// Like a pointer, two \c ArrayHandles are considered equal if they point
  /// to the same location in memory.
  ///
  VTKM_CONT
  bool operator==(const ArrayHandle<ValueType, StorageTag>& rhs) const
  {
    return (this->Internals == rhs.Internals);
  }

  VTKM_CONT
  bool operator!=(const ArrayHandle<ValueType, StorageTag>& rhs) const
  {
    return (this->Internals != rhs.Internals);
  }

  template <typename VT, typename ST>
  VTKM_CONT bool operator==(const ArrayHandle<VT, ST>&) const
  {
    return false; // different valuetype and/or storage
  }

  template <typename VT, typename ST>
  VTKM_CONT bool operator!=(const ArrayHandle<VT, ST>&) const
  {
    return true; // different valuetype and/or storage
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

  /// \brief Get an array portal that can be used in the control environment.
  ///
  /// The returned array can be used in the control environment to read values from the array. (It
  /// is not possible to write to the returned portal. That is `Get` will work on the portal, but
  /// `Set` will not.)
  ///
  /// **Note:** The returned portal will prevent any writes or modifications to the array. To
  /// ensure that the data pointed to by the portal is valid, this `ArrayHandle` will be locked to
  /// any modifications while the portal remains in scope. (You can call `Detach` on the returned
  /// portal to unlock the array. However, this will invalidate the portal.)
  ///
  /// **Note:** The returned portal cannot be used in the execution environment. This is because
  /// the portal will not work on some devices like GPUs. To get a portal that will work in the
  /// execution environment, use `PrepareForInput`.
  ///
  VTKM_CONT ReadPortalType ReadPortal() const;

  /// \brief Get an array portal that can be used in the control environment.
  ///
  /// The returned array can be used in the control environment to reand and write values to the
  /// array.
  ///
  /// **Note:** The returned portal will prevent any reads, writes, or modifications to the array.
  /// To ensure that the data pointed to by the portal is valid, this `ArrayHandle` will be locked
  /// to any modifications while the portal remains in scope. Also, to make sure that no reads get
  /// out of sync, reads other than the returned portal are also blocked. (You can call `Detach` on
  /// the returned portal to unlock the array. However, this will invalidate the portal.)
  ///
  /// **Note:** The returned portal cannot be used in the execution environment. This is because
  /// the portal will not work on some devices like GPUs. To get a portal that will work in the
  /// execution environment, use `PrepareForInput`.
  ///
  VTKM_CONT WritePortalType WritePortal() const;

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
    LockType lock = this->GetLock();
    this->WaitToWrite(lock, vtkm::cont::Token{});
    this->ReleaseResourcesExecutionInternal(lock);
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
  void Shrink(vtkm::Id numberOfValues);

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  VTKM_CONT void ReleaseResourcesExecution()
  {
    LockType lock = this->GetLock();
    this->WaitToWrite(lock, vtkm::cont::Token{});

    // Save any data in the execution environment by making sure it is synced
    // with the control environment.
    this->SyncControlArray(lock);

    this->ReleaseResourcesExecutionInternal(lock);
  }

  /// Releases all resources in both the control and execution environments.
  ///
  VTKM_CONT void ReleaseResources()
  {
    LockType lock = this->GetLock();

    this->ReleaseResourcesExecutionInternal(lock);

    if (this->Internals->IsControlArrayValid(lock))
    {
      this->Internals->GetControlArray(lock)->ReleaseResources();
      this->Internals->SetControlArrayValid(lock, false);
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
  VTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::PortalConst PrepareForInput(
    DeviceAdapterTag,
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
  VTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::Portal
  PrepareForOutput(vtkm::Id numberOfValues, DeviceAdapterTag, vtkm::cont::Token& token);

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
  VTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::Portal PrepareForInPlace(
    DeviceAdapterTag,
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
    LockType lock = this->GetLock();
    this->SyncControlArray(lock);
  }

private:
  /// Acquires a lock on the internals of this `ArrayHandle`. The calling
  /// function should keep the returned lock and let it go out of scope
  /// when the lock is no longer needed.
  ///
  LockType GetLock() const { return LockType(this->Internals->Mutex); }

  /// Returns true if read operations can currently be performed.
  ///
  VTKM_CONT bool CanRead(const LockType& lock, const vtkm::cont::Token& token) const
  {
    return ((*this->Internals->GetWriteCount(lock) < 1) ||
            (token.IsAttached(this->Internals->GetWriteCount(lock))));
  }

  //// Returns true if write operations can currently be performed.
  ///
  VTKM_CONT bool CanWrite(const LockType& lock, const vtkm::cont::Token& token) const
  {
    return (((*this->Internals->GetWriteCount(lock) < 1) ||
             (token.IsAttached(this->Internals->GetWriteCount(lock)))) &&
            ((*this->Internals->GetReadCount(lock) < 1) ||
             ((*this->Internals->GetReadCount(lock) == 1) &&
              token.IsAttached(this->Internals->GetReadCount(lock)))));
  }

  //// Will block the current thread until a read can be performed.
  ///
  VTKM_CONT void WaitToRead(LockType& lock, const vtkm::cont::Token& token) const
  {
    // Note that if you deadlocked here, that means that you are trying to do a read operation on
    // an array where an object is writing to it. This could happen on the same thread. For
    // example, if you call `GetPortalControl()` then no other operation that can result in reading
    // or writing data in the array can happen while the resulting portal is still in scope.
    this->Internals->ConditionVariable.wait(
      lock, [&lock, &token, this] { return this->CanRead(lock, token); });
  }

  //// Will block the current thread until a write can be performed.
  ///
  VTKM_CONT void WaitToWrite(LockType& lock, const vtkm::cont::Token& token) const
  {
    // Note that if you deadlocked here, that means that you are trying to do a write operation on
    // an array where an object is reading or writing to it. This could happen on the same thread.
    // For example, if you call `GetPortalControl()` then no other operation that can result in
    // reading or writing data in the array can happen while the resulting portal is still in
    // scope.
    this->Internals->ConditionVariable.wait(
      lock, [&lock, &token, this] { return this->CanWrite(lock, token); });
  }

  /// Gets this array handle ready to interact with the given device. If the
  /// array handle has already interacted with this device, then this method
  /// does nothing. Although the internal state of this class can change, the
  /// method is declared const because logically the data does not.
  ///
  template <typename DeviceAdapterTag>
  VTKM_CONT void PrepareForDevice(LockType& lock, DeviceAdapterTag) const;

  /// Synchronizes the control array with the execution array. If either the
  /// user array or control array is already valid, this method does nothing
  /// (because the data is already available in the control environment).
  /// Although the internal state of this class can change, the method is
  /// declared const because logically the data does not.
  ///
  VTKM_CONT void SyncControlArray(LockType& lock) const;

  vtkm::Id GetNumberOfValues(LockType& lock) const;

  VTKM_CONT
  void ReleaseResourcesExecutionInternal(LockType& lock) const
  {
    if (this->Internals->IsExecutionArrayValid(lock))
    {
      this->WaitToWrite(lock, vtkm::cont::Token{});
      // Note that it is possible that while waiting someone else deleted the execution array.
      // That is why we check again.
    }
    if (this->Internals->IsExecutionArrayValid(lock))
    {
      this->Internals->GetExecutionArray(lock)->ReleaseResources();
      this->Internals->SetExecutionArrayValid(lock, false);
    }
  }

  class VTKM_ALWAYS_EXPORT InternalStruct
  {
    mutable StorageType ControlArray;
    mutable std::shared_ptr<bool> ControlArrayValid;

    mutable std::unique_ptr<ExecutionManagerType> ExecutionArray;
    mutable bool ExecutionArrayValid = false;

    mutable vtkm::cont::Token::ReferenceCount ReadCount = 0;
    mutable vtkm::cont::Token::ReferenceCount WriteCount = 0;

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
  };

  VTKM_CONT
  ArrayHandle(const std::shared_ptr<InternalStruct>& i)
    : Internals(i)
  {
  }

  std::shared_ptr<InternalStruct> Internals;
};

/// A convenience function for creating an ArrayHandle from a standard C array.
///
template <typename T>
VTKM_CONT vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>
make_ArrayHandle(const T* array, vtkm::Id length, vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  using ArrayHandleType = vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>;
  if (copy == vtkm::CopyFlag::On)
  {
    ArrayHandleType handle;
    handle.Allocate(length);
    std::copy(array, array + length, vtkm::cont::ArrayPortalToIteratorBegin(handle.WritePortal()));
    return handle;
  }
  else
  {
    using StorageType = vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>;
    return ArrayHandleType(StorageType(array, length));
  }
}

/// A convenience function for creating an ArrayHandle from an std::vector.
///
template <typename T, typename Allocator>
VTKM_CONT vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> make_ArrayHandle(
  const std::vector<T, Allocator>& array,
  vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  if (!array.empty())
  {
    return make_ArrayHandle(&array.front(), static_cast<vtkm::Id>(array.size()), copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>();
  }
}

namespace detail
{

template <typename T>
VTKM_NEVER_EXPORT VTKM_CONT inline void
printSummary_ArrayHandle_Value(const T& value, std::ostream& out, vtkm::VecTraitsTagSingleComponent)
{
  out << value;
}

VTKM_NEVER_EXPORT
VTKM_CONT
inline void printSummary_ArrayHandle_Value(vtkm::UInt8 value,
                                           std::ostream& out,
                                           vtkm::VecTraitsTagSingleComponent)
{
  out << static_cast<int>(value);
}

VTKM_NEVER_EXPORT
VTKM_CONT
inline void printSummary_ArrayHandle_Value(vtkm::Int8 value,
                                           std::ostream& out,
                                           vtkm::VecTraitsTagSingleComponent)
{
  out << static_cast<int>(value);
}

template <typename T>
VTKM_NEVER_EXPORT VTKM_CONT inline void printSummary_ArrayHandle_Value(
  const T& value,
  std::ostream& out,
  vtkm::VecTraitsTagMultipleComponents)
{
  using Traits = vtkm::VecTraits<T>;
  using ComponentType = typename Traits::ComponentType;
  using IsVecOfVec = typename vtkm::VecTraits<ComponentType>::HasMultipleComponents;
  vtkm::IdComponent numComponents = Traits::GetNumberOfComponents(value);
  out << "(";
  printSummary_ArrayHandle_Value(Traits::GetComponent(value, 0), out, IsVecOfVec());
  for (vtkm::IdComponent index = 1; index < numComponents; ++index)
  {
    out << ",";
    printSummary_ArrayHandle_Value(Traits::GetComponent(value, index), out, IsVecOfVec());
  }
  out << ")";
}

template <typename T1, typename T2>
VTKM_NEVER_EXPORT VTKM_CONT inline void printSummary_ArrayHandle_Value(
  const vtkm::Pair<T1, T2>& value,
  std::ostream& out,
  vtkm::VecTraitsTagSingleComponent)
{
  out << "{";
  printSummary_ArrayHandle_Value(
    value.first, out, typename vtkm::VecTraits<T1>::HasMultipleComponents());
  out << ",";
  printSummary_ArrayHandle_Value(
    value.second, out, typename vtkm::VecTraits<T2>::HasMultipleComponents());
  out << "}";
}



} // namespace detail

template <typename T, typename StorageT>
VTKM_NEVER_EXPORT VTKM_CONT inline void printSummary_ArrayHandle(
  const vtkm::cont::ArrayHandle<T, StorageT>& array,
  std::ostream& out,
  bool full = false)
{
  using ArrayType = vtkm::cont::ArrayHandle<T, StorageT>;
  using PortalType = typename ArrayType::ReadPortalType;
  using IsVec = typename vtkm::VecTraits<T>::HasMultipleComponents;

  vtkm::Id sz = array.GetNumberOfValues();

  out << "valueType=" << typeid(T).name() << " storageType=" << typeid(StorageT).name()
      << " numValues=" << sz << " bytes=" << (static_cast<size_t>(sz) * sizeof(T)) << " [";

  PortalType portal = array.ReadPortal();
  if (full || sz <= 7)
  {
    for (vtkm::Id i = 0; i < sz; i++)
    {
      detail::printSummary_ArrayHandle_Value(portal.Get(i), out, IsVec());
      if (i != (sz - 1))
      {
        out << " ";
      }
    }
  }
  else
  {
    detail::printSummary_ArrayHandle_Value(portal.Get(0), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(1), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(2), out, IsVec());
    out << " ... ";
    detail::printSummary_ArrayHandle_Value(portal.Get(sz - 3), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(sz - 2), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(sz - 1), out, IsVec());
  }
  out << "]\n";
}
}
} //namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<ArrayHandle<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

namespace internal
{

template <typename T, typename S>
void VTKM_CONT ArrayHandleDefaultSerialization(vtkmdiy::BinaryBuffer& bb,
                                               const vtkm::cont::ArrayHandle<T, S>& obj);

} // internal
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandle<T>>
{
  static VTKM_CONT void save(BinaryBuffer& bb, const vtkm::cont::ArrayHandle<T>& obj)
  {
    vtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::ArrayHandle<T>& obj);
};

} // diy
/// @endcond SERIALIZATION

#ifndef vtk_m_cont_ArrayHandle_hxx
#include <vtkm/cont/ArrayHandle.hxx>
#endif

#ifndef vtk_m_cont_internal_ArrayHandleBasicImpl_h
#include <vtkm/cont/internal/ArrayHandleBasicImpl.h>
#endif

#include <vtkm/cont/internal/ArrayExportMacros.h>

#ifndef vtkm_cont_ArrayHandle_cxx

namespace vtkm
{
namespace cont
{

#define VTKM_ARRAYHANDLE_EXPORT(Type)                                                              \
  extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>;              \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayHandle<vtkm::Vec<Type, 2>, StorageTagBasic>;                                              \
  extern template class VTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayHandle<vtkm::Vec<Type, 3>, StorageTagBasic>;                                              \
  extern template class VTKM_CONT_TEMPLATE_EXPORT ArrayHandle<vtkm::Vec<Type, 4>, StorageTagBasic>;

VTKM_ARRAYHANDLE_EXPORT(char)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Int8)
VTKM_ARRAYHANDLE_EXPORT(vtkm::UInt8)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Int16)
VTKM_ARRAYHANDLE_EXPORT(vtkm::UInt16)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Int32)
VTKM_ARRAYHANDLE_EXPORT(vtkm::UInt32)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Int64)
VTKM_ARRAYHANDLE_EXPORT(vtkm::UInt64)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Float32)
VTKM_ARRAYHANDLE_EXPORT(vtkm::Float64)

#undef VTKM_ARRAYHANDLE_EXPORT
}
} // end vtkm::cont

#endif // !vtkm_cont_ArrayHandle_cxx

#endif //vtk_m_cont_ArrayHandle_h
