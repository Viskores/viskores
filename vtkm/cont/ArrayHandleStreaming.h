//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleStreaming_h
#define vtk_m_cont_ArrayHandleStreaming_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortal.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename P>
class VTKM_ALWAYS_EXPORT ArrayPortalStreaming
{
  using Writable = vtkm::internal::PortalSupportsSets<P>;

public:
  using PortalType = P;
  using ValueType = typename PortalType::ValueType;

  VTKM_EXEC_CONT
  ArrayPortalStreaming()
    : InputPortal()
    , BlockIndex(0)
    , BlockSize(0)
    , CurBlockSize(0)
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalStreaming(const PortalType& inputPortal,
                       vtkm::Id blockIndex,
                       vtkm::Id blockSize,
                       vtkm::Id curBlockSize)
    : InputPortal(inputPortal)
    , BlockIndex(blockIndex)
    , BlockSize(blockSize)
    , CurBlockSize(curBlockSize)
  {
  }

  template <typename OtherP>
  VTKM_EXEC_CONT ArrayPortalStreaming(const ArrayPortalStreaming<OtherP>& src)
    : InputPortal(src.GetPortal())
    , BlockIndex(src.GetBlockIndex())
    , BlockSize(src.GetBlockSize())
    , CurBlockSize(src.GetCurBlockSize())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->CurBlockSize; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    return this->InputPortal.Get(this->BlockIndex * this->BlockSize + index);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    this->InputPortal.Set(this->BlockIndex * this->BlockSize + index, value);
  }

  VTKM_EXEC_CONT
  const PortalType& GetPortal() const { return this->InputPortal; }

  VTKM_EXEC_CONT
  void SetBlockSize(vtkm::Id blockSize) { this->BlockSize = blockSize; }

  VTKM_EXEC_CONT
  void SetBlockIndex(vtkm::Id blockIndex) { this->BlockIndex = blockIndex; }

  VTKM_EXEC_CONT
  void SetCurBlockSize(vtkm::Id curBlockSize) { this->CurBlockSize = curBlockSize; }

  VTKM_EXEC_CONT
  vtkm::Id GetBlockSize() { return this->BlockSize; }

  VTKM_EXEC_CONT
  vtkm::Id GetBlockIndex() { return this->BlockIndex; }

  VTKM_EXEC_CONT
  vtkm::Id GetCurBlockSize() { return this->CurBlockSize; }

private:
  PortalType InputPortal;
  vtkm::Id BlockIndex;
  vtkm::Id BlockSize;
  vtkm::Id CurBlockSize;
};

} // internal

template <typename ArrayHandleInputType>
struct VTKM_ALWAYS_EXPORT StorageTagStreaming
{
};

namespace internal
{

template <typename ArrayHandleInputType>
class Storage<typename ArrayHandleInputType::ValueType, StorageTagStreaming<ArrayHandleInputType>>
{
public:
  using ValueType = typename ArrayHandleInputType::ValueType;

  using PortalType =
    vtkm::cont::internal::ArrayPortalStreaming<typename ArrayHandleInputType::PortalControl>;
  using PortalConstType =
    vtkm::cont::internal::ArrayPortalStreaming<typename ArrayHandleInputType::PortalConstControl>;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  VTKM_CONT
  Storage(const ArrayHandleInputType inputArray,
          vtkm::Id blockSize,
          vtkm::Id blockIndex,
          vtkm::Id curBlockSize)
    : InputArray(inputArray)
    , BlockSize(blockSize)
    , BlockIndex(blockIndex)
    , CurBlockSize(curBlockSize)
    , Valid(true)
  {
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->Valid);
    return PortalType(this->InputArray.GetPortalControl(), BlockSize, BlockIndex, CurBlockSize);
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    return PortalConstType(
      this->InputArray.GetPortalConstControl(), BlockSize, BlockIndex, CurBlockSize);
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    return CurBlockSize;
  }

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues) const
  {
    (void)numberOfValues;
    // Do nothing, since we only allocate a streaming array once at the beginning
  }

  VTKM_CONT
  void AllocateFullArray(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    this->InputArray.Allocate(numberOfValues);
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    this->InputArray.Shrink(numberOfValues);
  }

  VTKM_CONT
  void ReleaseResources()
  {
    VTKM_ASSERT(this->Valid);
    this->InputArray.ReleaseResources();
  }

  VTKM_CONT
  const ArrayHandleInputType& GetArray() const
  {
    VTKM_ASSERT(this->Valid);
    return this->InputArray;
  }

private:
  ArrayHandleInputType InputArray;
  vtkm::Id BlockSize;
  vtkm::Id BlockIndex;
  vtkm::Id CurBlockSize;
  bool Valid;
};
}
}
}

namespace vtkm
{
namespace cont
{

template <typename ArrayHandleInputType>
class ArrayHandleStreaming
  : public vtkm::cont::ArrayHandle<typename ArrayHandleInputType::ValueType,
                                   StorageTagStreaming<ArrayHandleInputType>>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleStreaming,
                             (ArrayHandleStreaming<ArrayHandleInputType>),
                             (vtkm::cont::ArrayHandle<typename ArrayHandleInputType::ValueType,
                                                      StorageTagStreaming<ArrayHandleInputType>>));

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleStreaming(const ArrayHandleInputType& inputArray,
                       const vtkm::Id blockIndex,
                       const vtkm::Id blockSize,
                       const vtkm::Id curBlockSize)
    : Superclass(StorageType(inputArray, blockIndex, blockSize, curBlockSize))
  {
    this->GetPortalConstControl().SetBlockIndex(blockIndex);
    this->GetPortalConstControl().SetBlockSize(blockSize);
    this->GetPortalConstControl().SetCurBlockSize(curBlockSize);
  }

  VTKM_CONT
  void AllocateFullArray(vtkm::Id numberOfValues)
  {
    auto lock = this->GetLock();

    this->ReleaseResourcesExecutionInternal(lock);
    this->Internals->GetControlArray(lock)->AllocateFullArray(numberOfValues);
    this->Internals->SetControlArrayValid(lock, true);
  }
};
}
}

#endif //vtk_m_cont_ArrayHandleStreaming_h
