//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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

#include <viskores/TypeTraits.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/BitField.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/Timer.h>

#include <viskores/worklet/StableSortIndices.h>
#include <viskores/worklet/WorkletMapField.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <utility>

#include <viskoresstd/integer_sequence.h>

#include <viskores/internal/Windows.h>

#ifdef VISKORES_ENABLE_OPENMP
#include <omp.h>
#endif

namespace
{

// Parametrize the input size samples for most of the benchmarks
//
// Define at compile time:
//
//   Being Viskores_BENCHS_RANGE_LOWER_BOUNDARY b0 and,
//   being Viskores_BENCHS_RANGE_UPPER_BOUNDARY b1
//
// This will create the following sample sizes b0, b0*2^3, b0*2^6, ..., b1.
//
// Notice that setting up Viskores_BENCHS_RANGE_LOWER_BOUNDARY / Viskores_BENCHS_RANGE_UPPER_BOUNDARY
// will affect both ShortRange and FullRange.
//
#ifndef Viskores_BENCHS_RANGE_LOWER_BOUNDARY
#define FULL_RANGE_LOWER_BOUNDARY (1 << 12)  //  4 KiB
#define SHORT_RANGE_LOWER_BOUNDARY (1 << 15) // 32 KiB

#else
#define FULL_RANGE_LOWER_BOUNDARY (Viskores_BENCHS_RANGE_LOWER_BOUNDARY)
#define SHORT_RANGE_LOWER_BOUNDARY (Viskores_BENCHS_RANGE_LOWER_BOUNDARY)

#endif

#ifndef Viskores_BENCHS_RANGE_UPPER_BOUNDARY
#define FULL_RANGE_UPPER_BOUNDARY (1 << 27)             // 128 MiB
#define SHORT_RANGE_UPPER_BOUNDARY (1 << 27)            // 128 MiB
#define BITFIELD_TO_UNORDEREDSET_MAX_SAMPLING (1 << 26) // 64 MiB

#else
#define FULL_RANGE_UPPER_BOUNDARY (Viskores_BENCHS_RANGE_UPPER_BOUNDARY)
#define SHORT_RANGE_UPPER_BOUNDARY (Viskores_BENCHS_RANGE_UPPER_BOUNDARY)
#define BITFIELD_TO_UNORDEREDSET_MAX_SAMPLING (Viskores_BENCHS_RANGE_UPPER_BOUNDARY)

#endif

// Default sampling rate is x8 and always includes min/max,
// so this will generate 7 samples at:
// 1: 4 KiB
// 2: 32 KiB
// 3: 256 KiB
// 4: 2 MiB
// 5: 16 MiB
// 6: 128 MiB
static const std::pair<int64_t, int64_t> FullRange{ FULL_RANGE_LOWER_BOUNDARY,
                                                    FULL_RANGE_UPPER_BOUNDARY };

// Smaller range that can be used to reduce the number of benchmarks. Used
// with `RangeMultiplier(SmallRangeMultiplier)`, this produces:
// 1: 32 KiB
// 2: 2 MiB
// 3: 128 MiB
static const std::pair<int64_t, int64_t> SmallRange{ SHORT_RANGE_LOWER_BOUNDARY,
                                                     SHORT_RANGE_UPPER_BOUNDARY };
static constexpr int SmallRangeMultiplier = 1 << 21; // Ensure a sample at 2MiB

#ifndef VISKORES_ENABLE_KOKKOS
using TypeList = viskores::List<viskores::UInt8,
                                viskores::Float32,
                                viskores::Int64,
                                viskores::Float64,
                                viskores::Vec3f_32,
                                viskores::Pair<viskores::Int32, viskores::Float64>>;

using SmallTypeList = viskores::List<viskores::UInt8, viskores::Float32, viskores::Int64>;
#else
// Kokkos requires 0 == (sizeof(Kokkos::MinMaxScalar<ValueType>) % sizeof(int)
// so removing viskores::UInt8
using TypeList = viskores::List<viskores::Float32,
                                viskores::Int64,
                                viskores::Float64,
                                viskores::Vec3f_32,
                                viskores::Pair<viskores::Int32, viskores::Float64>>;

using SmallTypeList = viskores::List<viskores::Float32, viskores::Int64>;
#endif

// Only 32-bit words are currently supported atomically across devices:
using AtomicWordTypes = viskores::List<viskores::UInt32>;

// The Fill algorithm uses different word types:
using FillWordTypes =
  viskores::List<viskores::UInt8, viskores::UInt16, viskores::UInt32, viskores::UInt64>;

using IdArrayHandle = viskores::cont::ArrayHandle<viskores::Id>;

// Hold configuration state (e.g. active device)
viskores::cont::InitializeResult Config;

// Helper function to convert numBytes to numWords:
template <typename T>
viskores::Id BytesToWords(viskores::Id numBytes)
{
  const viskores::Id wordSize = static_cast<viskores::Id>(sizeof(T));
  return numBytes / wordSize;
}

// Various kernels used by the different benchmarks to accelerate
// initialization of data
template <typename T>
struct TestValueFunctor
{
  VISKORES_EXEC_CONT
  T operator()(viskores::Id i) const { return static_cast<T>(i + 10); }
};

template <typename T>
VISKORES_EXEC_CONT T TestValue(viskores::Id index)
{
  return TestValueFunctor<T>{}(index);
}

template <typename T, typename U>
struct TestValueFunctor<viskores::Pair<T, U>>
{
  VISKORES_EXEC_CONT viskores::Pair<T, U> operator()(viskores::Id i) const
  {
    return viskores::make_Pair(TestValue<T>(i), TestValue<U>(i + 1));
  }
};

template <typename T, viskores::IdComponent N>
struct TestValueFunctor<viskores::Vec<T, N>>
{
  template <std::size_t... Ns>
  VISKORES_EXEC_CONT viskores::Vec<T, N> FillVec(viskores::Id i,
                                                 viskoresstd::index_sequence<Ns...>) const
  {
    return viskores::make_Vec(TestValue<T>(i + static_cast<viskores::Id>(Ns))...);
  }

