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
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/BitField.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/StorageBasic.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/internal/OptionParser.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/StableSortIndices.h>

#include <algorithm>
#include <cctype>
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

// This benchmark has a number of commandline options to customize its behavior.
// See The BenchDevAlgoConfig documentations for details.
// For the TBB implementation, the number of threads can be customized using a
// "NumThreads [numThreads]" argument.

namespace vtkm
{
namespace benchmarking
{

enum BenchmarkName
{
  BITFIELD_TO_UNORDERED_SET = 1 << 0,
  COPY = 1 << 1,
  COPY_IF = 1 << 2,
  LOWER_BOUNDS = 1 << 3,
  REDUCE = 1 << 4,
  REDUCE_BY_KEY = 1 << 5,
  SCAN_INCLUSIVE = 1 << 6,
  SCAN_EXCLUSIVE = 1 << 7,
  SORT = 1 << 8,
  SORT_BY_KEY = 1 << 9,
  STABLE_SORT_INDICES = 1 << 10,
  STABLE_SORT_INDICES_UNIQUE = 1 << 11,
  UNIQUE = 1 << 12,
  UPPER_BOUNDS = 1 << 13,

  ALL = BITFIELD_TO_UNORDERED_SET | COPY | COPY_IF | LOWER_BOUNDS | REDUCE | REDUCE_BY_KEY |
    SCAN_INCLUSIVE |
    SCAN_EXCLUSIVE |
    SORT |
    SORT_BY_KEY |
    STABLE_SORT_INDICES |
    STABLE_SORT_INDICES_UNIQUE |
    UNIQUE |
    UPPER_BOUNDS
};

/// Configuration options. Can be modified using via command line args as
/// described below:
struct BenchDevAlgoConfig
{
  /// Benchmarks to run. Possible values:
  /// Copy, CopyIf, LowerBounds, Reduce, ReduceByKey, ScanInclusive,
  /// ScanExclusive, Sort, SortByKey, StableSortIndices, StableSortIndicesUnique,
  /// Unique, UpperBounds, or All. (Default: All).
  // Zero is for parsing, will change to 'all' in main if needed.
  int BenchmarkFlags{ 0 };

  /// ValueTypes to test.
  /// CLI arg: "TypeList [Base|Extended]" (Base is default).
  bool ExtendedTypeList{ false };

  /// Run benchmarks using the same number of bytes for all arrays.
  /// CLI arg: "FixBytes [n|off]" (n is the number of bytes, default: 2097152, ie. 2MiB)
  /// @note FixBytes and FixSizes are not mutually exclusive. If both are
  /// specified, both will run.
  bool TestArraySizeBytes{ true };
  vtkm::UInt64 ArraySizeBytes{ 1 << 21 };

  /// Run benchmarks using the same number of values for all arrays.
  /// CLI arg: "FixSizes [n|off]" (n is the number of values, default: off)
  /// @note FixBytes and FixSizes are not mutually exclusive. If both are
  /// specified, both will run.
  bool TestArraySizeValues{ false };
  vtkm::UInt64 ArraySizeValues{ 1 << 21 };

  /// If true, operations like "Unique" will test with a wider range of unique
  /// values (5%, 10%, 15%, 20%, 25%, 30%, 35%, 40%, 45%, 50%, 75%, 100%
  /// unique). If false (default), the range is limited to 5%, 25%, 50%, 75%,
  /// 100%.
  /// CLI arg: "DetailedOutputRange" enables the extended range.
  bool DetailedOutputRangeScaling{ false };

  // Internal: The benchmarking code will set this depending on execution phase:
  bool DoByteSizes{ false };

  // Compute the number of values for an array with the given type:
  template <typename T>
  VTKM_CONT vtkm::Id ComputeSize()
  {
    return this->DoByteSizes
      ? static_cast<vtkm::Id>(this->ArraySizeBytes / static_cast<vtkm::UInt64>(sizeof(T)))
      : static_cast<vtkm::Id>(this->ArraySizeValues);
  }

  // Compute the number of words in a bit field with the given type.
  // If DoByteSizes is true, the specified buffer is rounded down to the nearest
  // number of words that fit into the byte limit. Otherwise, ArraySizeValues
  // is used to indicate the number of bits.
  template <typename WordType>
  VTKM_CONT vtkm::Id ComputeNumberOfWords()
  {
    static constexpr vtkm::UInt64 BytesPerWord = static_cast<vtkm::UInt64>(sizeof(WordType));
    static constexpr vtkm::UInt64 BitsPerWord = BytesPerWord * 8;

    return this->DoByteSizes ? static_cast<vtkm::Id>(this->ArraySizeBytes / BytesPerWord)
                             : static_cast<vtkm::Id>(this->ArraySizeValues / BitsPerWord);
  }
};

// Share a global instance of the config (only way to get it into the benchmark
// functors):
static BenchDevAlgoConfig Config = BenchDevAlgoConfig();

struct BaseTypes : vtkm::ListTagBase<vtkm::UInt8,
                                     vtkm::Int32,
                                     vtkm::Int64,
                                     vtkm::Pair<vtkm::Id, vtkm::Float32>,
                                     vtkm::Float32,
                                     vtkm::Vec<vtkm::Float32, 3>,
                                     vtkm::Float64,
                                     vtkm::Vec<vtkm::Float64, 3>>
{
};

struct ExtendedTypes : vtkm::ListTagBase<vtkm::UInt8,
                                         vtkm::Vec<vtkm::UInt8, 4>,
                                         vtkm::Int32,
                                         vtkm::Int64,
                                         vtkm::Pair<vtkm::Int32, vtkm::Float32>,
                                         vtkm::Pair<vtkm::Int32, vtkm::Float32>,
                                         vtkm::Pair<vtkm::Int64, vtkm::Float64>,
                                         vtkm::Pair<vtkm::Int64, vtkm::Float64>,
                                         vtkm::Float32,
                                         vtkm::Vec<vtkm::Float32, 3>,
                                         vtkm::Float64,
                                         vtkm::Vec<vtkm::Float64, 3>>
{
};

static const std::string DIVIDER(40, '-');

/// This class runs a series of micro-benchmarks to measure
/// performance of the parallel primitives provided by each
/// device adapter
class BenchmarkDeviceAdapter
{
  using StorageTag = vtkm::cont::StorageTagBasic;

