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

#include <vtkm/TypeTraits.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/BitField.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/StableSortIndices.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <utility>

#include <vtkm/internal/Windows.h>

#ifdef VTKM_ENABLE_TBB
#include <tbb/task_scheduler_init.h>
#endif
#ifdef VTKM_ENABLE_OPENMP
#include <omp.h>
#endif

namespace
{

// Default sampling rate is x8 and always includes min/max,
// so this will generate 7 samples at:
// 1: 4 KiB
// 2: 32 KiB
// 3: 256 KiB
// 4: 2 MiB
// 5: 16 MiB
// 6: 128 MiB
static const std::pair<int64_t, int64_t> FullRange{ 1 << 12, 1 << 27 }; // 4KiB, 128MiB

// Smaller range that can be used to reduce the number of benchmarks. Used
// with `RangeMultiplier(SmallRangeMultiplier)`, this produces:
// 1: 32 KiB
// 2: 2 MiB
// 3: 128 MiB
static const std::pair<int64_t, int64_t> SmallRange{ 1 << 15, 1 << 27 }; // 4KiB, 128MiB
static constexpr int SmallRangeMultiplier = 1 << 21;                     // Ensure a sample at 2MiB

using TypeList = vtkm::List<vtkm::UInt8,
                            vtkm::Float32,
                            vtkm::Int64,
                            vtkm::Float64,
                            vtkm::Vec3f_32,
                            vtkm::Pair<vtkm::Int32, vtkm::Float64>>;

using SmallTypeList = vtkm::List<vtkm::UInt8, vtkm::Float32, vtkm::Int64>;

// Only 32-bit words are currently supported atomically across devices:
using AtomicWordTypes = vtkm::List<vtkm::UInt32>;

// The Fill algorithm uses different word types:
using FillWordTypes = vtkm::List<vtkm::UInt8, vtkm::UInt16, vtkm::UInt32, vtkm::UInt64>;

using IdArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id>;

// Hold configuration state (e.g. active device)
vtkm::cont::InitializeResult Config;

// Helper function to convert numBytes to numWords:
template <typename T>
vtkm::Id BytesToWords(vtkm::Id numBytes)
{
  const vtkm::Id wordSize = static_cast<vtkm::Id>(sizeof(T));
  return numBytes / wordSize;
}

// Various kernels used by the different benchmarks to accelerate
// initialization of data
template <typename T>
struct TestValueFunctor
{
  VTKM_EXEC_CONT
  T operator()(vtkm::Id i) const { return TestValue(i, T{}); }
};

template <typename ArrayT>
VTKM_CONT void FillTestValue(ArrayT& array, vtkm::Id numValues)
{
  using T = typename ArrayT::ValueType;
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleImplicit(TestValueFunctor<T>{}, numValues), array);
}

template <typename T>
struct ScaledTestValueFunctor
{
  vtkm::Id Scale;
  VTKM_EXEC_CONT
  T operator()(vtkm::Id i) const { return TestValue(i * this->Scale, T{}); }
};

template <typename ArrayT>
VTKM_CONT void FillScaledTestValue(ArrayT& array, vtkm::Id scale, vtkm::Id numValues)
{
  using T = typename ArrayT::ValueType;
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleImplicit(ScaledTestValueFunctor<T>{ scale }, numValues), array);
}

template <typename T>
struct ModuloTestValueFunctor
{
  vtkm::Id Mod;
  VTKM_EXEC_CONT
  T operator()(vtkm::Id i) const { return TestValue(i % this->Mod, T{}); }
};

template <typename ArrayT>
VTKM_CONT void FillModuloTestValue(ArrayT& array, vtkm::Id mod, vtkm::Id numValues)
{
  using T = typename ArrayT::ValueType;
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleImplicit(ModuloTestValueFunctor<T>{ mod }, numValues), array);
}

template <typename T>
struct BinaryTestValueFunctor
{
  vtkm::Id Mod;
  VTKM_EXEC_CONT
  T operator()(vtkm::Id i) const
  {
    T zero = vtkm::TypeTraits<T>::ZeroInitialization();

    // Always return zero unless 1 == Mod
    if (i == this->Mod)
    { // Ensure that the result is not equal to zero
      T retVal;
      do
      {
        retVal = TestValue(i++, T{});
      } while (retVal == zero);
      return retVal;
    }
    return std::move(zero);
  }
};

