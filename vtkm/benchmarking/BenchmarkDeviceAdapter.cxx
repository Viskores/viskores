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

#include <vtkm/TypeTraits.h>
#include <vtkm/benchmarking/Benchmarker.h>
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
#include <cmath>
#include <random>
#include <string>
#include <utility>

#include <vtkm/internal/Windows.h>

namespace vtkm
{
namespace benchmarking
{

#define ARRAY_SIZE (1 << 21)
const static std::string DIVIDER(40, '-');

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

/// This class runs a series of micro-benchmarks to measure
/// performance of the parallel primitives provided by each
/// device adapter
template <class DeviceAdapterTag>
class BenchmarkDeviceAdapter
{
  typedef vtkm::cont::StorageTagBasic StorageTag;

  typedef vtkm::cont::ArrayHandle<vtkm::Id, StorageTag> IdArrayHandle;

  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag> Algorithm;

  typedef vtkm::cont::Timer<DeviceAdapterTag> Timer;

public:
  // Various kernels used by the different benchmarks to accelerate
  // initialization of data
  template <typename Value>
  struct FillTestValueKernel : vtkm::exec::FunctorBase
  {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;
    typedef typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>::Portal PortalType;

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
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;
    typedef typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>::Portal PortalType;

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
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;
    typedef typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>::Portal PortalType;

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
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;
    typedef typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>::Portal PortalType;

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
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    ValueArrayHandle ValueHandle_src;
    ValueArrayHandle ValueHandle_dst;
    std::mt19937 Rng;

    VTKM_CONT
    BenchCopy()
    {
      ValueHandle_src.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag());
      ValueHandle_dst.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag());
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      for (vtkm::Id i = 0; i < ValueHandle_src.GetNumberOfValues(); ++i)
      {
        ValueHandle_src.GetPortalControl().Set(vtkm::Id(i), TestValue(vtkm::Id(Rng()), Value()));
      }
      Timer timer;
      Algorithm::Copy(ValueHandle_src, ValueHandle_dst);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "Copy " << ARRAY_SIZE << " values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Copy, BenchCopy);

  template <typename Value>
  struct BenchCopyIf
  {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    const vtkm::Id N_VALID;
    ValueArrayHandle ValueHandle, OutHandle;
    IdArrayHandle StencilHandle;

