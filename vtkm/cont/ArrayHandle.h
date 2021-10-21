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

#include <vtkm/cont/DeviceAdapterList.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/ErrorInternal.h>
#include <vtkm/cont/Storage.h>
#include <vtkm/cont/Token.h>

#include <vtkm/internal/ArrayPortalHelpers.h>

#include <algorithm>
#include <deque>
#include <iterator>
#include <memory>
#include <mutex>
#include <vector>

#include <vtkm/cont/internal/Buffer.h>

namespace vtkm
{
namespace cont
{

// Normally this would be defined in ArrayHandleBasic.h, but we need this declared early for
// the default storage.

/// A tag for the basic implementation of a Storage object.
struct VTKM_ALWAYS_EXPORT StorageTagBasic
{
};
}
} // namespace vtkm::cont

#if VTKM_STORAGE == VTKM_STORAGE_BASIC

#define VTKM_DEFAULT_STORAGE_TAG ::vtkm::cont::StorageTagBasic

#elif VTKM_STORAGE == VTKM_STORAGE_ERROR

#include <vtkm/cont/internal/StorageError.h>
#define VTKM_DEFAULT_STORAGE_TAG ::vtkm::cont::internal::StorageTagError

#elif (VTKM_STORAGE == VTKM_STORAGE_UNDEFINED) || !defined(VTKM_STORAGE)

#ifndef VTKM_DEFAULT_STORAGE_TAG
#warning If array storage is undefined, VTKM_DEFAULT_STORAGE_TAG must be defined.
#endif

#endif

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
using IsInvalidArrayHandle =
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

#define VTKM_IS_ARRAY_HANDLE(T) \
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
  classname(Thisclass&& src) noexcept                                                              \
    : Superclass(std::move(src))                                                                   \
  {                                                                                                \
  }                                                                                                \
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
#define VTKM_ARRAY_HANDLE_SUBCLASS(classname, fullclasstype, superclass) \
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
#define VTKM_ARRAY_HANDLE_SUBCLASS_NT(classname, superclass) \
  VTK_M_ARRAY_HANDLE_SUBCLASS_IMPL(classname, (classname), superclass, )

namespace detail
{

VTKM_CONT_EXPORT VTKM_CONT void ArrayHandleReleaseResourcesExecution(
  const std::vector<vtkm::cont::internal::Buffer>& buffers);

VTKM_CONT_EXPORT VTKM_CONT bool ArrayHandleIsOnDevice(
  const std::vector<vtkm::cont::internal::Buffer>& buffers,
  vtkm::cont::DeviceAdapterId device);

VTKM_CONT_EXPORT VTKM_CONT vtkm::cont::DeviceAdapterId ArrayHandleGetDeviceAdapterId(
  const std::vector<vtkm::cont::internal::Buffer>& buffers);

} // namespace detail

/// \brief Manages an array-worth of data.
///
/// `ArrayHandle` manages as array of data that can be manipulated by VTKm
/// algorithms. The `ArrayHandle` may have up to two copies of the array, one
/// for the control environment and one for the execution environment, although
/// depending on the device and how the array is being used, the `ArrayHandle`
/// will only have one copy when possible.
///
/// An `ArrayHandle` is often constructed by instantiating one of the `ArrayHandle`
/// subclasses. Several basic `ArrayHandle` types can also be constructed directly
/// and then allocated. The `ArrayHandleBasic` subclass provides mechanisms for
/// importing user arrays into an `ArrayHandle`.
///
/// `ArrayHandle` behaves like a shared smart pointer in that when it is copied
/// each copy holds a reference to the same array.  These copies are reference
/// counted so that when all copies of the `ArrayHandle` are destroyed, any
/// allocated memory is released.
///
template <typename T, typename StorageTag_ = VTKM_DEFAULT_STORAGE_TAG>
class VTKM_ALWAYS_EXPORT ArrayHandle : public internal::ArrayHandleBase
{
  VTKM_STATIC_ASSERT_MSG(
    (internal::IsValidArrayHandle<T, StorageTag_>::value),
    "Attempted to create an ArrayHandle with an invalid type/storage combination.");

public:
  using ValueType = T;
  using StorageTag = StorageTag_;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

  using ReadPortalType = typename StorageType::ReadPortalType;
  using WritePortalType = typename StorageType::WritePortalType;

