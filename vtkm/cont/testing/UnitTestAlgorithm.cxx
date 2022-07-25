//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/BinaryOperators.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>

#include <vtkm/TypeTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{
// The goal of this unit test is not to verify the correctness
// of the various algorithms. Since Algorithm is a header, we
// need to ensure we instantiate each algorithm in a source
// file to verify compilation.
//
static constexpr vtkm::Id ARRAY_SIZE = 10;

TestEqualResult checkBitField(const vtkm::cont::BitField& bitfield,
                              std::initializer_list<bool>&& expected)
{
  TestEqualResult result;
  if (bitfield.GetNumberOfBits() != static_cast<vtkm::Id>(expected.size()))
  {
    result.PushMessage("Unexpected number of bits (" + std::to_string(bitfield.GetNumberOfBits()) +
                       ")");
    return result;
  }

  auto expectedBit = expected.begin();
  auto bitPortal = bitfield.ReadPortal();
  for (vtkm::Id index = 0; index < bitPortal.GetNumberOfBits(); ++index)
  {
    if (bitPortal.GetBit(index) != *expectedBit)
    {
      result.PushMessage("Bad bit at index " + std::to_string(index));
    }
    ++expectedBit;
  }

  return result;
}

template <typename T>
TestEqualResult checkArrayHandle(const vtkm::cont::UnknownArrayHandle& array,
                                 std::initializer_list<T>&& expected)
{
  return test_equal_ArrayHandles(array, vtkm::cont::make_ArrayHandle(std::move(expected)));
}

void FillTest()
{
  vtkm::cont::BitField bits;
  vtkm::cont::ArrayHandle<vtkm::Id> array;

  bits.Allocate(ARRAY_SIZE);
  array.Allocate(ARRAY_SIZE);

  vtkm::cont::Algorithm::Fill(bits, true);
  VTKM_TEST_ASSERT(
    checkBitField(bits, { true, true, true, true, true, true, true, true, true, true }));
  vtkm::cont::Algorithm::Fill(bits, false, 5);
  VTKM_TEST_ASSERT(checkBitField(bits, { false, false, false, false, false }));
  vtkm::cont::Algorithm::Fill(bits, vtkm::UInt8(0xab));
  bits.Allocate(8);
  VTKM_TEST_ASSERT(checkBitField(bits, { true, true, false, true, false, true, false, true }));
  vtkm::cont::Algorithm::Fill(bits, vtkm::UInt8(0xab), 5);
  VTKM_TEST_ASSERT(checkBitField(bits, { true, true, false, true, false }));
  vtkm::cont::Algorithm::Fill(array, vtkm::Id(5));
  VTKM_TEST_ASSERT(checkArrayHandle(array, { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 }));
  vtkm::cont::Algorithm::Fill(array, vtkm::Id(6), 5);
  VTKM_TEST_ASSERT(checkArrayHandle(array, { 6, 6, 6, 6, 6 }));
}

void CopyTest()
{
  vtkm::cont::ArrayHandleIndex input(ARRAY_SIZE);
  vtkm::cont::ArrayHandle<vtkm::Id> output;
  vtkm::cont::ArrayHandle<vtkm::Id> stencil =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 2, 3, 0, 0, 1, 8, 9, 2 });

  vtkm::cont::Algorithm::Copy(input, output);
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(input, output));
  vtkm::cont::Algorithm::CopyIf(input, stencil, output);
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 1, 2, 3, 6, 7, 8, 9 }));
  vtkm::cont::Algorithm::CopyIf(input, stencil, output, vtkm::LogicalNot());
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 0, 4, 5 }));
  vtkm::cont::Algorithm::CopySubRange(input, 2, 1, output);
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 2, 4, 5 }));
}

struct CustomCompare
{
  template <typename T>
  VTKM_EXEC bool operator()(T a, T b) const
  {
    return (2 * a) < b;
  }
};

void BoundsTest()
{

  vtkm::cont::ArrayHandle<vtkm::Id> input =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 1, 2, 3, 5, 8, 13, 21, 34 });
  vtkm::cont::ArrayHandle<vtkm::Id> values =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 4, 9, 16, 25, 36, 49 });
  vtkm::cont::ArrayHandle<vtkm::Id> output;

  vtkm::cont::Algorithm::LowerBounds(input, values, output);
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 0, 1, 5, 7, 8, 9, 10, 10 }));
  vtkm::cont::Algorithm::LowerBounds(input, values, output, CustomCompare{});
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 0, 1, 3, 5, 6, 7, 8, 9 }));
  vtkm::cont::ArrayCopy(values, output);
  vtkm::cont::Algorithm::LowerBounds(input, output);
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 0, 1, 5, 7, 8, 9, 10, 10 }));

  vtkm::cont::Algorithm::UpperBounds(input, values, output);
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 1, 3, 5, 7, 8, 9, 10, 10 }));
  vtkm::cont::Algorithm::UpperBounds(input, values, output, CustomCompare{});
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 1, 4, 7, 8, 9, 10, 10, 10 }));
  vtkm::cont::ArrayCopy(values, output);
  vtkm::cont::Algorithm::UpperBounds(input, output);
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 1, 3, 5, 7, 8, 9, 10, 10 }));
}