template <typename ArrayT>
VTKM_CONT void FillBinaryTestValue(ArrayT& array, vtkm::Id mod, vtkm::Id numValues)
{
  using T = typename ArrayT::ValueType;
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleImplicit(BinaryTestValueFunctor<T>{ mod }, numValues), array);
}

template <typename ArrayT>
VTKM_CONT void FillRandomTestValue(ArrayT& array, vtkm::Id numValues)
{
  using ValueType = typename ArrayT::ValueType;

  std::mt19937_64 rng;

  array.Allocate(numValues);
  auto portal = array.GetPortalControl();
  for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
  {
    portal.Set(i, TestValue(static_cast<vtkm::Id>(rng()), ValueType{}));
  }
}

template <typename ArrayT>
VTKM_CONT void FillRandomModTestValue(ArrayT& array, vtkm::Id mod, vtkm::Id numValues)
{
  using ValueType = typename ArrayT::ValueType;

  std::mt19937_64 rng;

  array.Allocate(numValues);
  auto portal = array.GetPortalControl();
  for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
  {
    portal.Set(i, TestValue(static_cast<vtkm::Id>(rng()) % mod, ValueType{}));
  }
}

static inline std::string SizeAndValuesString(vtkm::Id numBytes, vtkm::Id numValues)
{
  std::ostringstream str;
  str << vtkm::cont::GetHumanReadableSize(numBytes) << " | " << numValues << " values";
  return str.str();
}

template <typename WordType>
struct GenerateBitFieldWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn dummy, BitFieldOut);
  using ExecutionSignature = void(InputIndex, _2);

  WordType Exemplar;
  vtkm::Id Stride;
  vtkm::Id MaxMaskedWord;

  VTKM_CONT
  GenerateBitFieldWorklet(WordType exemplar, vtkm::Id stride, vtkm::Id maxMaskedWord)
    : Exemplar(exemplar)
    , Stride(stride)
    , MaxMaskedWord(maxMaskedWord)
  {
  }

  template <typename BitPortal>
  VTKM_EXEC void operator()(vtkm::Id wordIdx, BitPortal& portal) const
  {
    if (wordIdx <= this->MaxMaskedWord && (wordIdx % this->Stride) == 0)
    {
      portal.SetWordAtomic(wordIdx, this->Exemplar);
    }
    else
    {
      portal.SetWordAtomic(wordIdx, static_cast<WordType>(0));
    }
  }
};

// Create a bit field for testing. The bit array will contain numWords words.
// The exemplar word is used to set bits in the array. Stride indicates how
// many words will be set to 0 between words initialized to the exemplar.
// Words with indices higher than maxMaskedWord will be set to 0.
// Stride and maxMaskedWord may be used to test different types of imbalanced
// loads.
template <typename WordType>
VTKM_CONT vtkm::cont::BitField GenerateBitField(WordType exemplar,
                                                vtkm::Id stride,
                                                vtkm::Id maxMaskedWord,
                                                vtkm::Id numWords)
{
  if (stride == 0)
  {
    stride = 1;
  }

  vtkm::Id numBits = numWords * static_cast<vtkm::Id>(sizeof(WordType) * CHAR_BIT);

  vtkm::cont::BitField bits;
  bits.Allocate(numBits);

  // This array is just to set the input domain appropriately:
  auto dummy = vtkm::cont::make_ArrayHandleConstant<vtkm::Int32>(0, numWords);

  vtkm::cont::Invoker invoker{ Config.Device };
  invoker(GenerateBitFieldWorklet<WordType>{ exemplar, stride, maxMaskedWord }, dummy, bits);

  return bits;
};

//==============================================================================
// Benchmarks begin:

