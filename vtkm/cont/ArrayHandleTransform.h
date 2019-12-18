//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleTransform_h
#define vtk_m_cont_ArrayHandleTransform_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorInternal.h>
#include <vtkm/cont/ExecutionAndControlObjectBase.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/internal/ArrayPortalHelpers.h>

#include <vtkm/cont/serial/internal/DeviceAdapterRuntimeDetectorSerial.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// Tag used in place of an inverse functor.
struct NullFunctorType
{
};
}
}
} // namespace vtkm::cont::internal

namespace vtkm
{
namespace exec
{
namespace internal
{

using NullFunctorType = vtkm::cont::internal::NullFunctorType;

/// \brief An array portal that transforms a value from another portal.
///
template <typename ValueType_,
          typename PortalType_,
          typename FunctorType_,
          typename InverseFunctorType_ = NullFunctorType>
class VTKM_ALWAYS_EXPORT ArrayPortalTransform;

template <typename ValueType_, typename PortalType_, typename FunctorType_>
class VTKM_ALWAYS_EXPORT
  ArrayPortalTransform<ValueType_, PortalType_, FunctorType_, NullFunctorType>
{
public:
  using PortalType = PortalType_;
  using ValueType = ValueType_;
  using FunctorType = FunctorType_;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalTransform(const PortalType& portal = PortalType(),
                       const FunctorType& functor = FunctorType())
    : Portal(portal)
    , Functor(functor)
  {
  }

  /// Copy constructor for any other ArrayPortalTransform with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <class OtherV, class OtherP, class OtherF>
  VTKM_EXEC_CONT ArrayPortalTransform(const ArrayPortalTransform<OtherV, OtherP, OtherF>& src)
    : Portal(src.GetPortal())
    , Functor(src.GetFunctor())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return this->Functor(this->Portal.Get(index)); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const PortalType& GetPortal() const { return this->Portal; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const FunctorType& GetFunctor() const { return this->Functor; }

protected:
  PortalType Portal;
  FunctorType Functor;
};

template <typename ValueType_,
          typename PortalType_,
          typename FunctorType_,
          typename InverseFunctorType_>
class VTKM_ALWAYS_EXPORT ArrayPortalTransform
  : public ArrayPortalTransform<ValueType_, PortalType_, FunctorType_, NullFunctorType>
{
  using Writable = vtkm::internal::PortalSupportsSets<PortalType_>;

public:
  using Superclass = ArrayPortalTransform<ValueType_, PortalType_, FunctorType_, NullFunctorType>;
  using PortalType = PortalType_;
  using ValueType = ValueType_;
  using FunctorType = FunctorType_;
  using InverseFunctorType = InverseFunctorType_;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalTransform(const PortalType& portal = PortalType(),
                       const FunctorType& functor = FunctorType(),
                       const InverseFunctorType& inverseFunctor = InverseFunctorType())
    : Superclass(portal, functor)
    , InverseFunctor(inverseFunctor)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <class OtherV, class OtherP, class OtherF, class OtherInvF>
  VTKM_EXEC_CONT ArrayPortalTransform(
    const ArrayPortalTransform<OtherV, OtherP, OtherF, OtherInvF>& src)
    : Superclass(src)
    , InverseFunctor(src.GetInverseFunctor())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    this->Portal.Set(index, this->InverseFunctor(value));
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const InverseFunctorType& GetInverseFunctor() const { return this->InverseFunctor; }

private:
  InverseFunctorType InverseFunctor;
};
}
}
} // namespace vtkm::exec::internal

namespace vtkm
{
namespace cont
{

namespace internal
{

template <typename ProvidedFunctorType, typename FunctorIsExecContObject>
struct TransformFunctorManagerImpl;

template <typename ProvidedFunctorType>
struct TransformFunctorManagerImpl<ProvidedFunctorType, std::false_type>
{
  VTKM_STATIC_ASSERT_MSG(!vtkm::cont::internal::IsExecutionObjectBase<ProvidedFunctorType>::value,
                         "Must use an ExecutionAndControlObject instead of an ExecutionObject.");