void ReduceTest()
{

  vtkm::cont::ArrayHandle<vtkm::Id> input =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  vtkm::cont::ArrayHandle<vtkm::Id> keys =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 0, 0, 1, 2, 2, 5, 5, 5, 5 });
  vtkm::cont::ArrayHandle<vtkm::Id> keysOut;
  vtkm::cont::ArrayHandle<vtkm::Id> valsOut;

  vtkm::Id result;
  result = vtkm::cont::Algorithm::Reduce(input, vtkm::Id(0));
  VTKM_TEST_ASSERT(test_equal(result, 51));
  result = vtkm::cont::Algorithm::Reduce(input, vtkm::Id(0), vtkm::Maximum());
  VTKM_TEST_ASSERT(test_equal(result, 9));
  vtkm::cont::Algorithm::ReduceByKey(keys, input, keysOut, valsOut, vtkm::Maximum());
  VTKM_TEST_ASSERT(checkArrayHandle(keysOut, { 0, 1, 2, 5 }));
  VTKM_TEST_ASSERT(checkArrayHandle(valsOut, { 6, 1, 9, 8 }));
}

void ScanTest()
{

  vtkm::cont::ArrayHandle<vtkm::Id> input =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  vtkm::cont::ArrayHandle<vtkm::Id> keys =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 0, 0, 1, 2, 2, 5, 5, 5, 5 });
  vtkm::cont::ArrayHandle<vtkm::Id> output;

  vtkm::Id out;
  out = vtkm::cont::Algorithm::ScanInclusive(input, output);
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 6, 8, 13, 14, 23, 29, 30, 35, 43, 51 }));
  VTKM_TEST_ASSERT(test_equal(out, 51));
  out = vtkm::cont::Algorithm::ScanInclusive(input, output, vtkm::Maximum());
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 6, 6, 6, 6, 9, 9, 9, 9, 9, 9 }));
  VTKM_TEST_ASSERT(test_equal(out, 9));
  vtkm::cont::Algorithm::ScanInclusiveByKey(keys, input, output, vtkm::Maximum());
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 6, 6, 6, 1, 9, 9, 1, 5, 8, 8 }));
  vtkm::cont::Algorithm::ScanInclusiveByKey(keys, input, output);
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 6, 8, 13, 1, 9, 15, 1, 6, 14, 22 }));
  out = vtkm::cont::Algorithm::ScanExclusive(input, output, vtkm::Maximum(), vtkm::Id(0));
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 0, 6, 6, 6, 6, 9, 9, 9, 9, 9 }));
  VTKM_TEST_ASSERT(test_equal(out, 9));
  vtkm::cont::Algorithm::ScanExclusiveByKey(keys, input, output, vtkm::Id(0), vtkm::Maximum());
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 0, 6, 6, 0, 0, 9, 0, 1, 5, 8 }));
  vtkm::cont::Algorithm::ScanExclusiveByKey(keys, input, output);
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 0, 6, 8, 0, 0, 9, 0, 1, 6, 14 }));
  vtkm::cont::Algorithm::ScanExtended(input, output);
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 0, 6, 8, 13, 14, 23, 29, 30, 35, 43, 51 }));
  vtkm::cont::Algorithm::ScanExtended(input, output, vtkm::Maximum(), vtkm::Id(0));
  VTKM_TEST_ASSERT(checkArrayHandle(output, { 0, 6, 6, 6, 6, 9, 9, 9, 9, 9, 9 }));
}

struct DummyFunctor : public vtkm::exec::FunctorBase
{
  template <typename IdType>
  VTKM_EXEC void operator()(IdType) const
  {
  }
};

void ScheduleTest()
{
  vtkm::cont::Algorithm::Schedule(DummyFunctor(), vtkm::Id(1));
  vtkm::Id3 id3(1, 1, 1);
  vtkm::cont::Algorithm::Schedule(DummyFunctor(), id3);
}

struct CompFunctor
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x, const T& y) const
  {
    return x > y;
  }
};

struct CompExecObject : vtkm::cont::ExecutionObjectBase
{
  VTKM_CONT CompFunctor PrepareForExecution(vtkm::cont::DeviceAdapterId, vtkm::cont::Token&)
  {
    return CompFunctor();
  }
};

