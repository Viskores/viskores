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

#include <vtkm/TypeListTag.h>
#include <vtkm/TypeTraits.h>

#include <vtkm/internal/Variant.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/StorageListTag.h>

namespace vtkm
{

namespace internal
{

namespace detail
{

struct ArrayPortalMultiplexerGetNumberOfValuesFunctor
{
  template <typename PortalType>
  VTKM_EXEC_CONT vtkm::Id operator()(const PortalType& portal) const
  {
    return portal.GetNumberOfValues();
  }
};

struct ArrayPortalMultiplexerGetFunctor
{
  template <typename PortalType>
  VTKM_EXEC_CONT typename PortalType::ValueType operator()(const PortalType& portal,
                                                           vtkm::Id index) const
  {
    return portal.Get(index);
  }
};

struct ArrayPortalMultiplexerSetFunctor
{
  template <typename PortalType>
  VTKM_EXEC_CONT void operator()(const PortalType& portal,
                                 vtkm::Id index,
                                 const typename PortalType::ValueType& value) const
  {
    portal.Set(index, value);
  }
};

} // namespace detail

template <typename... PortalTypes>
struct ArrayPortalMultiplexer
{
  using PortalVariantType = vtkm::internal::Variant<PortalTypes...>;
  PortalVariantType PortalVariant;

  using ValueType = typename PortalVariantType::template TypeAt<0>::ValueType;

  VTKM_EXEC_CONT ArrayPortalMultiplexer() = default;
  VTKM_EXEC_CONT ~ArrayPortalMultiplexer() = default;
  VTKM_EXEC_CONT ArrayPortalMultiplexer(ArrayPortalMultiplexer&&) = default;
  VTKM_EXEC_CONT ArrayPortalMultiplexer(const ArrayPortalMultiplexer&) = default;
  VTKM_EXEC_CONT ArrayPortalMultiplexer& operator=(ArrayPortalMultiplexer&&) = default;
  VTKM_EXEC_CONT ArrayPortalMultiplexer& operator=(const ArrayPortalMultiplexer&) = default;

  template <typename Portal>
  VTKM_EXEC_CONT ArrayPortalMultiplexer(const Portal& src)
    : PortalVariant(src)
  {
  }

  template <typename Portal>
  VTKM_EXEC_CONT ArrayPortalMultiplexer& operator=(const Portal& src)
  {
    this->PortalVariant = src;
    return *this;
  }

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const
  {
    return this->PortalVariant.CastAndCall(
      detail::ArrayPortalMultiplexerGetNumberOfValuesFunctor{});
  }

  VTKM_EXEC_CONT ValueType Get(vtkm::Id index) const
  {
    return this->PortalVariant.CastAndCall(detail::ArrayPortalMultiplexerGetFunctor{}, index);
  }

  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
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
  template <typename ArrayHandleType>
  VTKM_CONT vtkm::Id operator()(ArrayHandleType&& array) const
  {
    return array.GetNumberOfValues();
  }
};

struct MultiplexerAllocateFunctor
{
  template <typename ArrayHandleType>
  VTKM_CONT void operator()(ArrayHandleType&& array, vtkm::Id numberOfValues) const
  {
    array.Allocate(numberOfValues);
  }
};

struct MultiplexerShrinkFunctor
{
  template <typename ArrayHandleType>
  VTKM_CONT void operator()(ArrayHandleType&& array, vtkm::Id numberOfValues) const
  {
    array.Shrink(numberOfValues);
  }
};

struct MultiplexerReleaseResourcesFunctor
{
  template <typename ArrayHandleType>
  VTKM_CONT vtkm::Id operator()(ArrayHandleType&& array) const
  {
    return array.ReleaseResources();
  }
};

struct MultiplexerReleaseResourcesExecutionFunctor
{
  template <typename ArrayHandleType>
  VTKM_CONT void operator()(ArrayHandleType&& array) const
  {
    array.ReleaseResourcesExecution();
  }
};

} // namespace detail

template <typename ValueType_, typename... StorageTags>
class Storage<ValueType_, StorageTagMultiplexer<StorageTags...>>
{
public:
  using ValueType = ValueType_;

private:
  template <typename S>
  using StorageToArrayHandle = vtkm::cont::ArrayHandle<ValueType, S>;

  template <typename S>
  using StorageToPortalControl = typename StorageToArrayHandle<S>::PortalControl;

  template <typename S>
  using StorageToPortalConstControl = typename StorageToArrayHandle<S>::PortalConstControl;

  using ArrayHandleVariantType = vtkm::internal::Variant<StorageToArrayHandle<StorageTags>...>;
  ArrayHandleVariantType ArrayHandleVariant;

public:
  using PortalType = vtkm::internal::ArrayPortalMultiplexer<StorageToPortalControl<StorageTags>...>;
  using PortalConstType =
    vtkm::internal::ArrayPortalMultiplexer<StorageToPortalConstControl<StorageTags>...>;

