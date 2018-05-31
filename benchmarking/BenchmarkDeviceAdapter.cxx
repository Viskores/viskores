//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include "Benchmarker.h"
#include <vtkm/TypeTraits.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/StorageBasic.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/internal/DeviceAdapterError.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/StableSortIndices.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <random>
#include <string>
#include <utility>

#include <vtkm/internal/Windows.h>

#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_TBB
#include <tbb/task_scheduler_init.h>
#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_OPENMP
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
  COPY = 1,
  COPY_IF = 1 << 1,
  LOWER_BOUNDS = 1 << 2,
  REDUCE = 1 << 3,
  REDUCE_BY_KEY = 1 << 4,
  SCAN_INCLUSIVE = 1 << 5,
  SCAN_EXCLUSIVE = 1 << 6,
  SORT = 1 << 7,
  SORT_BY_KEY = 1 << 8,
  STABLE_SORT_INDICES = 1 << 9,
  STABLE_SORT_INDICES_UNIQUE = 1 << 10,
  UNIQUE = 1 << 11,
  UPPER_BOUNDS = 1 << 12,
  ALL = COPY | COPY_IF | LOWER_BOUNDS | REDUCE | REDUCE_BY_KEY | SCAN_INCLUSIVE | SCAN_EXCLUSIVE |
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
template <class DeviceAdapterTag>
class BenchmarkDeviceAdapter
{
  using StorageTag = vtkm::cont::StorageTagBasic;

  using IdArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id, StorageTag>;

  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;

  using Timer = vtkm::cont::Timer<DeviceAdapterTag>;

public:
  // Various kernels used by the different benchmarks to accelerate
  // initialization of data
  template <typename Value>
  struct FillTestValueKernel : vtkm::exec::FunctorBase
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;
    using PortalType = typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>::Portal;

    PortalType Output;

    VTKM_CONT
    FillTestValueKernel(PortalType out)
      : Output(out)
    {
    }

    VTKM_EXEC void operator()(vtkm::Id i) const { Output.Set(i, TestValue(i, Value())); }
  };

  template <typename Value>
  struct FillScaledTestValueKernel : vtkm::exec::FunctorBase
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;
    using PortalType = typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>::Portal;

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

  template <typename Value>
  struct FillModuloTestValueKernel : vtkm::exec::FunctorBase
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;
    using PortalType = typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>::Portal;

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

  template <typename Value>
  struct FillBinaryTestValueKernel : vtkm::exec::FunctorBase
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;
    using PortalType = typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>::Portal;

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