    VTKM_CONT
    BenchCopyIf(vtkm::Id percent_valid)
      : N_VALID((ARRAY_SIZE * percent_valid) / 100)
    {
      vtkm::Id modulo = ARRAY_SIZE / N_VALID;
      Algorithm::Schedule(
        FillTestValueKernel<Value>(ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
        ARRAY_SIZE);
      Algorithm::Schedule(FillBinaryTestValueKernel<vtkm::Id>(
                            modulo, StencilHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                          ARRAY_SIZE);
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
      std::stringstream description;
      description << "CopyIf on " << ARRAY_SIZE << " "
                  << " values with " << OutHandle.GetNumberOfValues() << " valid values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(CopyIf5, BenchCopyIf, 5);
  VTKM_MAKE_BENCHMARK(CopyIf10, BenchCopyIf, 10);
  VTKM_MAKE_BENCHMARK(CopyIf15, BenchCopyIf, 15);
  VTKM_MAKE_BENCHMARK(CopyIf20, BenchCopyIf, 20);
  VTKM_MAKE_BENCHMARK(CopyIf25, BenchCopyIf, 25);
  VTKM_MAKE_BENCHMARK(CopyIf30, BenchCopyIf, 30);

  template <typename Value>
  struct BenchLowerBounds
  {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    const vtkm::Id N_VALS;
    ValueArrayHandle InputHandle, ValueHandle;
    IdArrayHandle OutHandle;

    VTKM_CONT
    BenchLowerBounds(vtkm::Id value_percent)
      : N_VALS((ARRAY_SIZE * value_percent) / 100)
    {
      Algorithm::Schedule(
        FillTestValueKernel<Value>(InputHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
        ARRAY_SIZE);
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
      std::stringstream description;
      description << "LowerBounds on " << ARRAY_SIZE << " input and " << N_VALS << " values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(LowerBounds5, BenchLowerBounds, 5);
  VTKM_MAKE_BENCHMARK(LowerBounds10, BenchLowerBounds, 10);
  VTKM_MAKE_BENCHMARK(LowerBounds15, BenchLowerBounds, 15);
  VTKM_MAKE_BENCHMARK(LowerBounds20, BenchLowerBounds, 20);
  VTKM_MAKE_BENCHMARK(LowerBounds25, BenchLowerBounds, 25);
  VTKM_MAKE_BENCHMARK(LowerBounds30, BenchLowerBounds, 30);

  template <typename Value>
  struct BenchReduce
  {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    ValueArrayHandle InputHandle;
    // We don't actually use this, but we need it to prevent sufficently
    // smart compilers from optimizing the Reduce call out.
    Value Result;

    VTKM_CONT
    BenchReduce()
    {
      Algorithm::Schedule(
        FillTestValueKernel<Value>(InputHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
        ARRAY_SIZE);
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
      std::stringstream description;
      description << "Reduce on " << ARRAY_SIZE << " values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Reduce, BenchReduce);

  template <typename Value>
  struct BenchReduceByKey
  {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    const vtkm::Id N_KEYS;
    ValueArrayHandle ValueHandle, ValuesOut;
    IdArrayHandle KeyHandle, KeysOut;

    VTKM_CONT
    BenchReduceByKey(vtkm::Id key_percent)
      : N_KEYS((ARRAY_SIZE * key_percent) / 100)
    {
      Algorithm::Schedule(
        FillTestValueKernel<Value>(ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
        ARRAY_SIZE);
      Algorithm::Schedule(FillModuloTestValueKernel<vtkm::Id>(
                            N_KEYS, KeyHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                          ARRAY_SIZE);
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
      std::stringstream description;
      description << "ReduceByKey on " << ARRAY_SIZE << " values with " << N_KEYS
                  << " distinct vtkm::Id keys";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ReduceByKey5, BenchReduceByKey, 5);
  VTKM_MAKE_BENCHMARK(ReduceByKey10, BenchReduceByKey, 10);
  VTKM_MAKE_BENCHMARK(ReduceByKey15, BenchReduceByKey, 15);
  VTKM_MAKE_BENCHMARK(ReduceByKey20, BenchReduceByKey, 20);
  VTKM_MAKE_BENCHMARK(ReduceByKey25, BenchReduceByKey, 25);
  VTKM_MAKE_BENCHMARK(ReduceByKey30, BenchReduceByKey, 30);
  VTKM_MAKE_BENCHMARK(ReduceByKey100, BenchReduceByKey, 100);

  template <typename Value>
  struct BenchScanInclusive
  {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;
    ValueArrayHandle ValueHandle, OutHandle;

    VTKM_CONT
    BenchScanInclusive()
    {
      Algorithm::Schedule(
        FillTestValueKernel<Value>(ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
        ARRAY_SIZE);
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
      std::stringstream description;
      description << "ScanInclusive on " << ARRAY_SIZE << " values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ScanInclusive, BenchScanInclusive);

  template <typename Value>
  struct BenchScanExclusive
  {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    ValueArrayHandle ValueHandle, OutHandle;

    VTKM_CONT
    BenchScanExclusive()
    {
      Algorithm::Schedule(
        FillTestValueKernel<Value>(ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
        ARRAY_SIZE);
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
      std::stringstream description;
      description << "ScanExclusive on " << ARRAY_SIZE << " values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ScanExclusive, BenchScanExclusive);

  template <typename Value>
  struct BenchSort
  {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    ValueArrayHandle ValueHandle;
    std::mt19937 Rng;

    VTKM_CONT
    BenchSort() { ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag()); }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      for (vtkm::Id i = 0; i < ValueHandle.GetNumberOfValues(); ++i)
      {
        ValueHandle.GetPortalControl().Set(vtkm::Id(i), TestValue(vtkm::Id(Rng()), Value()));
      }
      Timer timer;
      Algorithm::Sort(ValueHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "Sort on " << ARRAY_SIZE << " random values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Sort, BenchSort);

  template <typename Value>
  struct BenchSortByKey
  {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    std::mt19937 Rng;
    vtkm::Id N_KEYS;
    ValueArrayHandle ValueHandle;
    IdArrayHandle KeyHandle;

    VTKM_CONT
    BenchSortByKey(vtkm::Id percent_key)
      : N_KEYS((ARRAY_SIZE * percent_key) / 100)
    {
      ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag());
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      for (vtkm::Id i = 0; i < ValueHandle.GetNumberOfValues(); ++i)
      {
        ValueHandle.GetPortalControl().Set(vtkm::Id(i), TestValue(vtkm::Id(Rng()), Value()));
      }
      Algorithm::Schedule(FillModuloTestValueKernel<vtkm::Id>(
                            N_KEYS, KeyHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                          ARRAY_SIZE);
      Timer timer;
      Algorithm::SortByKey(ValueHandle, KeyHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "SortByKey on " << ARRAY_SIZE << " random values with " << N_KEYS
                  << " different vtkm::Id keys";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(SortByKey5, BenchSortByKey, 5);
  VTKM_MAKE_BENCHMARK(SortByKey10, BenchSortByKey, 10);
  VTKM_MAKE_BENCHMARK(SortByKey15, BenchSortByKey, 15);
  VTKM_MAKE_BENCHMARK(SortByKey20, BenchSortByKey, 20);
  VTKM_MAKE_BENCHMARK(SortByKey25, BenchSortByKey, 25);
  VTKM_MAKE_BENCHMARK(SortByKey30, BenchSortByKey, 30);
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
      this->ValueHandle.Allocate(ARRAY_SIZE);
      auto portal = this->ValueHandle.GetPortalControl();
      for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
      {
        portal.Set(vtkm::Id(i), TestValue(vtkm::Id(Rng()), Value()));
      }
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::ArrayHandle<vtkm::Id> indices;
      Algorithm::Copy(vtkm::cont::ArrayHandleIndex(ARRAY_SIZE), indices);

      Timer timer;
      SSI::Sort(ValueHandle, indices);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "StableSortIndices::Sort on " << ARRAY_SIZE << " random values";
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
    ValueArrayHandle ValueHandle;

    VTKM_CONT
    BenchStableSortIndicesUnique(vtkm::Id percent_valid)
      : N_VALID((ARRAY_SIZE * percent_valid) / 100)
    {
      Algorithm::Schedule(
        FillModuloTestValueKernel<Value>(
          N_VALID, this->ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
        ARRAY_SIZE);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      IndexArrayHandle indices = SSI::Sort(this->ValueHandle);
      Timer timer;
      SSI::Unique(this->ValueHandle, indices);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "StableSortIndices::Unique on " << ARRAY_SIZE << " values with "
                  << this->N_VALID << " valid values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique5, BenchStableSortIndicesUnique, 5);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique10, BenchStableSortIndicesUnique, 10);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique15, BenchStableSortIndicesUnique, 15);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique20, BenchStableSortIndicesUnique, 20);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique25, BenchStableSortIndicesUnique, 25);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique30, BenchStableSortIndicesUnique, 30);
  VTKM_MAKE_BENCHMARK(StableSortIndicesUnique100, BenchStableSortIndicesUnique, 100);

  template <typename Value>
  struct BenchUnique
  {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    const vtkm::Id N_VALID;
    ValueArrayHandle ValueHandle;

    VTKM_CONT
    BenchUnique(vtkm::Id percent_valid)
      : N_VALID((ARRAY_SIZE * percent_valid) / 100)
    {
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Algorithm::Schedule(FillModuloTestValueKernel<Value>(
                            N_VALID, ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
                          ARRAY_SIZE);
      Algorithm::Sort(ValueHandle);
      Timer timer;
      Algorithm::Unique(ValueHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "Unique on " << ARRAY_SIZE << " values with "
                  << ValueHandle.GetNumberOfValues() << " valid values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Unique5, BenchUnique, 5);
  VTKM_MAKE_BENCHMARK(Unique10, BenchUnique, 10);
  VTKM_MAKE_BENCHMARK(Unique15, BenchUnique, 15);
  VTKM_MAKE_BENCHMARK(Unique20, BenchUnique, 20);
  VTKM_MAKE_BENCHMARK(Unique25, BenchUnique, 25);
  VTKM_MAKE_BENCHMARK(Unique30, BenchUnique, 30);

  template <typename Value>
  struct BenchUpperBounds
  {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    const vtkm::Id N_VALS;
    ValueArrayHandle InputHandle, ValueHandle;
    IdArrayHandle OutHandle;

    VTKM_CONT
    BenchUpperBounds(vtkm::Id percent_vals)
      : N_VALS((ARRAY_SIZE * percent_vals) / 100)
    {
      Algorithm::Schedule(
        FillTestValueKernel<Value>(InputHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())),
        ARRAY_SIZE);
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
      std::stringstream description;
      description << "UpperBounds on " << ARRAY_SIZE << " input and " << N_VALS << " values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(UpperBounds5, BenchUpperBounds, 5);
  VTKM_MAKE_BENCHMARK(UpperBounds10, BenchUpperBounds, 10);
  VTKM_MAKE_BENCHMARK(UpperBounds15, BenchUpperBounds, 15);
  VTKM_MAKE_BENCHMARK(UpperBounds20, BenchUpperBounds, 20);
  VTKM_MAKE_BENCHMARK(UpperBounds25, BenchUpperBounds, 25);
  VTKM_MAKE_BENCHMARK(UpperBounds30, BenchUpperBounds, 30);

public:
  struct ValueTypes : vtkm::ListTagBase<vtkm::UInt8,
                                        vtkm::UInt32,
                                        vtkm::Int32,
                                        vtkm::Int64,
                                        vtkm::Vec<vtkm::Int32, 2>,
                                        vtkm::Vec<vtkm::UInt8, 4>,
                                        vtkm::Float32,
                                        vtkm::Float64,
                                        vtkm::Vec<vtkm::Float64, 3>,
                                        vtkm::Vec<vtkm::Float32, 4>>
  {
  };

  static VTKM_CONT int Run(int benchmarks)
  {
    std::cout << DIVIDER << "\nRunning DeviceAdapter benchmarks\n";

    if (benchmarks & COPY)
    {
      std::cout << DIVIDER << "\nBenchmarking Copy\n";
      VTKM_RUN_BENCHMARK(Copy, ValueTypes());
    }

    if (benchmarks & COPY_IF)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking CopyIf\n";
      VTKM_RUN_BENCHMARK(CopyIf5, ValueTypes());
      VTKM_RUN_BENCHMARK(CopyIf10, ValueTypes());
      VTKM_RUN_BENCHMARK(CopyIf15, ValueTypes());
      VTKM_RUN_BENCHMARK(CopyIf20, ValueTypes());
      VTKM_RUN_BENCHMARK(CopyIf25, ValueTypes());
      VTKM_RUN_BENCHMARK(CopyIf30, ValueTypes());
    }

    if (benchmarks & LOWER_BOUNDS)
    {
      std::cout << DIVIDER << "\nBenchmarking LowerBounds\n";
      VTKM_RUN_BENCHMARK(LowerBounds5, ValueTypes());
      VTKM_RUN_BENCHMARK(LowerBounds10, ValueTypes());
      VTKM_RUN_BENCHMARK(LowerBounds15, ValueTypes());
      VTKM_RUN_BENCHMARK(LowerBounds20, ValueTypes());
      VTKM_RUN_BENCHMARK(LowerBounds25, ValueTypes());
      VTKM_RUN_BENCHMARK(LowerBounds30, ValueTypes());
    }

    if (benchmarks & REDUCE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking Reduce\n";
      VTKM_RUN_BENCHMARK(Reduce, ValueTypes());
    }

    if (benchmarks & REDUCE_BY_KEY)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking ReduceByKey\n";
      VTKM_RUN_BENCHMARK(ReduceByKey5, ValueTypes());
      VTKM_RUN_BENCHMARK(ReduceByKey10, ValueTypes());
      VTKM_RUN_BENCHMARK(ReduceByKey15, ValueTypes());
      VTKM_RUN_BENCHMARK(ReduceByKey20, ValueTypes());
      VTKM_RUN_BENCHMARK(ReduceByKey25, ValueTypes());
      VTKM_RUN_BENCHMARK(ReduceByKey30, ValueTypes());
      VTKM_RUN_BENCHMARK(ReduceByKey100, ValueTypes());
    }

    if (benchmarks & SCAN_INCLUSIVE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking ScanInclusive\n";
      VTKM_RUN_BENCHMARK(ScanInclusive, ValueTypes());
    }

    if (benchmarks & SCAN_EXCLUSIVE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking ScanExclusive\n";
      VTKM_RUN_BENCHMARK(ScanExclusive, ValueTypes());
    }

    if (benchmarks & SORT)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking Sort\n";
      VTKM_RUN_BENCHMARK(Sort, ValueTypes());
    }

    if (benchmarks & SORT_BY_KEY)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking SortByKey\n";
      VTKM_RUN_BENCHMARK(SortByKey5, ValueTypes());
      VTKM_RUN_BENCHMARK(SortByKey10, ValueTypes());
      VTKM_RUN_BENCHMARK(SortByKey15, ValueTypes());
      VTKM_RUN_BENCHMARK(SortByKey20, ValueTypes());
      VTKM_RUN_BENCHMARK(SortByKey25, ValueTypes());
      VTKM_RUN_BENCHMARK(SortByKey30, ValueTypes());
      VTKM_RUN_BENCHMARK(SortByKey100, ValueTypes());
    }

    if (benchmarks & STABLE_SORT_INDICES)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking StableSortIndices::Sort\n";
      VTKM_RUN_BENCHMARK(StableSortIndices, ValueTypes());
    }

    if (benchmarks & STABLE_SORT_INDICES_UNIQUE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking StableSortIndices::Unique\n";
      VTKM_RUN_BENCHMARK(StableSortIndicesUnique5, ValueTypes());
      VTKM_RUN_BENCHMARK(StableSortIndicesUnique10, ValueTypes());
      VTKM_RUN_BENCHMARK(StableSortIndicesUnique15, ValueTypes());
      VTKM_RUN_BENCHMARK(StableSortIndicesUnique20, ValueTypes());
      VTKM_RUN_BENCHMARK(StableSortIndicesUnique25, ValueTypes());
      VTKM_RUN_BENCHMARK(StableSortIndicesUnique30, ValueTypes());
      VTKM_RUN_BENCHMARK(StableSortIndicesUnique100, ValueTypes());
    }

    if (benchmarks & UNIQUE)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking Unique\n";
      VTKM_RUN_BENCHMARK(Unique5, ValueTypes());
      VTKM_RUN_BENCHMARK(Unique10, ValueTypes());
      VTKM_RUN_BENCHMARK(Unique15, ValueTypes());
      VTKM_RUN_BENCHMARK(Unique20, ValueTypes());
      VTKM_RUN_BENCHMARK(Unique25, ValueTypes());
      VTKM_RUN_BENCHMARK(Unique30, ValueTypes());
    }

    if (benchmarks & UPPER_BOUNDS)
    {
      std::cout << "\n" << DIVIDER << "\nBenchmarking UpperBounds\n";
      VTKM_RUN_BENCHMARK(UpperBounds5, ValueTypes());
      VTKM_RUN_BENCHMARK(UpperBounds10, ValueTypes());
      VTKM_RUN_BENCHMARK(UpperBounds15, ValueTypes());
      VTKM_RUN_BENCHMARK(UpperBounds20, ValueTypes());
      VTKM_RUN_BENCHMARK(UpperBounds25, ValueTypes());
      VTKM_RUN_BENCHMARK(UpperBounds30, ValueTypes());
    }
    return 0;
  }
};

#undef ARRAY_SIZE
}
} // namespace vtkm::benchmarking

int main(int argc, char* argv[])
{
  int benchmarks = 0;
  if (argc < 2)
  {
    benchmarks = vtkm::benchmarking::ALL;
  }
  else
  {
    for (int i = 1; i < argc; ++i)
    {
      std::string arg = argv[i];
      std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
      if (arg == "copy")
      {
        benchmarks |= vtkm::benchmarking::COPY;
      }
      else if (arg == "copyif")
      {
        benchmarks |= vtkm::benchmarking::COPY_IF;
      }
      else if (arg == "lowerbounds")
      {
        benchmarks |= vtkm::benchmarking::LOWER_BOUNDS;
      }
      else if (arg == "reduce")
      {
        benchmarks |= vtkm::benchmarking::REDUCE;
      }
      else if (arg == "reducebykey")
      {
        benchmarks |= vtkm::benchmarking::REDUCE_BY_KEY;
      }
      else if (arg == "scaninclusive")
      {
        benchmarks |= vtkm::benchmarking::SCAN_INCLUSIVE;
      }
      else if (arg == "scanexclusive")
      {
        benchmarks |= vtkm::benchmarking::SCAN_EXCLUSIVE;
      }
      else if (arg == "sort")
      {
        benchmarks |= vtkm::benchmarking::SORT;
      }
      else if (arg == "sortbykey")
      {
        benchmarks |= vtkm::benchmarking::SORT_BY_KEY;
      }
      else if (arg == "stablesortindices")
      {
        benchmarks |= vtkm::benchmarking::STABLE_SORT_INDICES;
      }
      else if (arg == "stablesortindicesunique")
      {
        benchmarks |= vtkm::benchmarking::STABLE_SORT_INDICES_UNIQUE;
      }
      else if (arg == "unique")
      {
        benchmarks |= vtkm::benchmarking::UNIQUE;
      }
      else if (arg == "upperbounds")
      {
        benchmarks |= vtkm::benchmarking::UPPER_BOUNDS;
      }
      else
      {
        std::cout << "Unrecognized benchmark: " << argv[i] << std::endl;
        return 1;
      }
    }
  }

  //now actually execute the benchmarks
  return vtkm::benchmarking::BenchmarkDeviceAdapter<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Run(
    benchmarks);
}