  ProvidedFunctorType Functor;
  using FunctorType = ProvidedFunctorType;

  TransformFunctorManagerImpl() = default;

  VTKM_CONT
  TransformFunctorManagerImpl(const ProvidedFunctorType& functor)
    : Functor(functor)
  {
  }

  VTKM_CONT
  ProvidedFunctorType PrepareForControl() const { return this->Functor; }

  template <typename Device>
  VTKM_CONT ProvidedFunctorType PrepareForExecution(Device) const
  {
    return this->Functor;
  }
};

template <typename ProvidedFunctorType>
struct TransformFunctorManagerImpl<ProvidedFunctorType, std::true_type>
{
  VTKM_IS_EXECUTION_AND_CONTROL_OBJECT(ProvidedFunctorType);

  ProvidedFunctorType Functor;
  //  using FunctorType = decltype(std::declval<ProvidedFunctorType>().PrepareForControl());
  using FunctorType = decltype(Functor.PrepareForControl());

  TransformFunctorManagerImpl() = default;

  VTKM_CONT
  TransformFunctorManagerImpl(const ProvidedFunctorType& functor)
    : Functor(functor)
  {
  }

  VTKM_CONT
  auto PrepareForControl() const -> decltype(this->Functor.PrepareForControl())
  {
    return this->Functor.PrepareForControl();
  }

  template <typename Device>
  VTKM_CONT auto PrepareForExecution(Device device) const
    -> decltype(this->Functor.PrepareForExecution(device))
  {
    return this->Functor.PrepareForExecution(device);
  }
};

template <typename ProvidedFunctorType>
struct TransformFunctorManager
  : TransformFunctorManagerImpl<
      ProvidedFunctorType,
      typename vtkm::cont::internal::IsExecutionAndControlObjectBase<ProvidedFunctorType>::type>
{
  using Superclass = TransformFunctorManagerImpl<
    ProvidedFunctorType,
    typename vtkm::cont::internal::IsExecutionAndControlObjectBase<ProvidedFunctorType>::type>;
  using FunctorType = typename Superclass::FunctorType;

  VTKM_CONT TransformFunctorManager() = default;

  VTKM_CONT TransformFunctorManager(const TransformFunctorManager&) = default;

  VTKM_CONT TransformFunctorManager(const ProvidedFunctorType& functor)
    : Superclass(functor)
  {
  }

  template <typename ValueType>
  using TransformedValueType = decltype(std::declval<FunctorType>()(ValueType{}));
};

template <typename ArrayHandleType,
          typename FunctorType,
          typename InverseFunctorType = NullFunctorType>
struct VTKM_ALWAYS_EXPORT StorageTagTransform
{
  using FunctorManager = TransformFunctorManager<FunctorType>;
  using ValueType =
    typename FunctorManager::template TransformedValueType<typename ArrayHandleType::ValueType>;
};

template <typename ArrayHandleType, typename FunctorType>
class Storage<typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType,
              StorageTagTransform<ArrayHandleType, FunctorType>>
{
  using FunctorManager = TransformFunctorManager<FunctorType>;

public:
  using ValueType = typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType;

  using PortalConstType =
    vtkm::exec::internal::ArrayPortalTransform<ValueType,
                                               typename ArrayHandleType::PortalConstControl,
                                               typename FunctorManager::FunctorType>;

  // Note that this array is read only, so you really should only be getting the const
  // version of the portal. If you actually try to write to this portal, you will
  // get an error.
  using PortalType = PortalConstType;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  VTKM_CONT
  Storage(const ArrayHandleType& array, const FunctorType& functor = FunctorType())
    : Array(array)
    , Functor(functor)
    , Valid(true)
  {
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    throw vtkm::cont::ErrorBadType(
      "ArrayHandleTransform is read only. Cannot get writable portal.");
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    vtkm::cont::ScopedRuntimeDeviceTracker trackerScope(vtkm::cont::DeviceAdapterTagSerial{});
    return PortalConstType(this->Array.GetPortalConstControl(), this->Functor.PrepareForControl());
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Array.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayHandleTransform is read only. It cannot be allocated.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayHandleTransform is read only. It cannot shrink.");
  }

  VTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the delegate array, which may be used elsewhere. Should the behavior
    // be different?
  }