  using IdArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id, StorageTag>;

  using Algorithm = vtkm::cont::Algorithm;

  using Timer = vtkm::cont::Timer;

public:
  // Various kernels used by the different benchmarks to accelerate
  // initialization of data
  template <typename Value, typename PortalType>
  struct FillTestValueKernel : vtkm::exec::FunctorBase
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    PortalType Output;

    VTKM_CONT
    FillTestValueKernel(PortalType out)
      : Output(out)
    {
    }

    VTKM_EXEC void operator()(vtkm::Id i) const { Output.Set(i, TestValue(i, Value())); }
  };

  template <typename Value, typename PortalType>
  struct FillScaledTestValueKernel : vtkm::exec::FunctorBase
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    PortalType Output;
    const vtkm::Id IdScale;

    VTKM_CONT
    FillScaledTestValueKernel(vtkm::Id id_scale, PortalType out)
      : Output(out)
      , IdScale(id_scale)
    {
    }

    VTKM_EXEC void operator()(vtkm::Id i) const { Output.Set(i, TestValue(i * IdScale, Value())); }
  };

  template <typename Value, typename PortalType>
  struct FillModuloTestValueKernel : vtkm::exec::FunctorBase
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    PortalType Output;
    const vtkm::Id Modulus;

    VTKM_CONT
    FillModuloTestValueKernel(vtkm::Id modulus, PortalType out)
      : Output(out)
      , Modulus(modulus)
    {
    }

    VTKM_EXEC void operator()(vtkm::Id i) const { Output.Set(i, TestValue(i % Modulus, Value())); }
  };

  template <typename Value, typename PortalType>
  struct FillBinaryTestValueKernel : vtkm::exec::FunctorBase
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    PortalType Output;
    const vtkm::Id Modulus;

    VTKM_CONT
    FillBinaryTestValueKernel(vtkm::Id modulus, PortalType out)
      : Output(out)
      , Modulus(modulus)
    {
    }

    VTKM_EXEC void operator()(vtkm::Id i) const
    {
      Output.Set(i, i % Modulus == 0 ? TestValue(vtkm::Id(1), Value()) : Value());
    }
  };

  template <typename WordType, typename BitFieldPortal>
  struct GenerateBitFieldFunctor : public vtkm::exec::FunctorBase
  {
    WordType Exemplar;
    vtkm::Id Stride;
    vtkm::Id MaxMaskedWord;
    BitFieldPortal Portal;

    VTKM_EXEC_CONT
    GenerateBitFieldFunctor(WordType exemplar,
                            vtkm::Id stride,
                            vtkm::Id maxMaskedWord,
                            const BitFieldPortal& portal)
      : Exemplar(exemplar)
      , Stride(stride)
      , MaxMaskedWord(maxMaskedWord)
      , Portal(portal)
    {
    }

    VTKM_EXEC
    void operator()(vtkm::Id wordIdx) const
    {
      if (wordIdx <= this->MaxMaskedWord && (wordIdx % this->Stride) == 0)
      {
        this->Portal.SetWord(wordIdx, this->Exemplar);
      }
      else
      {
        this->Portal.SetWord(wordIdx, static_cast<WordType>(0));
      }
    }
  };

  // Create a bit field for testing. The bit array will contain numWords words.
  // The exemplar word is used to set bits in the array. Stride indicates how
  // many words will be set to 0 between words initialized to the exemplar.
  // Words with indices higher than maxMaskedWord will be set to 0.
  // Stride and maxMaskedWord may be used to test different types of imbalanced
  // loads.
  template <typename WordType, typename DeviceAdapterTag>
  static VTKM_CONT vtkm::cont::BitField GenerateBitField(WordType exemplar,
                                                         vtkm::Id stride,
                                                         vtkm::Id maxMaskedWord,
                                                         vtkm::Id numWords)
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;

    if (stride == 0)
    {
      stride = 1;
    }

    vtkm::cont::BitField bits;
    auto portal = bits.PrepareForOutput(numWords, DeviceAdapterTag{});

    using Functor = GenerateBitFieldFunctor<WordType, decltype(portal)>;

    Algo::Schedule(Functor{ exemplar, stride, maxMaskedWord, portal }, numWords);
    Algo::Synchronize();

    return bits;
  }