  VISKORES_EXEC_CONT viskores::Vec<T, N> operator()(viskores::Id i) const
  {
    return FillVec(i, viskoresstd::make_index_sequence<static_cast<std::size_t>(N)>{});
  }
};

template <typename ArrayT>
VISKORES_CONT void FillTestValue(ArrayT& array, viskores::Id numValues)
{
  using T = typename ArrayT::ValueType;
  viskores::cont::Algorithm::Copy(
    viskores::cont::make_ArrayHandleImplicit(TestValueFunctor<T>{}, numValues), array);
}

template <typename T>
struct ModuloTestValueFunctor
{
  viskores::Id Mod;
  VISKORES_EXEC_CONT
  T operator()(viskores::Id i) const { return TestValue<T>(i % this->Mod); }
};

template <typename ArrayT>
VISKORES_CONT void FillModuloTestValue(ArrayT& array, viskores::Id mod, viskores::Id numValues)
{
  using T = typename ArrayT::ValueType;
  viskores::cont::Algorithm::Copy(
    viskores::cont::make_ArrayHandleImplicit(ModuloTestValueFunctor<T>{ mod }, numValues), array);
}

template <typename T>
struct BinaryTestValueFunctor
{
  viskores::Id Mod;
  VISKORES_EXEC_CONT
  T operator()(viskores::Id i) const
  {
    T zero = viskores::TypeTraits<T>::ZeroInitialization();

    // Always return zero unless 1 == Mod
    if (i == this->Mod)
    { // Ensure that the result is not equal to zero
      T retVal;
      do
      {
        retVal = TestValue<T>(i++);
      } while (retVal == zero);
      return retVal;
    }
    return std::move(zero);
  }
};

template <typename ArrayT>
VISKORES_CONT void FillBinaryTestValue(ArrayT& array, viskores::Id mod, viskores::Id numValues)
{
  using T = typename ArrayT::ValueType;
  viskores::cont::Algorithm::Copy(
    viskores::cont::make_ArrayHandleImplicit(BinaryTestValueFunctor<T>{ mod }, numValues), array);
}

template <typename ArrayT>
VISKORES_CONT void FillRandomTestValue(ArrayT& array, viskores::Id numValues)
{
  using ValueType = typename ArrayT::ValueType;

  std::mt19937_64 rng;

  array.Allocate(numValues);
  auto portal = array.WritePortal();
  for (viskores::Id i = 0; i < portal.GetNumberOfValues(); ++i)
  {
    portal.Set(i, TestValue<ValueType>(static_cast<viskores::Id>(rng())));
  }
}

template <typename ArrayT>
VISKORES_CONT void FillRandomModTestValue(ArrayT& array, viskores::Id mod, viskores::Id numValues)
{
  using ValueType = typename ArrayT::ValueType;

  std::mt19937_64 rng;

  array.Allocate(numValues);
  auto portal = array.WritePortal();
  for (viskores::Id i = 0; i < portal.GetNumberOfValues(); ++i)
  {
    portal.Set(i, TestValue<ValueType>(static_cast<viskores::Id>(rng()) % mod));
  }
}

static inline std::string SizeAndValuesString(viskores::Id numBytes, viskores::Id numValues)
{
  std::ostringstream str;
  str << viskores::cont::GetHumanReadableSize(numBytes) << " | " << numValues << " values";
  return str.str();
}

template <typename WordType>
struct GenerateBitFieldWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn dummy, BitFieldOut);
  using ExecutionSignature = void(InputIndex, _2);

  WordType Exemplar;
  viskores::Id Stride;
  viskores::Id MaxMaskedWord;

  VISKORES_CONT
  GenerateBitFieldWorklet(WordType exemplar, viskores::Id stride, viskores::Id maxMaskedWord)
    : Exemplar(exemplar)
    , Stride(stride)
    , MaxMaskedWord(maxMaskedWord)
  {
  }

  template <typename BitPortal>
  VISKORES_EXEC void operator()(viskores::Id wordIdx, BitPortal& portal) const
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
VISKORES_CONT viskores::cont::BitField GenerateBitField(WordType exemplar,
                                                        viskores::Id stride,
                                                        viskores::Id maxMaskedWord,
                                                        viskores::Id numWords)
{
  if (stride == 0)
  {
    stride = 1;
  }

  viskores::Id numBits = numWords * static_cast<viskores::Id>(sizeof(WordType) * CHAR_BIT);

  viskores::cont::BitField bits;
  bits.Allocate(numBits);

  // This array is just to set the input domain appropriately:
  auto dummy = viskores::cont::make_ArrayHandleConstant<viskores::Int32>(0, numWords);

  viskores::cont::Invoker invoker{ Config.Device };
  invoker(GenerateBitFieldWorklet<WordType>{ exemplar, stride, maxMaskedWord }, dummy, bits);

  return bits;
};

