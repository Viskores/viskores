//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Algorithm.h>

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

void FillTest()
{
  vtkm::cont::BitField bits;
  vtkm::cont::ArrayHandle<vtkm::Id> array;

  bits.Allocate(ARRAY_SIZE);
  array.Allocate(ARRAY_SIZE);

  vtkm::cont::Algorithm::Fill(bits, true);
  vtkm::cont::Algorithm::Fill(bits, true, 5);
  vtkm::cont::Algorithm::Fill(bits, vtkm::UInt8(0xab));
  vtkm::cont::Algorithm::Fill(bits, vtkm::UInt8(0xab), 5);
  vtkm::cont::Algorithm::Fill(array, vtkm::Id(5));
  vtkm::cont::Algorithm::Fill(array, vtkm::Id(5), 5);
}

void CopyTest()
{
  vtkm::cont::ArrayHandle<vtkm::Id> input;
  vtkm::cont::ArrayHandle<vtkm::Id> output;
  vtkm::cont::ArrayHandle<vtkm::Id> stencil;

  input.Allocate(ARRAY_SIZE);
  output.Allocate(ARRAY_SIZE);
  stencil.Allocate(ARRAY_SIZE);

  vtkm::cont::Algorithm::Copy(input, output);
  vtkm::cont::Algorithm::CopyIf(input, stencil, output);
  vtkm::cont::Algorithm::CopyIf(input, stencil, output, vtkm::LogicalNot());
  vtkm::cont::Algorithm::CopySubRange(input, 2, 1, output);
}

void BoundsTest()
{

  vtkm::cont::ArrayHandle<vtkm::Id> input;
  vtkm::cont::ArrayHandle<vtkm::Id> output;
  vtkm::cont::ArrayHandle<vtkm::Id> values;

  input.Allocate(ARRAY_SIZE);
  output.Allocate(ARRAY_SIZE);
  values.Allocate(ARRAY_SIZE);

  vtkm::cont::Algorithm::LowerBounds(input, values, output);
  vtkm::cont::Algorithm::LowerBounds(input, values, output, vtkm::Sum());
  vtkm::cont::Algorithm::LowerBounds(input, values);

  vtkm::cont::Algorithm::UpperBounds(input, values, output);
  vtkm::cont::Algorithm::UpperBounds(input, values, output, vtkm::Sum());
  vtkm::cont::Algorithm::UpperBounds(input, values);
}

void ReduceTest()
{

  vtkm::cont::ArrayHandle<vtkm::Id> input;
  vtkm::cont::ArrayHandle<vtkm::Id> keys;
  vtkm::cont::ArrayHandle<vtkm::Id> keysOut;
  vtkm::cont::ArrayHandle<vtkm::Id> valsOut;

  input.Allocate(ARRAY_SIZE);
  keys.Allocate(ARRAY_SIZE);
  keysOut.Allocate(ARRAY_SIZE);
  valsOut.Allocate(ARRAY_SIZE);

  vtkm::Id result;
  result = vtkm::cont::Algorithm::Reduce(input, vtkm::Id(0));
  result = vtkm::cont::Algorithm::Reduce(input, vtkm::Id(0), vtkm::Maximum());
  vtkm::cont::Algorithm::ReduceByKey(keys, input, keysOut, valsOut, vtkm::Maximum());
  (void)result;
}

void ScanTest()
{

  vtkm::cont::ArrayHandle<vtkm::Id> input;
  vtkm::cont::ArrayHandle<vtkm::Id> output;
  vtkm::cont::ArrayHandle<vtkm::Id> keys;

  input.Allocate(ARRAY_SIZE);
  output.Allocate(ARRAY_SIZE);
  keys.Allocate(ARRAY_SIZE);

  vtkm::Id out;
  out = vtkm::cont::Algorithm::ScanInclusive(input, output);
  out = vtkm::cont::Algorithm::ScanInclusive(input, output, vtkm::Maximum());
  out = vtkm::cont::Algorithm::StreamingScanExclusive(1, input, output);
  vtkm::cont::Algorithm::ScanInclusiveByKey(keys, input, output, vtkm::Maximum());
  vtkm::cont::Algorithm::ScanInclusiveByKey(keys, input, output);
  out = vtkm::cont::Algorithm::ScanExclusive(input, output, vtkm::Maximum(), vtkm::Id(0));
  vtkm::cont::Algorithm::ScanExclusiveByKey(keys, input, output, vtkm::Id(0), vtkm::Maximum());
  vtkm::cont::Algorithm::ScanExclusiveByKey(keys, input, output);
  vtkm::cont::Algorithm::ScanExtended(input, output);
  vtkm::cont::Algorithm::ScanExtended(input, output, vtkm::Maximum(), vtkm::Id(0));
  (void)out;
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
    return x < y;
  }
};

struct CompExecObject : vtkm::cont::ExecutionObjectBase
{
  template <typename Device>
  VTKM_CONT CompFunctor PrepareForExecution(Device)
  {
    return CompFunctor();
  }
};

void SortTest()
{
  vtkm::cont::ArrayHandle<vtkm::Id> input;
  vtkm::cont::ArrayHandle<vtkm::Id> keys;

  input.Allocate(ARRAY_SIZE);
  keys.Allocate(ARRAY_SIZE);

  vtkm::cont::Algorithm::Sort(input);
  vtkm::cont::Algorithm::Sort(input, CompFunctor());
  vtkm::cont::Algorithm::Sort(input, CompExecObject());
  vtkm::cont::Algorithm::SortByKey(keys, input);
  vtkm::cont::Algorithm::SortByKey(keys, input, CompFunctor());
  vtkm::cont::Algorithm::SortByKey(keys, input, CompExecObject());
}

void SynchronizeTest()
{
  vtkm::cont::Algorithm::Synchronize();
}

void UniqueTest()
{
  vtkm::cont::ArrayHandle<vtkm::Id> input;

  input.Allocate(ARRAY_SIZE);

  vtkm::cont::Algorithm::Unique(input);
  vtkm::cont::Algorithm::Unique(input, CompFunctor());
  vtkm::cont::Algorithm::Unique(input, CompExecObject());
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
  UniqueTest();
}

} // anonymous namespace

int UnitTestAlgorithm(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestAll, argc, argv);
}
