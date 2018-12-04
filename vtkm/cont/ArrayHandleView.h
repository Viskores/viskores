//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleView_h
#define vtk_m_cont_ArrayHandleView_h

#include <vtkm/Assert.h>

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
  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const
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

template <typename ArrayHandleType>
struct StorageTagView
{
};

namespace internal
{

template <typename ArrayHandleType>
class Storage<typename ArrayHandleType::ValueType, StorageTagView<ArrayHandleType>>
{
public:
  using ValueType = typename ArrayHandleType::ValueType;

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

template <typename ArrayHandleType, typename Device>
class ArrayTransfer<typename ArrayHandleType::ValueType, StorageTagView<ArrayHandleType>, Device>
{
public:
  using ValueType = typename ArrayHandleType::ValueType;

private:
  using StorageTag = StorageTagView<ArrayHandleType>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
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
class ArrayHandleView : public vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                                       StorageTagView<ArrayHandleType>>
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleView,
                             (ArrayHandleView<ArrayHandleType>),
                             (vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                                      StorageTagView<ArrayHandleType>>));

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