//==============================================================================
// Benchmarks begin:

template <typename WordType>
void BenchBitFieldToUnorderedSetImpl(benchmark::State& state,
                                     viskores::Id numBytes,
                                     WordType exemplar,
                                     viskores::Id stride,
                                     viskores::Float32 fillRatio,
                                     const std::string& name)
{
  const viskores::Id numWords = BytesToWords<WordType>(numBytes);
  const viskores::Id maxMaskedWord =
    static_cast<viskores::Id>(static_cast<viskores::Float32>(numWords) * fillRatio);

  { // Set label:
    const viskores::Id numFilledWords = maxMaskedWord / stride;
    const viskores::Id numSetBits = numFilledWords * viskores::CountSetBits(exemplar);
    std::stringstream desc;
    desc << viskores::cont::GetHumanReadableSize(numBytes) << " | " << name << " | "
         << "SetBits:" << numSetBits;
    state.SetLabel(desc.str());
  }

  viskores::cont::BitField bits =
    GenerateBitField<WordType>(exemplar, stride, maxMaskedWord, numWords);

  IdArrayHandle indices;

  viskores::cont::Timer timer{ Config.Device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::BitFieldToUnorderedSet(Config.Device, bits, indices);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
};

void BenchBitFieldToUnorderedSet(benchmark::State& state)
{
  using WordType = viskores::WordTypeDefault;

  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
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
      VISKORES_UNREACHABLE("Internal error.");
  }
}

