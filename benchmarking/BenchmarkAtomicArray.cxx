//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include "Benchmarker.h"

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/AtomicArray.h>
#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/Timer.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/TypeTraits.h>

#include <sstream>
#include <string>

namespace
{

// Provide access to the requested device to the benchmark functions:
viskores::cont::InitializeResult Config;

// Range for array sizes
static constexpr viskores::Id ARRAY_SIZE_MIN = 1;
static constexpr viskores::Id ARRAY_SIZE_MAX = 1 << 20;

// This is 32x larger than the largest array size.
static constexpr viskores::Id NUM_WRITES = 33554432; // 2^25

static constexpr viskores::Id STRIDE = 32;

// Benchmarks AtomicArray::Add such that each work index writes to adjacent indices.
struct AddSeqWorker : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename AtomicPortal>
  VISKORES_EXEC void operator()(const viskores::Id i, const T& val, AtomicPortal& portal) const
  {
    portal.Add(i % portal.GetNumberOfValues(), val);
  }
};

template <typename ValueType>
void BenchAddSeq(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numValues = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numWrites = static_cast<viskores::Id>(state.range(1));

  auto ones = viskores::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  viskores::cont::ArrayHandle<ValueType> atomicArray;
  atomicArray.AllocateAndFill(numValues, viskores::TypeTraits<ValueType>::ZeroInitialization());

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(AddSeqWorker{}, ones, atomicArray);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchAddSeq,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "AtomicsValues", "AtomicOps" }),
                              viskores::cont::AtomicArrayTypeList);

// Provides a non-atomic baseline for BenchAddSeq
struct AddSeqBaselineWorker : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename Portal>
  VISKORES_EXEC void operator()(const viskores::Id i, const T& val, Portal& portal) const
  {
    const viskores::Id j = i % portal.GetNumberOfValues();
    portal.Set(j, portal.Get(j) + val);
  }
};

template <typename ValueType>
void BenchAddSeqBaseline(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numValues = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numWrites = static_cast<viskores::Id>(state.range(1));

  auto ones = viskores::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  viskores::cont::ArrayHandle<ValueType> array;
  array.AllocateAndFill(numValues, viskores::TypeTraits<ValueType>::ZeroInitialization());

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(AddSeqBaselineWorker{}, ones, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchAddSeqBaseline,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "Values", "Ops" }),
                              viskores::cont::AtomicArrayTypeList);

// Benchmarks AtomicArray::Add such that each work index writes to a strided
// index ( floor(i / stride) + stride * (i % stride)
struct AddStrideWorker : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  viskores::Id Stride;

  AddStrideWorker(viskores::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename AtomicPortal>
  VISKORES_EXEC void operator()(const viskores::Id i, const T& val, AtomicPortal& portal) const
  {
    const viskores::Id numVals = portal.GetNumberOfValues();
    const viskores::Id j = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    portal.Add(j, val);
  }
};

template <typename ValueType>
void BenchAddStride(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numValues = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numWrites = static_cast<viskores::Id>(state.range(1));
  const viskores::Id stride = static_cast<viskores::Id>(state.range(2));

  auto ones = viskores::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  viskores::cont::ArrayHandle<ValueType> atomicArray;
  atomicArray.AllocateAndFill(numValues, viskores::TypeTraits<ValueType>::ZeroInitialization());

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(AddStrideWorker{ stride }, ones, atomicArray);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
VISKORES_BENCHMARK_TEMPLATES_OPTS(
  BenchAddStride,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "AtomicsValues", "AtomicOps", "Stride" }),
  viskores::cont::AtomicArrayTypeList);

// Non-atomic baseline for AddStride
struct AddStrideBaselineWorker : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  viskores::Id Stride;

  AddStrideBaselineWorker(viskores::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename Portal>
  VISKORES_EXEC void operator()(const viskores::Id i, const T& val, Portal& portal) const
  {
    const viskores::Id numVals = portal.GetNumberOfValues();
    const viskores::Id j = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    portal.Set(j, portal.Get(j) + val);
  }
};

template <typename ValueType>
void BenchAddStrideBaseline(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numValues = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numWrites = static_cast<viskores::Id>(state.range(1));
  const viskores::Id stride = static_cast<viskores::Id>(state.range(2));

  auto ones = viskores::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  viskores::cont::ArrayHandle<ValueType> array;
  array.AllocateAndFill(numValues, viskores::TypeTraits<ValueType>::ZeroInitialization());

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(AddStrideBaselineWorker{ stride }, ones, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
VISKORES_BENCHMARK_TEMPLATES_OPTS(
  BenchAddStrideBaseline,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "Values", "Ops", "Stride" }),
  viskores::cont::AtomicArrayTypeList);

// Benchmarks AtomicArray::CompareExchange such that each work index writes to adjacent
// indices.
struct CASSeqWorker : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename AtomicPortal>
  VISKORES_EXEC void operator()(const viskores::Id i, const T& in, AtomicPortal& portal) const
  {
    const viskores::Id idx = i % portal.GetNumberOfValues();
    const T val = static_cast<T>(i) + in;
    T oldVal = portal.Get(idx);
    while (!portal.CompareExchange(idx, &oldVal, oldVal + val))
      ;
  }
};