  VTKM_CONT
  const ArrayHandleType& GetArray() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Array;
  }

  VTKM_CONT
  const FunctorManager& GetFunctor() const { return this->Functor; }

private:
  ArrayHandleType Array;
  FunctorManager Functor;
  bool Valid;
};

template <typename ArrayHandleType, typename FunctorType, typename InverseFunctorType>
class Storage<
  typename StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::ValueType,
  StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>>
{
  using FunctorManager = TransformFunctorManager<FunctorType>;
  using InverseFunctorManager = TransformFunctorManager<InverseFunctorType>;

public:
  using ValueType =
    typename StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::ValueType;

  using PortalType =
    vtkm::exec::internal::ArrayPortalTransform<ValueType,
                                               typename ArrayHandleType::PortalControl,
                                               typename FunctorManager::FunctorType,
                                               typename InverseFunctorManager::FunctorType>;
  using PortalConstType =
    vtkm::exec::internal::ArrayPortalTransform<ValueType,
                                               typename ArrayHandleType::PortalConstControl,
                                               typename FunctorManager::FunctorType,
                                               typename InverseFunctorManager::FunctorType>;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  VTKM_CONT
  Storage(const ArrayHandleType& array,
          const FunctorType& functor = FunctorType(),
          const InverseFunctorType& inverseFunctor = InverseFunctorType())
    : Array(array)
    , Functor(functor)
    , InverseFunctor(inverseFunctor)
    , Valid(true)
  {
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->Valid);
    vtkm::cont::ScopedRuntimeDeviceTracker trackerScope(vtkm::cont::DeviceAdapterTagSerial{});
    return PortalType(this->Array.GetPortalControl(),
                      this->Functor.PrepareForControl(),
                      this->InverseFunctor.PrepareForControl());
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    vtkm::cont::ScopedRuntimeDeviceTracker trackerScope(vtkm::cont::DeviceAdapterTagSerial{});
    return PortalConstType(this->Array.GetPortalConstControl(),
                           this->Functor.PrepareForControl(),
                           this->InverseFunctor.PrepareForControl());
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Array.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues)
  {
    this->Array.Allocate(numberOfValues);
    this->Valid = true;
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->Array.Shrink(numberOfValues); }

  VTKM_CONT
  void ReleaseResources()
  {
    this->Array.ReleaseResources();
    this->Valid = false;
  }

  VTKM_CONT
  const ArrayHandleType& GetArray() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Array;
  }

  VTKM_CONT
  const FunctorManager& GetFunctor() const { return this->Functor; }

  VTKM_CONT
  const InverseFunctorManager& GetInverseFunctor() const { return this->InverseFunctor; }

private:
  ArrayHandleType Array;
  FunctorManager Functor;
  InverseFunctorManager InverseFunctor;
  bool Valid;
};

