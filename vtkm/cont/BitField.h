//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_BitField_h
#define vtk_m_cont_BitField_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Atomic.h>
#include <vtkm/List.h>
#include <vtkm/Types.h>

#include <cassert>
#include <climits>
#include <memory>
#include <type_traits>

namespace vtkm
{
namespace cont
{

class BitField;

namespace internal
{

struct StorageTagBitField;

struct VTKM_ALWAYS_EXPORT BitFieldMetaData
{
  vtkm::Id NumberOfBits = 0;
};

}

namespace detail
{

struct BitFieldTraits
{
  // Allocations will occur in blocks of BlockSize bytes. This ensures that
  // power-of-two word sizes up to BlockSize will not access invalid data
  // during word-based access, and that atomic values will be properly aligned.
  // We use the default StorageBasic alignment for this.
  constexpr static vtkm::Id BlockSize = VTKM_ALLOCATION_ALIGNMENT;

  // Make sure the blocksize is at least 64. Eventually we may implement SIMD
  // bit operations, and the current largest vector width is 512 bits.
  VTKM_STATIC_ASSERT(BlockSize >= 64);

  /// Require an unsigned integral type that is <= BlockSize bytes.
  template <typename WordType>
  using IsValidWordType =
    std::integral_constant<bool,
                           /* is unsigned */
                           std::is_unsigned<WordType>::value &&
                             /* doesn't exceed blocksize */
                             sizeof(WordType) <= static_cast<size_t>(BlockSize) &&
                             /* BlockSize is a multiple of WordType */
                             static_cast<size_t>(BlockSize) % sizeof(WordType) == 0>;

  /// Require an unsigned integral type that is <= BlockSize bytes, and is
  /// is supported by the specified AtomicInterface.
  template <typename WordType>
  using IsValidWordTypeAtomic =
    std::integral_constant<bool,
                           /* is unsigned */
                           std::is_unsigned<WordType>::value &&
                             /* doesn't exceed blocksize */
                             sizeof(WordType) <= static_cast<size_t>(BlockSize) &&
                             /* BlockSize is a multiple of WordType */
                             static_cast<size_t>(BlockSize) % sizeof(WordType) == 0 &&
                             /* Supported by atomic interface */
                             vtkm::ListHas<vtkm::AtomicTypesSupported, WordType>::value>;
};

/// Identifies a bit in a BitField by Word and BitOffset. Note that these
/// values are dependent on the type of word used to generate the coordinate.
struct BitCoordinate
{
  /// The word containing the specified bit.
  vtkm::Id WordIndex;

  /// The zero-indexed bit in the word.
  vtkm::Int32 BitOffset; // [0, bitsInWord)
};

/// Portal for performing bit or word operations on a BitField.
///
/// This is the implementation used by BitPortal and BitPortalConst.
template <bool IsConst>
class BitPortalBase
{
  // Checks if PortalType has a GetIteratorBegin() method that returns a
  // pointer.
  template <typename PortalType,
            typename PointerType = decltype(std::declval<PortalType>().GetIteratorBegin())>
  struct HasPointerAccess : public std::is_pointer<PointerType>
  {
  };

  // Determine whether we should store a const vs. mutable pointer:
  template <typename T>
  using MaybeConstPointer = typename std::conditional<IsConst, T const*, T*>::type;
  using BufferType = MaybeConstPointer<void>; // void* or void const*, as appropriate

public:
  /// The fastest word type for performing bitwise operations through AtomicInterface.
  using WordTypePreferred = vtkm::AtomicTypePreferred;

  /// MPL check for whether a WordType may be used for non-atomic operations.
  template <typename WordType>
  using IsValidWordType = BitFieldTraits::IsValidWordType<WordType>;

  /// MPL check for whether a WordType may be used for atomic operations.
  template <typename WordType>
  using IsValidWordTypeAtomic = BitFieldTraits::IsValidWordTypeAtomic<WordType>;

  VTKM_STATIC_ASSERT_MSG(IsValidWordType<WordTypeDefault>::value,
                         "Internal error: Default word type is invalid.");
  VTKM_STATIC_ASSERT_MSG(IsValidWordType<WordTypePreferred>::value,
                         "Device-specific fast word type is invalid.");

  VTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordTypeDefault>::value,
                         "Internal error: Default word type is invalid.");
  VTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordTypePreferred>::value,
                         "Device-specific fast word type is invalid for atomic operations.");

protected:
  friend class vtkm::cont::BitField;
  friend class vtkm::cont::internal::Storage<bool, vtkm::cont::internal::StorageTagBitField>;

  /// Construct a BitPortal from a raw array.
  VTKM_CONT BitPortalBase(BufferType rawArray, vtkm::Id numberOfBits)
    : Data{ rawArray }
    , NumberOfBits{ numberOfBits }
  {
  }

public:
  BitPortalBase() noexcept = default;
  BitPortalBase(const BitPortalBase&) noexcept = default;
  BitPortalBase(BitPortalBase&&) noexcept = default;
  BitPortalBase& operator=(const BitPortalBase&) noexcept = default;
  BitPortalBase& operator=(BitPortalBase&&) noexcept = default;

  /// Returns the number of bits in the BitField.
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfBits() const noexcept { return this->NumberOfBits; }

  /// Returns how many words of type @a WordTypePreferred exist in the dataset.
  /// Note that this is rounded up and may contain partial words. See
  /// also GetFinalWordMask to handle the trailing partial word.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT vtkm::Id GetNumberOfWords() const noexcept
  {
    VTKM_STATIC_ASSERT(IsValidWordType<WordType>::value);
    static constexpr vtkm::Id WordSize = static_cast<vtkm::Id>(sizeof(WordType));
    static constexpr vtkm::Id WordBits = WordSize * CHAR_BIT;
    return (this->NumberOfBits + WordBits - 1) / WordBits;
  }

  /// Return a mask in which the valid bits in the final word (of type @a
  /// WordType) are set to 1.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT WordType GetFinalWordMask() const noexcept
  {
    if (this->NumberOfBits == 0)
    {
      return WordType{ 0 };
    }

    static constexpr vtkm::Int32 BitsPerWord =
      static_cast<vtkm::Int32>(sizeof(WordType) * CHAR_BIT);

    const auto maxBit = this->NumberOfBits - 1;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(maxBit);
    const vtkm::Int32 shift = BitsPerWord - coord.BitOffset - 1;
    return (~WordType{ 0 }) >> shift;
  }

  /// Given a bit index, compute a @a BitCoordinate that identifies the
  /// corresponding word index and bit offset.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT static BitCoordinate GetBitCoordinateFromIndex(vtkm::Id bitIdx) noexcept
  {
    VTKM_STATIC_ASSERT(IsValidWordType<WordType>::value);
    static constexpr vtkm::Id BitsPerWord = static_cast<vtkm::Id>(sizeof(WordType) * CHAR_BIT);
    return { static_cast<vtkm::Id>(bitIdx / BitsPerWord),
             static_cast<vtkm::Int32>(bitIdx % BitsPerWord) };
  }

  /// Set the bit at @a bitIdx to @a val. This method is not thread-safe --
  /// threads modifying bits nearby may interfere with this operation.
  /// Additionally, this should not be used for synchronization, as there are
  /// no memory ordering requirements. See SetBitAtomic for those usecases.
  VTKM_EXEC_CONT
  void SetBit(vtkm::Id bitIdx, bool val) const noexcept
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto mask = WordType(1) << coord.BitOffset;
    WordType* wordAddr = this->GetWordAddress<WordType>(coord.WordIndex);
    if (val)
    {
      *wordAddr |= mask;
    }
    else
    {
      *wordAddr &= ~mask;
    }
  }