template <typename ValueType>
void BenchCASSeq(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numValues = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numWrites = static_cast<viskores::Id>(state.range(1));

  auto ones = viskores::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  viskores::cont::ArrayHandle<ValueType> atomicArray;
  atomicArray.AllocateAndFill(numValues, viskores::TypeTraits<ValueType>::ZeroInitialization());

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(CASSeqWorker{}, ones, atomicArray);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchCASSeq,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "AtomicsValues", "AtomicOps" }),
                              viskores::cont::AtomicArrayTypeList);

// Provides a non-atomic baseline for BenchCASSeq
struct CASSeqBaselineWorker : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename Portal>
  VISKORES_EXEC void operator()(const viskores::Id i, const T& in, Portal& portal) const
  {
    const viskores::Id idx = i % portal.GetNumberOfValues();
    const T val = static_cast<T>(i) + in;
    const T oldVal = portal.Get(idx);
    portal.Set(idx, oldVal + val);
  }
};

template <typename ValueType>
void BenchCASSeqBaseline(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numValues = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numWrites = static_cast<viskores::Id>(state.range(1));

  auto ones = viskores::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  viskores::cont::ArrayHandle<ValueType> array;
  array.AllocateAndFill(numValues, viskores::TypeTraits<ValueType>::ZeroInitialization());

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(CASSeqBaselineWorker{}, ones, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchCASSeqBaseline,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "Values", "Ops" }),
                              viskores::cont::AtomicArrayTypeList);

// Benchmarks AtomicArray::CompareExchange such that each work index writes to
// a strided index:
// ( floor(i / stride) + stride * (i % stride)
struct CASStrideWorker : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  viskores::Id Stride;

  CASStrideWorker(viskores::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename AtomicPortal>
  VISKORES_EXEC void operator()(const viskores::Id i, const T& in, AtomicPortal& portal) const
  {
    const viskores::Id numVals = portal.GetNumberOfValues();
    const viskores::Id idx = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    const T val = static_cast<T>(i) + in;
    T oldVal = portal.Get(idx);
    while (!portal.CompareExchange(idx, &oldVal, oldVal + val))
      ;
  }
};

template <typename ValueType>
void BenchCASStride(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numValues = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numWrites = static_cast<viskores::Id>(state.range(1));
  const viskores::Id stride = static_cast<viskores::Id>(state.range(2));

  auto ones = viskores::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  viskores::cont::ArrayHandle<ValueType> atomicArray;
  atomicArray.AllocateAndFill(numValues, viskores::TypeTraits<ValueType>::ZeroInitialization());

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(CASStrideWorker{ stride }, ones, atomicArray);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
VISKORES_BENCHMARK_TEMPLATES_OPTS(
  BenchCASStride,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "AtomicsValues", "AtomicOps", "Stride" }),
  viskores::cont::AtomicArrayTypeList);

// Non-atomic baseline for CASStride
struct CASStrideBaselineWorker : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  viskores::Id Stride;

  CASStrideBaselineWorker(viskores::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename AtomicPortal>
  VISKORES_EXEC void operator()(const viskores::Id i, const T& in, AtomicPortal& portal) const
  {
    const viskores::Id numVals = portal.GetNumberOfValues();
    const viskores::Id idx = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    const T val = static_cast<T>(i) + in;
    T oldVal = portal.Get(idx);
    portal.Set(idx, oldVal + val);
  }
};

template <typename ValueType>
void BenchCASStrideBaseline(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numValues = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numWrites = static_cast<viskores::Id>(state.range(1));
  const viskores::Id stride = static_cast<viskores::Id>(state.range(2));

  auto ones = viskores::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  viskores::cont::ArrayHandle<ValueType> array;
  array.AllocateAndFill(numValues, viskores::TypeTraits<ValueType>::ZeroInitialization());

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(CASStrideBaselineWorker{ stride }, ones, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  const int64_t valsWritten = static_cast<int64_t>(numWrites);
  const int64_t bytesWritten = static_cast<int64_t>(sizeof(ValueType)) * valsWritten;
  state.SetItemsProcessed(valsWritten * iterations);
  state.SetItemsProcessed(bytesWritten * iterations);
}
VISKORES_BENCHMARK_TEMPLATES_OPTS(
  BenchCASStrideBaseline,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "AtomicsValues", "AtomicOps", "Stride" }),
  viskores::cont::AtomicArrayTypeList);

} // end anon namespace

int main(int argc, char* argv[])
{
  // Parse Viskores options:
  auto opts = viskores::cont::InitializeOptions::RequireDevice;

  std::vector<char*> args(argv, argv + argc);
  viskores::bench::detail::InitializeArgs(&argc, args, opts);

  // Parse Viskores options:
  Config = viskores::cont::Initialize(argc, args.data(), opts);

  // This occurs when it is help
  if (opts == viskores::cont::InitializeOptions::None)
  {
    std::cout << Config.Usage << std::endl;
  }
  else
  {
    viskores::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);
  }

  // handle benchmarking related args and run benchmarks:
  VISKORES_EXECUTE_BENCHMARKS(argc, args.data());
}
