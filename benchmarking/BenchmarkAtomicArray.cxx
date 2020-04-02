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

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/AtomicArray.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/TypeTraits.h>

#include <sstream>
#include <string>

namespace
{

// Provide access to the requested device to the benchmark functions:
vtkm::cont::InitializeResult Config;

// Range for array sizes
static constexpr vtkm::Id ARRAY_SIZE_MIN = 1;
static constexpr vtkm::Id ARRAY_SIZE_MAX = 1 << 20;

// This is 32x larger than the largest array size.
static constexpr vtkm::Id NUM_WRITES = 33554432; // 2^25

static constexpr vtkm::Id STRIDE = 32;

// Benchmarks AtomicArray::Add such that each work index writes to adjacent indices.
struct AddSeqWorker : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename AtomicPortal>
  VTKM_EXEC void operator()(const vtkm::Id i, const T& val, AtomicPortal& portal) const
  {
    portal.Add(i % portal.GetNumberOfValues(), val);
  }
};

template <typename ValueType>
void BenchAddSeq(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numValues = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numWrites = static_cast<vtkm::Id>(state.range(1));

  auto ones = vtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  vtkm::cont::ArrayHandle<ValueType> atomicArray;
  vtkm::cont::Algorithm::Fill(
    atomicArray, vtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
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
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchAddSeq,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "AtomicsValues", "AtomicOps" }),
                              vtkm::cont::AtomicArrayTypeList);

// Provides a non-atomic baseline for BenchAddSeq
struct AddSeqBaselineWorker : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename Portal>
  VTKM_EXEC void operator()(const vtkm::Id i, const T& val, Portal& portal) const
  {
    const vtkm::Id j = i % portal.GetNumberOfValues();
    portal.Set(j, portal.Get(j) + val);
  }
};

template <typename ValueType>
void BenchAddSeqBaseline(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numValues = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numWrites = static_cast<vtkm::Id>(state.range(1));

  auto ones = vtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  vtkm::cont::ArrayHandle<ValueType> array;
  vtkm::cont::Algorithm::Fill(array, vtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
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
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchAddSeqBaseline,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "Values", "Ops" }),
                              vtkm::cont::AtomicArrayTypeList);

// Benchmarks AtomicArray::Add such that each work index writes to a strided
// index ( floor(i / stride) + stride * (i % stride)
struct AddStrideWorker : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  vtkm::Id Stride;

  AddStrideWorker(vtkm::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename AtomicPortal>
  VTKM_EXEC void operator()(const vtkm::Id i, const T& val, AtomicPortal& portal) const
  {
    const vtkm::Id numVals = portal.GetNumberOfValues();
    const vtkm::Id j = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    portal.Add(j, val);
  }
};

template <typename ValueType>
void BenchAddStride(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numValues = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numWrites = static_cast<vtkm::Id>(state.range(1));
  const vtkm::Id stride = static_cast<vtkm::Id>(state.range(2));

  auto ones = vtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  vtkm::cont::ArrayHandle<ValueType> atomicArray;
  vtkm::cont::Algorithm::Fill(
    atomicArray, vtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
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
VTKM_BENCHMARK_TEMPLATES_OPTS(
  BenchAddStride,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "AtomicsValues", "AtomicOps", "Stride" }),
  vtkm::cont::AtomicArrayTypeList);

// Non-atomic baseline for AddStride
struct AddStrideBaselineWorker : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  vtkm::Id Stride;

  AddStrideBaselineWorker(vtkm::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename Portal>
  VTKM_EXEC void operator()(const vtkm::Id i, const T& val, Portal& portal) const
  {
    const vtkm::Id numVals = portal.GetNumberOfValues();
    const vtkm::Id j = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    portal.Set(j, portal.Get(j) + val);
  }
};

template <typename ValueType>
void BenchAddStrideBaseline(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numValues = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numWrites = static_cast<vtkm::Id>(state.range(1));
  const vtkm::Id stride = static_cast<vtkm::Id>(state.range(2));

  auto ones = vtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  vtkm::cont::ArrayHandle<ValueType> array;
  vtkm::cont::Algorithm::Fill(array, vtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
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
VTKM_BENCHMARK_TEMPLATES_OPTS(
  BenchAddStrideBaseline,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "Values", "Ops", "Stride" }),
  vtkm::cont::AtomicArrayTypeList);

// Benchmarks AtomicArray::CompareAndSwap such that each work index writes to adjacent
// indices.
struct CASSeqWorker : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename AtomicPortal>
  VTKM_EXEC void operator()(const vtkm::Id i, const T& in, AtomicPortal& portal) const
  {
    const vtkm::Id idx = i % portal.GetNumberOfValues();
    const T val = static_cast<T>(i) + in;
    T oldVal = portal.Get(idx);
    T assumed = static_cast<T>(0);
    do
    {
      assumed = oldVal;
      oldVal = portal.CompareAndSwap(idx, assumed + val, assumed);
    } while (assumed != oldVal);
  }
};