template <typename ArrayHandleType, typename FunctorType, typename Device>
class ArrayTransfer<typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType,
                    StorageTagTransform<ArrayHandleType, FunctorType>,
                    Device>
{
  using StorageTag = StorageTagTransform<ArrayHandleType, FunctorType>;
  using FunctorManager = TransformFunctorManager<FunctorType>;

public:
  using ValueType = typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  //meant to be an invalid writeable execution portal
  using PortalExecution = typename StorageType::PortalType;
  using PortalConstExecution = vtkm::exec::internal::ArrayPortalTransform<
    ValueType,
    typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst,
    typename FunctorManager::FunctorType>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
    , Functor(storage->GetFunctor())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->Array.PrepareForInput(Device()),
                                this->Functor.PrepareForExecution(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool& vtkmNotUsed(updateData))
  {
    throw vtkm::cont::ErrorBadType("ArrayHandleTransform read only. "
                                   "Cannot be used for in-place operations.");
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayHandleTransform read only. Cannot be used as output.");
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandleTransform read only. "
      "There should be no occurrence of the ArrayHandle trying to pull "
      "data from the execution environment.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayHandleTransform read only. Cannot shrink.");
  }

  VTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }

private:
  ArrayHandleType Array;
  FunctorManager Functor;
};

template <typename ArrayHandleType,
          typename FunctorType,
          typename InverseFunctorType,
          typename Device>
class ArrayTransfer<
  typename StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::ValueType,
  StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>,
  Device>
{
  using StorageTag = StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>;
  using FunctorManager = TransformFunctorManager<FunctorType>;
  using InverseFunctorManager = TransformFunctorManager<InverseFunctorType>;

public:
  using ValueType = typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = vtkm::exec::internal::ArrayPortalTransform<
    ValueType,
    typename ArrayHandleType::template ExecutionTypes<Device>::Portal,
    typename FunctorManager::FunctorType,
    typename InverseFunctorManager::FunctorType>;
  using PortalConstExecution = vtkm::exec::internal::ArrayPortalTransform<
    ValueType,
    typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst,
    typename FunctorManager::FunctorType,
    typename InverseFunctorManager::FunctorType>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
    , Functor(storage->GetFunctor())
    , InverseFunctor(storage->GetInverseFunctor())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->Array.PrepareForInput(Device()),
                                this->Functor.PrepareForExecution(Device()),
                                this->InverseFunctor.PrepareForExecution(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool& vtkmNotUsed(updateData))
  {
    return PortalExecution(this->Array.PrepareForInPlace(Device()),
                           this->Functor.PrepareForExecution(Device()),
                           this->InverseFunctor.PrepareForExecution(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return PortalExecution(this->Array.PrepareForOutput(numberOfValues, Device()),
                           this->Functor.PrepareForExecution(Device()),
                           this->InverseFunctor.PrepareForExecution(Device()));
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->Array.Shrink(numberOfValues); }

  VTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }

private:
  ArrayHandleType Array;
  FunctorManager Functor;
  InverseFunctorManager InverseFunctor;
};

} // namespace internal

/// \brief Implicitly transform values of one array to another with a functor.
///
/// ArrayHandleTransforms is a specialization of ArrayHandle. It takes a
/// delegate array handle and makes a new handle that calls a given unary
/// functor with the element at a given index and returns the result of the
/// functor as the value of this array at that position. This transformation is
/// done on demand. That is, rather than make a new copy of the array with new
/// values, the transformation is done as values are read from the array. Thus,
/// the functor operator should work in both the control and execution
/// environments.
///
template <typename ArrayHandleType,
          typename FunctorType,
          typename InverseFunctorType = internal::NullFunctorType>
class ArrayHandleTransform;

template <typename ArrayHandleType, typename FunctorType>
class ArrayHandleTransform<ArrayHandleType, FunctorType, internal::NullFunctorType>
  : public vtkm::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType>::ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType>>
{
  // If the following line gives a compile error, then the ArrayHandleType
  // template argument is not a valid ArrayHandle type.
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleTransform,
    (ArrayHandleTransform<ArrayHandleType, FunctorType>),
    (vtkm::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType>::ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType>>));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleTransform(const ArrayHandleType& handle, const FunctorType& functor = FunctorType())
    : Superclass(StorageType(handle, functor))
  {
  }
};