  VTKM_CONT Storage() = default;
  VTKM_CONT Storage(Storage&&) = default;
  VTKM_CONT Storage(const Storage&) = default;
  VTKM_CONT Storage& operator=(Storage&&) = default;
  VTKM_CONT Storage& operator=(const Storage&) = default;

  template <typename S>
  VTKM_CONT Storage(vtkm::cont::ArrayHandle<ValueType, S>&& rhs)
    : ArrayHandleVariant(std::move(rhs))
  {
  }

  template <typename S>
  VTKM_CONT Storage(const vtkm::cont::ArrayHandle<ValueType, S>& src)
    : ArrayHandleVariant(src)
  {
  }

private:
  struct GetPortalFunctor
  {
    template <typename ArrayHandleType>
    VTKM_CONT PortalType operator()(ArrayHandleType&& array) const
    {
      return PortalType(array.GetPortalControl());
    }
  };

  struct GetPortalConstFunctor
  {
    template <typename ArrayHandleType>
    VTKM_CONT PortalConstType operator()(ArrayHandleType&& array) const
    {
      return PortalConstType(array.GetPortalConstControl());
    }
  };

public:
  VTKM_CONT PortalType GetPortal()
  {
    return this->ArrayHandleVariant.CastAndCall(GetPortalFunctor{});
  }

  VTKM_CONT PortalConstType GetPortalConst() const
  {
    return this->ArrayHandleVariant.CastAndCall(GetPortalConstFunctor{});
  }

  VTKM_CONT vtkm::Id GetNumberOfValues() const
  {
    return this->ArrayHandleVariant.CastAndCall(detail::MultiplexerGetNumberOfValuesFunctor{});
  }

  VTKM_CONT void Allocate(vtkm::Id numberOfValues)
  {
    this->ArrayHandleVariant.CastAndCall(detail::MultiplexerAllocateFunctor{}, numberOfValues);
  }

  VTKM_CONT void Shrink(vtkm::Id numberOfValues)
  {
    this->ArrayHandleVariant.CastAndCall(detail::MultiplexerShrinkFunctor{}, numberOfValues);
  }

  VTKM_CONT void ReleaseResources()
  {
    this->ArrayHandleVariant.CastAndCall(detail::MultiplexerReleaseResourcesFunctor{});
  }

  VTKM_CONT ArrayHandleVariantType& GetArrayHandleVariant() { return this->ArrayHandleVariant; }
};


template <typename ValueType_, typename... StorageTags, typename Device>
class ArrayTransfer<ValueType_, StorageTagMultiplexer<StorageTags...>, Device>
{
public:
  using ValueType = ValueType_;

private:
  using StorageTag = StorageTagMultiplexer<StorageTags...>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

  template <typename S>
  using StorageToArrayHandle = vtkm::cont::ArrayHandle<ValueType, S>;

  template <typename S>
  using StorageToPortalExecution =
    typename StorageToArrayHandle<S>::template ExecutionTypes<Device>::Portal;

  template <typename S>
  using StorageToPortalConstExecution =
    typename StorageToArrayHandle<S>::template ExecutionTypes<Device>::PortalConst;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution =
    vtkm::internal::ArrayPortalMultiplexer<StorageToPortalExecution<StorageTags>...>;
  using PortalConstExecution =
    vtkm::internal::ArrayPortalMultiplexer<StorageToPortalConstExecution<StorageTags>...>;

private:
  StorageType* StoragePointer;

public:
  VTKM_CONT ArrayTransfer(StorageType* storage)
    : StoragePointer(storage)
  {
  }

  VTKM_CONT vtkm::Id GetNumberOfValues() const { return this->StoragePointer->GetNumberOfValues(); }

  VTKM_CONT PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return this->StoragePointer->GetArrayHandleVariant().CastAndCall(PrepareForInputFunctor{});
  }

  VTKM_CONT PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return this->StoragePointer->GetArrayHandleVariant().CastAndCall(PrepareForInPlaceFunctor{});
  }

  VTKM_CONT PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return this->StoragePointer->GetArrayHandleVariant().CastAndCall(PrepareForOutputFunctor{},
                                                                     numberOfValues);
  }

  VTKM_CONT void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handles should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT void Shrink(vtkm::Id numberOfValues)
  {
    this->StoragePointer->GetArrayHandleVariant().CastAndCall(detail::MultiplexerShrinkFunctor{},
                                                              numberOfValues);
  }

  VTKM_CONT void ReleaseResources()
  {
    this->StoragePointer->GetArrayHandleVariant().CastAndCall(
      detail::MultiplexerReleaseResourcesExecutionFunctor{});
  }