private:
  template <typename Value>
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
      Timer timer;
      Algorithm::Copy(ValueHandle_src, ValueHandle_dst);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "Copy " << arraySize << " values ("
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Copy, BenchCopy);

  template <typename Value>
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
      Algorithm::Schedule(
        FillTestValueKernel<Value>(ValueHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
        arraySize);
      Algorithm::Schedule(FillBinaryTestValueKernel<vtkm::Id>(
                            modulo, StencilHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
                          arraySize);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer;
      Algorithm::CopyIf(ValueHandle, StencilHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "CopyIf on " << arraySize << " values ("
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ") with "
                  << PERCENT_VALID << "% valid values";
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

  template <typename Value>
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
      Algorithm::Schedule(
        FillTestValueKernel<Value>(InputHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
        arraySize);
      Algorithm::Schedule(FillScaledTestValueKernel<Value>(
                            2, ValueHandle.PrepareForOutput(N_VALS, DeviceAdapterTag())),
                          N_VALS);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer;
      Algorithm::LowerBounds(InputHandle, ValueHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "LowerBounds on " << arraySize << " input values ("
                  << "(" << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ") ("
                  << PERCENT_VALUES << "% configuration)";
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

  template <typename Value>
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
      Algorithm::Schedule(
        FillTestValueKernel<Value>(InputHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
        arraySize);
      this->Result =
        Algorithm::Reduce(this->InputHandle, vtkm::TypeTraits<Value>::ZeroInitialization());
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer;
      Value tmp = Algorithm::Reduce(InputHandle, vtkm::TypeTraits<Value>::ZeroInitialization());
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
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Reduce, BenchReduce);

  template <typename Value>
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
      Algorithm::Schedule(
        FillTestValueKernel<Value>(ValueHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
        arraySize);
      Algorithm::Schedule(FillModuloTestValueKernel<vtkm::Id>(
                            N_KEYS, KeyHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
                          arraySize);
      Algorithm::SortByKey(KeyHandle, ValueHandle);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer;
      Algorithm::ReduceByKey(KeyHandle, ValueHandle, KeysOut, ValuesOut, vtkm::Add());
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "ReduceByKey on " << arraySize << " values ("
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ") with "
                  << N_KEYS << " (" << PERCENT_KEYS << "%) distinct vtkm::Id keys";
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

  template <typename Value>
  struct BenchScanInclusive
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;
    ValueArrayHandle ValueHandle, OutHandle;

    VTKM_CONT
    BenchScanInclusive()
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      Algorithm::Schedule(
        FillTestValueKernel<Value>(ValueHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
        arraySize);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer;
      Algorithm::ScanInclusive(ValueHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "ScanInclusive on " << arraySize << " values ("
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ScanInclusive, BenchScanInclusive);

  template <typename Value>
  struct BenchScanExclusive
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    ValueArrayHandle ValueHandle, OutHandle;

    VTKM_CONT
    BenchScanExclusive()
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      Algorithm::Schedule(
        FillTestValueKernel<Value>(ValueHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
        arraySize);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer;
      Algorithm::ScanExclusive(ValueHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "ScanExclusive on " << arraySize << " values ("
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ScanExclusive, BenchScanExclusive);

  template <typename Value>
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

      Timer timer;
      Algorithm::Sort(array);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "Sort on " << arraySize << " random values ("
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Sort, BenchSort);

  template <typename Value>
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
      Algorithm::Schedule(FillModuloTestValueKernel<vtkm::Id>(
                            N_KEYS, KeyHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
                          arraySize);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      IdArrayHandle keys;
      ValueArrayHandle values;
      Algorithm::Copy(this->KeyHandle, keys);
      Algorithm::Copy(this->ValueHandle, values);

      Timer timer;
      Algorithm::SortByKey(keys, values);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "SortByKey on " << arraySize << " random values ("
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ") with "
                  << N_KEYS << " (" << PERCENT_KEYS << "%) different vtkm::Id keys";
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

  template <typename Value>
  struct BenchStableSortIndices
  {
    using SSI = vtkm::worklet::StableSortIndices<DeviceAdapterTag>;
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

      Timer timer;
      SSI::Sort(ValueHandle, indices);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "StableSortIndices::Sort on " << arraySize << " random values ("
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ")";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(StableSortIndices, BenchStableSortIndices);

  template <typename Value>
  struct BenchStableSortIndicesUnique
  {
    using SSI = vtkm::worklet::StableSortIndices<DeviceAdapterTag>;
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
      Algorithm::Schedule(
        FillModuloTestValueKernel<Value>(
          N_VALID, this->ValueHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
        arraySize);
      this->IndexHandle = SSI::Sort(this->ValueHandle);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      IndexArrayHandle indices;
      Algorithm::Copy(this->IndexHandle, indices);
      Timer timer;
      SSI::Unique(this->ValueHandle, indices);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "StableSortIndices::Unique on " << arraySize << " values ("
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ") with "
                  << this->N_VALID << " (" << PERCENT_VALID << "%) valid values";
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

  template <typename Value>
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
      Algorithm::Schedule(FillModuloTestValueKernel<Value>(
                            N_VALID, ValueHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
                          arraySize);
      Algorithm::Sort(ValueHandle);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      ValueArrayHandle array;
      Algorithm::Copy(this->ValueHandle, array);

      Timer timer;
      Algorithm::Unique(array);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id arraySize = Config.ComputeSize<Value>();
      std::stringstream description;
      description << "Unique on " << arraySize << " values ("
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ") with "
                  << N_VALID << " (" << PERCENT_VALID << "%) valid values";
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

  template <typename Value>
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
      Algorithm::Schedule(
        FillTestValueKernel<Value>(InputHandle.PrepareForOutput(arraySize, DeviceAdapterTag())),
        arraySize);
      Algorithm::Schedule(FillScaledTestValueKernel<Value>(
                            2, ValueHandle.PrepareForOutput(N_VALS, DeviceAdapterTag())),
                          N_VALS);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer;
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
                  << HumanSize(static_cast<vtkm::UInt64>(arraySize) * sizeof(Value)) << ")";
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
  static VTKM_CONT int Run()
  {
    std::cout << DIVIDER << "\nRunning DeviceAdapter benchmarks\n";

    // Run fixed bytes / size tests:
    for (int sizeType = 0; sizeType < 2; ++sizeType)
    {
      if (sizeType == 0 && Config.TestArraySizeBytes)
      {
        std::cout << DIVIDER << "\nTesting fixed array byte sizes\n";
        Config.DoByteSizes = true;
        if (!Config.ExtendedTypeList)
        {
          RunInternal<BaseTypes>();
        }
        else
        {
          RunInternal<ExtendedTypes>();
        }
      }
      if (sizeType == 1 && Config.TestArraySizeValues)
      {
        std::cout << DIVIDER << "\nTesting fixed array element counts\n";
        Config.DoByteSizes = false;
        if (!Config.ExtendedTypeList)
        {
          RunInternal<BaseTypes>();
        }
        else
        {
          RunInternal<ExtendedTypes>();
        }
      }
    }

    return 0;
  }

  template <typename ValueTypes>
  static VTKM_CONT void RunInternal()
  {
    if (Config.BenchmarkFlags & COPY)
    {
      std::cout << DIVIDER << "\nBenchmarking Copy\n";
      VTKM_RUN_BENCHMARK(Copy, ValueTypes());
    }

    if (Config.BenchmarkFlags & COPY_IF)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking CopyIf\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(CopyIf5, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf10, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf15, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf20, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf25, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf30, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf35, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf40, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf45, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf50, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf75, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf100, ValueTypes());
      }
      else
      {
        VTKM_RUN_BENCHMARK(CopyIf5, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf25, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf50, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf75, ValueTypes());
        VTKM_RUN_BENCHMARK(CopyIf100, ValueTypes());
      }
    }

    if (Config.BenchmarkFlags & LOWER_BOUNDS)
    {
      std::cout << DIVIDER << "\nBenchmarking LowerBounds\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(LowerBounds5, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds10, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds15, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds20, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds25, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds30, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds35, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds40, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds45, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds50, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds75, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds100, ValueTypes());
      }
      else
      {
        VTKM_RUN_BENCHMARK(LowerBounds5, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds25, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds50, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds75, ValueTypes());
        VTKM_RUN_BENCHMARK(LowerBounds100, ValueTypes());
      }
    }

    if (Config.BenchmarkFlags & REDUCE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking Reduce\n";
      VTKM_RUN_BENCHMARK(Reduce, ValueTypes());
    }

    if (Config.BenchmarkFlags & REDUCE_BY_KEY)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking ReduceByKey\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(ReduceByKey5, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey10, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey15, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey20, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey25, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey30, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey35, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey40, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey45, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey50, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey75, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey100, ValueTypes());
      }
      else
      {
        VTKM_RUN_BENCHMARK(ReduceByKey5, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey25, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey50, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey75, ValueTypes());
        VTKM_RUN_BENCHMARK(ReduceByKey100, ValueTypes());
      }
    }

    if (Config.BenchmarkFlags & SCAN_INCLUSIVE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking ScanInclusive\n";
      VTKM_RUN_BENCHMARK(ScanInclusive, ValueTypes());
    }

    if (Config.BenchmarkFlags & SCAN_EXCLUSIVE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking ScanExclusive\n";
      VTKM_RUN_BENCHMARK(ScanExclusive, ValueTypes());
    }

    if (Config.BenchmarkFlags & SORT)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking Sort\n";
      VTKM_RUN_BENCHMARK(Sort, ValueTypes());
    }

    if (Config.BenchmarkFlags & SORT_BY_KEY)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking SortByKey\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(SortByKey5, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey10, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey15, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey20, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey25, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey30, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey35, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey40, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey45, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey50, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey75, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey100, ValueTypes());
      }
      else
      {
        VTKM_RUN_BENCHMARK(SortByKey5, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey25, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey50, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey75, ValueTypes());
        VTKM_RUN_BENCHMARK(SortByKey100, ValueTypes());
      }
    }

    if (Config.BenchmarkFlags & STABLE_SORT_INDICES)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking StableSortIndices::Sort\n";
      VTKM_RUN_BENCHMARK(StableSortIndices, ValueTypes());
    }

    if (Config.BenchmarkFlags & STABLE_SORT_INDICES_UNIQUE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking StableSortIndices::Unique\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique5, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique10, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique15, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique20, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique25, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique30, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique35, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique40, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique45, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique50, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique75, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique100, ValueTypes());
      }
      else
      {
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique5, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique25, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique50, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique75, ValueTypes());
        VTKM_RUN_BENCHMARK(StableSortIndicesUnique100, ValueTypes());
      }
    }

    if (Config.BenchmarkFlags & UNIQUE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking Unique\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(Unique5, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique10, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique15, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique20, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique25, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique30, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique35, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique40, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique45, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique50, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique75, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique100, ValueTypes());
      }
      else
      {
        VTKM_RUN_BENCHMARK(Unique5, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique25, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique50, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique75, ValueTypes());
        VTKM_RUN_BENCHMARK(Unique100, ValueTypes());
      }
    }

    if (Config.BenchmarkFlags & UPPER_BOUNDS)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking UpperBounds\n";
      if (Config.DetailedOutputRangeScaling)
      {
        VTKM_RUN_BENCHMARK(UpperBounds5, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds10, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds15, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds20, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds25, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds30, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds35, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds40, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds45, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds50, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds75, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds100, ValueTypes());
      }
      else
      {
        VTKM_RUN_BENCHMARK(UpperBounds5, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds25, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds50, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds75, ValueTypes());
        VTKM_RUN_BENCHMARK(UpperBounds100, ValueTypes());
      }
    }
  }
};

