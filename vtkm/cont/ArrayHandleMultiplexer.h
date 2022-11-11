//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleMultiplexer_h
#define vtk_m_cont_ArrayHandleMultiplexer_h

#include <vtkm/Assert.h>
#include <vtkm/TypeTraits.h>

#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <vtkm/cont/Variant.h>
#include <vtkm/exec/Variant.h>

namespace vtkm
{

namespace internal
{

namespace detail
{

struct ArrayPortalMultiplexerGetNumberOfValuesFunctor
{
  template <typename PortalType>
  VTKM_EXEC_CONT vtkm::Id operator()(const PortalType& portal) const noexcept
  {
    return portal.GetNumberOfValues();
  }
};

struct ArrayPortalMultiplexerGetFunctor
{
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  VTKM_EXEC_CONT typename PortalType::ValueType operator()(const PortalType& portal,
                                                           vtkm::Id index) const noexcept
  {
    return portal.Get(index);
  }
};

struct ArrayPortalMultiplexerSetFunctor
{
  template <typename PortalType>
  VTKM_EXEC_CONT void operator()(const PortalType& portal,
                                 vtkm::Id index,
                                 const typename PortalType::ValueType& value) const noexcept
  {
    this->DoSet(
      portal, index, value, typename vtkm::internal::PortalSupportsSets<PortalType>::type{});
  }

private:
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  VTKM_EXEC_CONT void DoSet(const PortalType& portal,
                            vtkm::Id index,
                            const typename PortalType::ValueType& value,
                            std::true_type) const noexcept
  {
    portal.Set(index, value);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  VTKM_EXEC_CONT void DoSet(const PortalType&,
                            vtkm::Id,
                            const typename PortalType::ValueType&,
                            std::false_type) const noexcept
  {
    // This is an error but whatever.
    VTKM_ASSERT(false && "Calling Set on a portal that does not support it.");
  }
};

} // namespace detail

template <typename... PortalTypes>
struct ArrayPortalMultiplexer
{
  using PortalVariantType = vtkm::exec::Variant<PortalTypes...>;
  PortalVariantType PortalVariant;

  using ValueType = typename PortalVariantType::template TypeAt<0>::ValueType;

  ArrayPortalMultiplexer() = default;
  ~ArrayPortalMultiplexer() = default;
  ArrayPortalMultiplexer(ArrayPortalMultiplexer&&) = default;
  ArrayPortalMultiplexer(const ArrayPortalMultiplexer&) = default;
  ArrayPortalMultiplexer& operator=(ArrayPortalMultiplexer&&) = default;
  ArrayPortalMultiplexer& operator=(const ArrayPortalMultiplexer&) = default;

  template <typename Portal>
  VTKM_EXEC_CONT ArrayPortalMultiplexer(const Portal& src) noexcept
    : PortalVariant(src)
  {
  }

  template <typename Portal>
  VTKM_EXEC_CONT ArrayPortalMultiplexer& operator=(const Portal& src) noexcept
  {
    this->PortalVariant = src;
    return *this;
  }

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const noexcept
  {
    return this->PortalVariant.CastAndCall(
      detail::ArrayPortalMultiplexerGetNumberOfValuesFunctor{});
  }

  VTKM_EXEC_CONT ValueType Get(vtkm::Id index) const noexcept
  {
    return this->PortalVariant.CastAndCall(detail::ArrayPortalMultiplexerGetFunctor{}, index);
  }

  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const noexcept
  {
    this->PortalVariant.CastAndCall(detail::ArrayPortalMultiplexerSetFunctor{}, index, value);
  }
};

} // namespace internal

namespace cont
{

template <typename... StorageTags>
struct StorageTagMultiplexer
{
};

namespace internal
{

namespace detail
{

struct MultiplexerGetNumberOfValuesFunctor
{
  template <typename StorageType>
  VTKM_CONT vtkm::Id operator()(StorageType,
                                const std::vector<vtkm::cont::internal::Buffer>& buffers) const
  {
    return StorageType::GetNumberOfValues(buffers);
  }
};

struct MultiplexerResizeBuffersFunctor
{
  template <typename StorageType>
  VTKM_CONT void operator()(StorageType,
                            vtkm::Id numValues,
                            const std::vector<vtkm::cont::internal::Buffer>& buffers,
                            vtkm::CopyFlag preserve,
                            vtkm::cont::Token& token) const
  {
    StorageType::ResizeBuffers(numValues, buffers, preserve, token);
  }
};

struct MultiplexerFillFunctor
{
  template <typename ValueType, typename StorageType>
  VTKM_CONT void operator()(StorageType,
                            const std::vector<vtkm::cont::internal::Buffer>& buffers,
                            const ValueType& fillValue,
                            vtkm::Id startIndex,
                            vtkm::Id endIndex,
                            vtkm::cont::Token& token) const
  {
    StorageType::Fill(buffers, fillValue, startIndex, endIndex, token);
  }
};

template <typename ReadPortalType>
struct MultiplexerCreateReadPortalFunctor
{
  template <typename StorageType>
  VTKM_CONT ReadPortalType operator()(StorageType,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                      vtkm::cont::DeviceAdapterId device,
                                      vtkm::cont::Token& token) const
  {
    return ReadPortalType(StorageType::CreateReadPortal(buffers, device, token));
  }
};

template <typename WritePortalType>
struct MultiplexerCreateWritePortalFunctor
{
  template <typename StorageType>
  VTKM_CONT WritePortalType operator()(StorageType,
                                       const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                       vtkm::cont::DeviceAdapterId device,
                                       vtkm::cont::Token& token) const
  {
    return WritePortalType(StorageType::CreateWritePortal(buffers, device, token));
  }
};

template <typename T, typename... Ss>
struct MultiplexerArrayHandleVariantFunctor
{
  using VariantType = vtkm::cont::Variant<vtkm::cont::ArrayHandle<T, Ss>...>;

