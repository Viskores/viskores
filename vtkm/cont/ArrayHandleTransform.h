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
namespace internal
{

/// Tag used in place of an inverse functor.
struct NullFunctorType
{
};

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
  template <class OtherV, class OtherP, class OtherF>
  VTKM_EXEC_CONT ArrayPortalTransform(const ArrayPortalTransform<OtherV, OtherP, OtherF>& src)
    : Portal(src.GetPortal())
    , Functor(src.GetFunctor())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return this->Functor(this->Portal.Get(index)); }

  VTKM_EXEC_CONT
  const PortalType& GetPortal() const { return this->Portal; }

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

  VTKM_EXEC_CONT
  ArrayPortalTransform(const PortalType& portal = PortalType(),
                       const FunctorType& functor = FunctorType(),
                       const InverseFunctorType& inverseFunctor = InverseFunctorType())
    : Superclass(portal, functor)
    , InverseFunctor(inverseFunctor)
  {
  }

  template <class OtherV, class OtherP, class OtherF, class OtherInvF>
  VTKM_EXEC_CONT ArrayPortalTransform(
    const ArrayPortalTransform<OtherV, OtherP, OtherF, OtherInvF>& src)
    : Superclass(src)
    , InverseFunctor(src.GetInverseFunctor())
  {
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    this->Portal.Set(index, this->InverseFunctor(value));
  }

  VTKM_EXEC_CONT
  const InverseFunctorType& GetInverseFunctor() const { return this->InverseFunctor; }