template <typename WordType>
void BenchBitFieldToUnorderedSetImpl(benchmark::State& state,
                                     vtkm::Id numBytes,
                                     WordType exemplar,
                                     vtkm::Id stride,
                                     vtkm::Float32 fillRatio,
                                     const std::string& name)
{
  const vtkm::Id numWords = BytesToWords<WordType>(numBytes);
  const vtkm::Id maxMaskedWord =
    static_cast<vtkm::Id>(static_cast<vtkm::Float32>(numWords) * fillRatio);

  { // Set label:
    const vtkm::Id numFilledWords = maxMaskedWord / stride;
    const vtkm::Id numSetBits = numFilledWords * vtkm::CountSetBits(exemplar);
    std::stringstream desc;
    desc << vtkm::cont::GetHumanReadableSize(numBytes) << " | " << name << " | "
         << "SetBits:" << numSetBits;
    state.SetLabel(desc.str());
  }

  vtkm::cont::BitField bits = GenerateBitField<WordType>(exemplar, stride, maxMaskedWord, numWords);

  IdArrayHandle indices;

  vtkm::cont::Timer timer{ Config.Device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::BitFieldToUnorderedSet(Config.Device, bits, indices);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
};

void BenchBitFieldToUnorderedSet(benchmark::State& state)
{
  using WordType = vtkm::WordTypeDefault;

  const vtkm::Id numBytes = static_cast<vtkm::Id>(state.range(0));
  const auto fillPattern = state.range(1);

  // Launch the implementation with the appropriate fill pattern:
  switch (fillPattern)
  {
    case 0:
      BenchBitFieldToUnorderedSetImpl<WordType>(state, numBytes, 0x00000000, 1, 0.f, "Null");
      break;

    case 1:
      BenchBitFieldToUnorderedSetImpl<WordType>(state, numBytes, 0xffffffff, 1, 1.f, "Full");
      break;

    case 2:
      BenchBitFieldToUnorderedSetImpl<WordType>(state, numBytes, 0xffff0000, 1, 0.f, "HalfWord");
      break;

    case 3:
      BenchBitFieldToUnorderedSetImpl<WordType>(state, numBytes, 0xffffffff, 1, 0.5f, "HalfField");
      break;

    case 4:
      BenchBitFieldToUnorderedSetImpl<WordType>(state, numBytes, 0xffffffff, 2, 1.f, "AltWords");
      break;

    case 5:
      BenchBitFieldToUnorderedSetImpl<WordType>(state, numBytes, 0x55555555, 1, 1.f, "AltBits");
      break;

    default:
      VTKM_UNREACHABLE("Internal error.");
  }
}

void BenchBitFieldToUnorderedSetGenerator(benchmark::internal::Benchmark* bm)
{
  // Use a reduced NUM_BYTES_MAX value here -- these benchmarks allocate one
  // 8-byte id per bit, so this caps the index array out at 512 MB:
  static constexpr int64_t numBytesMax = 1 << 26; // 64 MiB of bits

  bm->UseManualTime();
  bm->ArgNames({ "Size", "C" });

  for (int64_t config = 0; config < 6; ++config)
  {
    bm->Ranges({ { FullRange.first, numBytesMax }, { config, config } });
  }
}

VTKM_BENCHMARK_APPLY(BenchBitFieldToUnorderedSet, BenchBitFieldToUnorderedSetGenerator);

template <typename ValueType>
void BenchCopy(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  vtkm::cont::ArrayHandle<ValueType> src;
  vtkm::cont::ArrayHandle<ValueType> dst;

  FillTestValue(src, numValues);

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::Copy(device, src, dst);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchCopy, ->Ranges({ FullRange })->ArgName("Size"), TypeList);

template <typename ValueType>
void BenchCopyIf(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  const vtkm::Id percentValid = static_cast<vtkm::Id>(state.range(1));
  const vtkm::Id numValid = (numValues * percentValid) / 100;
  const vtkm::Id modulo = numValid != 0 ? numValues / numValid : numValues + 1;

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numBytes, numValues) << " | " << numValid << " valid ("
         << (numValid * 100 / numValues) << "%)";
    state.SetLabel(desc.str());
  }

  vtkm::cont::ArrayHandle<ValueType> src;
  vtkm::cont::ArrayHandle<vtkm::Id> stencil;
  vtkm::cont::ArrayHandle<ValueType> dst;

  FillTestValue(src, numValues);
  FillBinaryTestValue(stencil, modulo, numValues);

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::CopyIf(device, src, stencil, dst);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};

void BenchCopyIfGenerator(benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "Size", "%Valid" });
  bm->RangeMultiplier(SmallRangeMultiplier);

  for (int64_t pcntValid = 0; pcntValid <= 100; pcntValid += 25)
  {
    bm->Ranges({ SmallRange, { pcntValid, pcntValid } });
  }
}

VTKM_BENCHMARK_TEMPLATES_APPLY(BenchCopyIf, BenchCopyIfGenerator, SmallTypeList);