  /// Set the bit at @a bitIdx to @a val using atomic operations. This method
  /// is thread-safe and guarantees, at minimum, "release" memory ordering.
  VTKM_EXEC_CONT
  void SetBitAtomic(vtkm::Id bitIdx, bool val) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto mask = WordType(1) << coord.BitOffset;
    if (val)
    {
      this->OrWordAtomic(coord.WordIndex, mask);
    }
    else
    {
      this->AndWordAtomic(coord.WordIndex, ~mask);
    }
  }

  /// Return whether or not the bit at @a bitIdx is set. Note that this uses
  /// non-atomic loads and thus should not be used for synchronization.
  VTKM_EXEC_CONT
  bool GetBit(vtkm::Id bitIdx) const noexcept
  {
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto word = this->GetWord<WordType>(coord.WordIndex);
    const auto mask = WordType(1) << coord.BitOffset;
    return (word & mask) != WordType(0);
  }

  /// Return whether or not the bit at @a bitIdx is set using atomic loads.
  /// This method is thread safe and guarantees, at minimum, "acquire" memory
  /// ordering.
  VTKM_EXEC_CONT
  bool GetBitAtomic(vtkm::Id bitIdx) const
  {
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto word = this->GetWordAtomic<WordType>(coord.WordIndex);
    const auto mask = WordType(1) << coord.BitOffset;
    return (word & mask) != WordType(0);
  }

  /// Set the word (of type @a WordType) at @a wordIdx to @a word using
  /// non-atomic operations.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT void SetWord(vtkm::Id wordIdx, WordType word) const noexcept
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    *this->GetWordAddress<WordType>(wordIdx) = word;
  }

  /// Set the word (of type @a WordType) at @a wordIdx to @a word using atomic
  /// operations. The store guarantees, at minimum, "release" memory ordering.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT void SetWordAtomic(vtkm::Id wordIdx, WordType word) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "'Set' method called on const BitField portal.");
    VTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    vtkm::AtomicStore(this->GetWordAddress<WordType>(wordIdx), word);
  }

  /// Get the word (of type @a WordType) at @a wordIdx using non-atomic
  /// operations.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT WordType GetWord(vtkm::Id wordIdx) const noexcept
  {
    return *this->GetWordAddress<WordType>(wordIdx);
  }

  /// Get the word (of type @a WordType) at @ wordIdx using an atomic read with,
  /// at minimum, "acquire" memory ordering.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT WordType GetWordAtomic(vtkm::Id wordIdx) const
  {
    VTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    return vtkm::AtomicLoad(this->GetWordAddress<WordType>(wordIdx));
  }

  /// Toggle the bit at @a bitIdx, returning the original value. This method
  /// uses atomic operations and a full memory barrier.
  VTKM_EXEC_CONT
  bool NotBitAtomic(vtkm::Id bitIdx) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto mask = WordType(1) << coord.BitOffset;
    const auto oldWord = this->XorWordAtomic(coord.WordIndex, mask);
    return (oldWord & mask) != WordType(0);
  }

  /// Perform a bitwise "not" operation on the word at @a wordIdx, returning the
  /// original word. This uses atomic operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT WordType NotWordAtomic(vtkm::Id wordIdx) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    VTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return vtkm::AtomicNot(addr);
  }

  /// Perform an "and" operation between the bit at @a bitIdx and @a val,
  /// returning the original value at @a bitIdx. This method uses atomic
  /// operations and a full memory barrier.
  VTKM_EXEC_CONT
  bool AndBitAtomic(vtkm::Id bitIdx, bool val) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;
    // wordmask is all 1's, except for BitOffset which is (val ? 1 : 0)
    const auto wordmask = val ? ~WordType(0) : ~bitmask;
    const auto oldWord = this->AndWordAtomic(coord.WordIndex, wordmask);
    return (oldWord & bitmask) != WordType(0);
  }

  /// Perform an "and" operation between the word at @a wordIdx and @a wordMask,
  /// returning the original word at @a wordIdx. This method uses atomic
  /// operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT WordType AndWordAtomic(vtkm::Id wordIdx, WordType wordmask) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    VTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return vtkm::AtomicAnd(addr, wordmask);
  }

  /// Perform an "of" operation between the bit at @a bitIdx and @a val,
  /// returning the original value at @a bitIdx. This method uses atomic
  /// operations and a full memory barrier.
  VTKM_EXEC_CONT
  bool OrBitAtomic(vtkm::Id bitIdx, bool val) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;
    // wordmask is all 0's, except for BitOffset which is (val ? 1 : 0)
    const auto wordmask = val ? bitmask : WordType(0);
    const auto oldWord = this->OrWordAtomic(coord.WordIndex, wordmask);
    return (oldWord & bitmask) != WordType(0);
  }

  /// Perform an "or" operation between the word at @a wordIdx and @a wordMask,
  /// returning the original word at @a wordIdx. This method uses atomic
  /// operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT WordType OrWordAtomic(vtkm::Id wordIdx, WordType wordmask) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    VTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return vtkm::AtomicOr(addr, wordmask);
  }

  /// Perform an "xor" operation between the bit at @a bitIdx and @a val,
  /// returning the original value at @a bitIdx. This method uses atomic
  /// operations and a full memory barrier.
  VTKM_EXEC_CONT
  bool XorBitAtomic(vtkm::Id bitIdx, bool val) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;
    // wordmask is all 0's, except for BitOffset which is (val ? 1 : 0)
    const auto wordmask = val ? bitmask : WordType(0);
    const auto oldWord = this->XorWordAtomic(coord.WordIndex, wordmask);
    return (oldWord & bitmask) != WordType(0);
  }

  /// Perform an "xor" operation between the word at @a wordIdx and @a wordMask,
  /// returning the original word at @a wordIdx. This method uses atomic
  /// operations and a full memory barrier.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT WordType XorWordAtomic(vtkm::Id wordIdx, WordType wordmask) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    VTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return vtkm::AtomicXor(addr, wordmask);
  }

  /// Perform an atomic compare-and-swap operation on the bit at @a bitIdx.
  /// If the value in memory is equal to @a oldBit, it is replaced with
  /// the value of @a newBit and true is returned. If the value in memory is
  /// not equal to @oldBit, @oldBit is changed to that value and false is
  /// returned. This method implements a full memory barrier around the atomic
  /// operation.
  VTKM_EXEC_CONT
  bool CompareExchangeBitAtomic(vtkm::Id bitIdx, bool* oldBit, bool newBit) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    using WordType = WordTypePreferred;
    const auto coord = this->GetBitCoordinateFromIndex<WordType>(bitIdx);
    const auto bitmask = WordType(1) << coord.BitOffset;

    WordType oldWord = this->GetWord<WordType>(coord.WordIndex);
    do
    {
      bool actualBit = (oldWord & bitmask) != WordType(0);
      if (actualBit != *oldBit)
      { // The bit-of-interest does not match what we expected.
        *oldBit = actualBit;
        return false;
      }
      else if (actualBit == newBit)
      { // The bit hasn't changed, but also already matches newVal. We're done.
        return true;
      }

      // Attempt to update the word with a compare-exchange in the loop condition.
      // If the old word changed since last queried, oldWord will get updated and
      // the loop will continue until it succeeds.
    } while (!this->CompareExchangeWordAtomic(coord.WordIndex, &oldWord, oldWord ^ bitmask));

    return true;
  }

  /// Perform an atomic compare-exchange operation on the word at @a wordIdx.
  /// If the word in memory is equal to @a oldWord, it is replaced with
  /// the value of @a newWord and true returned. If the word in memory is not
  /// equal to @oldWord, @oldWord is set to the word in memory and false is
  /// returned. This method implements a full memory barrier around the atomic
  /// operation.
  template <typename WordType = WordTypePreferred>
  VTKM_EXEC_CONT bool CompareExchangeWordAtomic(vtkm::Id wordIdx,
                                                WordType* oldWord,
                                                WordType newWord) const
  {
    VTKM_STATIC_ASSERT_MSG(!IsConst, "Attempt to modify const BitField portal.");
    VTKM_STATIC_ASSERT_MSG(IsValidWordTypeAtomic<WordType>::value,
                           "Requested WordType does not support atomic"
                           " operations on target execution platform.");
    WordType* addr = this->GetWordAddress<WordType>(wordIdx);
    return vtkm::AtomicCompareExchange(addr, oldWord, newWord);
  }