private:
  InverseFunctorType InverseFunctor;
};
}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{

namespace internal
{

using NullFunctorType = vtkm::internal::NullFunctorType;

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

  VTKM_CONT ProvidedFunctorType PrepareForExecution(vtkm::cont::DeviceAdapterId,
                                                    vtkm::cont::Token&) const
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
  //  using FunctorType = decltype(Functor.PrepareForControl());
  using FunctorType = vtkm::cont::internal::ControlObjectType<ProvidedFunctorType>;

  TransformFunctorManagerImpl() = default;

  VTKM_CONT
  TransformFunctorManagerImpl(const ProvidedFunctorType& functor)
    : Functor(functor)
  {
  }

  VTKM_CONT
  auto PrepareForControl() const
    -> decltype(vtkm::cont::internal::CallPrepareForControl(this->Functor))
  {
    return vtkm::cont::internal::CallPrepareForControl(this->Functor);
  }

  VTKM_CONT auto PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                     vtkm::cont::Token& token) const
    -> decltype(vtkm::cont::internal::CallPrepareForExecution(this->Functor, device, token))
  {
    return vtkm::cont::internal::CallPrepareForExecution(this->Functor, device, token);
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
  using ValueType = typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType;

  using SourceStorage =
    Storage<typename ArrayHandleType::ValueType, typename ArrayHandleType::StorageTag>;

  static constexpr vtkm::IdComponent NUM_METADATA_BUFFERS = 1;

public:
  VTKM_STORAGE_NO_RESIZE;
  VTKM_STORAGE_NO_WRITE_PORTAL;

  using ReadPortalType =
    vtkm::internal::ArrayPortalTransform<ValueType,
                                         typename ArrayHandleType::ReadPortalType,
                                         typename FunctorManager::FunctorType>;

  VTKM_CONT static vtkm::IdComponent GetNumberOfBuffers()
  {
    return SourceStorage::GetNumberOfBuffers() + NUM_METADATA_BUFFERS;
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(const vtkm::cont::internal::Buffer* buffers)
  {
    return SourceStorage::GetNumberOfValues(buffers + NUM_METADATA_BUFFERS);
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(const vtkm::cont::internal::Buffer* buffers,
                                                   vtkm::cont::DeviceAdapterId device,
                                                   vtkm::cont::Token& token)
  {
    if (device == vtkm::cont::DeviceAdapterTagUndefined{})
    {
      return ReadPortalType(
        SourceStorage::CreateReadPortal(buffers + NUM_METADATA_BUFFERS, device, token),
        buffers[0].GetMetaData<FunctorManager>().PrepareForControl());
    }
    else
    {
      return ReadPortalType(
        SourceStorage::CreateReadPortal(buffers + NUM_METADATA_BUFFERS, device, token),
        buffers[0].GetMetaData<FunctorManager>().PrepareForExecution(device, token));
    }
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(
    const ArrayHandleType& handle,
    const FunctorType& functor = FunctorType())
  {
    return vtkm::cont::internal::CreateBuffers(FunctorManager(functor), handle);
  }

  VTKM_CONT static ArrayHandleType GetArray(const vtkm::cont::internal::Buffer* buffers)
  {
    return vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                   typename ArrayHandleType::StorageTag>(buffers +
                                                                         NUM_METADATA_BUFFERS);
  }

  VTKM_CONT static FunctorType GetFunctor(const vtkm::cont::internal::Buffer* buffers)
  {
    return buffers[0].GetMetaData<FunctorManager>().Functor;
  }

  VTKM_CONT static NullFunctorType GetInverseFunctor(const vtkm::cont::internal::Buffer*)
  {
    return NullFunctorType{};
  }
};

template <typename ArrayHandleType, typename FunctorType, typename InverseFunctorType>
class Storage<
  typename StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::ValueType,
  StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>>
{
  using FunctorManager = TransformFunctorManager<FunctorType>;
  using InverseFunctorManager = TransformFunctorManager<InverseFunctorType>;
  using ValueType = typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType;

  using SourceStorage =
    Storage<typename ArrayHandleType::ValueType, typename ArrayHandleType::StorageTag>;

  static constexpr vtkm::IdComponent NUM_METADATA_BUFFERS = 2;

public:
  using ReadPortalType =
    vtkm::internal::ArrayPortalTransform<ValueType,
                                         typename ArrayHandleType::ReadPortalType,
                                         typename FunctorManager::FunctorType,
                                         typename InverseFunctorManager::FunctorType>;
  using WritePortalType =
    vtkm::internal::ArrayPortalTransform<ValueType,
                                         typename ArrayHandleType::WritePortalType,
                                         typename FunctorManager::FunctorType,
                                         typename InverseFunctorManager::FunctorType>;

  VTKM_CONT constexpr static vtkm::IdComponent GetNumberOfBuffers()
  {
    return SourceStorage::GetNumberOfBuffers() + NUM_METADATA_BUFFERS;
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(const vtkm::cont::internal::Buffer* buffers)
  {
    return SourceStorage::GetNumberOfValues(buffers + NUM_METADATA_BUFFERS);
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      vtkm::cont::internal::Buffer* buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    SourceStorage::ResizeBuffers(numValues, buffers + NUM_METADATA_BUFFERS, preserve, token);
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(const vtkm::cont::internal::Buffer* buffers,
                                                   vtkm::cont::DeviceAdapterId device,
                                                   vtkm::cont::Token& token)
  {
    if (device == vtkm::cont::DeviceAdapterTagUndefined{})
    {
      return ReadPortalType(
        SourceStorage::CreateReadPortal(buffers + NUM_METADATA_BUFFERS, device, token),
        buffers[0].GetMetaData<FunctorManager>().PrepareForControl(),
        buffers[1].GetMetaData<InverseFunctorManager>().PrepareForControl());
    }
    else
    {
      return ReadPortalType(
        SourceStorage::CreateReadPortal(buffers + NUM_METADATA_BUFFERS, device, token),
        buffers[0].GetMetaData<FunctorManager>().PrepareForExecution(device, token),
        buffers[1].GetMetaData<InverseFunctorManager>().PrepareForExecution(device, token));
    }
  }

  VTKM_CONT static WritePortalType CreateWritePortal(vtkm::cont::internal::Buffer* buffers,
                                                     vtkm::cont::DeviceAdapterId device,
                                                     vtkm::cont::Token& token)
  {
    return WritePortalType(
      SourceStorage::CreateWritePortal(buffers + NUM_METADATA_BUFFERS, device, token),
      buffers[0].GetMetaData<FunctorManager>().PrepareForExecution(device, token),
      buffers[1].GetMetaData<InverseFunctorManager>().PrepareForExecution(device, token));
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(
    const ArrayHandleType& handle,
    const FunctorType& functor = FunctorType(),
    const InverseFunctorType& inverseFunctor = InverseFunctorType())
  {
    return vtkm::cont::internal::CreateBuffers(
      FunctorManager(functor), InverseFunctorManager(inverseFunctor), handle);
  }

  VTKM_CONT static ArrayHandleType GetArray(const vtkm::cont::internal::Buffer* buffers)
  {
    return vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                   typename ArrayHandleType::StorageTag>(buffers +
                                                                         NUM_METADATA_BUFFERS);
  }

  VTKM_CONT static FunctorType GetFunctor(const vtkm::cont::internal::Buffer* buffers)
  {
    return buffers[0].GetMetaData<FunctorManager>().Functor;
  }

  VTKM_CONT static InverseFunctorType GetInverseFunctor(const vtkm::cont::internal::Buffer* buffers)
  {
    return buffers[1].GetMetaData<InverseFunctorManager>().Functor;
  }
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
  ArrayHandleTransform(const ArrayHandleType& handle,
                       const FunctorType& functor = FunctorType{},
                       internal::NullFunctorType = internal::NullFunctorType{})
    : Superclass(StorageType::CreateBuffers(handle, functor))
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
    : Superclass(StorageType::CreateBuffers(handle, functor, inverseFunctor))
  {
  }

  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleTransform() {}

  /// \brief Returns the `ArrayHandle` that is being transformed.
  ///
  ArrayHandleType GetTransformedArray() const { return StorageType::GetArray(this->GetBuffers()); }

  /// \brief Returns the functor transforming the `ArrayHandle`.
  ///
  FunctorType GetFunctor() const { return StorageType::GetFunctor(this->GetBuffers()); }

  /// \brief Returns the inverse functor transforming the `ArrayHandle`
  ///
  InverseFunctorType GetInverseFunctor() const
  {
    return StorageType::GetInverseFunctor(this->GetBuffers());
  }
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
    Type transformedArray = obj;
    vtkmdiy::save(bb, obj.GetArray());
    vtkmdiy::save(bb, obj.GetFunctor());
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
    Type transformedArray = obj;
    vtkmdiy::save(bb, transformedArray.GetTransformedArray());
    vtkmdiy::save(bb, transformedArray.GetFunctor());
    vtkmdiy::save(bb, transformedArray.GetInverseFunctor());
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
