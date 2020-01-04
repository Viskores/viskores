//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleCast_h
#define vtk_m_cont_ArrayHandleCast_h

#include <vtkm/cont/ArrayHandleTransform.h>

#include <vtkm/cont/Logging.h>

#include <vtkm/Range.h>
#include <vtkm/VecTraits.h>

#include <limits>

namespace vtkm
{
namespace cont
{

template <typename SourceT, typename SourceStorage>
struct VTKM_ALWAYS_EXPORT StorageTagCast
{
};

namespace internal
{

template <typename FromType, typename ToType>
struct VTKM_ALWAYS_EXPORT Cast
{
// The following operator looks like it should never issue a cast warning because of
// the static_cast (and we don't want it to issue a warning). However, if ToType is
// an object that has a constructor that takes a value that FromType can be cast to,
// that cast can cause a warning. For example, if FromType is vtkm::Float64 and ToType
// is vtkm::Vec<vtkm::Float32, 3>, the static_cast will first implicitly cast the
// Float64 to a Float32 (which causes a warning) before using the static_cast to
// construct the Vec with the Float64. The easiest way around the problem is to
// just disable all conversion warnings here. (The pragmas are taken from those
// used in Types.h for the VecBase class.)
#if (!(defined(VTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif // gcc || clang
#endif //not using cuda < 8
#if defined(VTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

  VTKM_EXEC_CONT
  ToType operator()(const FromType& val) const { return static_cast<ToType>(val); }

#if (!(defined(VTKM_CUDA) && (__CUDACC_VER_MAJOR__ < 8)))
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang
#endif // not using cuda < 8
#if defined(VTKM_MSVC)
#pragma warning(pop)
#endif
};

namespace detail
{

template <typename TargetT, typename SourceT, typename SourceStorage, bool... CastFlags>
struct ArrayHandleCastTraits;

template <typename TargetT, typename SourceT, typename SourceStorage>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage>
  : ArrayHandleCastTraits<TargetT,
                          SourceT,
                          SourceStorage,
                          std::is_convertible<SourceT, TargetT>::value,
                          std::is_convertible<TargetT, SourceT>::value>
{
};

// Case where the forward cast is invalid, so this array is invalid.
template <typename TargetT, typename SourceT, typename SourceStorage, bool CanCastBackward>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage, false, CanCastBackward>
{
  struct StorageSuperclass : vtkm::cont::internal::UndefinedStorage
  {
    using PortalType = vtkm::cont::internal::detail::UndefinedArrayPortal<TargetT>;
    using PortalConstType = vtkm::cont::internal::detail::UndefinedArrayPortal<TargetT>;
  };
};

// Case where the forward cast is valid but the backward cast is invalid.
template <typename TargetT, typename SourceT, typename SourceStorage>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage, true, false>
{
  using StorageTagSuperclass = StorageTagTransform<vtkm::cont::ArrayHandle<SourceT, SourceStorage>,
                                                   vtkm::cont::internal::Cast<SourceT, TargetT>>;
  using StorageSuperclass = vtkm::cont::internal::Storage<TargetT, StorageTagSuperclass>;
  template <typename Device>
  using ArrayTransferSuperclass = ArrayTransfer<TargetT, StorageTagSuperclass, Device>;
};

// Case where both forward and backward casts are valid.
template <typename TargetT, typename SourceT, typename SourceStorage>
struct ArrayHandleCastTraits<TargetT, SourceT, SourceStorage, true, true>
{
  using StorageTagSuperclass = StorageTagTransform<vtkm::cont::ArrayHandle<SourceT, SourceStorage>,
                                                   vtkm::cont::internal::Cast<SourceT, TargetT>,
                                                   vtkm::cont::internal::Cast<TargetT, SourceT>>;
  using StorageSuperclass = vtkm::cont::internal::Storage<TargetT, StorageTagSuperclass>;
  template <typename Device>
  using ArrayTransferSuperclass = ArrayTransfer<TargetT, StorageTagSuperclass, Device>;
};

} // namespace detail

template <typename TargetT, typename SourceT, typename SourceStorage>
struct Storage<TargetT, vtkm::cont::StorageTagCast<SourceT, SourceStorage>>
  : detail::ArrayHandleCastTraits<TargetT, SourceT, SourceStorage>::StorageSuperclass
{
  using Superclass =
    typename detail::ArrayHandleCastTraits<TargetT, SourceT, SourceStorage>::StorageSuperclass;

  using Superclass::Superclass;
};

template <typename TargetT, typename SourceT, typename SourceStorage, typename Device>
struct ArrayTransfer<TargetT, vtkm::cont::StorageTagCast<SourceT, SourceStorage>, Device>
  : detail::ArrayHandleCastTraits<TargetT,
                                  SourceT,
                                  SourceStorage>::template ArrayTransferSuperclass<Device>
{
  using Superclass =
    typename detail::ArrayHandleCastTraits<TargetT,
                                           SourceT,
                                           SourceStorage>::template ArrayTransferSuperclass<Device>;

  using Superclass::Superclass;
};

} // namespace internal

/// \brief Cast the values of an array to the specified type, on demand.
///
/// ArrayHandleCast is a specialization of ArrayHandleTransform. Given an ArrayHandle
/// and a type, it creates a new handle that returns the elements of the array cast
/// to the specified type.
///
template <typename T, typename ArrayHandleType>
class ArrayHandleCast
  : public vtkm::cont::ArrayHandle<
      T,
      StorageTagCast<typename ArrayHandleType::ValueType, typename ArrayHandleType::StorageTag>>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCast,
    (ArrayHandleCast<T, ArrayHandleType>),
    (vtkm::cont::ArrayHandle<
      T,
      StorageTagCast<typename ArrayHandleType::ValueType, typename ArrayHandleType::StorageTag>>));