private:
  template <typename WordType>
  VTKM_EXEC_CONT MaybeConstPointer<WordType> GetWordAddress(vtkm::Id wordId) const noexcept
  {
    VTKM_STATIC_ASSERT(IsValidWordType<WordType>::value);
    return reinterpret_cast<MaybeConstPointer<WordType>>(this->Data) + wordId;
  }

  BufferType Data{ nullptr };
  vtkm::Id NumberOfBits{ 0 };
};

using BitPortal = BitPortalBase<false>;

using BitPortalConst = BitPortalBase<true>;

} // end namespace detail

class VTKM_CONT_EXPORT BitField
{
  static constexpr vtkm::Id BlockSize = detail::BitFieldTraits::BlockSize;

public:
  /// The BitPortal used in the control environment.
  using WritePortalType = detail::BitPortal;

  /// A read-only BitPortal used in the control environment.
  using ReadPortalType = detail::BitPortalConst;

  using WordTypePreferred = vtkm::AtomicTypePreferred;

  template <typename Device>
  struct ExecutionTypes
  {
    /// The preferred word type used by the specified device.
    using WordTypePreferred = vtkm::AtomicTypePreferred;

    /// A BitPortal that is usable on the specified device.
    using Portal = detail::BitPortal;

    /// A read-only BitPortal that is usable on the specified device.
    using PortalConst = detail::BitPortalConst;
  };