template <typename WordType>
void BenchCountSetBitsImpl(benchmark::State& state,
                           vtkm::Id numBytes,
                           WordType exemplar,
                           vtkm::Id stride,
                           vtkm::Float32 fillRatio,
                           const std::string& name)
{
  const vtkm::Id numWords = BytesToWords<WordType>(numBytes);
  const vtkm::Id maxMaskedWord =
    static_cast<vtkm::Id>(static_cast<vtkm::Float32>(numWords) * fillRatio);

  { // Set label:
    const vtkm::Id numFilledWords = maxMaskedWord / stride;
    const vtkm::Id numSetBits = numFilledWords * vtkm::CountSetBits(exemplar);
    std::stringstream desc;
    desc << vtkm::cont::GetHumanReadableSize(numBytes) << " | " << name << " | "
         << "SetBits:" << numSetBits;
    state.SetLabel(desc.str());
  }

  vtkm::cont::BitField bits = GenerateBitField<WordType>(exemplar, stride, maxMaskedWord, numWords);

  vtkm::cont::Timer timer{ Config.Device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    const vtkm::Id setBits = vtkm::cont::Algorithm::CountSetBits(Config.Device, bits);
    benchmark::DoNotOptimize(setBits);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
};

void BenchCountSetBits(benchmark::State& state)
{
  using WordType = vtkm::WordTypeDefault;

  const vtkm::Id numBytes = static_cast<vtkm::Id>(state.range(0));
  const auto fillPattern = state.range(1);

  // Launch the implementation with the appropriate fill pattern:
  switch (fillPattern)
  {
    case 0:
      BenchCountSetBitsImpl<WordType>(state, numBytes, 0x00000000, 1, 0.f, "Null");
      break;

    case 1:
      BenchCountSetBitsImpl<WordType>(state, numBytes, 0xffffffff, 1, 1.f, "Full");
      break;

    case 2:
      BenchCountSetBitsImpl<WordType>(state, numBytes, 0xffff0000, 1, 0.f, "HalfWord");
      break;

    case 3:
      BenchCountSetBitsImpl<WordType>(state, numBytes, 0xffffffff, 1, 0.5f, "HalfField");
      break;

    case 4:
      BenchCountSetBitsImpl<WordType>(state, numBytes, 0xffffffff, 2, 1.f, "AltWords");
      break;

    case 5:
      BenchCountSetBitsImpl<WordType>(state, numBytes, 0x55555555, 1, 1.f, "AltBits");
      break;

    default:
      VTKM_UNREACHABLE("Internal error.");
  }
}

void BenchCountSetBitsGenerator(benchmark::internal::Benchmark* bm)
{
  bm->UseManualTime();
  bm->ArgNames({ "Size", "C" });

  for (int64_t config = 0; config < 6; ++config)
  {
    bm->Ranges({ FullRange, { config, config } });
  }
}
VTKM_BENCHMARK_APPLY(BenchCountSetBits, BenchCountSetBitsGenerator);

template <typename ValueType>
void BenchFillArrayHandle(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  vtkm::cont::ArrayHandle<ValueType> array;

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::Fill(device, array, TestValue(19, ValueType{}), numValues);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchFillArrayHandle,
                                ->Range(FullRange.first, FullRange.second)
                                ->ArgName("Size"),
                              TypeList);

void BenchFillBitFieldBool(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numBits = numBytes * CHAR_BIT;
  const bool value = state.range(1) != 0;

  state.SetLabel(vtkm::cont::GetHumanReadableSize(numBytes));

  vtkm::cont::BitField bits;

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::Fill(device, bits, value, numBits);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
};
VTKM_BENCHMARK_OPTS(BenchFillBitFieldBool,
                      ->Ranges({ { FullRange.first, FullRange.second }, { 0, 1 } })
                      ->ArgNames({ "Size", "Val" }));

template <typename WordType>
void BenchFillBitFieldMask(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numBits = numBytes * CHAR_BIT;
  const WordType mask = static_cast<WordType>(0x1);

  state.SetLabel(vtkm::cont::GetHumanReadableSize(numBytes));

  vtkm::cont::BitField bits;

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::Fill(device, bits, mask, numBits);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
};
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchFillBitFieldMask,
                                ->Range(FullRange.first, FullRange.second)
                                ->ArgName("Size"),
                              FillWordTypes);

