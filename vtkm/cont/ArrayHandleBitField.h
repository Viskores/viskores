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

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers()
  {
    return std::vector<vtkm::cont::internal::Buffer>(1);
  }

  VTKM_CONT static void ResizeBuffers(vtkm::Id numberOfBits,
                                      const std::vector<vtkm::cont::internal::Buffer>& buffers,
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
    buffers[0].GetMetaData<vtkm::cont::internal::BitFieldMetaData>().NumberOfBits = numberOfBits;
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    VTKM_ASSERT(buffers.size() == 1);
    vtkm::Id numberOfBits =
      buffers[0].GetMetaData<vtkm::cont::internal::BitFieldMetaData>().NumberOfBits;
    VTKM_ASSERT((buffers[0].GetNumberOfBytes() * CHAR_BIT) >= numberOfBits);
    return numberOfBits;
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>& buffers,
                             bool fillValue,
                             vtkm::Id startBit,
                             vtkm::Id endBit,
                             vtkm::cont::Token& token)
  {
    VTKM_ASSERT(buffers.size() == 1);
    constexpr vtkm::BufferSizeType wordTypeSize =
      static_cast<vtkm::BufferSizeType>(sizeof(WordType));
    constexpr vtkm::BufferSizeType wordNumBits = wordTypeSize * CHAR_BIT;
    // Special case where filling to end of array.
    vtkm::Id totalBitsInArray = GetNumberOfValues(buffers);
    if (endBit >= totalBitsInArray)
    {
      endBit = ((totalBitsInArray + (wordNumBits - 1)) / wordNumBits) * wordNumBits;
    }
    if (((startBit % wordNumBits) == 0) && ((endBit % wordNumBits) == 0))
    {
      WordType fillWord = (fillValue ? ~WordType{ 0 } : WordType{ 0 });
      buffers[0].Fill(&fillWord, wordTypeSize, startBit / CHAR_BIT, endBit / CHAR_BIT, token);
    }
    else if (((startBit % CHAR_BIT) == 0) && ((endBit % CHAR_BIT) == 0))
    {
      vtkm::UInt8 fillWord = (fillValue ? ~vtkm::UInt8{ 0 } : vtkm::UInt8{ 0 });
      buffers[0].Fill(&fillWord, 1, startBit / CHAR_BIT, endBit / CHAR_BIT, token);
    }
    else
    {
      throw vtkm::cont::ErrorBadValue("Can only fill ArrayHandleBitField on 8-bit boundaries.");
    }
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    VTKM_ASSERT(buffers.size() == 1);
    vtkm::Id numberOfBits = GetNumberOfValues(buffers);
    VTKM_ASSERT((buffers[0].GetNumberOfBytes() * CHAR_BIT) >= numberOfBits);

    return ReadPortalType(
      BitPortalConstType(buffers[0].ReadPointerDevice(device, token), numberOfBits));
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    VTKM_ASSERT(buffers.size() == 1);
    vtkm::Id numberOfBits = GetNumberOfValues(buffers);
    VTKM_ASSERT((buffers[0].GetNumberOfBytes() * CHAR_BIT) >= numberOfBits);

    return WritePortalType(
      BitPortalType(buffers[0].WritePointerDevice(device, token), numberOfBits));
  }
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