  template <typename StorageTag>
  VTKM_CONT VariantType operator()(vtkm::cont::internal::Storage<T, StorageTag>,
                                   const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return VariantType(vtkm::cont::ArrayHandle<T, StorageTag>(buffers));
  }
};

} // namespace detail

template <typename ValueType, typename... StorageTags>
class Storage<ValueType, StorageTagMultiplexer<StorageTags...>>
{
  template <typename S>
  using StorageFor = vtkm::cont::internal::Storage<ValueType, S>;

  using StorageVariant = vtkm::cont::Variant<StorageFor<StorageTags>...>;

  VTKM_CONT static StorageVariant Variant(const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<StorageVariant>();
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> ArrayBuffers(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return std::vector<vtkm::cont::internal::Buffer>(buffers.begin() + 1, buffers.end());
  }

public:
  using ReadPortalType =
    vtkm::internal::ArrayPortalMultiplexer<typename StorageFor<StorageTags>::ReadPortalType...>;
  using WritePortalType =
    vtkm::internal::ArrayPortalMultiplexer<typename StorageFor<StorageTags>::WritePortalType...>;

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return Variant(buffers).CastAndCall(detail::MultiplexerGetNumberOfValuesFunctor{},
                                        ArrayBuffers(buffers));
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numValues,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
                                      vtkm::CopyFlag preserve,
                                      vtkm::cont::Token& token)
  {
    Variant(buffers).CastAndCall(
      detail::MultiplexerResizeBuffersFunctor{}, numValues, ArrayBuffers(buffers), preserve, token);
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>& buffers,
                             const ValueType& fillValue,
                             vtkm::Id startIndex,
                             vtkm::Id endIndex,
                             vtkm::cont::Token& token)
  {
    Variant(buffers).CastAndCall(detail::MultiplexerFillFunctor{},
                                 ArrayBuffers(buffers),
                                 fillValue,
                                 startIndex,
                                 endIndex,
                                 token);
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return Variant(buffers).CastAndCall(
      detail::MultiplexerCreateReadPortalFunctor<ReadPortalType>{},
      ArrayBuffers(buffers),
      device,
      token);
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return Variant(buffers).CastAndCall(
      detail::MultiplexerCreateWritePortalFunctor<WritePortalType>{},
      ArrayBuffers(buffers),
      device,
      token);
  }

  VTKM_CONT static bool IsValid(const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return Variant(buffers).IsValid();
  }

  template <typename ArrayType>
  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(const ArrayType& array)
  {
    VTKM_IS_ARRAY_HANDLE(ArrayType);
    return vtkm::cont::internal::CreateBuffers(StorageVariant{ array.GetStorage() }, array);
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers()
  {
    return vtkm::cont::internal::CreateBuffers(StorageVariant{});
  }

  VTKM_CONT static
    typename detail::MultiplexerArrayHandleVariantFunctor<ValueType, StorageTags...>::VariantType
    GetArrayHandleVariant(const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return Variant(buffers).CastAndCall(
      detail::MultiplexerArrayHandleVariantFunctor<ValueType, StorageTags...>{},
      ArrayBuffers(buffers));
  }
};

} // namespace internal

namespace detail
{

template <typename... ArrayHandleTypes>
struct ArrayHandleMultiplexerTraits
{
  using ArrayHandleType0 = vtkm::ListAt<vtkm::List<ArrayHandleTypes...>, 0>;
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType0);
  using ValueType = typename ArrayHandleType0::ValueType;

  // If there is a compile error in this group of lines, then one of the array types given to
  // ArrayHandleMultiplexer must contain an invalid ArrayHandle. That could mean that it is not an
  // ArrayHandle type or it could mean that the value type does not match the appropriate value
  // type.
  template <typename ArrayHandle>
  struct CheckArrayHandleTransform
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandle);
    VTKM_STATIC_ASSERT((std::is_same<ValueType, typename ArrayHandle::ValueType>::value));
  };
  using CheckArrayHandle = vtkm::List<CheckArrayHandleTransform<ArrayHandleTypes>...>;