  /// Check whether a word type is valid for non-atomic operations.
  template <typename WordType>
  using IsValidWordType = detail::BitFieldTraits::IsValidWordType<WordType>;

  /// Check whether a word type is valid for atomic operations.
  template <typename WordType, typename Device = void>
  using IsValidWordTypeAtomic = detail::BitFieldTraits::IsValidWordTypeAtomic<WordType>;

  VTKM_CONT BitField();
  VTKM_CONT BitField(const BitField&) = default;
  VTKM_CONT BitField(BitField&&) noexcept = default;
  VTKM_CONT ~BitField() = default;
  VTKM_CONT BitField& operator=(const BitField&) = default;
  VTKM_CONT BitField& operator=(BitField&&) noexcept = default;

  VTKM_CONT
  bool operator==(const BitField& rhs) const { return this->Buffer == rhs.Buffer; }

  VTKM_CONT
  bool operator!=(const BitField& rhs) const { return this->Buffer != rhs.Buffer; }

  /// Return the internal `Buffer` used to store the `BitField`.
  VTKM_CONT vtkm::cont::internal::Buffer GetBuffer() const { return this->Buffer; }

  /// Return the number of bits stored by this BitField.
  VTKM_CONT vtkm::Id GetNumberOfBits() const;

  /// Return the number of words (of @a WordType) stored in this bit fields.
  ///
  template <typename WordType>
  VTKM_CONT vtkm::Id GetNumberOfWords() const
  {
    VTKM_STATIC_ASSERT(IsValidWordType<WordType>::value);
    static constexpr vtkm::Id WordBits = static_cast<vtkm::Id>(sizeof(WordType) * CHAR_BIT);
    return (this->GetNumberOfBits() + WordBits - 1) / WordBits;
  }

  /// Allocate the requested number of bits.
  VTKM_CONT void Allocate(vtkm::Id numberOfBits,
                          vtkm::CopyFlag preserve,
                          vtkm::cont::Token& token) const;

  /// Allocate the requested number of bits.
  VTKM_CONT void Allocate(vtkm::Id numberOfBits,
                          vtkm::CopyFlag preserve = vtkm::CopyFlag::Off) const
  {
    vtkm::cont::Token token;
    this->Allocate(numberOfBits, preserve, token);
  }

