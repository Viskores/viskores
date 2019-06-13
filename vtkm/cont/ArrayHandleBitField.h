//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleBitField_h
#define vtk_m_cont_ArrayHandleBitField_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/BitField.h>
#include <vtkm/cont/Storage.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

template <typename BitPortalType>
class ArrayPortalBitField
{
public:
  using ValueType = bool;

  VTKM_EXEC_CONT
  explicit ArrayPortalBitField(const BitPortalType& portal) noexcept : BitPortal{ portal } {}

  VTKM_EXEC_CONT
  explicit ArrayPortalBitField(BitPortalType&& portal) noexcept : BitPortal{ std::move(portal) } {}

  ArrayPortalBitField() noexcept = default;
  ArrayPortalBitField(const ArrayPortalBitField&) noexcept = default;
  ArrayPortalBitField(ArrayPortalBitField&&) noexcept = default;
  ArrayPortalBitField& operator=(const ArrayPortalBitField&) noexcept = default;
  ArrayPortalBitField& operator=(ArrayPortalBitField&&) noexcept = default;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const noexcept { return this->BitPortal.GetNumberOfBits(); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const noexcept { return this->BitPortal.GetBit(index); }

  VTKM_EXEC_CONT
  void Set(vtkm::Id index, ValueType value) const
  {
    // Use an atomic set so we don't clash with other threads writing nearby
    // bits.
    this->BitPortal.SetBitAtomic(index, value);
  }

private:
  BitPortalType BitPortal;
};

struct VTKM_ALWAYS_EXPORT StorageTagBitField
{
};

template <>
class Storage<bool, StorageTagBitField>
{
  using BitPortalType = vtkm::cont::detail::BitPortal<vtkm::cont::internal::AtomicInterfaceControl>;
  using BitPortalConstType =
    vtkm::cont::detail::BitPortalConst<vtkm::cont::internal::AtomicInterfaceControl>;

public:
  using ValueType = bool;
  using PortalType = vtkm::cont::internal::ArrayPortalBitField<BitPortalType>;
  using PortalConstType = vtkm::cont::internal::ArrayPortalBitField<BitPortalConstType>;

  explicit VTKM_CONT Storage(const vtkm::cont::BitField& data)
    : Data{ data }
  {
  }

  explicit VTKM_CONT Storage(vtkm::cont::BitField&& data) noexcept : Data{ std::move(data) } {}

  VTKM_CONT Storage() = default;
  VTKM_CONT Storage(const Storage&) = default;
  VTKM_CONT Storage(Storage&&) noexcept = default;
  VTKM_CONT Storage& operator=(const Storage&) = default;
  VTKM_CONT Storage& operator=(Storage&&) noexcept = default;

  VTKM_CONT
  PortalType GetPortal() { return PortalType{ this->Data.GetPortalControl() }; }

  VTKM_CONT
  PortalConstType GetPortalConst() { return PortalConstType{ this->Data.GetPortalConstControl() }; }

  VTKM_CONT vtkm::Id GetNumberOfValues() const { return this->Data.GetNumberOfBits(); }
  VTKM_CONT void Allocate(vtkm::Id numberOfValues) { this->Data.Allocate(numberOfValues); }
  VTKM_CONT void Shrink(vtkm::Id numberOfValues) { this->Data.Shrink(numberOfValues); }
  VTKM_CONT void ReleaseResources() { this->Data.ReleaseResources(); }

  VTKM_CONT vtkm::cont::BitField GetBitField() const { return this->Data; }

private:
  vtkm::cont::BitField Data;
};

template <typename Device>
class ArrayTransfer<bool, StorageTagBitField, Device>
{
  using AtomicInterface = AtomicInterfaceExecution<Device>;
  using StorageType = Storage<bool, StorageTagBitField>;
  using BitPortalExecution = vtkm::cont::detail::BitPortal<AtomicInterface>;
  using BitPortalConstExecution = vtkm::cont::detail::BitPortalConst<AtomicInterface>;

public:
  using ValueType = bool;
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;
  using PortalExecution = vtkm::cont::internal::ArrayPortalBitField<BitPortalExecution>;
  using PortalConstExecution = vtkm::cont::internal::ArrayPortalBitField<BitPortalConstExecution>;

  VTKM_CONT
  explicit ArrayTransfer(StorageType* storage)
    : Data{ storage->GetBitField() }
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Data.GetNumberOfBits(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution{ this->Data.PrepareForInput(Device{}) };
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution{ this->Data.PrepareForInPlace(Device{}) };
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return PortalExecution{ this->Data.PrepareForOutput(numberOfValues, Device{}) };
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // bitfield should automatically retrieve the output data as necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->Data.Shrink(numberOfValues); }

  VTKM_CONT
  void ReleaseResources() { this->Data.ReleaseResources(); }

private:
  vtkm::cont::BitField Data;
};

} // end namespace internal


/// The ArrayHandleBitField class is a boolean-valued ArrayHandle that is backed
/// by a BitField.
///
class ArrayHandleBitField : public ArrayHandle<bool, internal::StorageTagBitField>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandleBitField,
                                (ArrayHandle<bool, internal::StorageTagBitField>));

  VTKM_CONT
  explicit ArrayHandleBitField(const vtkm::cont::BitField& bitField)
    : Superclass{ StorageType{ bitField } }
  {
  }

  VTKM_CONT
  explicit ArrayHandleBitField(vtkm::cont::BitField&& bitField) noexcept
    : Superclass{ StorageType{ std::move(bitField) } }
  {
  }

  VTKM_CONT
  vtkm::cont::BitField GetBitField() const { return this->GetStorage().GetBitField(); }
};

VTKM_CONT inline vtkm::cont::ArrayHandleBitField make_ArrayHandleBitField(
  const vtkm::cont::BitField& bitField)
{
  return ArrayHandleBitField{ bitField };
}

VTKM_CONT inline vtkm::cont::ArrayHandleBitField make_ArrayHandleBitField(
  vtkm::cont::BitField&& bitField) noexcept
{
  return ArrayHandleBitField{ std::move(bitField) };
}
}
} // end namespace vtkm::cont

#endif // vtk_m_cont_ArrayHandleBitField_h