void BenchBitFieldToUnorderedSetGenerator(benchmark::internal::Benchmark* bm)
{
  // Use a reduced NUM_BYTES_MAX value here -- these benchmarks allocate one
  // 8-byte id per bit, so this caps the index array out at 512 MB:
  static int64_t numBytesMax = std::min(1 << 29, BITFIELD_TO_UNORDEREDSET_MAX_SAMPLING);

  bm->UseManualTime();
  bm->ArgNames({ "Size", "C" });

  for (int64_t config = 0; config < 6; ++config)
  {
    bm->Ranges({ { FullRange.first, numBytesMax }, { config, config } });
  }
}

VISKORES_BENCHMARK_APPLY(BenchBitFieldToUnorderedSet, BenchBitFieldToUnorderedSetGenerator);

template <typename ValueType>
void BenchCopy(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  viskores::cont::ArrayHandle<ValueType> src;
  viskores::cont::ArrayHandle<ValueType> dst;

  FillTestValue(src, numValues);

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::Copy(device, src, dst);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};

VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchCopy, ->Ranges({ FullRange })->ArgName("Size"), TypeList);

template <typename ValueType>
void BenchCopyIf(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  const viskores::Id percentValid = static_cast<viskores::Id>(state.range(1));
  const viskores::Id numValid = (numValues * percentValid) / 100;
  const viskores::Id modulo = numValid != 0 ? numValues / numValid : numValues + 1;

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numBytes, numValues) << " | " << numValid << " valid ("
         << (numValid * 100 / numValues) << "%)";
    state.SetLabel(desc.str());
  }

  viskores::cont::ArrayHandle<ValueType> src;
  viskores::cont::ArrayHandle<viskores::Id> stencil;
  viskores::cont::ArrayHandle<ValueType> dst;

  FillTestValue(src, numValues);
  FillBinaryTestValue(stencil, modulo, numValues);

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::CopyIf(device, src, stencil, dst);
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

VISKORES_BENCHMARK_TEMPLATES_APPLY(BenchCopyIf, BenchCopyIfGenerator, SmallTypeList);

template <typename WordType>
void BenchCountSetBitsImpl(benchmark::State& state,
                           viskores::Id numBytes,
                           WordType exemplar,
                           viskores::Id stride,
                           viskores::Float32 fillRatio,
                           const std::string& name)
{
  const viskores::Id numWords = BytesToWords<WordType>(numBytes);
  const viskores::Id maxMaskedWord =
    static_cast<viskores::Id>(static_cast<viskores::Float32>(numWords) * fillRatio);

  { // Set label:
    const viskores::Id numFilledWords = maxMaskedWord / stride;
    const viskores::Id numSetBits = numFilledWords * viskores::CountSetBits(exemplar);
    std::stringstream desc;
    desc << viskores::cont::GetHumanReadableSize(numBytes) << " | " << name << " | "
         << "SetBits:" << numSetBits;
    state.SetLabel(desc.str());
  }

  viskores::cont::BitField bits =
    GenerateBitField<WordType>(exemplar, stride, maxMaskedWord, numWords);

  viskores::cont::Timer timer{ Config.Device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::Id setBits = viskores::cont::Algorithm::CountSetBits(Config.Device, bits);
    benchmark::DoNotOptimize(setBits);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
};

void BenchCountSetBits(benchmark::State& state)
{
  using WordType = viskores::WordTypeDefault;

  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
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
      VISKORES_UNREACHABLE("Internal error.");
  }
}