template <typename ValueType>
void BenchLowerBounds(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;

  const vtkm::Id numValuesBytes = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numInputsBytes = static_cast<vtkm::Id>(state.range(1));

  const vtkm::Id numValues = BytesToWords<ValueType>(numValuesBytes);
  const vtkm::Id numInputs = BytesToWords<ValueType>(numInputsBytes);

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numValuesBytes, numValues) << " | " << numInputs << " lookups";
    state.SetLabel(desc.str());
  }

  vtkm::cont::ArrayHandle<ValueType> input;
  vtkm::cont::ArrayHandle<vtkm::Id> output;
  vtkm::cont::ArrayHandle<ValueType> values;

  FillRandomTestValue(input, numInputs);
  FillRandomTestValue(values, numValues);
  vtkm::cont::Algorithm::Sort(device, values);

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::LowerBounds(device, input, values, output);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};

VTKM_BENCHMARK_TEMPLATES_OPTS(BenchLowerBounds,
                                ->RangeMultiplier(SmallRangeMultiplier)
                                ->Ranges({ SmallRange, SmallRange })
                                ->ArgNames({ "Size", "InputSize" }),
                              TypeList);

template <typename ValueType>
void BenchReduce(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  vtkm::cont::ArrayHandle<ValueType> array;
  FillTestValue(array, numValues);

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = vtkm::cont::Algorithm::Reduce(
      device, array, vtkm::TypeTraits<ValueType>::ZeroInitialization());
    benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchReduce,
                                ->Range(FullRange.first, FullRange.second)
                                ->ArgName("Size"),
                              TypeList);

template <typename ValueType>
void BenchReduceByKey(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;

  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  const vtkm::Id percentKeys = state.range(1);
  const vtkm::Id numKeys = std::max((numValues * percentKeys) / 100, vtkm::Id{ 1 });

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numBytes, numValues) << " | " << numKeys << " ("
         << ((numKeys * 100) / numValues) << "%) unique";
    state.SetLabel(desc.str());
  }

  vtkm::cont::ArrayHandle<ValueType> valuesIn;
  vtkm::cont::ArrayHandle<ValueType> valuesOut;
  vtkm::cont::ArrayHandle<vtkm::Id> keysIn;
  vtkm::cont::ArrayHandle<vtkm::Id> keysOut;

  FillTestValue(valuesIn, numValues);
  FillModuloTestValue(keysIn, numKeys, numValues);
  vtkm::cont::Algorithm::Sort(device, keysIn);

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::ReduceByKey(device, keysIn, valuesIn, keysOut, valuesOut, vtkm::Add{});
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};

void BenchReduceByKeyGenerator(benchmark::internal::Benchmark* bm)
{
  bm->RangeMultiplier(SmallRangeMultiplier);
  bm->ArgNames({ "Size", "%Keys" });

  for (int64_t pcntKeys = 0; pcntKeys <= 100; pcntKeys += 25)
  {
    bm->Ranges({ SmallRange, { pcntKeys, pcntKeys } });
  }
}

VTKM_BENCHMARK_TEMPLATES_APPLY(BenchReduceByKey, BenchReduceByKeyGenerator, SmallTypeList);

template <typename ValueType>
void BenchScanExclusive(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  vtkm::cont::ArrayHandle<ValueType> src;
  vtkm::cont::ArrayHandle<ValueType> dst;

  FillTestValue(src, numValues);

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::ScanExclusive(device, src, dst);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchScanExclusive,
                                ->Range(FullRange.first, FullRange.second)
                                ->ArgName("Size"),
                              TypeList);

template <typename ValueType>
void BenchScanExtended(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  vtkm::cont::ArrayHandle<ValueType> src;
  vtkm::cont::ArrayHandle<ValueType> dst;

  FillTestValue(src, numValues);

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::ScanExtended(device, src, dst);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchScanExtended,
                                ->Range(FullRange.first, FullRange.second)
                                ->ArgName("Size"),
                              TypeList);

template <typename ValueType>
void BenchScanInclusive(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  vtkm::cont::ArrayHandle<ValueType> src;
  vtkm::cont::ArrayHandle<ValueType> dst;

  FillTestValue(src, numValues);

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::ScanInclusive(device, src, dst);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchScanInclusive,
                                ->Range(FullRange.first, FullRange.second)
                                ->ArgName("Size"),
                              TypeList);

