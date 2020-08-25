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
  explicit ArrayPortalBitField(const BitPortalType& portal) noexcept
    : BitPortal{ portal }
  {
  }

  VTKM_EXEC_CONT
  explicit ArrayPortalBitField(BitPortalType&& portal) noexcept
    : BitPortal{ std::move(portal) }
  {
  }

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
  using BitPortalType = vtkm::cont::detail::BitPortal;
  using BitPortalConstType = vtkm::cont::detail::BitPortalConst;

  using WordType = vtkm::WordTypeDefault;
  static constexpr vtkm::Id BlockSize = vtkm::cont::detail::BitFieldTraits::BlockSize;
  VTKM_STATIC_ASSERT(BlockSize >= static_cast<vtkm::Id>(sizeof(WordType)));

public:
  using ReadPortalType = vtkm::cont::internal::ArrayPortalBitField<BitPortalConstType>;
  using WritePortalType = vtkm::cont::internal::ArrayPortalBitField<BitPortalType>;

  VTKM_CONT vtkm::IdComponent GetNumberOfBuffers() const { return 1; }

  VTKM_CONT Storage() {}

  VTKM_CONT void ResizeBuffers(vtkm::Id numberOfBits,
                               vtkm::cont::internal::Buffer* buffers,
                               vtkm::CopyFlag preserve,
                               vtkm::cont::Token& token)
  {
    const vtkm::Id bytesNeeded = (numberOfBits + CHAR_BIT - 1) / CHAR_BIT;
    const vtkm::Id blocksNeeded = (bytesNeeded + BlockSize - 1) / BlockSize;
    const vtkm::Id numBytes = blocksNeeded * BlockSize;

    VTKM_LOG_F(vtkm::cont::LogLevel::MemCont,
               "BitField Allocation: %llu bits, blocked up to %s bytes.",
               static_cast<unsigned long long>(numberOfBits),
               vtkm::cont::GetSizeString(static_cast<vtkm::UInt64>(numBytes)).c_str());

    buffers[0].SetNumberOfBytes(numBytes, preserve, token);
    vtkm::cont::detail::GetBitFieldMetaData(buffers[0])->NumberOfBits = numberOfBits;
  }

  VTKM_CONT vtkm::Id GetNumberOfValues(const vtkm::cont::internal::Buffer* buffers)
  {
    vtkm::Id numberOfBits = vtkm::cont::detail::GetBitFieldMetaData(buffers[0])->NumberOfBits;
    VTKM_ASSERT((buffers[0].GetNumberOfBytes() * CHAR_BIT) >= numberOfBits);
    return numberOfBits;
  }

  VTKM_CONT ReadPortalType CreateReadPortal(const vtkm::cont::internal::Buffer* buffers,
                                            vtkm::cont::DeviceAdapterId device,
                                            vtkm::cont::Token& token)
  {
    vtkm::Id numberOfBits = this->GetNumberOfValues(buffers);
    VTKM_ASSERT((buffers[0].GetNumberOfBytes() * CHAR_BIT) >= numberOfBits);

    return ReadPortalType(
      BitPortalConstType(buffers[0].ReadPointerDevice(device, token), numberOfBits));
  }

  VTKM_CONT WritePortalType CreateWritePortal(const vtkm::cont::internal::Buffer* buffers,
                                              vtkm::cont::DeviceAdapterId device,
                                              vtkm::cont::Token& token)
  {
    vtkm::Id numberOfBits = this->GetNumberOfValues(buffers);
    VTKM_ASSERT((buffers[0].GetNumberOfBytes() * CHAR_BIT) >= numberOfBits);

    return WritePortalType(
      BitPortalType(buffers[0].WritePointerDevice(device, token), numberOfBits));
  }
};

} // end namespace internal


// This can go away once ArrayHandle is replaced with ArrayHandleNewStyle
template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandle<T, vtkm::cont::internal::StorageTagBitField>
  : public ArrayHandleNewStyle<T, vtkm::cont::internal::StorageTagBitField>
{
  using Superclass = ArrayHandleNewStyle<T, vtkm::cont::internal::StorageTagBitField>;

public:
  VTKM_CONT
  ArrayHandle()
    : Superclass()
  {
  }

  VTKM_CONT
  ArrayHandle(const ArrayHandle<T, vtkm::cont::internal::StorageTagBitField>& src)
    : Superclass(src)
  {
  }

  VTKM_CONT
  ArrayHandle(ArrayHandle<T, vtkm::cont::internal::StorageTagBitField>&& src) noexcept
    : Superclass(std::move(src))
  {
  }

  VTKM_CONT
  ArrayHandle(const ArrayHandleNewStyle<T, vtkm::cont::internal::StorageTagBitField>& src)
    : Superclass(src)
  {
  }

  VTKM_CONT
  ArrayHandle(ArrayHandleNewStyle<T, vtkm::cont::internal::StorageTagBitField>&& src) noexcept
    : Superclass(std::move(src))
  {
  }

  VTKM_CONT ArrayHandle(
    const vtkm::cont::internal::Buffer* buffers,
    const typename Superclass::StorageType& storage = typename Superclass::StorageType())
    : Superclass(buffers, storage)
  {
  }

  VTKM_CONT ArrayHandle(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    const typename Superclass::StorageType& storage = typename Superclass::StorageType())
    : Superclass(buffers, storage)
  {
  }

  VTKM_CONT
  ArrayHandle<T, vtkm::cont::internal::StorageTagBitField>& operator=(
    const ArrayHandle<T, vtkm::cont::internal::StorageTagBitField>& src)
  {
    this->Superclass::operator=(src);
    return *this;
  }

  VTKM_CONT
  ArrayHandle<T, vtkm::cont::internal::StorageTagBitField>& operator=(
    ArrayHandle<T, vtkm::cont::internal::StorageTagBitField>&& src) noexcept
  {
    this->Superclass::operator=(std::move(src));
    return *this;
  }

  VTKM_CONT ~ArrayHandle() {}
};

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
    : Superclass(std::vector<vtkm::cont::internal::Buffer>(1, bitField.GetBuffer()))
  {
  }
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