void BenchCountSetBitsGenerator(benchmark::internal::Benchmark* bm)
{
  bm->UseManualTime();
  bm->ArgNames({ "Size", "C" });

  for (int64_t config = 0; config < 6; ++config)
  {
    bm->Ranges({ { FullRange.first, FullRange.second }, { config, config } });
  }
}
VISKORES_BENCHMARK_APPLY(BenchCountSetBits, BenchCountSetBitsGenerator);

template <typename ValueType>
void BenchFillArrayHandle(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  viskores::cont::ArrayHandle<ValueType> array;

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::Fill(device, array, TestValue<ValueType>(19), numValues);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchFillArrayHandle,
                                    ->Range(FullRange.first, FullRange.second)
                                    ->ArgName("Size"),
                                  TypeList);

void BenchFillBitFieldBool(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numBits = numBytes * CHAR_BIT;
  const bool value = state.range(1) != 0;

  state.SetLabel(viskores::cont::GetHumanReadableSize(numBytes));

  viskores::cont::BitField bits;

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::Fill(device, bits, value, numBits);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
};
VISKORES_BENCHMARK_OPTS(BenchFillBitFieldBool,
                          ->Ranges({ { FullRange.first, FullRange.second }, { 0, 1 } })
                          ->ArgNames({ "Size", "Val" }));

template <typename WordType>
void BenchFillBitFieldMask(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numBits = numBytes * CHAR_BIT;
  const WordType mask = static_cast<WordType>(0x1);

  state.SetLabel(viskores::cont::GetHumanReadableSize(numBytes));

  viskores::cont::BitField bits;

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::Fill(device, bits, mask, numBits);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
};
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchFillBitFieldMask,
                                    ->Range(FullRange.first, FullRange.second)
                                    ->ArgName("Size"),
                                  FillWordTypes);

template <typename ValueType>
void BenchLowerBounds(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;

  const viskores::Id numValuesBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numInputsBytes = static_cast<viskores::Id>(state.range(1));

  const viskores::Id numValues = BytesToWords<ValueType>(numValuesBytes);
  const viskores::Id numInputs = BytesToWords<ValueType>(numInputsBytes);

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numValuesBytes, numValues) << " | " << numInputs << " lookups";
    state.SetLabel(desc.str());
  }

  viskores::cont::ArrayHandle<ValueType> input;
  viskores::cont::ArrayHandle<viskores::Id> output;
  viskores::cont::ArrayHandle<ValueType> values;

  FillRandomTestValue(input, numInputs);
  FillRandomTestValue(values, numValues);
  viskores::cont::Algorithm::Sort(device, values);

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::LowerBounds(device, input, values, output);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};

VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchLowerBounds,
                                    ->RangeMultiplier(SmallRangeMultiplier)
                                    ->Ranges({ SmallRange, SmallRange })
                                    ->ArgNames({ "Size", "InputSize" }),
                                  TypeList);

template <typename ValueType>
void BenchReduce(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  viskores::cont::ArrayHandle<ValueType> array;
  FillTestValue(array, numValues);

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = viskores::cont::Algorithm::Reduce(
      device, array, viskores::TypeTraits<ValueType>::ZeroInitialization());
    benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchReduce,
                                    ->Range(FullRange.first, FullRange.second)
                                    ->ArgName("Size"),
                                  TypeList);

template <typename ValueType>
void BenchReduceByKey(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;

  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  const viskores::Id percentKeys = static_cast<viskores::Id>(state.range(1));
  const viskores::Id numKeys = std::max((numValues * percentKeys) / 100, viskores::Id{ 1 });

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numBytes, numValues) << " | " << numKeys << " ("
         << ((numKeys * 100) / numValues) << "%) unique";
    state.SetLabel(desc.str());
  }

  viskores::cont::ArrayHandle<ValueType> valuesIn;
  viskores::cont::ArrayHandle<ValueType> valuesOut;
  viskores::cont::ArrayHandle<viskores::Id> keysIn;
  viskores::cont::ArrayHandle<viskores::Id> keysOut;

  FillTestValue(valuesIn, numValues);
  FillModuloTestValue(keysIn, numKeys, numValues);
  viskores::cont::Algorithm::Sort(device, keysIn);

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::ReduceByKey(
      device, keysIn, valuesIn, keysOut, valuesOut, viskores::Add{});
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