private:
  struct PrepareForInputFunctor
  {
    template <typename ArrayHandleType>
    VTKM_CONT PortalConstExecution operator()(const ArrayHandleType& array)
    {
      return PortalConstExecution(array.PrepareForInput(Device{}));
    }
  };

  struct PrepareForInPlaceFunctor
  {
    template <typename ArrayHandleType>
    VTKM_CONT PortalExecution operator()(ArrayHandleType& array)
    {
      return PortalExecution(array.PrepareForInPlace(Device{}));
    }
  };

  struct PrepareForOutputFunctor
  {
    template <typename ArrayHandleType>
    VTKM_CONT PortalExecution operator()(ArrayHandleType& array, vtkm::Id numberOfValues)
    {
      return PortalExecution(array.PrepareForOutput(numberOfValues, Device{}));
    }
  };
};

} // namespace internal

namespace detail
{

template <typename ValueType, typename... ArrayHandleTypes>
struct ArrayHandleMultiplexerTraits
{
  // If there is a compile error in this group of lines, then the list tag given to
  // ArrayHandleMultiplexer must contain an invalid ArrayHandle. That could mean that
  // it is not an ArrayHandle type or it could mean that the value type does not match
  // the appropriate value type.
  template <typename ArrayHandle>
  struct CheckArrayHandleTransform
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandle);
    VTKM_STATIC_ASSERT((std::is_same<ValueType, typename ArrayHandle::ValueType>::value));
  };
  using CheckArrayHandle = brigand::list<CheckArrayHandleTransform<ArrayHandleTypes>...>;

  template <typename ArrayHandle>
  using ArrayHandleToStorageTag = typename ArrayHandle::StorageTag;

  using StorageTag =
    vtkm::cont::StorageTagMultiplexer<ArrayHandleToStorageTag<ArrayHandleTypes>...>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;
};
}

/// \brief Base implementation of \c ArrayHandleMultiplexer.
///
/// This behavies the same as \c ArrayHandleMultiplexer, but the template parameters are
/// more explicit. The first template parameter must be the \c ValueType of the array.
/// The remaining template parameters are the array handles to support.
///
template <typename ValueType_, typename... ArrayHandleTypes>
class ArrayHandleMultiplexerBase
  : public vtkm::cont::ArrayHandle<
      ValueType_,
      typename detail::ArrayHandleMultiplexerTraits<ValueType_, ArrayHandleTypes...>::StorageTag>
{
  using Traits = detail::ArrayHandleMultiplexerTraits<ValueType_, ArrayHandleTypes...>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleMultiplexerBase,
                             (ArrayHandleMultiplexerBase<ValueType_, ArrayHandleTypes...>),
                             (vtkm::cont::ArrayHandle<ValueType_, typename Traits::StorageTag>));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  template <typename RealStorageTag>
  VTKM_CONT ArrayHandleMultiplexerBase(
    const vtkm::cont::ArrayHandle<ValueType, RealStorageTag>& src)
    : Superclass(StorageType(src))
  {
  }

  template <typename RealStorageTag>
  VTKM_CONT ArrayHandleMultiplexerBase(vtkm::cont::ArrayHandle<ValueType, RealStorageTag>&& rhs)
    : Superclass(StorageType(std::move(rhs)))
  {
  }
};

namespace internal
{

namespace detail
{

template <typename ValueType>
struct MakeArrayListFromStorage
{
  template <typename S>
  using Transform = vtkm::cont::ArrayHandle<ValueType, S>;
};

template <typename ValueType>
struct SupportedArrays
  : vtkm::ListTagTransform<vtkm::cont::StorageListTagSupported,
                           MakeArrayListFromStorage<ValueType>::template Transform>
{
};

template <typename DestType, typename SrcTypeList>
struct MakeCastArrayListImpl
{
  using TypeStoragePairs = vtkm::ListCrossProduct<SrcTypeList, vtkm::cont::StorageListTagSupported>;

  template <typename Pair>
  struct PairToCastArrayImpl;
  template <typename T, typename S>
  struct PairToCastArrayImpl<brigand::list<T, S>>
  {
    using Type = vtkm::cont::ArrayHandleCast<DestType, vtkm::cont::ArrayHandle<T, S>>;
  };
  template <typename Pair>
  using PairToCastArray = typename PairToCastArrayImpl<Pair>::Type;

  using Type = vtkm::ListTagTransform<TypeStoragePairs, PairToCastArray>;
};

template <typename DestType>
struct MakeCastArrayList
{
  using Type = typename MakeCastArrayListImpl<DestType, vtkm::TypeListTagScalarAll>::Type;
};

template <typename ComponentType, vtkm::IdComponent N>
struct MakeCastArrayList<vtkm::Vec<ComponentType, N>>
{
  template <typename T>
  using ScalarToVec = vtkm::Vec<T, N>;
  using SourceTypes = vtkm::ListTagTransform<vtkm::TypeListTagScalarAll, ScalarToVec>;