private:
  template <typename WordType, typename DeviceAdapter>
  struct BenchBitFieldToUnorderedSet
  {
    using IndicesArray = vtkm::cont::ArrayHandle<vtkm::Id>;

    vtkm::Id NumWords;
    vtkm::Id NumBits;
    WordType Exemplar;
    vtkm::Id Stride;
    vtkm::Float32 FillRatio;
    vtkm::Id MaxMaskedIndex;
    std::string Name;

    vtkm::cont::BitField Bits;
    IndicesArray Indices;

    // See GenerateBitField for details. fillRatio is used to compute
    // maxMaskedWord.
    VTKM_CONT
    BenchBitFieldToUnorderedSet(WordType exemplar,
                                vtkm::Id stride,
                                vtkm::Float32 fillRatio,
                                const std::string& name)
      : NumWords(Config.ComputeNumberOfWords<WordType>())
      , NumBits(this->NumWords * static_cast<vtkm::Id>(sizeof(WordType) * CHAR_BIT))
      , Exemplar(exemplar)
      , Stride(stride)
      , FillRatio(fillRatio)
      , MaxMaskedIndex(this->NumWords / static_cast<vtkm::Id>(1. / this->FillRatio))
      , Name(name)
      , Bits(GenerateBitField<WordType, DeviceAdapter>(this->Exemplar,
                                                       this->Stride,
                                                       this->MaxMaskedIndex,
                                                       this->NumWords))
    {
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer(DeviceAdapter{});
      timer.Start();
      Algorithm::BitFieldToUnorderedSet(DeviceAdapter{}, this->Bits, this->Indices);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      const vtkm::Id numFilledWords = this->MaxMaskedIndex / this->Stride;
      const vtkm::Id numSetBits = numFilledWords * vtkm::CountSetBits(this->Exemplar);

      std::stringstream description;
      description << "BitFieldToUnorderedSet" << this->Name << " ( "
                  << "NumWords: " << this->NumWords << " "
                  << "Exemplar: " << std::hex << this->Exemplar << std::dec << " "
                  << "FillRatio: " << this->FillRatio << " "
                  << "Stride: " << this->Stride << " "
                  << "NumSetBits: " << numSetBits << " )";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(BitFieldToUnorderedSetNull,
                      BenchBitFieldToUnorderedSet,
                      0x00000000,
                      1,
                      0.f,
                      "Null");
  VTKM_MAKE_BENCHMARK(BitFieldToUnorderedSetFull,
                      BenchBitFieldToUnorderedSet,
                      0xffffffff,
                      1,
                      1.f,
                      "Full");
  VTKM_MAKE_BENCHMARK(BitFieldToUnorderedSetHalfWord,
                      BenchBitFieldToUnorderedSet,
                      0xffff0000,
                      1,
                      1.f,
                      "HalfWord");
  VTKM_MAKE_BENCHMARK(BitFieldToUnorderedSetHalfField,
                      BenchBitFieldToUnorderedSet,
                      0xffffffff,
                      1,
                      0.5f,
                      "HalfField");
  VTKM_MAKE_BENCHMARK(BitFieldToUnorderedSetAlternateWords,
                      BenchBitFieldToUnorderedSet,
                      0xffffffff,
                      2,
                      1.f,
                      "AlternateWords");
  VTKM_MAKE_BENCHMARK(BitFieldToUnorderedSetAlternateBits,
                      BenchBitFieldToUnorderedSet,
                      0x55555555,
                      1,
                      1.f,
                      "AlternateBits");

  template <typename Value, typename DeviceAdapter>
  struct BenchCopy
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    ValueArrayHandle ValueHandle_src;
    ValueArrayHandle ValueHandle_dst;
    std::mt19937 Rng;

    VTKM_CONT
    BenchCopy()
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      this->ValueHandle_src.Allocate(arraySize);
      auto portal = this->ValueHandle_src.GetPortalControl();
      for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
      {
        portal.Set(vtkm::Id(i), TestValue(vtkm::Id(Rng()), Value()));
      }
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      Algorithm::Copy(ValueHandle_src, ValueHandle_dst);

      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "Copy " << arraySize << " values ("
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Copy, BenchCopy);

  template <typename Value, typename DeviceAdapter>
  struct BenchCopyIf
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    const vtkm::Id PERCENT_VALID;
    const vtkm::Id N_VALID;
    ValueArrayHandle ValueHandle, OutHandle;
    IdArrayHandle StencilHandle;

    VTKM_CONT
    BenchCopyIf(vtkm::Id percent_valid)
      : PERCENT_VALID(percent_valid)
      , N_VALID((Config.ComputeSize<Value>() * percent_valid) / 100)
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      vtkm::Id modulo = arraySize / N_VALID;
      auto vHPortal = ValueHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillTestValueKernel<Value, decltype(vHPortal)>(vHPortal), arraySize);

      auto sHPortal = StencilHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillBinaryTestValueKernel<vtkm::Id, decltype(sHPortal)>(modulo, sHPortal),
                          arraySize);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      Algorithm::CopyIf(ValueHandle, StencilHandle, OutHandle);

      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "CopyIf on " << arraySize << " values ("
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ") with " << PERCENT_VALID << "% valid values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(CopyIf5, BenchCopyIf, 5);
  VTKM_MAKE_BENCHMARK(CopyIf10, BenchCopyIf, 10);
  VTKM_MAKE_BENCHMARK(CopyIf15, BenchCopyIf, 15);
  VTKM_MAKE_BENCHMARK(CopyIf20, BenchCopyIf, 20);
  VTKM_MAKE_BENCHMARK(CopyIf25, BenchCopyIf, 25);
  VTKM_MAKE_BENCHMARK(CopyIf30, BenchCopyIf, 30);
  VTKM_MAKE_BENCHMARK(CopyIf35, BenchCopyIf, 35);
  VTKM_MAKE_BENCHMARK(CopyIf40, BenchCopyIf, 40);
  VTKM_MAKE_BENCHMARK(CopyIf45, BenchCopyIf, 45);
  VTKM_MAKE_BENCHMARK(CopyIf50, BenchCopyIf, 50);
  VTKM_MAKE_BENCHMARK(CopyIf75, BenchCopyIf, 75);
  VTKM_MAKE_BENCHMARK(CopyIf100, BenchCopyIf, 100);

  template <typename Value, typename DeviceAdapter>
  struct BenchLowerBounds
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    const vtkm::Id N_VALS;
    const vtkm::Id PERCENT_VALUES;
    ValueArrayHandle InputHandle, ValueHandle;
    IdArrayHandle OutHandle;

    VTKM_CONT
    BenchLowerBounds(vtkm::Id value_percent)
      : N_VALS((Config.ComputeSize<Value>() * value_percent) / 100)
      , PERCENT_VALUES(value_percent)
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      auto iHPortal = InputHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillTestValueKernel<Value, decltype(iHPortal)>(iHPortal), arraySize);
      auto vHPortal = ValueHandle.PrepareForOutput(N_VALS, DeviceAdapter());
      Algorithm::Schedule(FillScaledTestValueKernel<Value, decltype(vHPortal)>(2, vHPortal),
                          N_VALS);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {

      Timer timer{ DeviceAdapter() };
      timer.Start();
      Algorithm::LowerBounds(InputHandle, ValueHandle, OutHandle);

      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "LowerBounds on " << arraySize << " input values ("
                  << "(" << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                             sizeof(Value))
                  << ") (" << PERCENT_VALUES << "% configuration)";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(LowerBounds5, BenchLowerBounds, 5);
  VTKM_MAKE_BENCHMARK(LowerBounds10, BenchLowerBounds, 10);
  VTKM_MAKE_BENCHMARK(LowerBounds15, BenchLowerBounds, 15);
  VTKM_MAKE_BENCHMARK(LowerBounds20, BenchLowerBounds, 20);
  VTKM_MAKE_BENCHMARK(LowerBounds25, BenchLowerBounds, 25);
  VTKM_MAKE_BENCHMARK(LowerBounds30, BenchLowerBounds, 30);
  VTKM_MAKE_BENCHMARK(LowerBounds35, BenchLowerBounds, 35);
  VTKM_MAKE_BENCHMARK(LowerBounds40, BenchLowerBounds, 40);
  VTKM_MAKE_BENCHMARK(LowerBounds45, BenchLowerBounds, 45);
  VTKM_MAKE_BENCHMARK(LowerBounds50, BenchLowerBounds, 50);
  VTKM_MAKE_BENCHMARK(LowerBounds75, BenchLowerBounds, 75);
  VTKM_MAKE_BENCHMARK(LowerBounds100, BenchLowerBounds, 100);

  template <typename Value, typename DeviceAdapter>
  struct BenchReduce
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    ValueArrayHandle InputHandle;
    // We don't actually use this, but we need it to prevent sufficiently
    // smart compilers from optimizing the Reduce call out.
    Value Result;

    VTKM_CONT
    BenchReduce()
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      auto iHPortal = this->InputHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillTestValueKernel<Value, decltype(iHPortal)>(iHPortal), arraySize);
      this->Result =
        Algorithm::Reduce(this->InputHandle, vtkm::TypeTraits<Value>::ZeroInitialization());
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {

      Timer timer{ DeviceAdapter() };
      timer.Start();
      Value tmp =
        Algorithm::Reduce(this->InputHandle, vtkm::TypeTraits<Value>::ZeroInitialization());
      vtkm::Float64 time = timer.GetElapsedTime();
      if (tmp != this->Result)
      {
        this->Result = tmp;
      }
      return time;
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "Reduce on " << arraySize << " values ("
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Reduce, BenchReduce);

  template <typename Value, typename DeviceAdapter>
  struct BenchReduceByKey
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    const vtkm::Id N_KEYS;
    const vtkm::Id PERCENT_KEYS;
    ValueArrayHandle ValueHandle, ValuesOut;
    IdArrayHandle KeyHandle, KeysOut;

    VTKM_CONT
    BenchReduceByKey(vtkm::Id key_percent)
      : N_KEYS((Config.ComputeSize<Value>() * key_percent) / 100)
      , PERCENT_KEYS(key_percent)
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      auto vHPortal = ValueHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillTestValueKernel<Value, decltype(vHPortal)>(vHPortal), arraySize);
      auto kHPortal = KeyHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillModuloTestValueKernel<vtkm::Id, decltype(kHPortal)>(N_KEYS, kHPortal),
                          arraySize);
      Algorithm::SortByKey(KeyHandle, ValueHandle);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      Algorithm::ReduceByKey(KeyHandle, ValueHandle, KeysOut, ValuesOut, vtkm::Add());
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "ReduceByKey on " << arraySize << " values ("
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ") with " << N_KEYS << " (" << PERCENT_KEYS << "%) distinct vtkm::Id keys";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ReduceByKey5, BenchReduceByKey, 5);
  VTKM_MAKE_BENCHMARK(ReduceByKey10, BenchReduceByKey, 10);
  VTKM_MAKE_BENCHMARK(ReduceByKey15, BenchReduceByKey, 15);
  VTKM_MAKE_BENCHMARK(ReduceByKey20, BenchReduceByKey, 20);
  VTKM_MAKE_BENCHMARK(ReduceByKey25, BenchReduceByKey, 25);
  VTKM_MAKE_BENCHMARK(ReduceByKey30, BenchReduceByKey, 30);
  VTKM_MAKE_BENCHMARK(ReduceByKey35, BenchReduceByKey, 35);
  VTKM_MAKE_BENCHMARK(ReduceByKey40, BenchReduceByKey, 40);
  VTKM_MAKE_BENCHMARK(ReduceByKey45, BenchReduceByKey, 45);
  VTKM_MAKE_BENCHMARK(ReduceByKey50, BenchReduceByKey, 50);
  VTKM_MAKE_BENCHMARK(ReduceByKey75, BenchReduceByKey, 75);
  VTKM_MAKE_BENCHMARK(ReduceByKey100, BenchReduceByKey, 100);

  template <typename Value, typename DeviceAdapter>
  struct BenchScanInclusive
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;
    ValueArrayHandle ValueHandle, OutHandle;

    VTKM_CONT
    BenchScanInclusive()
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      auto vHPortal = ValueHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillTestValueKernel<Value, decltype(vHPortal)>(vHPortal), arraySize);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      Algorithm::ScanInclusive(ValueHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "ScanInclusive on " << arraySize << " values ("
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ScanInclusive, BenchScanInclusive);

  template <typename Value, typename DeviceAdapter>
  struct BenchScanExclusive
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    ValueArrayHandle ValueHandle, OutHandle;

    VTKM_CONT
    BenchScanExclusive()
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      auto vHPortal = ValueHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillTestValueKernel<Value, decltype(vHPortal)>(vHPortal), arraySize);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {

      Timer timer{ DeviceAdapter() };
      timer.Start();
      Algorithm::ScanExclusive(ValueHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "ScanExclusive on " << arraySize << " values ("
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ScanExclusive, BenchScanExclusive);

  template <typename Value, typename DeviceAdapter>
  struct BenchSort
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    ValueArrayHandle ValueHandle;
    std::mt19937 Rng;

    VTKM_CONT
    BenchSort()
    {
      this->ValueHandle.Allocate(Config.ComputeSize<Value>());
      auto portal = this->ValueHandle.GetPortalControl();
      for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
      {
        portal.Set(vtkm::Id(i), TestValue(vtkm::Id(Rng()), Value()));
      }
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      ValueArrayHandle array;
      Algorithm::Copy(this->ValueHandle, array);

      Timer timer{ DeviceAdapter() };
      timer.Start();
      Algorithm::Sort(array);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "Sort on " << arraySize << " random values ("
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Sort, BenchSort);

  template <typename Value, typename DeviceAdapter>
  struct BenchSortByKey
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    std::mt19937 Rng;
    vtkm::Id N_KEYS;
    vtkm::Id PERCENT_KEYS;
    ValueArrayHandle ValueHandle;
    IdArrayHandle KeyHandle;

    VTKM_CONT
    BenchSortByKey(vtkm::Id percent_key)
      : N_KEYS((Config.ComputeSize<Value>() * percent_key) / 100)
      , PERCENT_KEYS(percent_key)
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      this->ValueHandle.Allocate(arraySize);
      auto portal = this->ValueHandle.GetPortalControl();
      for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
      {
        portal.Set(vtkm::Id(i), TestValue(vtkm::Id(Rng()), Value()));
      }
      auto kHPortal = KeyHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillModuloTestValueKernel<vtkm::Id, decltype(kHPortal)>(N_KEYS, kHPortal),
                          arraySize);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      IdArrayHandle keys;
      ValueArrayHandle values;
      Algorithm::Copy(this->KeyHandle, keys);
      Algorithm::Copy(this->ValueHandle, values);

      Timer timer{ DeviceAdapter() };
      timer.Start();
      Algorithm::SortByKey(keys, values);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "SortByKey on " << arraySize << " random values ("
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ") with " << N_KEYS << " (" << PERCENT_KEYS << "%) different vtkm::Id keys";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(SortByKey5, BenchSortByKey, 5);
  VTKM_MAKE_BENCHMARK(SortByKey10, BenchSortByKey, 10);
  VTKM_MAKE_BENCHMARK(SortByKey15, BenchSortByKey, 15);
  VTKM_MAKE_BENCHMARK(SortByKey20, BenchSortByKey, 20);
  VTKM_MAKE_BENCHMARK(SortByKey25, BenchSortByKey, 25);
  VTKM_MAKE_BENCHMARK(SortByKey30, BenchSortByKey, 30);
  VTKM_MAKE_BENCHMARK(SortByKey35, BenchSortByKey, 35);
  VTKM_MAKE_BENCHMARK(SortByKey40, BenchSortByKey, 40);
  VTKM_MAKE_BENCHMARK(SortByKey45, BenchSortByKey, 45);
  VTKM_MAKE_BENCHMARK(SortByKey50, BenchSortByKey, 50);
  VTKM_MAKE_BENCHMARK(SortByKey75, BenchSortByKey, 75);
  VTKM_MAKE_BENCHMARK(SortByKey100, BenchSortByKey, 100);

  template <typename Value, typename DeviceAdapter>
  struct BenchStableSortIndices
  {
    using SSI = vtkm::worklet::StableSortIndices;
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    ValueArrayHandle ValueHandle;
    std::mt19937 Rng;

    VTKM_CONT
    BenchStableSortIndices()
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      this->ValueHandle.Allocate(arraySize);
      auto portal = this->ValueHandle.GetPortalControl();
      for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
      {
        portal.Set(vtkm::Id(i), TestValue(vtkm::Id(Rng()), Value()));
      }
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      vtkm::cont::ArrayHandle<vtkm::Id> indices;
      Algorithm::Copy(vtkm::cont::ArrayHandleIndex(arraySize), indices);

      Timer timer{ DeviceAdapter() };
      timer.Start();
      SSI::Sort(ValueHandle, indices);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "StableSortIndices::Sort on " << arraySize << " random values ("
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(StableSortIndices, BenchStableSortIndices);

  template <typename Value, typename DeviceAdapter>
  struct BenchStableSortIndicesUnique
  {
    using SSI = vtkm::worklet::StableSortIndices;
    using IndexArrayHandle = typename SSI::IndexArrayType;
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    const vtkm::Id N_VALID;
    const vtkm::Id PERCENT_VALID;
    ValueArrayHandle ValueHandle;
    IndexArrayHandle IndexHandle;

    VTKM_CONT
    BenchStableSortIndicesUnique(vtkm::Id percent_valid)
      : N_VALID((Config.ComputeSize<Value>() * percent_valid) / 100)
      , PERCENT_VALID(percent_valid)
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      auto vHPortal = this->ValueHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillModuloTestValueKernel<Value, decltype(vHPortal)>(N_VALID, vHPortal),
                          arraySize);
      this->IndexHandle = SSI::Sort(this->ValueHandle);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {

      IndexArrayHandle indices;
      Algorithm::Copy(this->IndexHandle, indices);
      Timer timer{ DeviceAdapter() };
      timer.Start();
      SSI::Unique(this->ValueHandle, indices);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "StableSortIndices::Unique on " << arraySize << " values ("
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ") with " << this->N_VALID << " (" << PERCENT_VALID << "%) valid values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique5, BenchStableSortIndicesUnique, 5);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique10, BenchStableSortIndicesUnique, 10);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique15, BenchStableSortIndicesUnique, 15);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique20, BenchStableSortIndicesUnique, 20);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique25, BenchStableSortIndicesUnique, 25);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique30, BenchStableSortIndicesUnique, 30);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique35, BenchStableSortIndicesUnique, 35);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique40, BenchStableSortIndicesUnique, 40);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique45, BenchStableSortIndicesUnique, 45);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique50, BenchStableSortIndicesUnique, 50);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique75, BenchStableSortIndicesUnique, 75);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique100, BenchStableSortIndicesUnique, 100);

  template <typename Value, typename DeviceAdapter>
  struct BenchUnique
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    const vtkm::Id N_VALID;
    const vtkm::Id PERCENT_VALID;
    ValueArrayHandle ValueHandle;

    VTKM_CONT
    BenchUnique(vtkm::Id percent_valid)
      : N_VALID((Config.ComputeSize<Value>() * percent_valid) / 100)
      , PERCENT_VALID(percent_valid)
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      auto vHPortal = ValueHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillModuloTestValueKernel<Value, decltype(vHPortal)>(N_VALID, vHPortal),
                          arraySize);
      Algorithm::Sort(ValueHandle);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {

      ValueArrayHandle array;
      Algorithm::Copy(this->ValueHandle, array);

      Timer timer{ DeviceAdapter() };
      timer.Start();
      Algorithm::Unique(array);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "Unique on " << arraySize << " values ("
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ") with " << N_VALID << " (" << PERCENT_VALID << "%) valid values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Unique5, BenchUnique, 5);
  VTKM_MAKE_BENCHMARK(Unique10, BenchUnique, 10);
  VTKM_MAKE_BENCHMARK(Unique15, BenchUnique, 15);
  VTKM_MAKE_BENCHMARK(Unique20, BenchUnique, 20);
  VTKM_MAKE_BENCHMARK(Unique25, BenchUnique, 25);
  VTKM_MAKE_BENCHMARK(Unique30, BenchUnique, 30);
  VTKM_MAKE_BENCHMARK(Unique35, BenchUnique, 35);
  VTKM_MAKE_BENCHMARK(Unique40, BenchUnique, 40);
  VTKM_MAKE_BENCHMARK(Unique45, BenchUnique, 45);
  VTKM_MAKE_BENCHMARK(Unique50, BenchUnique, 50);
  VTKM_MAKE_BENCHMARK(Unique75, BenchUnique, 75);
  VTKM_MAKE_BENCHMARK(Unique100, BenchUnique, 100);

  template <typename Value, typename DeviceAdapter>
  struct BenchUpperBounds
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    const vtkm::Id N_VALS;
    const vtkm::Id PERCENT_VALS;
    ValueArrayHandle InputHandle, ValueHandle;
    IdArrayHandle OutHandle;

    VTKM_CONT
    BenchUpperBounds(vtkm::Id percent_vals)
      : N_VALS((Config.ComputeSize<Value>() * percent_vals) / 100)
      , PERCENT_VALS(percent_vals)
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      auto iHPortal = InputHandle.PrepareForOutput(arraySize, DeviceAdapter());
      Algorithm::Schedule(FillTestValueKernel<Value, decltype(iHPortal)>(iHPortal), arraySize);
      auto vHPortal = ValueHandle.PrepareForOutput(N_VALS, DeviceAdapter());
      Algorithm::Schedule(FillScaledTestValueKernel<Value, decltype(vHPortal)>(2, vHPortal),
                          N_VALS);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::Timer timer{ DeviceAdapter() };
      timer.Start();
      Algorithm::UpperBounds(InputHandle, ValueHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "UpperBounds on " << arraySize << " input and " << N_VALS << " ("
                  << PERCENT_VALS << "%) values (input array size: "
                  << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(arraySize) *
                                                      sizeof(Value))
                  << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(UpperBounds5, BenchUpperBounds, 5);
  VTKM_MAKE_BENCHMARK(UpperBounds10, BenchUpperBounds, 10);
  VTKM_MAKE_BENCHMARK(UpperBounds15, BenchUpperBounds, 15);
  VTKM_MAKE_BENCHMARK(UpperBounds20, BenchUpperBounds, 20);
  VTKM_MAKE_BENCHMARK(UpperBounds25, BenchUpperBounds, 25);
  VTKM_MAKE_BENCHMARK(UpperBounds30, BenchUpperBounds, 30);
  VTKM_MAKE_BENCHMARK(UpperBounds35, BenchUpperBounds, 35);
  VTKM_MAKE_BENCHMARK(UpperBounds40, BenchUpperBounds, 40);
  VTKM_MAKE_BENCHMARK(UpperBounds45, BenchUpperBounds, 45);
  VTKM_MAKE_BENCHMARK(UpperBounds50, BenchUpperBounds, 50);
  VTKM_MAKE_BENCHMARK(UpperBounds75, BenchUpperBounds, 75);
  VTKM_MAKE_BENCHMARK(UpperBounds100, BenchUpperBounds, 100);

public:
  static VTKM_CONT int Run(vtkm::cont::DeviceAdapterId id)
  {
    // Run fixed bytes / size tests:
    for (int sizeType = 0; sizeType < 2; ++sizeType)
    {
      if (sizeType == 0 && Config.TestArraySizeBytes)
      {
        std::cout << DIVIDER << "\nTesting fixed array byte sizes\n";
        Config.DoByteSizes = true;
        if (!Config.ExtendedTypeList)
        {
          RunInternal<BaseTypes>(id);
        }
        else
        {
          RunInternal<ExtendedTypes>(id);
        }
      }
      if (sizeType == 1 && Config.TestArraySizeValues)
      {
        std::cout << DIVIDER << "\nTesting fixed array element counts\n";
        Config.DoByteSizes = false;
        if (!Config.ExtendedTypeList)
        {
          RunInternal<BaseTypes>(id);
        }
        else
        {
          RunInternal<ExtendedTypes>(id);
        }
      }
    }

    return 0;
  }

  template <typename ValueTypes>
  static VTKM_CONT void RunInternal(vtkm::cont::DeviceAdapterId id)
  {
    using BitFieldWordTypes = vtkm::ListTagBase<vtkm::UInt32>;

    if (Config.BenchmarkFlags & BITFIELD_TO_UNORDERED_SET)
    {
      std::cout << DIVIDER << "\nBenchmarking BitFieldToUnorderedSet\n";
      VTKM_RUN_BENCHMARK(BitFieldToUnorderedSetNull, BitFieldWordTypes{}, id);
      VTKM_RUN_BENCHMARK(BitFieldToUnorderedSetFull, BitFieldWordTypes{}, id);
      VTKM_RUN_BENCHMARK(BitFieldToUnorderedSetHalfWord, BitFieldWordTypes{}, id);
      VTKM_RUN_BENCHMARK(BitFieldToUnorderedSetHalfField, BitFieldWordTypes{}, id);
      VTKM_RUN_BENCHMARK(BitFieldToUnorderedSetAlternateWords, BitFieldWordTypes{}, id);
      VTKM_RUN_BENCHMARK(BitFieldToUnorderedSetAlternateBits, BitFieldWordTypes{}, id);
    }

    if (Config.BenchmarkFlags & COPY)
    {
      std::cout << DIVIDER << "\nBenchmarking Copy\n";
      VTKM_RUN_BENCHMARK(Copy, ValueTypes(), id);
    }

    if (Config.BenchmarkFlags & COPY_IF)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking CopyIf\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(CopyIf5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf10, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf15, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf20, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf30, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf35, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf40, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf45, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf100, ValueTypes(), id);
      }
      else
      {
        VTKM_RUN_BENCHMARK(CopyIf5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(CopyIf100, ValueTypes(), id);
      }
    }

    if (Config.BenchmarkFlags & LOWER_BOUNDS)
    {
      std::cout << DIVIDER << "\nBenchmarking LowerBounds\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(LowerBounds5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds10, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds15, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds20, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds30, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds35, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds40, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds45, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds100, ValueTypes(), id);
      }
      else
      {
        VTKM_RUN_BENCHMARK(LowerBounds5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(LowerBounds100, ValueTypes(), id);
      }
    }

    if (Config.BenchmarkFlags & REDUCE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking Reduce\n";
      VTKM_RUN_BENCHMARK(Reduce, ValueTypes(), id);
    }

    if (Config.BenchmarkFlags & REDUCE_BY_KEY)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking ReduceByKey\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(ReduceByKey5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey10, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey15, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey20, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey30, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey35, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey40, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey45, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey100, ValueTypes(), id);
      }
      else
      {
        VTKM_RUN_BENCHMARK(ReduceByKey5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(ReduceByKey100, ValueTypes(), id);
      }
    }

    if (Config.BenchmarkFlags & SCAN_INCLUSIVE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking ScanInclusive\n";
      VTKM_RUN_BENCHMARK(ScanInclusive, ValueTypes(), id);
    }

    if (Config.BenchmarkFlags & SCAN_EXCLUSIVE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking ScanExclusive\n";
      VTKM_RUN_BENCHMARK(ScanExclusive, ValueTypes(), id);
    }

    if (Config.BenchmarkFlags & SORT)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking Sort\n";
      VTKM_RUN_BENCHMARK(Sort, ValueTypes(), id);
    }

    if (Config.BenchmarkFlags & SORT_BY_KEY)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking SortByKey\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(SortByKey5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey10, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey15, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey20, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey30, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey35, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey40, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey45, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey100, ValueTypes(), id);
      }
      else
      {
        VTKM_RUN_BENCHMARK(SortByKey5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(SortByKey100, ValueTypes(), id);
      }
    }

    if (Config.BenchmarkFlags & STABLE_SORT_INDICES)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking StableSortIndices::Sort\n";
      VTKM_RUN_BENCHMARK(StableSortIndices, ValueTypes(), id);
    }

    if (Config.BenchmarkFlags & STABLE_SORT_INDICES_UNIQUE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking StableSortIndices::Unique\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique10, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique15, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique20, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique30, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique35, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique40, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique45, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique100, ValueTypes(), id);
      }
      else
      {
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique100, ValueTypes(), id);
      }
    }

    if (Config.BenchmarkFlags & UNIQUE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking Unique\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(Unique5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique10, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique15, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique20, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique30, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique35, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique40, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique45, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique100, ValueTypes(), id);
      }
      else
      {
        VTKM_RUN_BENCHMARK(Unique5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(Unique100, ValueTypes(), id);
      }
    }

    if (Config.BenchmarkFlags & UPPER_BOUNDS)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking UpperBounds\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(UpperBounds5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds10, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds15, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds20, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds30, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds35, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds40, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds45, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds100, ValueTypes(), id);
      }
      else
      {
        VTKM_RUN_BENCHMARK(UpperBounds5, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds25, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds50, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds75, ValueTypes(), id);
        VTKM_RUN_BENCHMARK(UpperBounds100, ValueTypes(), id);
      }
    }
  }
};