  /// Allocate the requested number of bits and fill with the requested bit or word.
  template <typename ValueType>
  VTKM_CONT void AllocateAndFill(vtkm::Id numberOfBits,
                                 ValueType value,
                                 vtkm::cont::Token& token) const
  {
    this->Allocate(numberOfBits, vtkm::CopyFlag::Off, token);
    this->Fill(value, token);
  }
  template <typename ValueType>
  VTKM_CONT void AllocateAndFill(vtkm::Id numberOfBits, ValueType value) const
  {
    vtkm::cont::Token token;
    this->AllocateAndFill(numberOfBits, value, token);
  }

private:
  VTKM_CONT void FillImpl(const void* word,
                          vtkm::BufferSizeType wordSize,
                          vtkm::cont::Token& token) const;

public:
  /// Set subsequent words to the given word of bits.
  template <typename WordType>
  VTKM_CONT void Fill(WordType word, vtkm::cont::Token& token) const
  {
    this->FillImpl(&word, static_cast<vtkm::BufferSizeType>(sizeof(WordType)), token);
  }
  template <typename WordType>
  VTKM_CONT void Fill(WordType word) const
  {
    vtkm::cont::Token token;
    this->Fill(word, token);
  }

  /// Set all the bits to the given value
  VTKM_CONT void Fill(bool value, vtkm::cont::Token& token) const
  {
    using WordType = WordTypePreferred;
    this->Fill(value ? ~WordType{ 0 } : WordType{ 0 }, token);
  }
  VTKM_CONT void Fill(bool value) const
  {
    vtkm::cont::Token token;
    this->Fill(value, token);
  }

  /// Release all execution-side resources held by this BitField.
  VTKM_CONT void ReleaseResourcesExecution();

  /// Release all resources held by this BitField and reset to empty.
  VTKM_CONT void ReleaseResources();

  /// Force the control array to sync with the last-used device.
  VTKM_CONT void SyncControlArray() const;

  /// Returns true if the `BitField`'s data is on the given device. If the data are on the given
  /// device, then preparing for that device should not require any data movement.
  ///
  VTKM_CONT bool IsOnDevice(vtkm::cont::DeviceAdapterId device) const;

  /// Returns true if the `BitField`'s data is on the host. If the data are on the given
  /// device, then calling `ReadPortal` or `WritePortal` should not require any data movement.
  ///
  VTKM_CONT bool IsOnHost() const
  {
    return this->IsOnDevice(vtkm::cont::DeviceAdapterTagUndefined{});
  }

  /// \brief Get a portal to the data that is usable from the control environment.
  ///
  /// As long as this portal is in scope, no one else will be able to read or write the BitField.
  VTKM_CONT WritePortalType WritePortal() const;

  /// \brief Get a read-only portal to the data that is usable from the control environment.
  ///
  /// As long as this portal is in scope, no one else will be able to write in the BitField.
  VTKM_CONT ReadPortalType ReadPortal() const;

  /// Prepares this BitField to be used as an input to an operation in the
  /// execution environment. If necessary, copies data to the execution
  /// environment. Can throw an exception if this BitField does not yet contain
  /// any data. Returns a portal that can be used in code running in the
  /// execution environment.
  VTKM_CONT ReadPortalType PrepareForInput(vtkm::cont::DeviceAdapterId device,
                                           vtkm::cont::Token& token) const;

  /// Prepares (allocates) this BitField to be used as an output from an
  /// operation in the execution environment. The internal state of this class
  /// is set to have valid data in the execution BitField with the assumption
  /// that the array will be filled soon (i.e. before any other methods of this
  /// object are called). Returns a portal that can be used in code running in
  /// the execution environment.
  VTKM_CONT WritePortalType PrepareForOutput(vtkm::Id numBits,
                                             vtkm::cont::DeviceAdapterId device,
                                             vtkm::cont::Token& token) const;

  /// Prepares this BitField to be used in an in-place operation (both as input
  /// and output) in the execution environment. If necessary, copies data to
  /// the execution environment. Can throw an exception if this BitField does
  /// not yet contain any data. Returns a portal that can be used in code
  /// running in the execution environment.
  VTKM_CONT WritePortalType PrepareForInPlace(vtkm::cont::DeviceAdapterId device,
                                              vtkm::cont::Token& token) const;

private:
  mutable vtkm::cont::internal::Buffer Buffer;
};
}
} // end namespace vtkm::cont

#endif // vtk_m_cont_BitField_h