void SortTest()
{
  vtkm::cont::ArrayHandle<vtkm::Id> input;
  vtkm::cont::ArrayHandle<vtkm::Id> keys =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 0, 0, 1, 2, 2, 5, 5, 5, 5 });

  input = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  vtkm::cont::Algorithm::Sort(input);
  VTKM_TEST_ASSERT(checkArrayHandle(input, { 1, 1, 2, 5, 5, 6, 6, 8, 8, 9 }));

  input = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  vtkm::cont::Algorithm::Sort(input, CompFunctor());
  VTKM_TEST_ASSERT(checkArrayHandle(input, { 9, 8, 8, 6, 6, 5, 5, 2, 1, 1 }));

  input = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  vtkm::cont::Algorithm::Sort(input, CompExecObject());
  VTKM_TEST_ASSERT(checkArrayHandle(input, { 9, 8, 8, 6, 6, 5, 5, 2, 1, 1 }));

  keys = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  input = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 2, 3, 4, 0, 3, 2, 5, 5 });
  vtkm::cont::Algorithm::SortByKey(keys, input);
  VTKM_TEST_ASSERT(checkArrayHandle(keys, { 1, 1, 2, 5, 5, 6, 6, 8, 8, 9 }));
  VTKM_TEST_ASSERT(checkArrayHandle(input, { 3, 3, 1, 2, 2, 0, 0, 5, 5, 4 }));

  keys = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 2, 5, 1, 9, 6, 1, 5, 8, 8 });
  input = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 2, 3, 4, 0, 3, 2, 5, 5 });
  vtkm::cont::Algorithm::SortByKey(keys, input, CompFunctor());
  VTKM_TEST_ASSERT(checkArrayHandle(keys, { 9, 8, 8, 6, 6, 5, 5, 2, 1, 1 }));
  VTKM_TEST_ASSERT(checkArrayHandle(input, { 4, 5, 5, 0, 0, 2, 2, 1, 3, 3 }));
  vtkm::cont::Algorithm::SortByKey(keys, input, CompExecObject());
}

void SynchronizeTest()
{
  vtkm::cont::Algorithm::Synchronize();
}

void TransformTest()
{
  auto transformInput = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 1, 3, 5, 7, 9, 11, 13, 15 });
  auto transformInputOutput =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 2, 4, 8, 10, 12, 14, 16 });
  auto transformExpectedResult =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 1, 5, 9, 15, 19, 23, 27, 31 });

  // Test simple call on two different arrays
  std::cout << "Testing Transform for summing arrays" << std::endl;
  vtkm::cont::ArrayHandle<vtkm::Id> transformOutput;
  vtkm::cont::Algorithm::Transform(
    transformInput, transformInputOutput, transformOutput, vtkm::Sum{});
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(transformOutput, transformExpectedResult));

  // Test using an array as both input and output
  std::cout << "Testing Transform with array for both input and output" << std::endl;
  vtkm::cont::Algorithm::Transform(
    transformInputOutput, transformInput, transformInputOutput, vtkm::Sum{});
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(transformInputOutput, transformExpectedResult));
}

struct Within3Functor
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x, const T& y) const
  {
    return (x / 3) == (y / 3);
  }
};

struct Within3ExecObject : vtkm::cont::ExecutionObjectBase
{
  VTKM_CONT Within3Functor PrepareForExecution(vtkm::cont::DeviceAdapterId, vtkm::cont::Token&)
  {
    return Within3Functor();
  }
};

void UniqueTest()
{
  vtkm::cont::ArrayHandle<vtkm::Id> input;

  input = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 1, 1, 2, 5, 5, 6, 6, 8, 8, 9 });
  vtkm::cont::Algorithm::Unique(input);
  VTKM_TEST_ASSERT(checkArrayHandle(input, { 1, 2, 5, 6, 8, 9 }));

  input = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 1, 1, 2, 5, 5, 6, 6, 8, 8, 9 });
  vtkm::cont::Algorithm::Unique(input, Within3Functor());
  vtkm::cont::printSummary_ArrayHandle(input, std::cout, true);
  // The result should be an array of size 4 with the first entry 1 or 2, the second 5,
  // the third 6 or 8, and the fourth 9.
  VTKM_TEST_ASSERT(input.GetNumberOfValues() == 4);
  VTKM_TEST_ASSERT(input.ReadPortal().Get(1) == 5);

  input = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 1, 1, 2, 5, 5, 6, 6, 8, 8, 9 });
  vtkm::cont::Algorithm::Unique(input, Within3ExecObject());
  // The result should be an array of size 4 with the first entry 1 or 2, the second 5,
  // the third 6 or 8, and the fourth 9.
  VTKM_TEST_ASSERT(input.GetNumberOfValues() == 4);
  VTKM_TEST_ASSERT(input.ReadPortal().Get(1) == 5);
}

void TestAll()
{
  FillTest();
  CopyTest();
  BoundsTest();
  ReduceTest();
  ScanTest();
  ScheduleTest();
  SortTest();
  SynchronizeTest();
  TransformTest();
  UniqueTest();
}

} // anonymous namespace

int UnitTestAlgorithm(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestAll, argc, argv);
}
