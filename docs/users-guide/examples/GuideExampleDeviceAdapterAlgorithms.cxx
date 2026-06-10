//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include <viskores/BinaryOperators.h>
#include <viskores/BinaryPredicates.h>
#include <viskores/UnaryPredicates.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/Types.h>
#include <viskores/cont/testing/Testing.h>

#include <vector>

////
//// BEGIN-EXAMPLE DeviceAdapterAlgorithmPrototype
////
namespace viskores
{
namespace cont
{

struct Algorithm;
}
} // namespace viskores
////
//// END-EXAMPLE DeviceAdapterAlgorithmPrototype
////

namespace
{

template<typename T>
void CheckArray(const std::string& name,
                const viskores::cont::ArrayHandle<T>& array,
                const std::vector<T>& expected)
{
  viskores::Id numValues = array.GetNumberOfValues();
  auto portal = array.ReadPortal();
  std::cout << name << ": { ";
  for (viskores::Id index = 0; index < numValues; ++index)
  {
    std::cout << portal.Get(index) << ", ";
  }
  std::cout << "}" << std::endl;

  VISKORES_TEST_ASSERT(numValues == static_cast<viskores::Id>(expected.size()),
                       "Array is wrong size.");

  for (viskores::Id index = 0; index < numValues; ++index)
  {
    VISKORES_TEST_ASSERT(test_equal(portal.Get(index), expected[std::size_t(index)]),
                         "Bad values.");
  }
}

#define CHECK_ARRAY(array, T, ...)                                                      \
  CheckArray(#array, array, std::vector<T>{ __VA_ARGS__ })

struct DoFunctor
{
  VISKORES_CONT void DoBitFieldToUnorderedSet()
  {
    std::cout << "Testing BitFieldToUnorderedSet" << std::endl;
    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmBitFieldToUnorderedSet
    ////
    viskores::cont::BitField bits;
    bits.Allocate(32);

    auto fillPortal = bits.WritePortal();
    fillPortal.SetWord(0, viskores::UInt32(0xaa770011));

    viskores::cont::ArrayHandle<viskores::Id> output;
    auto setBits = viskores::cont::Algorithm::BitFieldToUnorderedSet(bits, output);
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmBitFieldToUnorderedSet
    ////
    VISKORES_TEST_ASSERT(setBits == 12, "Wrong number of Values.");
    viskores::cont::Algorithm::Sort(output);
    CHECK_ARRAY(output, viskores::Id, 0, 4, 16, 17, 18, 20, 21, 22, 25, 27, 29, 31);
  }

  VISKORES_CONT void DoCopy()
  {
    std::cout << "Testing Copy" << std::endl;
    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmCopy
    ////
    viskores::cont::ArrayHandleIndex input(12);

    viskores::cont::ArrayHandle<viskores::Int32> output;

    viskores::cont::Algorithm::Copy(input, output);

    // output has { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmCopy
    ////
    CHECK_ARRAY(output, viskores::Int32, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
  }

  ////
  //// BEGIN-EXAMPLE DeviceAdapterAlgorithmCopyIf
  ////
  struct LessThan5
  {
    VISKORES_EXEC_CONT bool operator()(viskores::Int32 x) const { return x < 5; }
  };


  //// PAUSE-EXAMPLE
  VISKORES_CONT void DoCopyIf()
  {
    std::cout << "Testing CopyIf" << std::endl;
    //// RESUME-EXAMPLE
    viskores::cont::ArrayHandle<viskores::Int32> input =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });
    viskores::cont::ArrayHandle<viskores::UInt8> stencil =
      viskores::cont::make_ArrayHandle<viskores::UInt8>(
        { 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1 });

    viskores::cont::ArrayHandle<viskores::Int32> output;

    viskores::cont::Algorithm::CopyIf(input, stencil, output);

    // output has { 0, 5, 3, 8, 3 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(output, viskores::Int32, 0, 5, 3, 8, 3);
    //// RESUME-EXAMPLE

    viskores::cont::Algorithm::CopyIf(input, input, output, LessThan5());

    // output has { 0, 1, 1, 4, 3, 3 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(output, viskores::Int32, 0, 1, 1, 4, 3, 3);
    //// RESUME-EXAMPLE
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmCopyIf
    ////
  }

  VISKORES_CONT void DoCopySubRange()
  {
    std::cout << "Testing CopySubRange" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmCopySubRange
    ////
    viskores::cont::ArrayHandle<viskores::Int32> input =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });

    viskores::cont::ArrayHandle<viskores::Int32> output;

    viskores::cont::Algorithm::CopySubRange(input, 1, 7, output);

    // output has { 0, 1, 1, 5, 5, 4, 3 }
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmCopySubRange
    ////
    CHECK_ARRAY(output, viskores::Int32, 0, 1, 1, 5, 5, 4, 3);
  }

  VISKORES_CONT void DoCountSetBits()
  {
    std::cout << "Testing CountSetBits" << std::endl;
    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmCountSetBits
    ////
    viskores::cont::BitField bits;
    bits.Allocate(32);

    auto fillPortal = bits.WritePortal();
    fillPortal.SetWord(0, viskores::UInt32(0xaa770011));

    viskores::cont::ArrayHandle<viskores::Id> output;
    auto setBits = viskores::cont::Algorithm::CountSetBits(bits);

    // Will return that there are 12 set bits
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmCountSetBits
    ////
    VISKORES_TEST_ASSERT(setBits == 12, "Wrong number of Values.");
  }

  VISKORES_CONT void DoFill()
  {
    std::cout << "Testing Fill" << std::endl;
    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmFill
    ////
    // Fill a BitField
    viskores::cont::BitField bits;
    bits.Allocate(32);
    viskores::cont::Algorithm::Fill(bits, true);
    //// PAUSE-EXAMPLE
    VISKORES_TEST_ASSERT(viskores::cont::Algorithm::CountSetBits(bits) == 32,
                         "Wrong number of Values.");
    //// RESUME-EXAMPLE
    // Will stamp the 8 bit word across 32 bits to result in bits = 0xf0f0f0f0
    viskores::cont::Algorithm::Fill(bits, viskores::UInt8(0xf0));
    //// PAUSE-EXAMPLE
    VISKORES_TEST_ASSERT(viskores::cont::Algorithm::CountSetBits(bits) == 16,
                         "Wrong number of Values.");
    //// RESUME-EXAMPLE
    viskores::cont::Algorithm::Fill(bits, viskores::UInt8(0xf0), 16);
    //// PAUSE-EXAMPLE
    VISKORES_TEST_ASSERT(viskores::cont::Algorithm::CountSetBits(bits) == 8,
                         "Wrong number of Values.");
    //// RESUME-EXAMPLE

    // Fill an ArrayHandle
    viskores::cont::ArrayHandle<viskores::Id> arrayHandle;
    arrayHandle.Allocate(10);
    viskores::cont::Algorithm::Fill(arrayHandle, viskores::Id(5));
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(arrayHandle, viskores::Id, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5);
    //// RESUME-EXAMPLE
    viskores::cont::Algorithm::Fill(arrayHandle, viskores::Id(10), 5);
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmFill
    ////
    CHECK_ARRAY(arrayHandle, viskores::Id, 10, 10, 10, 10, 10);
  }


  VISKORES_CONT void DoLowerBounds()
  {
    std::cout << "Testing LowerBounds" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmLowerBounds
    ////
    viskores::cont::ArrayHandle<viskores::Int32> sorted =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 0, 1, 1, 3, 3, 4, 5, 5, 7, 7, 8, 9 });
    viskores::cont::ArrayHandle<viskores::Int32> values =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });

    viskores::cont::ArrayHandle<viskores::Id> output;

    viskores::cont::Algorithm::LowerBounds(sorted, values, output);

    // output has { 8, 0, 1, 1, 6, 6, 5, 3, 8, 10, 11, 3 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(output, viskores::Id, 8, 0, 1, 1, 6, 6, 5, 3, 8, 10, 11, 3);
    //// RESUME-EXAMPLE

    viskores::cont::ArrayHandle<viskores::Int32> reverseSorted =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 9, 8, 7, 7, 5, 5, 4, 3, 3, 1, 1, 0 });

    viskores::cont::Algorithm::LowerBounds(
      reverseSorted, values, output, viskores::SortGreater());

    // output has { 2, 11, 9, 9, 4, 4, 6, 7, 2, 1, 0, 7 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(output, viskores::Id, 2, 11, 9, 9, 4, 4, 6, 7, 2, 1, 0, 7);
    //// RESUME-EXAMPLE
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmLowerBounds
    ////
  }

  VISKORES_CONT void DoReduce()
  {
    std::cout << "Testing Reduce" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmReduce
    ////
    viskores::cont::ArrayHandle<viskores::Id> input =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 5, 1, 1, 6 });

    //// LABEL reduce-literal
    viskores::Id sum = viskores::cont::Algorithm::Reduce(input, viskores::Id{ 0 });
    // sum is 13

    viskores::Id product =
      viskores::cont::Algorithm::Reduce(input, viskores::Id{ 1 }, viskores::Multiply());
    // product is 30
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmReduce
    ////

    VISKORES_TEST_ASSERT(sum == 13, "Sum wrong.");
    VISKORES_TEST_ASSERT(product == 30, "Product wrong.");
  }

  VISKORES_CONT void DoReduceByKey()
  {
    std::cout << "Testing ReduceByKey" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmReduceByKey
    ////
    viskores::cont::ArrayHandle<viskores::Id> keys =
      viskores::cont::make_ArrayHandle<viskores::Id>(
        { 0, 0, 3, 3, 3, 3, 5, 6, 6, 6, 6, 6 });
    viskores::cont::ArrayHandle<viskores::Int32> input =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });

    viskores::cont::ArrayHandle<viskores::Id> uniqueKeys;
    viskores::cont::ArrayHandle<viskores::Int32> sums;

    viskores::cont::Algorithm::ReduceByKey(
      keys, input, uniqueKeys, sums, viskores::Add());

    // uniqueKeys is { 0, 3, 5, 6 }
    // sums is { 7, 12, 4, 30 }

    viskores::cont::ArrayHandle<viskores::Int32> products;

    viskores::cont::Algorithm::ReduceByKey(
      keys, input, uniqueKeys, products, viskores::Multiply());

    // products is { 0, 25, 4, 4536 }
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmReduceByKey
    ////

    CHECK_ARRAY(uniqueKeys, viskores::Id, 0, 3, 5, 6);
    CHECK_ARRAY(sums, viskores::Int32, 7, 12, 4, 30);
    CHECK_ARRAY(products, viskores::Int32, 0, 25, 4, 4536);
  }

  VISKORES_CONT void DoScanInclusive()
  {
    std::cout << "Testing ScanInclusive" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmScanInclusive
    ////
    viskores::cont::ArrayHandle<viskores::Int32> input =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });

    viskores::cont::ArrayHandle<viskores::Int32> runningSum;

    viskores::cont::Algorithm::ScanInclusive(input, runningSum);

    // runningSum is { 7, 7, 8, 9, 14, 19, 23, 26, 33, 41, 50, 53 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(runningSum, viskores::Int32, 7, 7, 8, 9, 14, 19, 23, 26, 33, 41, 50, 53);
    //// RESUME-EXAMPLE

    viskores::cont::ArrayHandle<viskores::Int32> runningMax;

    viskores::cont::Algorithm::ScanInclusive(input, runningMax, viskores::Maximum());

    // runningMax is { 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 9, 9 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(runningMax, viskores::Int32, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 9, 9);
    //// RESUME-EXAMPLE
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmScanInclusive
    ////
  }

  VISKORES_CONT void DoScanInclusiveByKey()
  {
    std::cout << "Testing ScanInclusiveByKey" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmScanInclusiveByKey
    ////
    viskores::cont::ArrayHandle<viskores::Id> keys =
      viskores::cont::make_ArrayHandle<viskores::Id>(
        { 0, 0, 3, 3, 3, 3, 5, 6, 6, 6, 6, 6 });
    viskores::cont::ArrayHandle<viskores::Int32> input =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });

    viskores::cont::ArrayHandle<viskores::Int32> runningSums;

    viskores::cont::Algorithm::ScanInclusiveByKey(keys, input, runningSums);

    // runningSums is { 7, 7, 1, 2, 7, 12, 4, 3, 10, 18, 27, 30 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(runningSums, viskores::Int32, 7, 7, 1, 2, 7, 12, 4, 3, 10, 18, 27, 30);
    //// RESUME-EXAMPLE

    viskores::cont::ArrayHandle<viskores::Int32> runningMaxes;

    viskores::cont::Algorithm::ScanInclusiveByKey(
      keys, input, runningMaxes, viskores::Maximum());

    // runningMax is { 7, 7, 1, 1, 5, 5, 4, 3, 7, 8, 9, 9 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(runningMaxes, viskores::Int32, 7, 7, 1, 1, 5, 5, 4, 3, 7, 8, 9, 9);
    //// RESUME-EXAMPLE
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmScanInclusiveByKey
    ////
  }

  VISKORES_CONT void DoScanExclusive()
  {
    std::cout << "Testing ScanExclusive" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmScanExclusive
    ////
    viskores::cont::ArrayHandle<viskores::Int32> input =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });

    viskores::cont::ArrayHandle<viskores::Int32> runningSum;

    viskores::cont::Algorithm::ScanExclusive(input, runningSum);

    // runningSum is { 0, 7, 7, 8, 9, 14, 19, 23, 26, 33, 41, 50 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(runningSum, viskores::Int32, 0, 7, 7, 8, 9, 14, 19, 23, 26, 33, 41, 50);
    //// RESUME-EXAMPLE

    viskores::cont::ArrayHandle<viskores::Int32> runningMax;

    viskores::cont::Algorithm::ScanExclusive(input, runningMax, viskores::Maximum(), -1);

    // runningMax is { -1, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 9 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(runningMax, viskores::Int32, -1, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 9);
    //// RESUME-EXAMPLE
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmScanExclusive
    ////
  }

  VISKORES_CONT void DoScanExclusiveByKey()
  {
    std::cout << "Testing ScanExclusiveByKey" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmScanExclusiveByKey
    ////
    viskores::cont::ArrayHandle<viskores::Id> keys =
      viskores::cont::make_ArrayHandle<viskores::Id>(
        { 0, 0, 3, 3, 3, 3, 5, 6, 6, 6, 6, 6 });
    viskores::cont::ArrayHandle<viskores::Int32> input =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });

    viskores::cont::ArrayHandle<viskores::Int32> runningSums;

    viskores::cont::Algorithm::ScanExclusiveByKey(keys, input, runningSums);

    // runningSums is { 0, 7, 0, 1, 2, 7, 0, 0, 3, 10, 18, 27 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(runningSums, viskores::Int32, 0, 7, 0, 1, 2, 7, 0, 0, 3, 10, 18, 27);
    //// RESUME-EXAMPLE

    viskores::cont::ArrayHandle<viskores::Int32> runningMaxes;

    viskores::cont::Algorithm::ScanExclusiveByKey(
      keys, input, runningMaxes, -1, viskores::Maximum());

    // runningMax is { -1, 7, -1, 1, 1, 5, -1, -1, 3, 7, 8, 9 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(runningMaxes, viskores::Int32, -1, 7, -1, 1, 1, 5, -1, -1, 3, 7, 8, 9);
    //// RESUME-EXAMPLE
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmScanExclusiveByKey
    ////
  }

  VISKORES_CONT void DoScanExtended()
  {
    std::cout << "Testing ScanExtended" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmScanExtended
    ////
    viskores::cont::ArrayHandle<viskores::Int32> input =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });

    viskores::cont::ArrayHandle<viskores::Int32> runningSum;
    viskores::cont::Algorithm::ScanExtended(input, runningSum);

    // runningSum is { 0, 7, 7, 8, 9, 14, 19, 23, 26, 33, 41, 50, 53 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(
      runningSum, viskores::Int32, 0, 7, 7, 8, 9, 14, 19, 23, 26, 33, 41, 50, 53);
    //// RESUME-EXAMPLE

    viskores::cont::ArrayHandle<viskores::Int32> runningMax;
    viskores::cont::Algorithm::ScanExtended(input, runningMax, viskores::Maximum(), -1);

    // runningMax is { -1, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 9, 9 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(runningMax, viskores::Int32, -1, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 9, 9);
    //// RESUME-EXAMPLE
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmScanExtended
    ////
  }

  VISKORES_CONT void DoSort()
  {
    std::cout << "Testing Sort" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmSort
    ////
    viskores::cont::ArrayHandle<viskores::Int32> array =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });

    viskores::cont::Algorithm::Sort(array);

    // array has { 0, 1, 1, 3, 3, 4, 5, 5, 7, 7, 8, 9 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(array, viskores::Int32, 0, 1, 1, 3, 3, 4, 5, 5, 7, 7, 8, 9);
    //// RESUME-EXAMPLE

    viskores::cont::Algorithm::Sort(array, viskores::SortGreater());

    // array has { 9, 8, 7, 7, 5, 5, 4, 3, 3, 1, 1, 0 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(array, viskores::Int32, 9, 8, 7, 7, 5, 5, 4, 3, 3, 1, 1, 0);
    //// RESUME-EXAMPLE
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmSort
    ////
  }

  VISKORES_CONT void DoSortByKey()
  {
    std::cout << "Testing SortByKey" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmSortByKey
    ////
    viskores::cont::ArrayHandle<viskores::Int32> keys =
      viskores::cont::make_ArrayHandle<viskores::Int32>({ 7, 0, 1, 5, 4, 8, 9, 3 });
    viskores::cont::ArrayHandle<viskores::Id> values =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 2, 3, 4, 5, 6, 7 });

    viskores::cont::Algorithm::SortByKey(keys, values);

    // keys has   { 0, 1, 3, 4, 5, 7, 8, 9 }
    // values has { 1, 2, 7, 4, 3, 0, 5, 6 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(keys, viskores::Int32, 0, 1, 3, 4, 5, 7, 8, 9);
    CHECK_ARRAY(values, viskores::Id, 1, 2, 7, 4, 3, 0, 5, 6);
    //// RESUME-EXAMPLE

    viskores::cont::Algorithm::SortByKey(keys, values, viskores::SortGreater());

    // keys has   { 9, 8, 7, 5, 4, 3, 1, 0 }
    // values has { 6, 5, 0, 3, 4, 7, 2, 1 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(keys, viskores::Int32, 9, 8, 7, 5, 4, 3, 1, 0);
    CHECK_ARRAY(values, viskores::Id, 6, 5, 0, 3, 4, 7, 2, 1);
    //// RESUME-EXAMPLE
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmSortByKey
    ////
  }

  VISKORES_CONT void DoTransform()
  {
    std::cout << "Testing Transform" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmTransform
    ////
    viskores::cont::ArrayHandle<viskores::Int32> input1 =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });
    viskores::cont::ArrayHandle<viskores::Int32> input2 =
      viskores::cont::make_ArrayHandle<viskores::Int32>({ 2, 3, 4, 5, 6, 7, 8, 9, 10 });

    viskores::cont::ArrayHandle<viskores::Int32> output;
    viskores::cont::Algorithm::Transform(input1, input2, output, viskores::Sum());

    // output is { 3, 5, 7, 9, 11, 13, 15, 17, 19 }
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmTransform
    ////
    CHECK_ARRAY(output, viskores::Int32, 3, 5, 7, 9, 11, 13, 15, 17, 19);
  }

  ////
  //// BEGIN-EXAMPLE DeviceAdapterAlgorithmUnique
  ////
  struct AlmostEqualFunctor
  {
    VISKORES_EXEC_CONT bool operator()(viskores::Float64 x, viskores::Float64 y) const
    {
      return (viskores::Abs(x - y) < 0.1);
    }
  };

  //// PAUSE-EXAMPLE
  VISKORES_CONT void DoUnique()
  {
    std::cout << "Testing Unqiue" << std::endl;

    //// RESUME-EXAMPLE
    viskores::cont::ArrayHandle<viskores::Int32> values =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 0, 1, 1, 3, 3, 4, 5, 5, 7, 7, 7, 9 });

    viskores::cont::Algorithm::Unique(values);

    // values has {0, 1, 3, 4, 5, 7, 9}

    viskores::cont::ArrayHandle<viskores::Float64> fvalues =
      viskores::cont::make_ArrayHandle<viskores::Float64>(
        { 0.0, 0.001, 0.0, 1.5, 1.499, 2.0 });

    viskores::cont::Algorithm::Unique(fvalues, AlmostEqualFunctor());

    // values has {0.0, 1.5, 2.0}
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmUnique
    ////

    CHECK_ARRAY(values, viskores::Int32, 0, 1, 3, 4, 5, 7, 9);
    CHECK_ARRAY(fvalues, viskores::Float64, 0.0, 1.5, 2.0);
  }

  VISKORES_CONT void DoUpperBounds()
  {
    std::cout << "Testing UpperBounds" << std::endl;

    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmUpperBounds
    ////
    viskores::cont::ArrayHandle<viskores::Int32> sorted =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 0, 1, 1, 3, 3, 4, 5, 5, 7, 7, 8, 9 });
    viskores::cont::ArrayHandle<viskores::Int32> values =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });

    viskores::cont::ArrayHandle<viskores::Id> output;

    viskores::cont::Algorithm::UpperBounds(sorted, values, output);

    // output has { 10, 1, 3, 3, 8, 8, 6, 5, 10, 11, 12, 5 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(output, viskores::Id, 10, 1, 3, 3, 8, 8, 6, 5, 10, 11, 12, 5);
    //// RESUME-EXAMPLE

    viskores::cont::ArrayHandle<viskores::Int32> reverseSorted =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 9, 8, 7, 7, 5, 5, 4, 3, 3, 1, 1, 0 });

    viskores::cont::Algorithm::UpperBounds(
      reverseSorted, values, output, viskores::SortGreater());

    // output has { 4, 12, 11, 11, 6, 6, 7, 9, 4, 2, 1, 9 }
    //// PAUSE-EXAMPLE
    CHECK_ARRAY(output, viskores::Id, 4, 12, 11, 11, 6, 6, 7, 9, 4, 2, 1, 9);
    //// RESUME-EXAMPLE
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmUpperBounds
    ////
  }

  VISKORES_CONT void DoDeviceAdapter()
  {
    std::cout << "Testing Copy - specified device" << std::endl;
    // Make sure serial device available.
    viskores::cont::ScopedRuntimeDeviceTracker tracker;
    tracker.Reset();
    ////
    //// BEGIN-EXAMPLE DeviceAdapterAlgorithmDeviceAdapter
    ////
    viskores::cont::ArrayHandle<viskores::Int32> input =
      viskores::cont::make_ArrayHandle<viskores::Int32>(
        { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 });

    viskores::cont::ArrayHandle<viskores::Int32> output_no_device_specified;

    viskores::cont::ArrayHandle<viskores::Int32> output_device_specified;

    viskores::cont::Algorithm::Copy(input, output_no_device_specified);

    //optional we can pass the device or int id number
    viskores::cont::Algorithm::Copy(
      viskores::cont::DeviceAdapterTagSerial(), input, output_device_specified);

    // output has { 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3 }
    ////
    //// END-EXAMPLE DeviceAdapterAlgorithmDeviceAdapter
    ////
    CHECK_ARRAY(
      output_no_device_specified, viskores::Int32, 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3);
    CHECK_ARRAY(
      output_device_specified, viskores::Int32, 7, 0, 1, 1, 5, 5, 4, 3, 7, 8, 9, 3);
  }

  VISKORES_CONT void DoUnaryPredicates()
  {
    std::cout << "Testing Unary Predicates" << std::endl;
    ////
    //// BEGIN-EXAMPLE UnaryPredicates
    ////
    viskores::IsZeroInitialized isZero;
    viskores::NotZeroInitialized isNotZero;
    viskores::LogicalNot logicalNot;

    bool zeroIsZero = isZero(0);
    bool fiveIsNotZero = isNotZero(5);
    bool notFalse = logicalNot(false);
    ////
    //// END-EXAMPLE UnaryPredicates
    ////
    VISKORES_TEST_ASSERT(zeroIsZero, "Unexpected unary predicate result.");
    VISKORES_TEST_ASSERT(fiveIsNotZero, "Unexpected unary predicate result.");
    VISKORES_TEST_ASSERT(notFalse, "Unexpected unary predicate result.");
  }

  VISKORES_CONT void DoBinaryPredicates()
  {
    std::cout << "Testing Binary Predicates" << std::endl;
    ////
    //// BEGIN-EXAMPLE BinaryPredicates
    ////
    viskores::Equal equal;
    viskores::NotEqual notEqual;
    viskores::SortLess less;
    viskores::SortGreater greater;
    viskores::LogicalAnd logicalAnd;
    viskores::LogicalOr logicalOr;

    bool same = equal(4, 4);
    bool different = notEqual(4, 5);
    bool ordered = less(4, 5);
    bool reverseOrdered = greater(5, 4);
    bool both = logicalAnd(true, true);
    bool either = logicalOr(false, true);
    // All above are true.
    ////
    //// END-EXAMPLE BinaryPredicates
    ////
    VISKORES_TEST_ASSERT(same, "Unexpected binary predicate result.");
    VISKORES_TEST_ASSERT(different, "Unexpected binary predicate result.");
    VISKORES_TEST_ASSERT(ordered, "Unexpected binary predicate result.");
    VISKORES_TEST_ASSERT(reverseOrdered, "Unexpected binary predicate result.");
    VISKORES_TEST_ASSERT(both, "Unexpected binary predicate result.");
    VISKORES_TEST_ASSERT(either, "Unexpected binary predicate result.");
  }

  VISKORES_CONT void DoBinaryOperators()
  {
    std::cout << "Testing Binary Operators" << std::endl;
    ////
    //// BEGIN-EXAMPLE BinaryOperators
    ////
    viskores::Sum sum;
    viskores::Product product;
    viskores::Maximum maximum;
    viskores::Minimum minimum;
    viskores::MinAndMax<viskores::Int32> minAndMax;
    viskores::BitwiseAnd bitwiseAnd;
    viskores::BitwiseOr bitwiseOr;
    viskores::BitwiseXor bitwiseXor;

    viskores::Int32 added = sum(2, 3);                         // added == 5
    viskores::Int32 multiplied = product(2, 3);                // multiplied == 6
    viskores::Int32 larger = maximum(2, 3);                    // larger == 3
    viskores::Int32 smaller = minimum(2, 3);                   // smaller == 2
    viskores::Vec<viskores::Int32, 2> range = minAndMax(2, 3); // range == [2, 3]
    viskores::Int32 anded = bitwiseAnd(0x0f, 0x33);            // anded == 0x03
    viskores::Int32 ored = bitwiseOr(0x0f, 0x33);              // ored == 0x3f
    viskores::Int32 xored = bitwiseXor(0x0f, 0x33);            // xored == 0x3c
    ////
    //// END-EXAMPLE BinaryOperators
    ////
    VISKORES_TEST_ASSERT(added == 5, "Unexpected binary operator result.");
    VISKORES_TEST_ASSERT(multiplied == 6, "Unexpected binary operator result.");
    VISKORES_TEST_ASSERT(larger == 3, "Unexpected binary operator result.");
    VISKORES_TEST_ASSERT(smaller == 2, "Unexpected binary operator result.");
    VISKORES_TEST_ASSERT(range[0] == 2 && range[1] == 3,
                         "Unexpected binary operator result.");
    VISKORES_TEST_ASSERT(anded == 0x03, "Unexpected binary operator result.");
    VISKORES_TEST_ASSERT(ored == 0x3f, "Unexpected binary operator result.");
    VISKORES_TEST_ASSERT(xored == 0x3c, "Unexpected binary operator result.");
  }

  ////
  //// BEGIN-EXAMPLE CustomUnaryPredicateImplementation
  ////
  struct IsPowerOfTwo
  {
    VISKORES_EXEC bool operator()(viskores::Id value) const
    {
      return (value > 0) && ((value & (value - 1)) == 0);
    }
  };
  ////
  //// END-EXAMPLE CustomUnaryPredicateImplementation
  ////

  VISKORES_CONT void DoCustomUnaryPredicate()
  {
    std::cout << "Testing Custom Unary Predicate" << std::endl;
    ////
    //// BEGIN-EXAMPLE CustomUnaryPredicateUsage
    ////
    viskores::cont::ArrayHandle<viskores::Id> input =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 2, 3, 4, 5, 8, 12, 16 });
    viskores::cont::ArrayHandle<viskores::Id> output;

    viskores::cont::Algorithm::CopyIf(input, input, output, IsPowerOfTwo());

    // output has { 1, 2, 4, 8, 16 }
    ////
    //// END-EXAMPLE CustomUnaryPredicateUsage
    ////
    CHECK_ARRAY(output, viskores::Id, 1, 2, 4, 8, 16);
  }

  VISKORES_CONT void Run()
  {
    this->DoBitFieldToUnorderedSet();
    this->DoCopy();
    this->DoCopyIf();
    this->DoCopySubRange();
    this->DoCountSetBits();
    this->DoFill();
    this->DoLowerBounds();
    this->DoReduce();
    this->DoReduceByKey();
    this->DoScanInclusive();
    this->DoScanInclusiveByKey();
    this->DoScanExclusive();
    this->DoScanExclusiveByKey();
    this->DoScanExtended();
    this->DoSort();
    this->DoSortByKey();
    this->DoTransform();
    this->DoUnique();
    this->DoUpperBounds();
    this->DoDeviceAdapter();
    this->DoUnaryPredicates();
    this->DoBinaryPredicates();
    this->DoBinaryOperators();
    this->DoCustomUnaryPredicate();
  }
};

void Test()
{
  DoFunctor functor;
  functor.Run();
}

} // anonymous namespace

int GuideExampleDeviceAdapterAlgorithms(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