  // TODO: Deprecate this
  template <typename Device>
  struct VTKM_DEPRECATED(1.6, "Use ReadPortalType and WritePortalType.") ExecutionTypes
  {
    using Portal = WritePortalType;
    using PortalConst = ReadPortalType;
  };

  using PortalControl VTKM_DEPRECATED(1.6, "Use ArrayHandle::WritePortalType instead.") =
    WritePortalType;
  using PortalConstControl VTKM_DEPRECATED(1.6, "Use ArrayHandle::ReadPortalType instead.") =
    ReadPortalType;

  /// Constructs an empty ArrayHandle.
  ///
  VTKM_CONT ArrayHandle()
    : Buffers(static_cast<std::size_t>(StorageType::GetNumberOfBuffers()))
  {
  }

  /// Copy constructor.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated copy constructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  VTKM_CONT ArrayHandle(const vtkm::cont::ArrayHandle<ValueType, StorageTag>& src)
    : Buffers(src.Buffers)
  {
  }

  /// Move constructor.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated move constructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  VTKM_CONT ArrayHandle(vtkm::cont::ArrayHandle<ValueType, StorageTag>&& src) noexcept
    : Buffers(std::move(src.Buffers))
  {
  }

  ///@{
  /// Special constructor for subclass specializations that need to set the
  /// initial state array. Used when pulling data from other sources.
  ///
  VTKM_CONT ArrayHandle(const std::vector<vtkm::cont::internal::Buffer>& buffers)
    : Buffers(buffers)
  {
    VTKM_ASSERT(static_cast<vtkm::IdComponent>(this->Buffers.size()) == this->GetNumberOfBuffers());
  }

  VTKM_CONT ArrayHandle(std::vector<vtkm::cont::internal::Buffer>&& buffers) noexcept
    : Buffers(std::move(buffers))
  {
    VTKM_ASSERT(static_cast<vtkm::IdComponent>(this->Buffers.size()) == this->GetNumberOfBuffers());
  }

  VTKM_CONT ArrayHandle(const vtkm::cont::internal::Buffer* buffers)
    : Buffers(buffers, buffers + StorageType::GetNumberOfBuffers())
  {
  }
  ///@}

  /// Destructs an empty ArrayHandle.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  VTKM_CONT ~ArrayHandle() {}

  /// \brief Copies an ArrayHandle
  ///
  VTKM_CONT
  vtkm::cont::ArrayHandle<ValueType, StorageTag>& operator=(
    const vtkm::cont::ArrayHandle<ValueType, StorageTag>& src)
  {
    this->Buffers = src.Buffers;
    return *this;
  }

  /// \brief Move and Assignment of an ArrayHandle
  ///
  VTKM_CONT
  vtkm::cont::ArrayHandle<ValueType, StorageTag>& operator=(
    vtkm::cont::ArrayHandle<ValueType, StorageTag>&& src) noexcept
  {
    this->Buffers = std::move(src.Buffers);
    return *this;
  }

  /// Like a pointer, two \c ArrayHandles are considered equal if they point
  /// to the same location in memory.
  ///
  VTKM_CONT
  bool operator==(const ArrayHandle<ValueType, StorageTag>& rhs) const
  {
    return this->Buffers == rhs.Buffers;
  }