template <typename ValueType>
void BenchSort(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  vtkm::cont::ArrayHandle<ValueType> unsorted;
  FillRandomTestValue(unsorted, numValues);

  vtkm::cont::ArrayHandle<ValueType> array;

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Reset the array to the unsorted state:
    vtkm::cont::Algorithm::Copy(device, unsorted, array);

    timer.Start();
    vtkm::cont::Algorithm::Sort(array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchSort,
                                ->Range(FullRange.first, FullRange.second)
                                ->ArgName("Size"),
                              TypeList);

template <typename ValueType>
void BenchSortByKey(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;

  const vtkm::Id numBytes = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  const vtkm::Id percentKeys = state.range(1);
  const vtkm::Id numKeys = std::max((numValues * percentKeys) / 100, vtkm::Id{ 1 });

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numBytes, numValues) << " | " << numKeys << " ("
         << ((numKeys * 100) / numValues) << "%) keys";
    state.SetLabel(desc.str());
  }

  vtkm::cont::ArrayHandle<ValueType> valuesUnsorted;
  vtkm::cont::ArrayHandle<ValueType> values;
  vtkm::cont::ArrayHandle<vtkm::Id> keysUnsorted;
  vtkm::cont::ArrayHandle<vtkm::Id> keys;

  FillRandomTestValue(valuesUnsorted, numValues);

  FillModuloTestValue(keysUnsorted, numKeys, numValues);
  vtkm::cont::Algorithm::Sort(device, keysUnsorted);

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    vtkm::cont::Algorithm::Copy(device, keysUnsorted, keys);
    vtkm::cont::Algorithm::Copy(device, valuesUnsorted, values);

    timer.Start();
    vtkm::cont::Algorithm::SortByKey(device, keys, values);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};

void BenchSortByKeyGenerator(benchmark::internal::Benchmark* bm)
{
  bm->RangeMultiplier(SmallRangeMultiplier);
  bm->ArgNames({ "Size", "%Keys" });
  for (int64_t pcntKeys = 0; pcntKeys <= 100; pcntKeys += 25)
  {
    bm->Ranges({ SmallRange, { pcntKeys, pcntKeys } });
  }
}

VTKM_BENCHMARK_TEMPLATES_APPLY(BenchSortByKey, BenchSortByKeyGenerator, SmallTypeList);

template <typename ValueType>
void BenchStableSortIndices(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  vtkm::cont::ArrayHandle<ValueType> values;
  FillRandomTestValue(values, numValues);

  vtkm::cont::ArrayHandle<vtkm::Id> indices;

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Reset the indices array:
    vtkm::cont::Algorithm::Copy(device, vtkm::cont::make_ArrayHandleIndex(numValues), indices);

    timer.Start();
    vtkm::worklet::StableSortIndices::Sort(device, values, indices);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchStableSortIndices,
                                ->Range(SmallRange.first, SmallRange.second)
                                ->ArgName("Size"),
                              TypeList);

template <typename ValueType>
void BenchStableSortIndicesUnique(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  const vtkm::Id percentUnique = state.range(1);
  const vtkm::Id numUnique = std::max((numValues * percentUnique) / 100, vtkm::Id{ 1 });

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numBytes, numValues) << " | " << numUnique << " ("
         << ((numUnique * 100) / numValues) << "%) unique";
    state.SetLabel(desc.str());
  }

  vtkm::cont::ArrayHandle<ValueType> values;
  FillRandomModTestValue(values, numUnique, numValues);

  // Prepare IndicesOrig to contain the sorted, non-unique index map:
  const vtkm::cont::ArrayHandle<vtkm::Id> indicesOrig =
    vtkm::worklet::StableSortIndices::Sort(device, values);

  // Working memory:
  vtkm::cont::ArrayHandle<vtkm::Id> indices;

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Reset the indices array:
    vtkm::cont::Algorithm::Copy(device, indicesOrig, indices);

    timer.Start();
    vtkm::worklet::StableSortIndices::Unique(device, values, indices);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};

void BenchmarkStableSortIndicesUniqueGenerator(benchmark::internal::Benchmark* bm)
{
  bm->RangeMultiplier(SmallRangeMultiplier);
  bm->ArgNames({ "Size", "%Uniq" });
  for (int64_t pcntUnique = 0; pcntUnique <= 100; pcntUnique += 25)
  {
    // Cap the max size here at 21 MiB. This sort is too slow.
    bm->Ranges({ { SmallRange.first, 1 << 21 }, { pcntUnique, pcntUnique } });
  }
}