VISKORES_BENCHMARK_TEMPLATES_APPLY(BenchReduceByKey, BenchReduceByKeyGenerator, SmallTypeList);

template <typename ValueType>
void BenchScanExclusive(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  viskores::cont::ArrayHandle<ValueType> src;
  viskores::cont::ArrayHandle<ValueType> dst;

  FillTestValue(src, numValues);

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::ScanExclusive(device, src, dst);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchScanExclusive,
                                    ->Range(FullRange.first, FullRange.second)
                                    ->ArgName("Size"),
                                  TypeList);

template <typename ValueType>
void BenchScanExtended(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  viskores::cont::ArrayHandle<ValueType> src;
  viskores::cont::ArrayHandle<ValueType> dst;

  FillTestValue(src, numValues);

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::ScanExtended(device, src, dst);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchScanExtended,
                                    ->Range(FullRange.first, FullRange.second)
                                    ->ArgName("Size"),
                                  TypeList);

template <typename ValueType>
void BenchScanInclusive(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  viskores::cont::ArrayHandle<ValueType> src;
  viskores::cont::ArrayHandle<ValueType> dst;

  FillTestValue(src, numValues);

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::ScanInclusive(device, src, dst);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchScanInclusive,
                                    ->Range(FullRange.first, FullRange.second)
                                    ->ArgName("Size"),
                                  TypeList);

template <typename ValueType>
void BenchSort(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  viskores::cont::ArrayHandle<ValueType> unsorted;
  FillRandomTestValue(unsorted, numValues);

  viskores::cont::ArrayHandle<ValueType> array;

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Reset the array to the unsorted state:
    viskores::cont::Algorithm::Copy(device, unsorted, array);

    timer.Start();
    viskores::cont::Algorithm::Sort(array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchSort,
                                    ->Range(FullRange.first, FullRange.second)
                                    ->ArgName("Size"),
                                  TypeList);

template <typename ValueType>
void BenchSortByKey(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;

  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  const viskores::Id percentKeys = static_cast<viskores::Id>(state.range(1));
  const viskores::Id numKeys = std::max((numValues * percentKeys) / 100, viskores::Id{ 1 });

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numBytes, numValues) << " | " << numKeys << " ("
         << ((numKeys * 100) / numValues) << "%) keys";
    state.SetLabel(desc.str());
  }

  viskores::cont::ArrayHandle<ValueType> valuesUnsorted;
  viskores::cont::ArrayHandle<ValueType> values;
  viskores::cont::ArrayHandle<viskores::Id> keysUnsorted;
  viskores::cont::ArrayHandle<viskores::Id> keys;

  FillRandomTestValue(valuesUnsorted, numValues);

  FillModuloTestValue(keysUnsorted, numKeys, numValues);
  viskores::cont::Algorithm::Sort(device, keysUnsorted);

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    viskores::cont::Algorithm::Copy(device, keysUnsorted, keys);
    viskores::cont::Algorithm::Copy(device, valuesUnsorted, values);

    timer.Start();
    viskores::cont::Algorithm::SortByKey(device, keys, values);
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

VISKORES_BENCHMARK_TEMPLATES_APPLY(BenchSortByKey, BenchSortByKeyGenerator, SmallTypeList);

template <typename ValueType>
void BenchStableSortIndices(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  state.SetLabel(SizeAndValuesString(numBytes, numValues));

  viskores::cont::ArrayHandle<ValueType> values;
  FillRandomTestValue(values, numValues);

  viskores::cont::ArrayHandle<viskores::Id> indices;

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Reset the indices array:
    viskores::cont::Algorithm::Copy(
      device, viskores::cont::make_ArrayHandleIndex(numValues), indices);

    timer.Start();
    viskores::worklet::StableSortIndices::Sort(device, values, indices);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchStableSortIndices,
                                    ->Range(SmallRange.first, SmallRange.second)
                                    ->ArgName("Size"),
                                  TypeList);

