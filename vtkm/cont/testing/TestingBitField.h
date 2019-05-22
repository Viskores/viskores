//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingBitFields_h
#define vtk_m_cont_testing_TestingBitFields_h

#include <vtkm/cont/ArrayHandleBitField.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/BitField.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/exec/FunctorBase.h>

#include <cstdio>

#define DEVICE_ASSERT_MSG(cond, message)                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(cond))                                                                                   \
    {                                                                                              \
      printf("Testing assert failed at %s:%d\n\t- Condition: %s\n\t- Subtest: %s\n",               \
             __FILE__,                                                                             \
             __LINE__,                                                                             \
             #cond,                                                                                \
             message);                                                                             \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

#define DEVICE_ASSERT(cond)                                                                        \
  do                                                                                               \
  {                                                                                                \
    if (!(cond))                                                                                   \
    {                                                                                              \
      printf("Testing assert failed at %s:%d\n\t- Condition: %s\n", __FILE__, __LINE__, #cond);    \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

// Test with some trailing bits in partial last word:
#define NUM_BITS                                                                                   \
  vtkm::Id { 7681 }

using vtkm::cont::BitField;

namespace vtkm
{
namespace cont
{
namespace testing
{

// Takes an ArrayHandleBitField as the boolean condition field
class ConditionalMergeWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn cond, FieldIn trueVals, FieldIn falseVals, FieldOut result);
  using ExecutionSignature = _4(_1, _2, _3);

  template <typename T>
  VTKM_EXEC T operator()(bool cond, const T& trueVal, const T& falseVal) const
  {
    return cond ? trueVal : falseVal;
  }
};

// Takes a BitFieldInOut as the condition information, and reverses
// the bits in place after performing the merge.
class ConditionalMergeWorklet2 : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(BitFieldInOut bits,
                                FieldIn trueVals,
                                FieldIn falseVal,
                                FieldOut result);
  using ExecutionSignature = _4(InputIndex, _1, _2, _3);
  using InputDomain = _2;

  template <typename BitPortal, typename T>
  VTKM_EXEC T
  operator()(const vtkm::Id i, BitPortal& bits, const T& trueVal, const T& falseVal) const
  {
    return bits.XorBitAtomic(i, true) ? trueVal : falseVal;
  }
};

/// This class has a single static member, Run, that runs all tests with the
/// given DeviceAdapter.
template <class DeviceAdapterTag>
struct TestingBitField
{
  using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;
  using AtomicInterface = vtkm::cont::internal::AtomicInterfaceExecution<DeviceAdapterTag>;
  using Traits = vtkm::cont::detail::BitFieldTraits;
  using WordTypes = typename AtomicInterface::WordTypes;
  using WordTypesControl = vtkm::cont::internal::AtomicInterfaceControl::WordTypes;

  VTKM_EXEC_CONT
  static bool RandomBitFromIndex(vtkm::Id idx) noexcept
  {
    // Some random operations that will give a pseudorandom stream of bits:
    auto m = idx + (idx * 2) - (idx / 3) + (idx * 5 / 7) - (idx * 11 / 13);
    return (m % 2) == 1;
  }

  template <typename WordType>
  VTKM_EXEC_CONT static WordType RandomWordFromIndex(vtkm::Id idx) noexcept
  {
    vtkm::UInt64 m = static_cast<vtkm::UInt64>(idx * (NUM_BITS - 1) + (idx + 1) * NUM_BITS);
    m ^= m << 3;
    m ^= m << 7;
    m ^= m << 15;
    m ^= m << 31;
    m = (m << 32) | (m >> 32);

    const size_t mBits = 64;
    const size_t wordBits = sizeof(WordType) * CHAR_BIT;

    const WordType highWord = static_cast<WordType>(m >> (mBits - wordBits));
    return highWord;
  }

  VTKM_CONT
  static BitField RandomBitField(vtkm::Id numBits = NUM_BITS)
  {
    BitField field;
    field.Allocate(numBits);
    auto portal = field.GetPortalControl();
    for (vtkm::Id i = 0; i < numBits; ++i)
    {
      portal.SetBit(i, RandomBitFromIndex(i));
    }

    return field;
  }

  VTKM_CONT
  static void TestBlockAllocation()
  {
    BitField field;
    field.Allocate(NUM_BITS);

    // NumBits should be rounded up to the nearest block of bytes, as defined in
    // the traits:
    const vtkm::Id bytesInFieldData =
      field.GetData().GetNumberOfValues() * static_cast<vtkm::Id>(sizeof(vtkm::WordTypeDefault));

    const vtkm::Id blockSize = vtkm::cont::detail::BitFieldTraits::BlockSize;
    const vtkm::Id numBytes = (NUM_BITS + CHAR_BIT - 1) / CHAR_BIT;
    const vtkm::Id numBlocks = (numBytes + blockSize - 1) / blockSize;
    const vtkm::Id expectedBytes = numBlocks * blockSize;

    VTKM_TEST_ASSERT(bytesInFieldData == expectedBytes,
                     "The BitField allocation does not round up to the nearest "
                     "block. This can cause access-by-word to read/write invalid "
                     "memory.");
  }

  template <typename PortalType, typename PortalConstType>
  VTKM_EXEC_CONT static bool HelpTestBit(vtkm::Id i, PortalType portal, PortalConstType portalConst)
  {
    const auto origBit = RandomBitFromIndex(i);
    auto bit = origBit;

    const auto mod = RandomBitFromIndex(i + NUM_BITS);

    auto testValues = [&](const char* op) -> bool {
      auto expected = bit;
      auto result = portal.GetBitAtomic(i);
      auto resultConst = portalConst.GetBitAtomic(i);
      DEVICE_ASSERT_MSG(result == expected, op);
      DEVICE_ASSERT_MSG(resultConst == expected, op);

      // Reset:
      bit = origBit;
      portal.SetBitAtomic(i, bit);
      return true;
    };

    portal.SetBit(i, bit);
    DEVICE_ASSERT(testValues("SetBit"));

    bit = mod;
    portal.SetBitAtomic(i, mod);
    DEVICE_ASSERT(testValues("SetBitAtomic"));

    bit = !bit;
    portal.NotBitAtomic(i);
    DEVICE_ASSERT(testValues("NotBitAtomic"));

    bit = bit && mod;
    portal.AndBitAtomic(i, mod);
    DEVICE_ASSERT(testValues("AndBitAtomic"));

    bit = bit || mod;
    portal.OrBitAtomic(i, mod);
    DEVICE_ASSERT(testValues("OrBitAtomic"));

    bit = bit != mod;
    portal.XorBitAtomic(i, mod);
    DEVICE_ASSERT(testValues("XorBitAtomic"));

    const auto notBit = !bit;
    bool casResult = portal.CompareAndSwapBitAtomic(i, bit, notBit);
    DEVICE_ASSERT(casResult == bit);
    DEVICE_ASSERT(portal.GetBit(i) == bit);
    DEVICE_ASSERT(portalConst.GetBit(i) == bit);
    casResult = portal.CompareAndSwapBitAtomic(i, notBit, bit);
    DEVICE_ASSERT(casResult == bit);
    DEVICE_ASSERT(portal.GetBit(i) == notBit);
    DEVICE_ASSERT(portalConst.GetBit(i) == notBit);

    return true;
  }

  template <typename WordType, typename PortalType, typename PortalConstType>
  VTKM_EXEC_CONT static bool HelpTestWord(vtkm::Id i,
                                          PortalType portal,
                                          PortalConstType portalConst)
  {
    const auto origWord = RandomWordFromIndex<WordType>(i);
    auto word = origWord;

    const auto mod = RandomWordFromIndex<WordType>(i + NUM_BITS);

    auto testValues = [&](const char* op) -> bool {
      auto expected = word;
      auto result = portal.template GetWordAtomic<WordType>(i);
      auto resultConst = portalConst.template GetWordAtomic<WordType>(i);
      DEVICE_ASSERT_MSG(result == expected, op);
      DEVICE_ASSERT_MSG(resultConst == expected, op);

      // Reset:
      word = origWord;
      portal.SetWordAtomic(i, word);

      return true;
    };

    portal.SetWord(i, word);
    DEVICE_ASSERT(testValues("SetWord"));

    word = mod;
    portal.SetWordAtomic(i, mod);
    DEVICE_ASSERT(testValues("SetWordAtomic"));

    // C++ promotes e.g. uint8 to int32 when performing bitwise not. Silence
    // conversion warning and mask unimportant bits:
    word = static_cast<WordType>(~word);
    portal.template NotWordAtomic<WordType>(i);
    DEVICE_ASSERT(testValues("NotWordAtomic"));

    word = word & mod;
    portal.AndWordAtomic(i, mod);
    DEVICE_ASSERT(testValues("AndWordAtomic"));

    word = word | mod;
    portal.OrWordAtomic(i, mod);
    DEVICE_ASSERT(testValues("OrWordAtomic"));

    word = word ^ mod;
    portal.XorWordAtomic(i, mod);
    DEVICE_ASSERT(testValues("XorWordAtomic"));

    const WordType notWord = static_cast<WordType>(~word);
    auto casResult = portal.CompareAndSwapWordAtomic(i, word, notWord);
    DEVICE_ASSERT(casResult == word);
    DEVICE_ASSERT(portal.template GetWord<WordType>(i) == word);
    DEVICE_ASSERT(portalConst.template GetWord<WordType>(i) == word);
    casResult = portal.CompareAndSwapWordAtomic(i, notWord, word);
    DEVICE_ASSERT(casResult == word);
    DEVICE_ASSERT(portal.template GetWord<WordType>(i) == notWord);
    DEVICE_ASSERT(portalConst.template GetWord<WordType>(i) == notWord);

    return true;
  }

  template <typename PortalType, typename PortalConstType>
  struct HelpTestWordOpsControl
  {
    PortalType Portal;
    PortalConstType PortalConst;

    VTKM_CONT
    HelpTestWordOpsControl(PortalType portal, PortalConstType portalConst)
      : Portal(portal)
      , PortalConst(portalConst)
    {
    }

    template <typename WordType>
    VTKM_CONT void operator()(WordType)
    {
      const auto numWords = this->Portal.template GetNumberOfWords<WordType>();
      VTKM_TEST_ASSERT(numWords == this->PortalConst.template GetNumberOfWords<WordType>());
      for (vtkm::Id i = 0; i < numWords; ++i)
      {
        VTKM_TEST_ASSERT(HelpTestWord<WordType>(i, this->Portal, this->PortalConst));
      }
    }
  };

  template <typename Portal, typename PortalConst>
  VTKM_CONT static void HelpTestPortalsControl(Portal portal, PortalConst portalConst)
  {
    const auto numWords8 = (NUM_BITS + 7) / 8;
    const auto numWords16 = (NUM_BITS + 15) / 16;
    const auto numWords32 = (NUM_BITS + 31) / 32;
    const auto numWords64 = (NUM_BITS + 63) / 64;

    VTKM_TEST_ASSERT(portal.GetNumberOfBits() == NUM_BITS);
    VTKM_TEST_ASSERT(portal.template GetNumberOfWords<vtkm::UInt8>() == numWords8);
    VTKM_TEST_ASSERT(portal.template GetNumberOfWords<vtkm::UInt16>() == numWords16);
    VTKM_TEST_ASSERT(portal.template GetNumberOfWords<vtkm::UInt32>() == numWords32);
    VTKM_TEST_ASSERT(portal.template GetNumberOfWords<vtkm::UInt64>() == numWords64);
    VTKM_TEST_ASSERT(portalConst.GetNumberOfBits() == NUM_BITS);
    VTKM_TEST_ASSERT(portalConst.template GetNumberOfWords<vtkm::UInt8>() == numWords8);
    VTKM_TEST_ASSERT(portalConst.template GetNumberOfWords<vtkm::UInt16>() == numWords16);
    VTKM_TEST_ASSERT(portalConst.template GetNumberOfWords<vtkm::UInt32>() == numWords32);
    VTKM_TEST_ASSERT(portalConst.template GetNumberOfWords<vtkm::UInt64>() == numWords64);

    for (vtkm::Id i = 0; i < NUM_BITS; ++i)
    {
      HelpTestBit(i, portal, portalConst);
    }

    HelpTestWordOpsControl<Portal, PortalConst> test(portal, portalConst);
    vtkm::ListForEach(test, typename Portal::AtomicInterface::WordTypes{});
  }

  VTKM_CONT
  static void TestControlPortals()
  {
    auto field = RandomBitField();
    auto portal = field.GetPortalControl();
    auto portalConst = field.GetPortalConstControl();

    HelpTestPortalsControl(portal, portalConst);
  }

  template <typename Portal>
  VTKM_EXEC_CONT static bool HelpTestPortalSanityExecution(Portal portal)
  {
    const auto numWords8 = (NUM_BITS + 7) / 8;
    const auto numWords16 = (NUM_BITS + 15) / 16;
    const auto numWords32 = (NUM_BITS + 31) / 32;
    const auto numWords64 = (NUM_BITS + 63) / 64;

    DEVICE_ASSERT(portal.GetNumberOfBits() == NUM_BITS);
    DEVICE_ASSERT(portal.template GetNumberOfWords<vtkm::UInt8>() == numWords8);
    DEVICE_ASSERT(portal.template GetNumberOfWords<vtkm::UInt16>() == numWords16);
    DEVICE_ASSERT(portal.template GetNumberOfWords<vtkm::UInt32>() == numWords32);
    DEVICE_ASSERT(portal.template GetNumberOfWords<vtkm::UInt64>() == numWords64);

    return true;
  }

  template <typename WordType, typename PortalType, typename PortalConstType>
  struct HelpTestPortalsExecutionWordsFunctor : vtkm::exec::FunctorBase
  {
    PortalType Portal;
    PortalConstType PortalConst;

    HelpTestPortalsExecutionWordsFunctor(PortalType portal, PortalConstType portalConst)
      : Portal(portal)
      , PortalConst(portalConst)
    {
    }

    VTKM_EXEC_CONT
    void operator()(vtkm::Id i) const
    {
      if (i == 0)
      {
        if (!HelpTestPortalSanityExecution(this->Portal))
        {
          this->RaiseError("Testing Portal sanity failed.");
          return;
        }
        if (!HelpTestPortalSanityExecution(this->PortalConst))
        {
          this->RaiseError("Testing PortalConst sanity failed.");
          return;
        }
      }

      if (!HelpTestWord<WordType>(i, this->Portal, this->PortalConst))
      {
        this->RaiseError("Testing word operations failed.");
        return;
      }
    }
  };

  template <typename PortalType, typename PortalConstType>
  struct HelpTestPortalsExecutionBitsFunctor : vtkm::exec::FunctorBase
  {
    PortalType Portal;
    PortalConstType PortalConst;

    HelpTestPortalsExecutionBitsFunctor(PortalType portal, PortalConstType portalConst)
      : Portal(portal)
      , PortalConst(portalConst)
    {
    }

    VTKM_EXEC_CONT
    void operator()(vtkm::Id i) const
    {
      if (!HelpTestBit(i, this->Portal, this->PortalConst))
      {
        this->RaiseError("Testing bit operations failed.");
        return;
      }
    }
  };

  template <typename PortalType, typename PortalConstType>
  struct HelpTestWordOpsExecution
  {
    PortalType Portal;
    PortalConstType PortalConst;

    VTKM_CONT
    HelpTestWordOpsExecution(PortalType portal, PortalConstType portalConst)
      : Portal(portal)
      , PortalConst(portalConst)
    {
    }

    template <typename WordType>
    VTKM_CONT void operator()(WordType)
    {
      const auto numWords = this->Portal.template GetNumberOfWords<WordType>();
      VTKM_TEST_ASSERT(numWords == this->PortalConst.template GetNumberOfWords<WordType>());

      using WordFunctor =
        HelpTestPortalsExecutionWordsFunctor<WordType, PortalType, PortalConstType>;
      WordFunctor test{ this->Portal, this->PortalConst };
      Algo::Schedule(test, numWords);
    }
  };

  template <typename Portal, typename PortalConst>
  VTKM_CONT static void HelpTestPortalsExecution(Portal portal, PortalConst portalConst)
  {
    HelpTestPortalsExecutionBitsFunctor<Portal, PortalConst> bitTest{ portal, portalConst };
    Algo::Schedule(bitTest, portal.GetNumberOfBits());


    HelpTestWordOpsExecution<Portal, PortalConst> test(portal, portalConst);
    vtkm::ListForEach(test, typename Portal::AtomicInterface::WordTypes{});
  }

  VTKM_CONT
  static void TestExecutionPortals()
  {
    auto field = RandomBitField();
    auto portal = field.PrepareForInPlace(DeviceAdapterTag{});
    auto portalConst = field.PrepareForInput(DeviceAdapterTag{});

    HelpTestPortalsExecution(portal, portalConst);
  }

  VTKM_CONT
  static void TestFinalWordMask()
  {
    auto testMask32 = [](vtkm::Id numBits, vtkm::UInt32 expectedMask) {
      vtkm::cont::BitField field;
      field.Allocate(numBits);
      auto mask = field.GetPortalConstControl().GetFinalWordMask<vtkm::UInt32>();

      VTKM_TEST_ASSERT(expectedMask == mask,
                       "Unexpected mask for BitField size ",
                       numBits,
                       ": Expected 0x",
                       std::hex,
                       expectedMask,
                       " got 0x",
                       mask);
    };

    auto testMask64 = [](vtkm::Id numBits, vtkm::UInt64 expectedMask) {
      vtkm::cont::BitField field;
      field.Allocate(numBits);
      auto mask = field.GetPortalConstControl().GetFinalWordMask<vtkm::UInt64>();

      VTKM_TEST_ASSERT(expectedMask == mask,
                       "Unexpected mask for BitField size ",
                       numBits,
                       ": Expected 0x",
                       std::hex,
                       expectedMask,
                       " got 0x",
                       mask);
    };

    testMask32(0, 0x00000000);
    testMask32(1, 0x00000001);
    testMask32(2, 0x00000003);
    testMask32(3, 0x00000007);
    testMask32(4, 0x0000000f);
    testMask32(5, 0x0000001f);
    testMask32(8, 0x000000ff);
    testMask32(16, 0x0000ffff);
    testMask32(24, 0x00ffffff);
    testMask32(25, 0x01ffffff);
    testMask32(31, 0x7fffffff);
    testMask32(32, 0xffffffff);
    testMask32(64, 0xffffffff);
    testMask32(128, 0xffffffff);
    testMask32(129, 0x00000001);

    testMask64(0, 0x0000000000000000);
    testMask64(1, 0x0000000000000001);
    testMask64(2, 0x0000000000000003);
    testMask64(3, 0x0000000000000007);
    testMask64(4, 0x000000000000000f);
    testMask64(5, 0x000000000000001f);
    testMask64(8, 0x00000000000000ff);
    testMask64(16, 0x000000000000ffff);
    testMask64(24, 0x0000000000ffffff);
    testMask64(25, 0x0000000001ffffff);
    testMask64(31, 0x000000007fffffff);
    testMask64(32, 0x00000000ffffffff);
    testMask64(40, 0x000000ffffffffff);
    testMask64(48, 0x0000ffffffffffff);
    testMask64(56, 0x00ffffffffffffff);
    testMask64(64, 0xffffffffffffffff);
    testMask64(128, 0xffffffffffffffff);
    testMask64(129, 0x0000000000000001);
  }

  struct ArrayHandleBitFieldChecker : vtkm::exec::FunctorBase
  {
    using PortalType = typename ArrayHandleBitField::ExecutionTypes<DeviceAdapterTag>::Portal;

    PortalType Portal;
    bool InvertReference;

    VTKM_EXEC_CONT
    ArrayHandleBitFieldChecker(PortalType portal, bool invert)
      : Portal(portal)
      , InvertReference(invert)
    {
    }

    VTKM_EXEC
    void operator()(vtkm::Id i) const
    {
      const bool ref = this->InvertReference ? !RandomBitFromIndex(i) : RandomBitFromIndex(i);
      if (this->Portal.Get(i) != ref)
      {
        this->RaiseError("Unexpected value from ArrayHandleBitField portal.");
        return;
      }

      // Flip the bit for the next kernel launch, which tests that the bitfield
      // is inverted.
      this->Portal.Set(i, !ref);
    }
  };

  VTKM_CONT
  static void TestArrayHandleBitField()
  {
    auto handle = vtkm::cont::make_ArrayHandleBitField(RandomBitField());
    const vtkm::Id numBits = handle.GetNumberOfValues();

    VTKM_TEST_ASSERT(numBits == NUM_BITS,
                     "ArrayHandleBitField returned the wrong number of values. "
                     "Expected: ",
                     NUM_BITS,
                     " got: ",
                     numBits);

    Algo::Schedule(
      ArrayHandleBitFieldChecker{ handle.PrepareForInPlace(DeviceAdapterTag{}), false }, numBits);
    Algo::Schedule(ArrayHandleBitFieldChecker{ handle.PrepareForInPlace(DeviceAdapterTag{}), true },
                   numBits);
  }

  VTKM_CONT
  static void TestArrayInvokeWorklet()
  {
    auto condArray = vtkm::cont::make_ArrayHandleBitField(RandomBitField());
    auto trueArray = vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(20, 2, NUM_BITS);
    auto falseArray = vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(13, 2, NUM_BITS);
    vtkm::cont::ArrayHandle<vtkm::Id> output;

    vtkm::cont::Invoker invoke;
    invoke(ConditionalMergeWorklet{}, condArray, trueArray, falseArray, output);

    auto condVals = condArray.GetPortalConstControl();
    auto trueVals = trueArray.GetPortalConstControl();
    auto falseVals = falseArray.GetPortalConstControl();
    auto outVals = output.GetPortalConstControl();

    VTKM_TEST_ASSERT(condVals.GetNumberOfValues() == trueVals.GetNumberOfValues());
    VTKM_TEST_ASSERT(condVals.GetNumberOfValues() == falseVals.GetNumberOfValues());
    VTKM_TEST_ASSERT(condVals.GetNumberOfValues() == outVals.GetNumberOfValues());

    for (vtkm::Id i = 0; i < condVals.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(outVals.Get(i) == (condVals.Get(i) ? trueVals.Get(i) : falseVals.Get(i)));
    }
  }

  VTKM_CONT
  static void TestArrayInvokeWorklet2()
  {
    auto condBits = RandomBitField();
    auto trueArray = vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(20, 2, NUM_BITS);
    auto falseArray = vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(13, 2, NUM_BITS);
    vtkm::cont::ArrayHandle<vtkm::Id> output;

    vtkm::cont::Invoker invoke;
    invoke(ConditionalMergeWorklet2{}, condBits, trueArray, falseArray, output);

    auto condVals = condBits.GetPortalConstControl();
    auto trueVals = trueArray.GetPortalConstControl();
    auto falseVals = falseArray.GetPortalConstControl();
    auto outVals = output.GetPortalConstControl();

    VTKM_TEST_ASSERT(condVals.GetNumberOfBits() == trueVals.GetNumberOfValues());
    VTKM_TEST_ASSERT(condVals.GetNumberOfBits() == falseVals.GetNumberOfValues());
    VTKM_TEST_ASSERT(condVals.GetNumberOfBits() == outVals.GetNumberOfValues());

    for (vtkm::Id i = 0; i < condVals.GetNumberOfBits(); ++i)
    {
      // The worklet flips the bitfield in place after choosing true/false paths
      VTKM_TEST_ASSERT(condVals.GetBit(i) == !RandomBitFromIndex(i));
      VTKM_TEST_ASSERT(outVals.Get(i) ==
                       (!condVals.GetBit(i) ? trueVals.Get(i) : falseVals.Get(i)));
    }
  }

  struct TestRunner
  {
    VTKM_CONT
    void operator()() const
    {
      TestingBitField::TestBlockAllocation();
      TestingBitField::TestControlPortals();
      TestingBitField::TestExecutionPortals();
      TestingBitField::TestFinalWordMask();
      TestingBitField::TestArrayHandleBitField();
      TestingBitField::TestArrayInvokeWorklet();
      TestingBitField::TestArrayInvokeWorklet2();
    }
  };

public:
  static VTKM_CONT int Run(int argc, char* argv[])
  {
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapterTag());
    return vtkm::cont::testing::Testing::Run(TestRunner{}, argc, argv);
  }
};
}
}
} // namespace vtkm::cont::testing

#endif // vtk_m_cont_testing_TestingBitFields_h
