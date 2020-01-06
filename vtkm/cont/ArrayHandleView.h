//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleView_h
#define vtk_m_cont_ArrayHandleView_h

#include <vtkm/Assert.h>
#include <vtkm/Deprecated.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortal.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

template <typename TargetPortalType>
class ArrayPortalView
{
  using Writable = vtkm::internal::PortalSupportsSets<TargetPortalType>;

public:
  using ValueType = typename TargetPortalType::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalView() {}

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalView(const TargetPortalType& targetPortal, vtkm::Id startIndex, vtkm::Id numValues)
    : TargetPortal(targetPortal)
    , StartIndex(startIndex)
    , NumValues(numValues)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OtherPortalType>
  VTKM_EXEC_CONT ArrayPortalView(const ArrayPortalView<OtherPortalType>& otherPortal)
    : TargetPortal(otherPortal.GetTargetPortal())
    , StartIndex(otherPortal.GetStartIndex())
    , NumValues(otherPortal.GetNumberOfValues())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumValues; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return this->TargetPortal.Get(index + this->StartIndex); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    this->TargetPortal.Set(index + this->StartIndex, value);
  }

  VTKM_EXEC_CONT
  const TargetPortalType& GetTargetPortal() const { return this->TargetPortal; }
  VTKM_EXEC_CONT
  vtkm::Id GetStartIndex() const { return this->StartIndex; }

private:
  TargetPortalType TargetPortal;
  vtkm::Id StartIndex;
  vtkm::Id NumValues;
};

} // namespace internal

template <typename StorageTag>
struct VTKM_ALWAYS_EXPORT StorageTagView
{
};

namespace internal
{

namespace detail
{

template <typename T, typename ArrayOrStorage, bool IsArrayType>
struct ViewTypeArgImpl;

template <typename T, typename Storage>
struct ViewTypeArgImpl<T, Storage, false>
{
  using StorageTag = Storage;
  using ArrayHandle = vtkm::cont::ArrayHandle<T, StorageTag>;
};

template <typename T, typename Array>
struct ViewTypeArgImpl<T, Array, true>
{
  VTKM_STATIC_ASSERT_MSG((std::is_same<T, typename Array::ValueType>::value),
                         "Used array with wrong type in ArrayHandleView.");
  using StorageTag VTKM_DEPRECATED(1.6,
                                   "Use storage tag instead of array handle in StorageTagView.") =
    typename Array::StorageTag;
  using ArrayHandle VTKM_DEPRECATED(1.6,
                                    "Use storage tag instead of array handle in StorageTagView.") =
    vtkm::cont::ArrayHandle<T, typename Array::StorageTag>;
};

template <typename T, typename ArrayOrStorage>
struct ViewTypeArg
  : ViewTypeArgImpl<T,
                    ArrayOrStorage,
                    vtkm::cont::internal::ArrayHandleCheck<ArrayOrStorage>::type::value>
{
};

} // detail

template <typename T, typename ST>
class Storage<T, StorageTagView<ST>>
{
  using ArrayHandleType = typename detail::ViewTypeArg<T, ST>::ArrayHandle;

public:
  using ValueType = T;

  using PortalType = ArrayPortalView<typename ArrayHandleType::PortalControl>;
  using PortalConstType = ArrayPortalView<typename ArrayHandleType::PortalConstControl>;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  VTKM_CONT
  Storage(const ArrayHandleType& array, vtkm::Id startIndex, vtkm::Id numValues)
    : Array(array)
    , StartIndex(startIndex)
    , NumValues(numValues)
    , Valid(true)
  {
    VTKM_ASSERT(this->StartIndex >= 0);
    VTKM_ASSERT((this->StartIndex + this->NumValues) <= this->Array.GetNumberOfValues());
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->Valid);
    return PortalType(this->Array.GetPortalControl(), this->StartIndex, this->NumValues);
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    return PortalConstType(this->Array.GetPortalConstControl(), this->StartIndex, this->NumValues);
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumValues; }

  VTKM_CONT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorInternal("ArrayHandleView should not be allocated explicitly. ");
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    if (numberOfValues > this->NumValues)
    {
      throw vtkm::cont::ErrorBadValue("Shrink method cannot be used to grow array.");
    }

    this->NumValues = numberOfValues;
  }

  VTKM_CONT
  void ReleaseResources()
  {
    VTKM_ASSERT(this->Valid);
    this->Array.ReleaseResources();
  }

  // Required for later use in ArrayTransfer class.
  VTKM_CONT
  const ArrayHandleType& GetArray() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Array;
  }
  VTKM_CONT
  vtkm::Id GetStartIndex() const { return this->StartIndex; }

private:
  ArrayHandleType Array;
  vtkm::Id StartIndex;
  vtkm::Id NumValues;
  bool Valid;
};

template <typename T, typename ST, typename Device>
class ArrayTransfer<T, StorageTagView<ST>, Device>
{
private:
  using StorageType = vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagView<ST>>;
  using ArrayHandleType = typename detail::ViewTypeArg<T, ST>::ArrayHandle;

public:
  using ValueType = T;
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution =
    ArrayPortalView<typename ArrayHandleType::template ExecutionTypes<Device>::Portal>;
  using PortalConstExecution =
    ArrayPortalView<typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
    , StartIndex(storage->GetStartIndex())
    , NumValues(storage->GetNumberOfValues())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumValues; }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(
      this->Array.PrepareForInput(Device()), this->StartIndex, this->NumValues);
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution(
      this->Array.PrepareForInPlace(Device()), this->StartIndex, this->NumValues);
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    if (numberOfValues != this->GetNumberOfValues())
    {
      throw vtkm::cont::ErrorBadValue(
        "An ArrayHandleView can be used as an output array, "
        "but it cannot be resized. Make sure the index array is sized "
        "to the appropriate length before trying to prepare for output.");
    }

    // We cannot practically allocate ValueArray because we do not know the
    // range of indices. We try to check by seeing if ValueArray has no
    // entries, which clearly indicates that it is not allocated. Otherwise,
    // we have to assume the allocation is correct.
    if ((numberOfValues > 0) && (this->Array.GetNumberOfValues() < 1))
    {
      throw vtkm::cont::ErrorBadValue(
        "The value array must be pre-allocated before it is used for the "
        "output of ArrayHandlePermutation.");
    }

    return PortalExecution(this->Array.PrepareForOutput(this->Array.GetNumberOfValues(), Device()),
                           this->StartIndex,
                           this->NumValues);
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // No implementation necessary
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->NumValues = numberOfValues; }

  VTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }

private:
  ArrayHandleType Array;
  vtkm::Id StartIndex;
  vtkm::Id NumValues;
};

} // namespace internal

template <typename ArrayHandleType>
class ArrayHandleView
  : public vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                   StorageTagView<typename ArrayHandleType::StorageTag>>
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleView,
    (ArrayHandleView<ArrayHandleType>),
    (vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                             StorageTagView<typename ArrayHandleType::StorageTag>>));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleView(const ArrayHandleType& array, vtkm::Id startIndex, vtkm::Id numValues)
    : Superclass(StorageType(array, startIndex, numValues))
  {
  }
};

template <typename ArrayHandleType>
ArrayHandleView<ArrayHandleType> make_ArrayHandleView(const ArrayHandleType& array,
                                                      vtkm::Id startIndex,
                                                      vtkm::Id numValues)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  return ArrayHandleView<ArrayHandleType>(array, startIndex, numValues);
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleView_h