  VTKM_CONT
  bool operator!=(const ArrayHandle<ValueType, StorageTag>& rhs) const
  {
    return this->Buffers != rhs.Buffers;
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

  VTKM_CONT static constexpr vtkm::IdComponent GetNumberOfBuffers()
  {
    return StorageType::GetNumberOfBuffers();
  }

  /// Get the storage.
  ///
  VTKM_CONT StorageType GetStorage() const { return StorageType{}; }

  /// Get the array portal of the control array.
  /// Since worklet invocations are asynchronous and this routine is a synchronization point,
  /// exceptions maybe thrown for errors from previously executed worklets.
  ///
  /// \deprecated Use `WritePortal` instead.
  ///
  VTKM_CONT
  VTKM_DEPRECATED(1.6,
                  "Use ArrayHandle::WritePortal() instead. "
                  "Note that the returned portal will lock the array while it is in scope.")

  /// \cond NOPE
  WritePortalType GetPortalControl() const { return this->WritePortal(); }
  /// \endcond

  /// Get the array portal of the control array.
  /// Since worklet invocations are asynchronous and this routine is a synchronization point,
  /// exceptions maybe thrown for errors from previously executed worklets.
  ///
  /// \deprecated Use `ReadPortal` instead.
  ///
  VTKM_CONT
  VTKM_DEPRECATED(1.6,
                  "Use ArrayHandle::ReadPortal() instead. "
                  "Note that the returned portal will lock the array while it is in scope.")
  /// \cond NOPE
  ReadPortalType GetPortalConstControl() const { return this->ReadPortal(); }
  /// \endcond

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
  VTKM_CONT ReadPortalType ReadPortal() const
  {
    vtkm::cont::Token token;
    return StorageType::CreateReadPortal(
      this->GetBuffers(), vtkm::cont::DeviceAdapterTagUndefined{}, token);
  }

  /// \brief Get an array portal that can be used in the control environment.
  ///
  /// The returned array can be used in the control environment to reand and write values to the
  /// array.
  ///
  /// **Note:** The returned portal cannot be used in the execution environment. This is because
  /// the portal will not work on some devices like GPUs. To get a portal that will work in the
  /// execution environment, use `PrepareForInput`.
  ///
  VTKM_CONT WritePortalType WritePortal() const
  {
    vtkm::cont::Token token;

    return StorageType::CreateWritePortal(
      this->GetBuffers(), vtkm::cont::DeviceAdapterTagUndefined{}, token);
  }

  /// Returns the number of entries in the array.
  ///
  VTKM_CONT vtkm::Id GetNumberOfValues() const
  {
    return StorageType::GetNumberOfValues(this->GetBuffers());
  }

  ///@{
  ///  \brief Allocates an array large enough to hold the given number of values.
  ///
  /// The allocation may be done on an already existing array. If so, then the data
  /// are preserved as best as possible if the preserve flag is set to `vtkm::CopyFlag::On`.
  /// If the preserve flag is set to `vtkm::CopyFlag::Off` (the default), any existing data
  /// could be wiped out.
  ///
  /// This method can throw `ErrorBadAllocation` if the array cannot be allocated or
  /// `ErrorBadValue` if the allocation is not feasible (for example, the
  /// array storage is read-only).
  ///
  VTKM_CONT void Allocate(vtkm::Id numberOfValues,
                          vtkm::CopyFlag preserve,
                          vtkm::cont::Token& token) const
  {
    StorageType::ResizeBuffers(numberOfValues, this->GetBuffers(), preserve, token);
  }

  VTKM_CONT void Allocate(vtkm::Id numberOfValues,
                          vtkm::CopyFlag preserve = vtkm::CopyFlag::Off) const
  {
    vtkm::cont::Token token;
    this->Allocate(numberOfValues, preserve, token);
  }
  ///@}

  VTKM_DEPRECATED(1.6, "Use Allocate(n, vtkm::CopyFlag::On) instead of Shrink(n).")
  VTKM_CONT void Shrink(vtkm::Id numberOfValues)
  {
    this->Allocate(numberOfValues, vtkm::CopyFlag::On);
  }

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  VTKM_CONT void ReleaseResourcesExecution() const
  {
    detail::ArrayHandleReleaseResourcesExecution(this->Buffers);
  }

  /// Releases all resources in both the control and execution environments.
  ///
  VTKM_CONT void ReleaseResources() const { this->Allocate(0); }

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
  VTKM_CONT ReadPortalType PrepareForInput(vtkm::cont::DeviceAdapterId device,
                                           vtkm::cont::Token& token) const
  {
    return StorageType::CreateReadPortal(this->GetBuffers(), device, token);
  }

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
  VTKM_CONT WritePortalType PrepareForInPlace(vtkm::cont::DeviceAdapterId device,
                                              vtkm::cont::Token& token) const
  {
    return StorageType::CreateWritePortal(this->GetBuffers(), device, token);
  }

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
  VTKM_CONT WritePortalType PrepareForOutput(vtkm::Id numberOfValues,
                                             vtkm::cont::DeviceAdapterId device,
                                             vtkm::cont::Token& token) const
  {
    this->Allocate(numberOfValues, vtkm::CopyFlag::Off, token);
    return StorageType::CreateWritePortal(this->GetBuffers(), device, token);
  }

  VTKM_CONT VTKM_DEPRECATED(1.6, "PrepareForInput now requires a vtkm::cont::Token object.")
    ReadPortalType PrepareForInput(vtkm::cont::DeviceAdapterId device) const
  {
    vtkm::cont::Token token;
    return this->PrepareForInput(device, token);
  }
  VTKM_CONT VTKM_DEPRECATED(1.6, "PrepareForOutput now requires a vtkm::cont::Token object.")
    WritePortalType PrepareForOutput(vtkm::Id numberOfValues, vtkm::cont::DeviceAdapterId device)
  {
    vtkm::cont::Token token;
    return this->PrepareForOutput(numberOfValues, device, token);
  }
  VTKM_CONT VTKM_DEPRECATED(1.6, "PrepareForInPlace now requires a vtkm::cont::Token object.")
    WritePortalType PrepareForInPlace(vtkm::cont::DeviceAdapterId device) const
  {
    vtkm::cont::Token token;
    return this->PrepareForInPlace(device, token);
  }

  /// Returns true if the ArrayHandle's data is on the given device. If the data are on the given
  /// device, then preparing for that device should not require any data movement.
  ///
  VTKM_CONT bool IsOnDevice(vtkm::cont::DeviceAdapterId device) const
  {
    return detail::ArrayHandleIsOnDevice(this->Buffers, device);
  }

  /// Returns true if the ArrayHandle's data is on the host. If the data are on the given
  /// device, then calling `ReadPortal` or `WritePortal` should not require any data movement.
  ///
  VTKM_CONT bool IsOnHost() const
  {
    return this->IsOnDevice(vtkm::cont::DeviceAdapterTagUndefined{});
  }

  /// Returns a DeviceAdapterId for a device currently allocated on. If there is no device
  /// with an up-to-date copy of the data, VTKM_DEVICE_ADAPTER_UNDEFINED is
  /// returned.
  ///
  /// Note that in a multithreaded environment the validity of this result can
  /// change.
  ///
  VTKM_CONT
  VTKM_DEPRECATED(1.7, "Use ArrayHandle::IsOnDevice.") DeviceAdapterId GetDeviceAdapterId() const
  {
    return detail::ArrayHandleGetDeviceAdapterId(this->Buffers);
  }

  /// Synchronizes the control array with the execution array. If either the
  /// user array or control array is already valid, this method does nothing
  /// (because the data is already available in the control environment).
  /// Although the internal state of this class can change, the method is
  /// declared const because logically the data does not.
  ///
  VTKM_CONT void SyncControlArray() const
  {
    // Creating a host read portal will force the data to be synced to the host.
    this->ReadPortal();
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
  VTKM_CONT void Enqueue(const vtkm::cont::Token& token) const
  {
    for (auto&& buffer : this->Buffers)
    {
      buffer.Enqueue(token);
    }
  }

  /// \brief Deep copies the data in the array.
  ///
  /// Takes the data that is in \a source and copies that data into this array.
  ///
  VTKM_CONT void DeepCopyFrom(const vtkm::cont::ArrayHandle<ValueType, StorageTag>& source) const
  {
    VTKM_ASSERT(this->Buffers.size() == source.Buffers.size());

    for (std::size_t bufferIndex = 0; bufferIndex < this->Buffers.size(); ++bufferIndex)
    {
      this->Buffers[bufferIndex].DeepCopyFrom(source.Buffers[bufferIndex]);
    }
  }

  /// Returns the internal `Buffer` structures that hold the data.
  ///
  VTKM_CONT vtkm::cont::internal::Buffer* GetBuffers() const { return this->Buffers.data(); }

private:
  mutable std::vector<vtkm::cont::internal::Buffer> Buffers;

protected:
  VTKM_CONT void SetBuffer(vtkm::IdComponent index, const vtkm::cont::internal::Buffer& buffer)
  {
    this->Buffers[static_cast<std::size_t>(index)] = buffer;
  }

  // BufferContainer must be an iteratable container of Buffer objects.
  template <typename BufferContainer>
  VTKM_CONT void SetBuffers(const BufferContainer& buffers)
  {
    std::copy(buffers.begin(), buffers.end(), this->Iterators->Buffers.begin());
  }
};

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

  out << "valueType=" << vtkm::cont::TypeToString<T>()
      << " storageType=" << vtkm::cont::TypeToString<StorageT>() << " " << sz
      << " values occupying " << (static_cast<size_t>(sz) * sizeof(T)) << " bytes [";

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

namespace internal
{

namespace detail
{

VTKM_CONT inline void CreateBuffersImpl(std::vector<vtkm::cont::internal::Buffer>&)
{
  // Nothing left to add.
}

template <typename T, typename S, typename... Args>
VTKM_CONT inline void CreateBuffersImpl(std::vector<vtkm::cont::internal::Buffer>& buffers,
                                        const vtkm::cont::ArrayHandle<T, S>& array,
                                        const Args&... args)
{
  vtkm::cont::internal::Buffer* arrayBuffers = array.GetBuffers();
  buffers.insert(buffers.end(), arrayBuffers, arrayBuffers + array.GetNumberOfBuffers());
  CreateBuffersImpl(buffers, args...);
}

template <typename... Args>
VTKM_CONT inline void CreateBuffersImpl(std::vector<vtkm::cont::internal::Buffer>& buffers,
                                        const vtkm::cont::internal::Buffer& buffer,
                                        const Args&... args)
{
  buffers.push_back(buffer);
  CreateBuffersImpl(buffers, args...);
}

template <typename... Args>
VTKM_CONT inline void CreateBuffersImpl(std::vector<vtkm::cont::internal::Buffer>& buffers,
                                        const std::vector<vtkm::cont::internal::Buffer>& addbuffs,
                                        const Args&... args)
{
  buffers.insert(buffers.end(), addbuffs.begin(), addbuffs.end());
  CreateBuffersImpl(buffers, args...);
}

template <typename Arg0, typename... Args>
VTKM_CONT inline void CreateBuffersImpl(std::vector<vtkm::cont::internal::Buffer>& buffers,
                                        const Arg0& arg0,
                                        const Args&... args);

template <typename T, typename S, typename... Args>
VTKM_CONT inline void CreateBuffersResolveArrays(std::vector<vtkm::cont::internal::Buffer>& buffers,
                                                 std::true_type,
                                                 const vtkm::cont::ArrayHandle<T, S>& array,
                                                 const Args&... args)
{
  CreateBuffersImpl(buffers, array, args...);
}

template <typename MetaData, typename... Args>
VTKM_CONT inline void CreateBuffersResolveArrays(std::vector<vtkm::cont::internal::Buffer>& buffers,
                                                 std::false_type,
                                                 const MetaData& metadata,
                                                 const Args&... args)
{
  vtkm::cont::internal::Buffer buffer;
  buffer.SetMetaData(metadata);
  buffers.push_back(std::move(buffer));
  CreateBuffersImpl(buffers, args...);
}

template <typename Arg0, typename... Args>
VTKM_CONT inline void CreateBuffersImpl(std::vector<vtkm::cont::internal::Buffer>& buffers,
                                        const Arg0& arg0,
                                        const Args&... args)
{
  // If the argument is a subclass of ArrayHandle, the template resolution will pick this
  // overload instead of the correct ArrayHandle overload. To resolve that, check to see
  // if the type is an `ArrayHandle` and use `CreateBuffersResolveArrays` to choose the
  // right path.
  using IsArray = typename vtkm::cont::internal::ArrayHandleCheck<Arg0>::type::type;
  CreateBuffersResolveArrays(buffers, IsArray{}, arg0, args...);
}

} // namespace detail

/// \brief Create the buffers for an `ArrayHandle` specialization.
///
/// When creating an `ArrayHandle` specialization, it is important to build a
/// `std::vector` of `Buffer` objects. This function simplifies creating
/// these buffer objects. Simply pass as arguments the things you want in the
/// buffers. The parameters to `CreateBuffers` are added to the `Buffer` `vector`
/// in the order provided. The actual object(s) added depends on the type of
/// parameter:
///
///   - `ArrayHandle`: The buffers from the `ArrayHandle` are added to the list.
///   - `Buffer`: A copy of the buffer is added to the list.
///   - `std::vector<Buffer>`: A copy of all buffers in this vector are added to the list.
///   - Anything else: A buffer with the given object attached as metadata is
///
template <typename... Args>
VTKM_CONT inline std::vector<vtkm::cont::internal::Buffer> CreateBuffers(const Args&... args)
{
  std::vector<vtkm::cont::internal::Buffer> buffers;
  buffers.reserve(sizeof...(args));
  detail::CreateBuffersImpl(buffers, args...);
  return buffers;
}

} // namespace internal

}
} //namespace vtkm::cont

#ifndef vtk_m_cont_ArrayHandleBasic_h
#include <vtkm/cont/ArrayHandleBasic.h>
#endif

#endif //vtk_m_cont_ArrayHandle_h