/// make_ArrayHandleTransform is convenience function to generate an
/// ArrayHandleTransform.  It takes in an ArrayHandle and a functor
/// to apply to each element of the Handle.
template <typename HandleType, typename FunctorType>
VTKM_CONT vtkm::cont::ArrayHandleTransform<HandleType, FunctorType> make_ArrayHandleTransform(
  HandleType handle,
  FunctorType functor)
{
  return ArrayHandleTransform<HandleType, FunctorType>(handle, functor);
}

// ArrayHandleTransform with inverse functors enabled (no need to subclass from
// ArrayHandleTransform without inverse functors: nothing to inherit).
template <typename ArrayHandleType, typename FunctorType, typename InverseFunctorType>
class ArrayHandleTransform
  : public vtkm::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::
        ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>>
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleTransform,
    (ArrayHandleTransform<ArrayHandleType, FunctorType, InverseFunctorType>),
    (vtkm::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::
        ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>>));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  ArrayHandleTransform(const ArrayHandleType& handle,
                       const FunctorType& functor = FunctorType(),
                       const InverseFunctorType& inverseFunctor = InverseFunctorType())
    : Superclass(StorageType(handle, functor, inverseFunctor))
  {
  }

  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleTransform() {}
};

template <typename HandleType, typename FunctorType, typename InverseFunctorType>
VTKM_CONT vtkm::cont::ArrayHandleTransform<HandleType, FunctorType, InverseFunctorType>
make_ArrayHandleTransform(HandleType handle, FunctorType functor, InverseFunctorType inverseFunctor)
{
  return ArrayHandleTransform<HandleType, FunctorType, InverseFunctorType>(
    handle, functor, inverseFunctor);
}
}

} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename AH, typename Functor, typename InvFunctor>
struct SerializableTypeString<vtkm::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Transform<" + SerializableTypeString<AH>::Get() + "," +
      SerializableTypeString<Functor>::Get() + "," + SerializableTypeString<InvFunctor>::Get() +
      ">";
    return name;
  }
};

template <typename AH, typename Functor>
struct SerializableTypeString<vtkm::cont::ArrayHandleTransform<AH, Functor>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Transform<" + SerializableTypeString<AH>::Get() + "," +
      SerializableTypeString<Functor>::Get() + ">";
    return name;
  }
};

template <typename AH, typename Functor, typename InvFunctor>
struct SerializableTypeString<vtkm::cont::ArrayHandle<
  typename vtkm::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>::ValueType,
  vtkm::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH, typename Functor>
struct Serialization<vtkm::cont::ArrayHandleTransform<AH, Functor>>
{
private:
  using Type = vtkm::cont::ArrayHandleTransform<AH, Functor>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    vtkmdiy::save(bb, storage.GetArray());
    vtkmdiy::save(bb, storage.GetFunctor());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    vtkmdiy::load(bb, array);
    Functor functor;
    vtkmdiy::load(bb, functor);
    obj = vtkm::cont::make_ArrayHandleTransform(array, functor);
  }
};

template <typename AH, typename Functor, typename InvFunctor>
struct Serialization<vtkm::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
private:
  using Type = vtkm::cont::ArrayHandleTransform<AH, Functor, InvFunctor>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    vtkmdiy::save(bb, storage.GetArray());
    vtkmdiy::save(bb, storage.GetFunctor());
    vtkmdiy::save(bb, storage.GetInverseFunctor());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    vtkmdiy::load(bb, array);
    Functor functor;
    vtkmdiy::load(bb, functor);
    InvFunctor invFunctor;
    vtkmdiy::load(bb, invFunctor);
    obj = vtkm::cont::make_ArrayHandleTransform(array, functor, invFunctor);
  }
};

template <typename AH, typename Functor, typename InvFunctor>
struct Serialization<vtkm::cont::ArrayHandle<
  typename vtkm::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>::ValueType,
  vtkm::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>>>
  : Serialization<vtkm::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleTransform_h