#undef ARRAY_SIZE

struct Arg : vtkm::cont::internal::option::Arg
{
  static vtkm::cont::internal::option::ArgStatus Number(
    const vtkm::cont::internal::option::Option& option,
    bool msg)
  {
    bool argIsNum = ((option.arg != nullptr) && (option.arg[0] != '\0'));
    const char* c = option.arg;
    while (argIsNum && (*c != '\0'))
    {
      argIsNum &= static_cast<bool>(std::isdigit(*c));
      ++c;
    }

    if (argIsNum)
    {
      return vtkm::cont::internal::option::ARG_OK;
    }
    else
    {
      if (msg)
      {
        std::cerr << "Option " << option.name << " requires a numeric argument." << std::endl;
      }

      return vtkm::cont::internal::option::ARG_ILLEGAL;
    }
  }
};
}
} // namespace vtkm::benchmarking

enum optionIndex
{
  UNKNOWN,
  HELP,
  NUM_THREADS,
  TYPELIST,
  ARRAY_SIZE,
  MORE_OUTPUT_RANGE
};

enum typelistType
{
  BASE,
  EXTENED
};

enum arraySizeType
{
  BYTES,
  VALUES
};

int main(int argc, char* argv[])
{
  auto initConfig = vtkm::cont::Initialize(argc, argv, vtkm::cont::InitializeOptions::None);

  if (initConfig.Device == vtkm::cont::DeviceAdapterTagUndefined())
  {
    initConfig.Device = vtkm::cont::DeviceAdapterTagAny();
  }

  namespace option = vtkm::cont::internal::option;
  using Arg = vtkm::benchmarking::Arg;

  std::vector<option::Descriptor> usage;
  std::string usageHeader{ "Usage: " };
  usageHeader.append(argv[0]);
  usageHeader.append(" [options] [benchmarks]");
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, usageHeader.c_str() });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, "Options are:" });
  usage.push_back({ HELP, 0, "h", "help", Arg::None, "  -h, --help\tDisplay this help." });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, initConfig.Usage.c_str() });
  usage.push_back({ NUM_THREADS,
                    0,
                    "",
                    "num-threads",
                    Arg::Number,
                    "  --num-threads <N> \tSpecify the number of threads to use." });
  usage.push_back({ TYPELIST,
                    BASE,
                    "",
                    "base-typelist",
                    Arg::None,
                    "  --base-typelist \tBenchmark using the base set of types. (default)" });
  usage.push_back({ TYPELIST,
                    EXTENED,
                    "",
                    "extended-typelist",
                    Arg::None,
                    "  --extended-tyupelist \tBenchmark using an extended set of types." });
  usage.push_back({ ARRAY_SIZE,
                    BYTES,
                    "",
                    "array-size-bytes",
                    Arg::Number,
                    "  --array-size-bytes <N> \tRun the benchmarks with arrays of the given "
                    "number of bytes. (Default is 2097152 (i.e. 2MB)" });
  usage.push_back({ ARRAY_SIZE,
                    VALUES,
                    "",
                    "array-size-values",
                    Arg::Number,
                    "  --array-size-values <N> \tRun the benchmarks with arrays of the given "
                    "number of values." });
  usage.push_back({ MORE_OUTPUT_RANGE,
                    0,
                    "",
                    "more-output-range",
                    Arg::None,
                    "  --more-output-range \tIf specified, operations like Unique will test with "
                    "a wider range of unique values (5%, 10%, 15%, 20%, 25%, 30%, 35%, 40%, 45%, "
                    "50%, 75%, 100% unique). By default, the range is limited to 5%, 25%, 50%, "
                    "75%, 100%." });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, "Benchmarks are one or more of:" });
  usage.push_back({ UNKNOWN,
                    0,
                    "",
                    "",
                    Arg::None,
                    "\tCopy, CopyIf, LowerBounds, Reduce, ReduceByKey, ScanExclusive, "
                    "ScanInclusive, Sort, SortByKey, StableSortIndices, StableSortIndicesUnique, "
                    "Unique, UpperBounds" });
  usage.push_back(
    { UNKNOWN, 0, "", "", Arg::None, "If no benchmarks are listed, all will be run." });
  usage.push_back({ 0, 0, nullptr, nullptr, nullptr, nullptr });

  vtkm::cont::internal::option::Stats stats(usage.data(), argc - 1, argv + 1);
  std::unique_ptr<option::Option[]> options{ new option::Option[stats.options_max] };
  std::unique_ptr<option::Option[]> buffer{ new option::Option[stats.buffer_max] };
  option::Parser commandLineParse(usage.data(), argc - 1, argv + 1, options.get(), buffer.get());

  if (options[UNKNOWN])
  {
    std::cerr << "Unknown option: " << options[UNKNOWN].name << std::endl;
    option::printUsage(std::cerr, usage.data());
    exit(1);
  }

  if (options[HELP])
  {
    option::printUsage(std::cerr, usage.data());
    exit(0);
  }

  vtkm::benchmarking::BenchDevAlgoConfig& config = vtkm::benchmarking::Config;

  int numThreads{ 0 };
  if (options[NUM_THREADS])
  {
    std::istringstream parse(options[NUM_THREADS].arg);
    parse >> numThreads;
    if (initConfig.Device == vtkm::cont::DeviceAdapterTagTBB() ||
        initConfig.Device == vtkm::cont::DeviceAdapterTagOpenMP())
    {
      std::cout << "Selected " << numThreads << " " << initConfig.Device.GetName() << " threads."
                << std::endl;
    }
    else
    {
      std::cerr << options[NUM_THREADS].name << " not valid on this device. Ignoring." << std::endl;
    }
  }

  if (options[TYPELIST])
  {
    switch (options[TYPELIST].last()->type())
    {
      case BASE:
        config.ExtendedTypeList = false;
        break;
      case EXTENED:
        config.ExtendedTypeList = true;
        break;
      default:
        std::cerr << "Internal error. Unknown typelist." << std::endl;
        break;
    }
  }

  if (options[ARRAY_SIZE])
  {
    config.TestArraySizeBytes = false;
    config.TestArraySizeValues = false;
    for (const option::Option* opt = options[ARRAY_SIZE]; opt; opt = opt->next())
    {
      std::istringstream parse(opt->arg);
      switch (opt->type())
      {
        case BYTES:
          config.TestArraySizeBytes = true;
          parse >> config.ArraySizeBytes;
          break;
        case VALUES:
          config.TestArraySizeValues = true;
          parse >> config.ArraySizeValues;
          break;
        default:
          std::cerr << "Internal error. Unknown array size type." << std::endl;
          break;
      }
    }
  }

  if (options[MORE_OUTPUT_RANGE])
  {
    config.DetailedOutputRangeScaling = true;
  }

  for (int i = 0; i < commandLineParse.nonOptionsCount(); ++i)
  {
    std::string arg = commandLineParse.nonOption(i);
    std::transform(arg.begin(), arg.end(), arg.begin(), [](char c) {
      return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
    if (arg == "bitfieldtounorderedset")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::BITFIELD_TO_UNORDERED_SET;
    }
    else if (arg == "copy")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::COPY;
    }
    else if (arg == "copyif")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::COPY_IF;
    }
    else if (arg == "lowerbounds")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::LOWER_BOUNDS;
    }
    else if (arg == "reduce")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::REDUCE;
    }
    else if (arg == "reducebykey")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::REDUCE_BY_KEY;
    }
    else if (arg == "scaninclusive")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::SCAN_INCLUSIVE;
    }
    else if (arg == "scanexclusive")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::SCAN_EXCLUSIVE;
    }
    else if (arg == "sort")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::SORT;
    }
    else if (arg == "sortbykey")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::SORT_BY_KEY;
    }
    else if (arg == "stablesortindices")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::STABLE_SORT_INDICES;
    }
    else if (arg == "stablesortindicesunique")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::STABLE_SORT_INDICES_UNIQUE;
    }
    else if (arg == "unique")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::UNIQUE;
    }
    else if (arg == "upperbounds")
    {
      config.BenchmarkFlags |= vtkm::benchmarking::UPPER_BOUNDS;
    }
    else
    {
      std::cerr << "Unrecognized benchmark: " << arg << std::endl;
      option::printUsage(std::cerr, usage.data());
      return 1;
    }
  }

#ifdef VTKM_ENABLE_TBB
  // Must not be destroyed as long as benchmarks are running:
  tbb::task_scheduler_init init((numThreads > 0) ? numThreads
                                                 : tbb::task_scheduler_init::automatic);
#endif
#ifdef VTKM_ENABLE_OPENMP
  omp_set_num_threads((numThreads > 0) ? numThreads : omp_get_max_threads());
#endif

  if (config.BenchmarkFlags == 0)
  {
    config.BenchmarkFlags = vtkm::benchmarking::ALL;
  }

  //now actually execute the benchmarks
  return vtkm::benchmarking::BenchmarkDeviceAdapter::Run(initConfig.Device);
}