template <typename ValueType>
void BenchCASSeq(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numValues = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numWrites = static_cast<vtkm::Id>(state.range(1));

  auto ones = vtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  vtkm::cont::ArrayHandle<ValueType> atomicArray;
  vtkm::cont::Algorithm::Fill(
    atomicArray, vtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
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
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchCASSeq,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "AtomicsValues", "AtomicOps" }),
                              vtkm::cont::AtomicArrayTypeList);

// Provides a non-atomic baseline for BenchCASSeq
struct CASSeqBaselineWorker : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename T, typename Portal>
  VTKM_EXEC void operator()(const vtkm::Id i, const T& in, Portal& portal) const
  {
    const vtkm::Id idx = i % portal.GetNumberOfValues();
    const T val = static_cast<T>(i) + in;
    const T oldVal = portal.Get(idx);
    portal.Set(idx, oldVal + val);
  }
};

template <typename ValueType>
void BenchCASSeqBaseline(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numValues = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numWrites = static_cast<vtkm::Id>(state.range(1));

  auto ones = vtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  vtkm::cont::ArrayHandle<ValueType> array;
  vtkm::cont::Algorithm::Fill(array, vtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
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
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchCASSeqBaseline,
                                ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX },
                                           { NUM_WRITES, NUM_WRITES } })
                                ->ArgNames({ "Values", "Ops" }),
                              vtkm::cont::AtomicArrayTypeList);

// Benchmarks AtomicArray::CompareAndSwap such that each work index writes to
// a strided index:
// ( floor(i / stride) + stride * (i % stride)
struct CASStrideWorker : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  vtkm::Id Stride;

  CASStrideWorker(vtkm::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename AtomicPortal>
  VTKM_EXEC void operator()(const vtkm::Id i, const T& in, AtomicPortal& portal) const
  {
    const vtkm::Id numVals = portal.GetNumberOfValues();
    const vtkm::Id idx = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    const T val = static_cast<T>(i) + in;
    T oldVal = portal.Get(idx);
    T assumed = static_cast<T>(0);
    do
    {
      assumed = oldVal;
      oldVal = portal.CompareAndSwap(idx, assumed + val, assumed);
    } while (assumed != oldVal);
  }
};

template <typename ValueType>
void BenchCASStride(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numValues = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numWrites = static_cast<vtkm::Id>(state.range(1));
  const vtkm::Id stride = static_cast<vtkm::Id>(state.range(2));

  auto ones = vtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  vtkm::cont::ArrayHandle<ValueType> atomicArray;
  vtkm::cont::Algorithm::Fill(
    atomicArray, vtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
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
VTKM_BENCHMARK_TEMPLATES_OPTS(
  BenchCASStride,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "AtomicsValues", "AtomicOps", "Stride" }),
  vtkm::cont::AtomicArrayTypeList);

// Non-atomic baseline for CASStride
struct CASStrideBaselineWorker : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  vtkm::Id Stride;

  CASStrideBaselineWorker(vtkm::Id stride)
    : Stride{ stride }
  {
  }

  template <typename T, typename AtomicPortal>
  VTKM_EXEC void operator()(const vtkm::Id i, const T& in, AtomicPortal& portal) const
  {
    const vtkm::Id numVals = portal.GetNumberOfValues();
    const vtkm::Id idx = (i / this->Stride + this->Stride * (i % this->Stride)) % numVals;
    const T val = static_cast<T>(i) + in;
    T oldVal = portal.Get(idx);
    portal.Set(idx, oldVal + val);
  }
};

template <typename ValueType>
void BenchCASStrideBaseline(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numValues = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numWrites = static_cast<vtkm::Id>(state.range(1));
  const vtkm::Id stride = static_cast<vtkm::Id>(state.range(2));

  auto ones = vtkm::cont::make_ArrayHandleConstant<ValueType>(static_cast<ValueType>(1), numWrites);

  vtkm::cont::ArrayHandle<ValueType> array;
  vtkm::cont::Algorithm::Fill(array, vtkm::TypeTraits<ValueType>::ZeroInitialization(), numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
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
VTKM_BENCHMARK_TEMPLATES_OPTS(
  BenchCASStrideBaseline,
    ->Ranges({ { ARRAY_SIZE_MIN, ARRAY_SIZE_MAX }, { NUM_WRITES, NUM_WRITES }, { STRIDE, STRIDE } })
    ->ArgNames({ "AtomicsValues", "AtomicOps", "Stride" }),
  vtkm::cont::AtomicArrayTypeList);

} // end anon namespace

int main(int argc, char* argv[])
{
  // Parse VTK-m options:
  auto opts = vtkm::cont::InitializeOptions::RequireDevice | vtkm::cont::InitializeOptions::AddHelp;
  Config = vtkm::cont::Initialize(argc, argv, opts);

  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);

  // handle benchmarking related args and run benchmarks:
  VTKM_EXECUTE_BENCHMARKS(argc, argv);
}
