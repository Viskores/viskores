//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_benchmarking_BenchmarkDeviceAdapter_h
#define vtk_m_benchmarking_BenchmarkDeviceAdapter_h

#include <vtkm/TypeTraits.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ErrorControlOutOfMemory.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/StorageBasic.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/internal/DeviceAdapterError.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/benchmarking/Benchmarker.h>

VTKM_BOOST_PRE_INCLUDE
#include <boost/random.hpp>
VTKM_BOOST_POST_INCLUDE

#include <algorithm>
#include <cmath>
#include <ctime>
#include <utility>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#endif

namespace vtkm {
namespace benchmarking {

#define ARRAY_SIZE (1 << 21)
const static std::string DIVIDER(40, '-');

enum BenchmarkName {
  LOWER_BOUNDS = 1,
  REDUCE = 1 << 1,
  REDUCE_BY_KEY = 1 << 2,
  SCAN_INCLUSIVE = 1 << 3,
  SCAN_EXCLUSIVE = 1 << 4,
  SORT = 1 << 5,
  SORT_BY_KEY = 1 << 6,
  STREAM_COMPACT = 1 << 7,
  UNIQUE = 1 << 8,
  UPPER_BOUNDS = 1 << 9,
  ALL = LOWER_BOUNDS | REDUCE | REDUCE_BY_KEY | SCAN_INCLUSIVE
    | SCAN_EXCLUSIVE | SORT | SORT_BY_KEY | STREAM_COMPACT | UNIQUE
    | UPPER_BOUNDS
};

/// This class runs a series of micro-benchmarks to measure
/// performance of the parallel primitives provided by each
/// device adapter
template<class DeviceAdapterTag>
class BenchmarkDeviceAdapter {
  typedef vtkm::cont::StorageTagBasic StorageTag;

  typedef vtkm::cont::ArrayHandle<vtkm::Id, StorageTag> IdArrayHandle;

  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>
      Algorithm;

  typedef vtkm::cont::Timer<DeviceAdapterTag> Timer;

public:
  // Various kernels used by the different benchmarks to accelerate
  // initialization of data
  template<typename Value>
  struct FillTestValueKernel : vtkm::exec::FunctorBase {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;
    typedef typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>
        ::Portal PortalType;

    PortalType Output;

    VTKM_CONT_EXPORT
    FillTestValueKernel(PortalType out) : Output(out){}

    VTKM_EXEC_EXPORT void operator()(vtkm::Id i) const {
      Output.Set(i, TestValue(i, Value()));
    }
  };

  template<typename Value>
  struct FillScaledTestValueKernel : vtkm::exec::FunctorBase {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;
    typedef typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>
        ::Portal PortalType;

    PortalType Output;
    const vtkm::Id IdScale;

    VTKM_CONT_EXPORT
    FillScaledTestValueKernel(vtkm::Id id_scale, PortalType out) : Output(out), IdScale(id_scale) {}

    VTKM_EXEC_EXPORT void operator()(vtkm::Id i) const {
      Output.Set(i, TestValue(i * IdScale, Value()));
    }
  };

  template<typename Value>
  struct FillModuloTestValueKernel : vtkm::exec::FunctorBase {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;
    typedef typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>
        ::Portal PortalType;

    PortalType Output;
    const vtkm::Id Modulus;

    VTKM_CONT_EXPORT
    FillModuloTestValueKernel(vtkm::Id modulus, PortalType out) : Output(out), Modulus(modulus) {}

    VTKM_EXEC_EXPORT void operator()(vtkm::Id i) const {
      Output.Set(i, TestValue(i % Modulus, Value()));
    }
  };

  template<typename Value>
  struct FillBinaryTestValueKernel : vtkm::exec::FunctorBase {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;
    typedef typename ValueArrayHandle::template ExecutionTypes<DeviceAdapterTag>
        ::Portal PortalType;

    PortalType Output;
    const vtkm::Id Modulus;

    VTKM_CONT_EXPORT
    FillBinaryTestValueKernel(vtkm::Id modulus, PortalType out) : Output(out), Modulus(modulus) {}