  ArrayHandleCast(const vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                                typename ArrayHandleType::StorageTag>& handle)
    : Superclass(typename Superclass::StorageType(handle))
  {
    this->ValidateTypeCast<typename ArrayHandleType::ValueType>();
  }

  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleCast() {}

private:
  // Log warnings if type cast is valid but lossy:
  template <typename SrcValueType>
  VTKM_CONT static typename std::enable_if<!std::is_same<T, SrcValueType>::value>::type
  ValidateTypeCast()
  {
#ifdef VTKM_ENABLE_LOGGING
    using DstValueType = T;
    using SrcComp = typename vtkm::VecTraits<SrcValueType>::BaseComponentType;
    using DstComp = typename vtkm::VecTraits<DstValueType>::BaseComponentType;
    using SrcLimits = std::numeric_limits<SrcComp>;
    using DstLimits = std::numeric_limits<DstComp>;

    const vtkm::Range SrcRange{ SrcLimits::min(), SrcLimits::max() };
    const vtkm::Range DstRange{ DstLimits::min(), DstLimits::max() };

    const bool RangeLoss = (SrcRange.Max > DstRange.Max || SrcRange.Min < DstRange.Min);
    const bool PrecLoss = SrcLimits::digits > DstLimits::digits;

    if (RangeLoss && PrecLoss)
    {
      VTKM_LOG_F(vtkm::cont::LogLevel::Warn,
                 "VariantArrayHandle::AsVirtual: Casting ComponentType of "
                 "%s to %s reduces range and precision.",
                 vtkm::cont::TypeToString<SrcValueType>().c_str(),
                 vtkm::cont::TypeToString<DstValueType>().c_str());
    }
    else if (RangeLoss)
    {
      VTKM_LOG_F(vtkm::cont::LogLevel::Warn,
                 "VariantArrayHandle::AsVirtual: Casting ComponentType of "
                 "%s to %s reduces range.",
                 vtkm::cont::TypeToString<SrcValueType>().c_str(),
                 vtkm::cont::TypeToString<DstValueType>().c_str());
    }
    else if (PrecLoss)
    {
      VTKM_LOG_F(vtkm::cont::LogLevel::Warn,
                 "VariantArrayHandle::AsVirtual: Casting ComponentType of "
                 "%s to %s reduces precision.",
                 vtkm::cont::TypeToString<SrcValueType>().c_str(),
                 vtkm::cont::TypeToString<DstValueType>().c_str());
    }
#endif // Logging
  }

  template <typename SrcValueType>
  VTKM_CONT static typename std::enable_if<std::is_same<T, SrcValueType>::value>::type
  ValidateTypeCast()
  {
    //no-op if types match
  }
};

namespace detail
{

template <typename CastType, typename OriginalType, typename ArrayType>
struct MakeArrayHandleCastImpl
{
  using ReturnType = vtkm::cont::ArrayHandleCast<CastType, ArrayType>;

  VTKM_CONT static ReturnType DoMake(const ArrayType& array) { return ReturnType(array); }
};

template <typename T, typename ArrayType>
struct MakeArrayHandleCastImpl<T, T, ArrayType>
{
  using ReturnType = ArrayType;

  VTKM_CONT static ReturnType DoMake(const ArrayType& array) { return array; }
};

} // namespace detail

/// make_ArrayHandleCast is convenience function to generate an
/// ArrayHandleCast.
///
template <typename T, typename ArrayType>
VTKM_CONT
  typename detail::MakeArrayHandleCastImpl<T, typename ArrayType::ValueType, ArrayType>::ReturnType
  make_ArrayHandleCast(const ArrayType& array, const T& = T())
{
  VTKM_IS_ARRAY_HANDLE(ArrayType);
  using MakeImpl = detail::MakeArrayHandleCastImpl<T, typename ArrayType::ValueType, ArrayType>;
  return MakeImpl::DoMake(array);
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

template <typename T, typename AH>
struct SerializableTypeString<vtkm::cont::ArrayHandleCast<T, AH>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Cast<" + SerializableTypeString<T>::Get() + "," +
      SerializableTypeString<typename AH::ValueType>::Get() + "," +
      SerializableTypeString<typename AH::StorageTag>::Get() + ">";
    return name;
  }
};

template <typename T1, typename T2, typename S>
struct SerializableTypeString<vtkm::cont::ArrayHandle<T1, vtkm::cont::StorageTagCast<T2, S>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleCast<T1, vtkm::cont::ArrayHandle<T2, S>>>
{
};
}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

template <typename TargetT, typename SourceT, typename SourceStorage>
struct Serialization<
  vtkm::cont::ArrayHandle<TargetT, vtkm::cont::StorageTagCast<SourceT, SourceStorage>>>
{
private:
  using BaseType =
    vtkm::cont::ArrayHandle<TargetT, vtkm::cont::StorageTagCast<SourceT, SourceStorage>>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, obj.GetStorage().GetArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    vtkm::cont::ArrayHandle<SourceT, SourceStorage> array;
    vtkmdiy::load(bb, array);
    obj = BaseType(array);
  }
};

template <typename TargetT, typename AH>
struct Serialization<vtkm::cont::ArrayHandleCast<TargetT, AH>>
  : Serialization<vtkm::cont::ArrayHandle<
      TargetT,
      vtkm::cont::StorageTagCast<typename AH::ValueType, typename AH::StorageTag>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif // vtk_m_cont_ArrayHandleCast_h