template <typename ValueType>
void BenchStableSortIndicesUnique(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  const viskores::Id percentUnique = static_cast<viskores::Id>(state.range(1));
  const viskores::Id numUnique = std::max((numValues * percentUnique) / 100, viskores::Id{ 1 });

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numBytes, numValues) << " | " << numUnique << " ("
         << ((numUnique * 100) / numValues) << "%) unique";
    state.SetLabel(desc.str());
  }

  viskores::cont::ArrayHandle<ValueType> values;
  FillRandomModTestValue(values, numUnique, numValues);

  // Prepare IndicesOrig to contain the sorted, non-unique index map:
  const viskores::cont::ArrayHandle<viskores::Id> indicesOrig =
    viskores::worklet::StableSortIndices::Sort(device, values);

  // Working memory:
  viskores::cont::ArrayHandle<viskores::Id> indices;

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Reset the indices array:
    viskores::cont::Algorithm::Copy(device, indicesOrig, indices);

    timer.Start();
    viskores::worklet::StableSortIndices::Unique(device, values, indices);
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
    // Cap the max size here at 2 MiB. This sort is too slow.
    const int64_t maxSize = 1 << 21;
    bm->Ranges(
      { { SmallRange.first, std::min(maxSize, SmallRange.second) }, { pcntUnique, pcntUnique } });
  }
}

VISKORES_BENCHMARK_TEMPLATES_APPLY(BenchStableSortIndicesUnique,
                                   BenchmarkStableSortIndicesUniqueGenerator,
                                   SmallTypeList);

template <typename ValueType>
void BenchUnique(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numValues = BytesToWords<ValueType>(numBytes);

  const viskores::Id percentUnique = static_cast<viskores::Id>(state.range(1));
  const viskores::Id numUnique = std::max((numValues * percentUnique) / 100, viskores::Id{ 1 });

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numBytes, numValues) << " | " << numUnique << " ("
         << ((numUnique * 100) / numValues) << "%) unique";
    state.SetLabel(desc.str());
  }

  viskores::cont::ArrayHandle<ValueType> valuesOrig;
  FillRandomModTestValue(valuesOrig, numUnique, numValues);

  // Presort the input:
  viskores::cont::Algorithm::Sort(device, valuesOrig);

  viskores::cont::ArrayHandle<ValueType> values;
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Make a working copy of the input:
    viskores::cont::Algorithm::Copy(device, valuesOrig, values);

    timer.Start();
    viskores::cont::Algorithm::Unique(device, values);
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

VISKORES_BENCHMARK_TEMPLATES_APPLY(BenchUnique, BenchmarkUniqueGenerator, SmallTypeList);

template <typename ValueType>
void BenchUpperBounds(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;

  const viskores::Id numValuesBytes = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numInputsBytes = static_cast<viskores::Id>(state.range(1));

  const viskores::Id numValues = BytesToWords<ValueType>(numValuesBytes);
  const viskores::Id numInputs = BytesToWords<ValueType>(numInputsBytes);

  {
    std::ostringstream desc;
    desc << SizeAndValuesString(numValuesBytes, numValues) << " | " << numInputs << " lookups";
    state.SetLabel(desc.str());
  }

  viskores::cont::ArrayHandle<ValueType> input;
  viskores::cont::ArrayHandle<viskores::Id> output;
  viskores::cont::ArrayHandle<ValueType> values;

  FillRandomTestValue(input, numInputs);
  FillRandomTestValue(values, numValues);
  viskores::cont::Algorithm::Sort(device, values);

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    viskores::cont::Algorithm::UpperBounds(device, input, values, output);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetItemsProcessed(static_cast<int64_t>(numInputs) * iterations);
};

VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchUpperBounds,
                                    ->RangeMultiplier(SmallRangeMultiplier)
                                    ->Ranges({ SmallRange, SmallRange })
                                    ->ArgNames({ "Size", "InputSize" }),
                                  SmallTypeList);

} // end anon namespace

int main(int argc, char* argv[])
{
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