#undef ARRAY_SIZE
}
} // namespace vtkm::benchmarking

int main(int argc, char* argv[])
{
#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_TBB
  int numThreads = tbb::task_scheduler_init::automatic;
#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_OPENMP
  int numThreads = omp_get_max_threads();
#endif // TBB

  vtkm::benchmarking::BenchDevAlgoConfig& config = vtkm::benchmarking::Config;

  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    std::transform(arg.begin(), arg.end(), arg.begin(), [](char c) {
      return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
    if (arg == "copy")
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
    else if (arg == "typelist")
    {
      ++i;
      arg = argv[i];
      std::transform(arg.begin(), arg.end(), arg.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      });
      if (arg == "base")
      {
        config.ExtendedTypeList = false;
      }
      else if (arg == "extended")
      {
        config.ExtendedTypeList = true;
      }
      else
      {
        std::cerr << "Unrecognized TypeList: " << argv[i] << std::endl;
        return 1;
      }
    }
    else if (arg == "fixbytes")
    {
      ++i;
      arg = argv[i];
      std::transform(arg.begin(), arg.end(), arg.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      });
      if (arg == "off")
      {
        config.TestArraySizeBytes = false;
      }
      else
      {
        std::istringstream parse(arg);
        config.TestArraySizeBytes = true;
        parse >> config.ArraySizeBytes;
      }
    }
    else if (arg == "fixsizes")
    {
      ++i;
      arg = argv[i];
      std::transform(arg.begin(), arg.end(), arg.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      });
      if (arg == "off")
      {
        config.TestArraySizeValues = false;
      }
      else
      {
        std::istringstream parse(arg);
        config.TestArraySizeValues = true;
        parse >> config.ArraySizeValues;
      }
    }
    else if (arg == "detailedoutputrange")
    {
      config.DetailedOutputRangeScaling = true;
    }
    else if (arg == "numthreads")
    {
      ++i;
#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_TBB
      std::istringstream parse(argv[i]);
      parse >> numThreads;
      std::cout << "Selected " << numThreads << " TBB threads." << std::endl;
#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_OPENMP
      std::istringstream parse(argv[i]);
      parse >> numThreads;
      std::cout << "Selected " << numThreads << " OpenMP threads." << std::endl;
#else
      std::cerr << "NumThreads not valid on this device. Ignoring." << std::endl;
#endif // TBB
    }
    else
    {
      std::cerr << "Unrecognized benchmark: " << argv[i] << std::endl;
      return 1;
    }
  }

#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_TBB
  // Must not be destroyed as long as benchmarks are running:
  tbb::task_scheduler_init init(numThreads);
#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_OPENMP
  omp_set_num_threads(numThreads);
#endif // TBB

  if (config.BenchmarkFlags == 0)
  {
    config.BenchmarkFlags = vtkm::benchmarking::ALL;
  }

  //now actually execute the benchmarks
  return vtkm::benchmarking::BenchmarkDeviceAdapter<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Run();
}