  // Note that this group of code could be simplified as the pair of lines:
  //   template <typename ArrayHandle>
  //   using ArrayHandleToStorageTag = typename ArrayHandle::StorageTag;
  // However, there are issues with older Visual Studio compilers that is not working
  // correctly with that form.
  template <typename ArrayHandle>
  struct ArrayHandleToStorageTagImpl
  {
    using Type = typename ArrayHandle::StorageTag;
  };
  template <typename ArrayHandle>
  using ArrayHandleToStorageTag = typename ArrayHandleToStorageTagImpl<ArrayHandle>::Type;

  using StorageTag =
    vtkm::cont::StorageTagMultiplexer<ArrayHandleToStorageTag<ArrayHandleTypes>...>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;
};
} // namespace detail

/// \brief An ArrayHandle that can behave like several other handles.
///
/// An \c ArrayHandleMultiplexer simply redirects its calls to another \c ArrayHandle. However
/// the type of that \c ArrayHandle does not need to be (completely) known at runtime. Rather,
/// \c ArrayHandleMultiplexer is defined over a set of possible \c ArrayHandle types. Any
/// one of these \c ArrayHandles may be assigned to the \c ArrayHandleMultiplexer.
///
/// When a value is retreived from the \c ArrayHandleMultiplexer, the multiplexer checks to
/// see which type of array is currently stored in it. It then redirects to the \c ArrayHandle
/// of the appropriate type.
///
/// The \c ArrayHandleMultiplexer template parameters are all the ArrayHandle types it
/// should support.
///
/// If only one template parameter is given, it is assumed to be the \c ValueType of the
/// array. A default list of supported arrays is supported (see
/// \c vtkm::cont::internal::ArrayHandleMultiplexerDefaultArrays.) If multiple template
/// parameters are given, they are all considered possible \c ArrayHandle types.
///
template <typename... ArrayHandleTypes>
class ArrayHandleMultiplexer
  : public vtkm::cont::ArrayHandle<
      typename detail::ArrayHandleMultiplexerTraits<ArrayHandleTypes...>::ValueType,
      typename detail::ArrayHandleMultiplexerTraits<ArrayHandleTypes...>::StorageTag>
{
  using Traits = detail::ArrayHandleMultiplexerTraits<ArrayHandleTypes...>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleMultiplexer,
    (ArrayHandleMultiplexer<ArrayHandleTypes...>),
    (vtkm::cont::ArrayHandle<typename Traits::ValueType, typename Traits::StorageTag>));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  template <typename RealStorageTag>
  VTKM_CONT ArrayHandleMultiplexer(const vtkm::cont::ArrayHandle<ValueType, RealStorageTag>& src)
    : Superclass(StorageType::CreateBuffers(src))
  {
  }

  VTKM_CONT bool IsValid() const { return StorageType::IsValid(this->GetBuffers()); }

  template <typename S>
  VTKM_CONT void SetArray(const vtkm::cont::ArrayHandle<ValueType, S>& src)
  {
    this->SetBuffers(StorageType::CreateBuffers(src));
  }

  VTKM_CONT auto GetArrayHandleVariant() const
    -> decltype(StorageType::GetArrayHandleVariant(this->GetBuffers()))
  {
    return StorageType::GetArrayHandleVariant(this->GetBuffers());
  }
};

/// \brief Converts a`vtkm::List` to an `ArrayHandleMultiplexer`
///
/// The argument of this template must be a `vtkm::List` and furthermore all the types in
/// the list tag must be some type of \c ArrayHandle. The templated type gets aliased to
/// an \c ArrayHandleMultiplexer that can store any of these ArrayHandle types.
///
template <typename List>
using ArrayHandleMultiplexerFromList = vtkm::ListApply<List, ArrayHandleMultiplexer>;

namespace internal
{

namespace detail
{

struct ArrayExtractComponentMultiplexerFunctor
{
  template <typename ArrayType>
  auto operator()(const ArrayType& array,
                  vtkm::IdComponent componentIndex,
                  vtkm::CopyFlag allowCopy) const
    -> decltype(vtkm::cont::ArrayExtractComponent(array, componentIndex, allowCopy))
  {
    return vtkm::cont::internal::ArrayExtractComponentImpl<typename ArrayType::StorageTag>{}(
      array, componentIndex, allowCopy);
  }
};

} // namespace detail

template <typename... Ss>
struct ArrayExtractComponentImpl<vtkm::cont::StorageTagMultiplexer<Ss...>>
{
  template <typename T>
  vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType> operator()(
    const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagMultiplexer<Ss...>>& src,
    vtkm::IdComponent componentIndex,
    vtkm::CopyFlag allowCopy)
  {
    vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandle<T, Ss>...> array(src);
    return array.GetArrayHandleVariant().CastAndCall(
      detail::ArrayExtractComponentMultiplexerFunctor{}, componentIndex, allowCopy);
  }
};

} // namespace internal

} // namespace cont

} // namespace vtkm

#endif //vtk_m_cont_ArrayHandleMultiplexer_h