VTKM_BENCHMARK_TEMPLATES_APPLY(BenchStableSortIndicesUnique,
                               BenchmarkStableSortIndicesUniqueGenerator,
                               SmallTypeList);

template <typename ValueType>
void BenchUnique(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id numBytes = state.range(0);
  const vtkm::Id numValues = BytesToWords<ValueType>(numBytes);

  const vtkm::Id percentUnique = state.range(1);
  const vtkm::Id numUnique = std::max((numValues * percentUnique) / 100, vtkm::Id{ 1 });

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numBytes, numValues) << " | " << numUnique << " ("
         << ((numUnique * 100) / numValues) << "%) unique";
    state.SetLabel(desc.str());
  }

  vtkm::cont::ArrayHandle<ValueType> valuesOrig;
  FillRandomModTestValue(valuesOrig, numUnique, numValues);

  // Presort the input:
  vtkm::cont::Algorithm::Sort(device, valuesOrig);

  vtkm::cont::ArrayHandle<ValueType> values;
  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Make a working copy of the input:
    vtkm::cont::Algorithm::Copy(device, valuesOrig, values);

    timer.Start();
    vtkm::cont::Algorithm::Unique(device, values);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};

void BenchmarkUniqueGenerator(benchmark::internal::Benchmark* bm)
{
  bm->RangeMultiplier(SmallRangeMultiplier);
  bm->ArgNames({ "Size", "%Uniq" });
  for (int64_t pcntUnique = 0; pcntUnique <= 100; pcntUnique += 25)
  {
    bm->Ranges({ SmallRange, { pcntUnique, pcntUnique } });
  }
}

VTKM_BENCHMARK_TEMPLATES_APPLY(BenchUnique, BenchmarkUniqueGenerator, SmallTypeList);

template <typename ValueType>
void BenchUpperBounds(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;

  const vtkm::Id numValuesBytes = static_cast<vtkm::Id>(state.range(0));
  const vtkm::Id numInputsBytes = static_cast<vtkm::Id>(state.range(1));

  const vtkm::Id numValues = BytesToWords<ValueType>(numValuesBytes);
  const vtkm::Id numInputs = BytesToWords<ValueType>(numInputsBytes);

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numValuesBytes, numValues) << " | " << numInputs << " lookups";
    state.SetLabel(desc.str());
  }

  vtkm::cont::ArrayHandle<ValueType> input;
  vtkm::cont::ArrayHandle<vtkm::Id> output;
  vtkm::cont::ArrayHandle<ValueType> values;

  FillRandomTestValue(input, numInputs);
  FillRandomTestValue(values, numValues);
  vtkm::cont::Algorithm::Sort(device, values);

  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::UpperBounds(device, input, values, output);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetItemsProcessed(static_cast<int64_t>(numInputs) * iterations);
};

VTKM_BENCHMARK_TEMPLATES_OPTS(BenchUpperBounds,
                                ->RangeMultiplier(SmallRangeMultiplier)
                                ->Ranges({ SmallRange, SmallRange })
                                ->ArgNames({ "Size", "InputSize" }),
                              SmallTypeList);

} // end anon namespace

int main(int argc, char* argv[])
{
  // Parse VTK-m options:
  auto opts = vtkm::cont::InitializeOptions::RequireDevice | vtkm::cont::InitializeOptions::AddHelp;
  Config = vtkm::cont::Initialize(argc, argv, opts);

  // Setup device:
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);

// Handle NumThreads command-line arg:
#ifdef VTKM_ENABLE_TBB
  int numThreads = tbb::task_scheduler_init::automatic;
#endif // TBB

  if (argc == 3)
  {
    if (std::string(argv[1]) == "NumThreads")
    {
#ifdef VTKM_ENABLE_TBB
      std::istringstream parse(argv[2]);
      parse >> numThreads;
      std::cout << "Selected " << numThreads << " TBB threads." << std::endl;
#else
      std::cerr << "NumThreads valid only on TBB. Ignoring." << std::endl;
#endif // TBB
    }
  }

#ifdef VTKM_ENABLE_TBB
  // Must not be destroyed as long as benchmarks are running:
  tbb::task_scheduler_init init(numThreads);
#endif // TBB

  // handle benchmarking related args and run benchmarks:
  VTKM_EXECUTE_BENCHMARKS(argc, argv);
}