  using Type = typename MakeCastArrayListImpl<vtkm::Vec<ComponentType, N>, SourceTypes>::Type;
};

template <typename T>
struct ArrayHandleMultiplexerDefaultArraysBase
  : vtkm::ListTagJoin<SupportedArrays<T>, typename MakeCastArrayList<T>::Type>
{
};

} // namespace detail

template <typename T>
struct ArrayHandleMultiplexerDefaultArrays : detail::ArrayHandleMultiplexerDefaultArraysBase<T>
{
};

template <>
struct ArrayHandleMultiplexerDefaultArrays<vtkm::Vec<vtkm::FloatDefault, 3>>
  : vtkm::ListTagJoin<
      detail::ArrayHandleMultiplexerDefaultArraysBase<vtkm::Vec<vtkm::FloatDefault, 3>>,
      vtkm::ListTagBase<
#if 1
        vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                                vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                                vtkm::cont::ArrayHandle<vtkm::FloatDefault>>,
#endif
        vtkm::cont::ArrayHandleUniformPointCoordinates>>
{
};

} // namespace internal

namespace detail
{

template <typename ValueType, typename ListTagArrays>
struct ArrayHandleMultiplexerChooseBaseImpl
{
  VTKM_IS_LIST_TAG(ListTagArrays);

  template <typename BrigandListArrays>
  struct BrigandListArraysToArrayHandleMultiplexerBase;

  template <typename... ArrayHandleTypes>
  struct BrigandListArraysToArrayHandleMultiplexerBase<brigand::list<ArrayHandleTypes...>>
  {
    using Type = vtkm::cont::ArrayHandleMultiplexerBase<ValueType, ArrayHandleTypes...>;
  };

  using Type = typename BrigandListArraysToArrayHandleMultiplexerBase<
    vtkm::internal::ListTagAsBrigandList<ListTagArrays>>::Type;
};

template <typename ValueType>
using ArrayHandleMultiplexerChooseBase = typename ArrayHandleMultiplexerChooseBaseImpl<
  ValueType,
  internal::ArrayHandleMultiplexerDefaultArrays<ValueType>>::Type;

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
/// If only one template parameter is given, it is assumed to be the \c ValueType of the
/// array. A default list of supported arrays is supported (see
/// \c vtkm::cont::internal::ArrayHandleMultiplexerDefaultArrays.) If multiple template
/// parameters are given, they are all considered possible \c ArrayHandle types.
///
template <typename... Ts>
class ArrayHandleMultiplexer;

template <typename ValueType_>
class ArrayHandleMultiplexer<ValueType_>
  : public detail::ArrayHandleMultiplexerChooseBase<ValueType_>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleMultiplexer,
                             (ArrayHandleMultiplexer<ValueType_>),
                             (detail::ArrayHandleMultiplexerChooseBase<ValueType_>));

  template <typename RealT, typename RealStorageTag>
  VTKM_CONT ArrayHandleMultiplexer(const vtkm::cont::ArrayHandle<RealT, RealStorageTag>& src)
    : Superclass(src)
  {
  }

  template <typename RealT, typename RealStorageTag>
  VTKM_CONT ArrayHandleMultiplexer(vtkm::cont::ArrayHandle<RealT, RealStorageTag>&& rhs)
    : Superclass(std::move(rhs))
  {
  }
};

template <typename ArrayType0, typename... ArrayTypes>
class ArrayHandleMultiplexer<ArrayType0, ArrayTypes...>
  : public vtkm::cont::ArrayHandleMultiplexerBase<typename ArrayType0::ValueType,
                                                  ArrayType0,
                                                  ArrayTypes...>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleMultiplexer,
                             (ArrayHandleMultiplexer<ArrayType0, ArrayTypes...>),
                             (vtkm::cont::ArrayHandleMultiplexerBase<typename ArrayType0::ValueType,
                                                                     ArrayType0,
                                                                     ArrayTypes...>));

  template <typename RealT, typename RealStorageTag>
  VTKM_CONT ArrayHandleMultiplexer(const vtkm::cont::ArrayHandle<RealT, RealStorageTag>& src)
    : Superclass(src)
  {
  }

  template <typename RealT, typename RealStorageTag>
  VTKM_CONT ArrayHandleMultiplexer(vtkm::cont::ArrayHandle<RealT, RealStorageTag>&& rhs)
    : Superclass(std::move(rhs))
  {
  }
};

} // namespace cont

} // namespace vtkm

#endif //vtk_m_cont_ArrayHandleMultiplexer_h
