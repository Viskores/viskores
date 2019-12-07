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

#include <vtkm/TypeTraits.h>

#include <vtkm/internal/Variant.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

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
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  VTKM_EXEC_CONT void operator()(const PortalType& portal,
                                 vtkm::Id index,
                                 const typename PortalType::ValueType& value) const noexcept
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

  ArrayPortalMultiplexer() = default;
  ~ArrayPortalMultiplexer() = default;
  ArrayPortalMultiplexer(ArrayPortalMultiplexer&&) = default;
  ArrayPortalMultiplexer(const ArrayPortalMultiplexer&) = default;
  ArrayPortalMultiplexer& operator=(ArrayPortalMultiplexer&&) = default;
  ArrayPortalMultiplexer& operator=(const ArrayPortalMultiplexer&) = default;

  template <typename Portal>
  VTKM_EXEC_CONT ArrayPortalMultiplexer(const Portal& src) noexcept : PortalVariant(src)
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

  Storage() = default;
  Storage(Storage&&) = default;
  Storage(const Storage&) = default;
  Storage& operator=(Storage&&) = default;
  Storage& operator=(const Storage&) = default;

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

  VTKM_CONT bool IsValid() const { return this->ArrayHandleVariant.IsValid(); }

  template <typename S>
  VTKM_CONT void SetArray(vtkm::cont::ArrayHandle<ValueType, S>&& rhs)
  {
    this->ArrayHandleVariant = std::move(rhs);
  }

  template <typename S>
  VTKM_CONT void SetArray(const vtkm::cont::ArrayHandle<ValueType, S>& src)
  {
    this->ArrayHandleVariant = src;
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
    if (this->IsValid())
    {
      return this->ArrayHandleVariant.CastAndCall(detail::MultiplexerGetNumberOfValuesFunctor{});
    }
    else
    {
      return 0;
    }
  }

  VTKM_CONT void Allocate(vtkm::Id numberOfValues)
  {
    if (this->IsValid())
    {
      this->ArrayHandleVariant.CastAndCall(detail::MultiplexerAllocateFunctor{}, numberOfValues);
    }
    else if (numberOfValues > 0)
    {
      throw vtkm::cont::ErrorBadValue(
        "Attempted to allocate an ArrayHandleMultiplexer with no underlying array.");
    }
    else
    {
      // Special case, OK to perform "0" allocation on invalid array.
    }
  }

  VTKM_CONT void Shrink(vtkm::Id numberOfValues)
  {
    if (this->IsValid())
    {
      this->ArrayHandleVariant.CastAndCall(detail::MultiplexerShrinkFunctor{}, numberOfValues);
    }
    else if (numberOfValues > 0)
    {
      throw vtkm::cont::ErrorBadValue(
        "Attempted to allocate an ArrayHandleMultiplexer with no underlying array.");
    }
    else
    {
      // Special case, OK to perform "0" allocation on invalid array.
    }
  }

  VTKM_CONT void ReleaseResources()
  {
    if (this->IsValid())
    {
      this->ArrayHandleVariant.CastAndCall(detail::MultiplexerReleaseResourcesFunctor{});
    }
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

template <typename... ArrayHandleTypes>
struct ArrayHandleMultiplexerTraits
{
  using ArrayHandleType0 =
    brigand::at<brigand::list<ArrayHandleTypes...>, std::integral_constant<vtkm::IdComponent, 0>>;
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
  using CheckArrayHandle = brigand::list<CheckArrayHandleTransform<ArrayHandleTypes>...>;

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
}

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
    : Superclass(StorageType(src))
  {
  }

  template <typename RealStorageTag>
  VTKM_CONT ArrayHandleMultiplexer(vtkm::cont::ArrayHandle<ValueType, RealStorageTag>&& rhs)
    : Superclass(StorageType(std::move(rhs)))
  {
  }

  VTKM_CONT bool IsValid() const { return this->GetStorage().IsValid(); }

  template <typename S>
  VTKM_CONT void SetArray(vtkm::cont::ArrayHandle<ValueType, S>&& rhs)
  {
    this->GetStorage().SetArray(std::move(rhs));
  }

  template <typename S>
  VTKM_CONT void SetArray(const vtkm::cont::ArrayHandle<ValueType, S>& src)
  {
    this->GetStorage().SetArray(src);
  }
};

/// \brief Converts a \c vtkm::ListTag to an \c ArrayHandleMultiplexer
///
/// The argument of this template must be a vtkm::ListTag and furthermore all the types in
/// the list tag must be some type of \c ArrayHandle. The templated type gets aliased to
/// an \c ArrayHandleMultiplexer that can store any of these ArrayHandle types.
///
/// Deprecated. Use `ArrayHandleMultiplexerFromList` instead.
///
template <typename ListTag>
using ArrayHandleMultiplexerFromListTag VTKM_DEPRECATED(
  1.6,
  "vtkm::ListTag is no longer supported. Use vtkm::List instead.") =
  vtkm::ListApply<ListTag, ArrayHandleMultiplexer>;

/// \brief Converts a`vtkm::List` to an `ArrayHandleMultiplexer`
///
/// The argument of this template must be a `vtkm::List` and furthermore all the types in
/// the list tag must be some type of \c ArrayHandle. The templated type gets aliased to
/// an \c ArrayHandleMultiplexer that can store any of these ArrayHandle types.
///
template <typename List>
using ArrayHandleMultiplexerFromList = vtkm::ListApply<List, ArrayHandleMultiplexer>;

} // namespace cont

} // namespace vtkm

#endif //vtk_m_cont_ArrayHandleMultiplexer_h