    VTKM_EXEC_EXPORT void operator()(vtkm::Id i) const {
      Output.Set(i, i % Modulus == 0 ? TestValue(vtkm::Id(1), Value()) : Value());
    }
  };

private:
  template<typename Value>
  struct BenchLowerBounds {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    const vtkm::Id N_VALS;
    ValueArrayHandle InputHandle, ValueHandle;
    IdArrayHandle OutHandle;

    VTKM_CONT_EXPORT
    BenchLowerBounds(vtkm::Id value_percent) : N_VALS((ARRAY_SIZE * value_percent) / 100)
    {
      Algorithm::Schedule(FillTestValueKernel<Value>(
            InputHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
      Algorithm::Schedule(FillScaledTestValueKernel<Value>(2,
            ValueHandle.PrepareForOutput(N_VALS, DeviceAdapterTag())), N_VALS);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()(){
      Timer timer;
      Algorithm::LowerBounds(InputHandle, ValueHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "LowerBounds on " << ARRAY_SIZE << " input and "
        << N_VALS << " values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(LowerBounds5, BenchLowerBounds, 5);
  VTKM_MAKE_BENCHMARK(LowerBounds10, BenchLowerBounds, 10);
  VTKM_MAKE_BENCHMARK(LowerBounds15, BenchLowerBounds, 15);
  VTKM_MAKE_BENCHMARK(LowerBounds20, BenchLowerBounds, 20);
  VTKM_MAKE_BENCHMARK(LowerBounds25, BenchLowerBounds, 25);
  VTKM_MAKE_BENCHMARK(LowerBounds30, BenchLowerBounds, 30);

  template<typename Value>
  struct BenchReduce {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    ValueArrayHandle InputHandle;

    VTKM_CONT_EXPORT
    BenchReduce(){
      Algorithm::Schedule(FillTestValueKernel<Value>(
            InputHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()(){
      Timer timer;
      Algorithm::Reduce(InputHandle, Value());
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "Reduce on " << ARRAY_SIZE << " values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Reduce, BenchReduce);

  template<typename Value>
  struct BenchReduceByKey {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    const vtkm::Id N_KEYS;
    ValueArrayHandle ValueHandle, ValuesOut;
    IdArrayHandle KeyHandle, KeysOut;

    VTKM_CONT_EXPORT
    BenchReduceByKey(vtkm::Id key_percent) : N_KEYS((ARRAY_SIZE * key_percent) / 100)
    {
      Algorithm::Schedule(FillTestValueKernel<Value>(
            ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
      Algorithm::Schedule(FillModuloTestValueKernel<vtkm::Id>(N_KEYS,
            KeyHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
      Algorithm::SortByKey(KeyHandle, ValueHandle);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()(){
      Timer timer;
      Algorithm::ReduceByKey(KeyHandle, ValueHandle, KeysOut, ValuesOut,
          vtkm::internal::Add());
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "ReduceByKey on " << ARRAY_SIZE
        << " values with " << N_KEYS << " distinct vtkm::Id keys";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ReduceByKey5, BenchReduceByKey, 5);
  VTKM_MAKE_BENCHMARK(ReduceByKey10, BenchReduceByKey, 10);
  VTKM_MAKE_BENCHMARK(ReduceByKey15, BenchReduceByKey, 15);
  VTKM_MAKE_BENCHMARK(ReduceByKey20, BenchReduceByKey, 20);
  VTKM_MAKE_BENCHMARK(ReduceByKey25, BenchReduceByKey, 25);
  VTKM_MAKE_BENCHMARK(ReduceByKey30, BenchReduceByKey, 30);

  template<typename Value>
  struct BenchScanInclusive {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;
    ValueArrayHandle ValueHandle, OutHandle;

    VTKM_CONT_EXPORT
    BenchScanInclusive(){
      Algorithm::Schedule(FillTestValueKernel<Value>(
            ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()(){
      Timer timer;
      Algorithm::ScanInclusive(ValueHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "ScanInclusive on " << ARRAY_SIZE << " values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ScanInclusive, BenchScanInclusive);

  template<typename Value>
  struct BenchScanExclusive {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    ValueArrayHandle ValueHandle, OutHandle;

    VTKM_CONT_EXPORT
    BenchScanExclusive(){
      Algorithm::Schedule(FillTestValueKernel<Value>(
            ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()(){
      Timer timer;
      Algorithm::ScanExclusive(ValueHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "ScanExclusive on " << ARRAY_SIZE << " values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ScanExclusive, BenchScanExclusive);

  template<typename Value>
  struct BenchSort {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    ValueArrayHandle ValueHandle;
    boost::mt19937 Rng;

    VTKM_CONT_EXPORT
    BenchSort(){
      ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag());
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()(){
      for (vtkm::Id i = 0; i < ValueHandle.GetNumberOfValues(); ++i){
        ValueHandle.GetPortalControl().Set(vtkm::Id(i), TestValue(vtkm::Id(Rng()), Value()));
      }
      Timer timer;
      Algorithm::Sort(ValueHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "Sort on " << ARRAY_SIZE << " random values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(Sort, BenchSort);

  template<typename Value>
  struct BenchSortByKey {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    boost::mt19937 Rng;
    vtkm::Id N_KEYS;
    ValueArrayHandle ValueHandle;
    IdArrayHandle KeyHandle;

    VTKM_CONT_EXPORT
    BenchSortByKey(vtkm::Id percent_key) : N_KEYS((ARRAY_SIZE * percent_key) / 100){
      ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag());
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()(){
      for (vtkm::Id i = 0; i < ValueHandle.GetNumberOfValues(); ++i){
        ValueHandle.GetPortalControl().Set(vtkm::Id(i), TestValue(vtkm::Id(Rng()), Value()));
      }
      Algorithm::Schedule(FillModuloTestValueKernel<vtkm::Id>(N_KEYS,
            KeyHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
      Timer timer;
      Algorithm::SortByKey(ValueHandle, KeyHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "SortByKey on " << ARRAY_SIZE
        << " random values with " << N_KEYS << " different vtkm::Id keys";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(SortByKey5, BenchSortByKey, 5);
  VTKM_MAKE_BENCHMARK(SortByKey10, BenchSortByKey, 10);
  VTKM_MAKE_BENCHMARK(SortByKey15, BenchSortByKey, 15);
  VTKM_MAKE_BENCHMARK(SortByKey20, BenchSortByKey, 20);
  VTKM_MAKE_BENCHMARK(SortByKey25, BenchSortByKey, 25);
  VTKM_MAKE_BENCHMARK(SortByKey30, BenchSortByKey, 30);

  template<typename Value>
  struct BenchStreamCompact {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    const vtkm::Id N_VALID;
    ValueArrayHandle ValueHandle;
    IdArrayHandle OutHandle;

    VTKM_CONT_EXPORT
    BenchStreamCompact(vtkm::Id percent_valid) : N_VALID((ARRAY_SIZE * percent_valid) / 100)
    {
      vtkm::Id modulo = ARRAY_SIZE / N_VALID;
      Algorithm::Schedule(FillBinaryTestValueKernel<Value>(modulo,
            ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()() {
      Timer timer;
      Algorithm::StreamCompact(ValueHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "StreamCompact on " << ARRAY_SIZE << " "
          << " values with " << OutHandle.GetNumberOfValues()
          << " valid values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(StreamCompact5, BenchStreamCompact, 5);
  VTKM_MAKE_BENCHMARK(StreamCompact10, BenchStreamCompact, 10);
  VTKM_MAKE_BENCHMARK(StreamCompact15, BenchStreamCompact, 15);
  VTKM_MAKE_BENCHMARK(StreamCompact20, BenchStreamCompact, 20);
  VTKM_MAKE_BENCHMARK(StreamCompact25, BenchStreamCompact, 25);
  VTKM_MAKE_BENCHMARK(StreamCompact30, BenchStreamCompact, 30);

  template<typename Value>
  struct BenchStreamCompactStencil {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    const vtkm::Id N_VALID;
    ValueArrayHandle ValueHandle;
    IdArrayHandle StencilHandle, OutHandle;

    VTKM_CONT_EXPORT
    BenchStreamCompactStencil(vtkm::Id percent_valid) : N_VALID((ARRAY_SIZE * percent_valid) / 100)
    {
      vtkm::Id modulo = ARRAY_SIZE / N_VALID;
      Algorithm::Schedule(FillTestValueKernel<Value>(
            ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
      Algorithm::Schdule(FillBinaryTestValueKernel<vtkm::Id>(modulo,
            StencilHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()() {
      Timer timer;
      Algorithm::StreamCompact(ValueHandle, StencilHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "StreamCompactStencil on " << ARRAY_SIZE << " "
          << " values with " << OutHandle.GetNumberOfValues()
          << " valid values";
      return description.str();
    }
  };
  VTKM_MAKE_BENCHMARK(StreamCompactStencil5, BenchStreamCompactStencil, 5);
  VTKM_MAKE_BENCHMARK(StreamCompactStencil10, BenchStreamCompactStencil, 10);
  VTKM_MAKE_BENCHMARK(StreamCompactStencil15, BenchStreamCompactStencil, 15);
  VTKM_MAKE_BENCHMARK(StreamCompactStencil20, BenchStreamCompactStencil, 20);
  VTKM_MAKE_BENCHMARK(StreamCompactStencil25, BenchStreamCompactStencil, 25);
  VTKM_MAKE_BENCHMARK(StreamCompactStencil30, BenchStreamCompactStencil, 30);

  template<typename Value>
  struct BenchUnique {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    const vtkm::Id N_VALID;
    ValueArrayHandle ValueHandle;

    VTKM_CONT_EXPORT
    BenchUnique(vtkm::Id percent_valid) : N_VALID((ARRAY_SIZE * percent_valid) / 100)
    {}

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()(){
      Algorithm::Schedule(FillModuloTestValueKernel<Value>(N_VALID,
            ValueHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
      Algorithm::Sort(ValueHandle);
      Timer timer;
      Algorithm::Unique(ValueHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
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

  template<typename Value>
  struct BenchUpperBounds {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    const vtkm::Id N_VALS;
    ValueArrayHandle InputHandle, ValueHandle;
    IdArrayHandle OutHandle;

    VTKM_CONT_EXPORT
    BenchUpperBounds(vtkm::Id percent_vals) : N_VALS((ARRAY_SIZE * percent_vals) / 100)
    {
      Algorithm::Schedule(FillTestValueKernel<Value>(
            InputHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag())), ARRAY_SIZE);
      Algorithm::Schedule(FillScaledTestValueKernel<Value>(2,
            ValueHandle.PrepareForOutput(N_VALS, DeviceAdapterTag())), N_VALS);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()(){
      Timer timer;
      Algorithm::UpperBounds(InputHandle, ValueHandle, OutHandle);
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "UpperBounds on " << ARRAY_SIZE << " input and "
        << N_VALS << " values";
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

  struct ValueTypes : vtkm::ListTagBase<vtkm::UInt8, vtkm::UInt32, vtkm::Int32,
                                        vtkm::Int64, vtkm::Vec<vtkm::Int32, 2>,
                                        vtkm::Vec<vtkm::UInt8, 4>, vtkm::Float32,
                                        vtkm::Float64, vtkm::Vec<vtkm::Float64, 3>,
                                        vtkm::Vec<vtkm::Float32, 4> >{};

  static VTKM_CONT_EXPORT int Run(int benchmarks){
    std::cout << DIVIDER << "\nRunning DeviceAdapter benchmarks\n";

    if (benchmarks & LOWER_BOUNDS){
      std::cout << DIVIDER << "\nBenchmarking LowerBounds\n";
      VTKM_RUN_BENCHMARK(LowerBounds5, ValueTypes());
      VTKM_RUN_BENCHMARK(LowerBounds10, ValueTypes());
      VTKM_RUN_BENCHMARK(LowerBounds15, ValueTypes());
      VTKM_RUN_BENCHMARK(LowerBounds20, ValueTypes());
      VTKM_RUN_BENCHMARK(LowerBounds25, ValueTypes());
      VTKM_RUN_BENCHMARK(LowerBounds30, ValueTypes());
    }

    if (benchmarks & REDUCE){
      std::cout << "\n" << DIVIDER << "\nBenchmarking Reduce\n";
      VTKM_RUN_BENCHMARK(Reduce, ValueTypes());
    }

    if (benchmarks & REDUCE_BY_KEY){
      std::cout << "\n" << DIVIDER << "\nBenchmarking ReduceByKey\n";
      VTKM_RUN_BENCHMARK(ReduceByKey5, ValueTypes());
      VTKM_RUN_BENCHMARK(ReduceByKey10, ValueTypes());
      VTKM_RUN_BENCHMARK(ReduceByKey15, ValueTypes());
      VTKM_RUN_BENCHMARK(ReduceByKey20, ValueTypes());
      VTKM_RUN_BENCHMARK(ReduceByKey25, ValueTypes());
      VTKM_RUN_BENCHMARK(ReduceByKey30, ValueTypes());
    }

    if (benchmarks & SCAN_INCLUSIVE){
      std::cout << "\n" << DIVIDER << "\nBenchmarking ScanInclusive\n";
      VTKM_RUN_BENCHMARK(ScanInclusive, ValueTypes());
    }

    if (benchmarks & SCAN_EXCLUSIVE){
      std::cout << "\n" << DIVIDER << "\nBenchmarking ScanExclusive\n";
      VTKM_RUN_BENCHMARK(ScanExclusive, ValueTypes());
    }

    if (benchmarks & SORT){
      std::cout << "\n" << DIVIDER << "\nBenchmarking Sort\n";
      VTKM_RUN_BENCHMARK(Sort, ValueTypes());
    }

    if (benchmarks & SORT_BY_KEY){
      std::cout << "\n" << DIVIDER << "\nBenchmarking SortByKey\n";
      VTKM_RUN_BENCHMARK(SortByKey5, ValueTypes());
      VTKM_RUN_BENCHMARK(SortByKey10, ValueTypes());
      VTKM_RUN_BENCHMARK(SortByKey15, ValueTypes());
      VTKM_RUN_BENCHMARK(SortByKey20, ValueTypes());
      VTKM_RUN_BENCHMARK(SortByKey25, ValueTypes());
      VTKM_RUN_BENCHMARK(SortByKey30, ValueTypes());
    }

    if (benchmarks & STREAM_COMPACT){
      std::cout << "\n" << DIVIDER << "\nBenchmarking StreamCompact\n";
      VTKM_RUN_BENCHMARK(StreamCompact5, ValueTypes());
      VTKM_RUN_BENCHMARK(StreamCompact10, ValueTypes());
      VTKM_RUN_BENCHMARK(StreamCompact15, ValueTypes());
      VTKM_RUN_BENCHMARK(StreamCompact20, ValueTypes());
      VTKM_RUN_BENCHMARK(StreamCompact25, ValueTypes());
      VTKM_RUN_BENCHMARK(StreamCompact30, ValueTypes());
    }

    if (benchmarks & UNIQUE){
      std::cout << "\n" << DIVIDER << "\nBenchmarking Unique\n";
      VTKM_RUN_BENCHMARK(Unique5, ValueTypes());
      VTKM_RUN_BENCHMARK(Unique10, ValueTypes());
      VTKM_RUN_BENCHMARK(Unique15, ValueTypes());
      VTKM_RUN_BENCHMARK(Unique20, ValueTypes());
      VTKM_RUN_BENCHMARK(Unique25, ValueTypes());
      VTKM_RUN_BENCHMARK(Unique30, ValueTypes());
    }

    if (benchmarks & UPPER_BOUNDS){
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

#endif

