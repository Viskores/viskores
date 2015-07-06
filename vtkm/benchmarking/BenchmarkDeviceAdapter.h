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

#include <boost/random.hpp>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <utility>
#include <vector>
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

#define ARRAY_SIZE (1 << 20)
const static std::string DIVIDER(40, '-');

/// This class runs a series of micro-benchmarks to measure
/// performance of the parallel primitives provided by each
/// device adapter
///
template<class DeviceAdapterTag>
struct BenchmarkDeviceAdapter {
private:
  typedef vtkm::cont::StorageTagBasic StorageTagBasic;
  typedef vtkm::cont::StorageTagBasic StorageTag;

  typedef vtkm::cont::ArrayHandle<vtkm::Id, StorageTag> IdArrayHandle;

  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>
      Algorithm;

  typedef vtkm::cont::Timer<DeviceAdapterTag> Timer;

  struct BenchLowerBounds {
    template<typename Value>
    VTKM_CONT_EXPORT void operator()(const Value vtkmNotUsed(v)) const {
      typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

      Timer timer;

      std::vector<Value> input(ARRAY_SIZE, Value());
      for (size_t i = 0; i < input.size(); ++i){
        input[i] = TestValue(vtkm::Id(i), Value());
      }
      ValueArrayHandle input_handle = vtkm::cont::make_ArrayHandle(input);

      // We benchmark finding indices for the elements using various
      // ratios of values to input from 5-30% of # of elements in input
      for (size_t p = 5; p <= 30; p += 5){
        size_t n_vals = (ARRAY_SIZE * p) / 100;
        std::vector<Value> values(n_vals, Value());
        for (size_t i = 0; i < values.size(); ++i){
          values[i] = TestValue(vtkm::Id(2 * i), Value());
        }
        ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
        IdArrayHandle out_handle;
        timer.Reset();
        Algorithm::LowerBounds(input_handle, value_handle, out_handle);
        vtkm::Float64 elapsed = timer.GetElapsedTime();
        std::cout << "LowerBounds on " << ARRAY_SIZE << " input and "
          << n_vals << " values took " << elapsed << "s\n";
      }
    }
  };

  struct BenchReduce {
    template<typename Value>
    VTKM_CONT_EXPORT void operator()(const Value vtkmNotUsed(v)) const {
      typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

      Timer timer;
      std::vector<Value> input(ARRAY_SIZE, Value());
      for (size_t i = 0; i < input.size(); ++i){
        input[i] = TestValue(vtkm::Id(i), Value());
      }
      ValueArrayHandle input_handle = vtkm::cont::make_ArrayHandle(input);
      timer.Reset();
      Algorithm::Reduce(input_handle, Value());
      vtkm::Float64 elapsed = timer.GetElapsedTime();
      std::cout << "Reduce on " << ARRAY_SIZE
        << " values took " << elapsed << "s\n";
    }
  };

  struct BenchReduceByKey {
    template<typename Value>
    VTKM_CONT_EXPORT void operator()(const Value vtkmNotUsed(v)) const {
      typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

      Timer timer;
      // We benchmark 5% to 30% of ARRAY_SIZE keys in 5% increments
      for (size_t p = 5; p <= 30; p += 5){
        size_t n_keys = (ARRAY_SIZE * p) / 100;
        std::vector<Value> values(ARRAY_SIZE, Value());
        std::vector<vtkm::Id> keys(ARRAY_SIZE, 0);
        for (size_t i = 0; i < values.size(); ++i){
          values[i] = TestValue(vtkm::Id(i), Value());
          keys[i] = vtkm::Id(i % n_keys);
        }
        ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
        ValueArrayHandle values_out;
        IdArrayHandle key_handle = vtkm::cont::make_ArrayHandle(keys);
        IdArrayHandle keys_out;
        Algorithm::SortByKey(key_handle, value_handle);
        timer.Reset();
        Algorithm::ReduceByKey(key_handle, value_handle, keys_out, values_out,
            vtkm::internal::Add());
        vtkm::Float64 elapsed = timer.GetElapsedTime();
        std::cout << "ReduceByKey on " << ARRAY_SIZE
          << " values with " << n_keys << " distinct vtkm::Id"
          << " keys took " << elapsed << "s\n";
      }
    }
  };

  struct BenchScanInclusive {
    template<typename Value>
    VTKM_CONT_EXPORT void operator()(const Value vtkmNotUsed(v)) const {
      typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

      Timer timer;
      std::vector<Value> values(ARRAY_SIZE, Value());
      for (size_t i = 0; i < values.size(); ++i){
        values[i] = TestValue(vtkm::Id(i), Value());
      }
      ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
      ValueArrayHandle out_handle;
      timer.Reset();
      Algorithm::ScanInclusive(value_handle, out_handle);
      vtkm::Float64 elapsed = timer.GetElapsedTime();
      std::cout << "ScanInclusive on " << ARRAY_SIZE
        << " values took " << elapsed << "s\n";
    }
  };

  struct BenchScanExclusive {
    template<typename Value>
    VTKM_CONT_EXPORT void operator()(const Value vtkmNotUsed(v)) const {
      typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

      Timer timer;
      std::vector<Value> values(ARRAY_SIZE, Value());
      for (size_t i = 0; i < values.size(); ++i){
        values[i] = TestValue(vtkm::Id(i), Value());
      }
      ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
      ValueArrayHandle out_handle;
      timer.Reset();
      Algorithm::ScanExclusive(value_handle, out_handle);
      vtkm::Float64 elapsed = timer.GetElapsedTime();
      std::cout << "ScanExclusive on " << ARRAY_SIZE
        << " values took " << elapsed << "s\n";
    }
  };

  /// This benchmark tests sort on a few configurations of data
  /// sorted, reverse-ordered, almost sorted and random
  /// TODO: Is it really worth testing all these possible configurations
  /// of data? How often will we really care about anything besides unsorted data?
  struct BenchSort {
    template<typename Value>
    VTKM_CONT_EXPORT void operator()(const Value vtkmNotUsed(v)) const {
      typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

      Timer timer;
      std::vector<Value> values(ARRAY_SIZE, Value());
      // Test sort on already sorted data
      {
        for (size_t i = 0; i < values.size(); ++i){
          values[i] = TestValue(vtkm::Id(i), Value());
        }
        ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
        timer.Reset();
        Algorithm::Sort(value_handle);
        vtkm::Float64 elapsed = timer.GetElapsedTime();
        std::cout << "Sort on " << ARRAY_SIZE << " already sorted "
          << " values took " << elapsed << "s\n";
      }
      // Test sort on reverse-sorted data
      {
        for (size_t i = 0; i < values.size(); ++i){
          values[i] = TestValue(vtkm::Id(values.size() - i), Value());
        }
        ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
        timer.Reset();
        Algorithm::Sort(value_handle);
        vtkm::Float64 elapsed = timer.GetElapsedTime();
        std::cout << "Sort on " << ARRAY_SIZE << " reverse-ordered "
          << " values took " << elapsed << "s\n";
      }
      // Test on almost sorted data
      {
        size_t modulus = values.size() / 4;
        for (size_t i = 0; i < values.size(); ++i){
          values[i] = TestValue(vtkm::Id(i % modulus), Value());
        }
        ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
        timer.Reset();
        Algorithm::Sort(value_handle);
        vtkm::Float64 elapsed = timer.GetElapsedTime();
        std::cout << "Sort on " << ARRAY_SIZE << " almost-sorted "
          << " values took " << elapsed << "s\n";
      }
      // Test on random data
      {
        boost::mt19937 rng;
        for (size_t i = 0; i < values.size(); ++i){
          values[i] = TestValue(vtkm::Id(rng()), Value());
        }
        ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
        timer.Reset();
        Algorithm::Sort(value_handle);
        vtkm::Float64 elapsed = timer.GetElapsedTime();
        std::cout << "Sort on " << ARRAY_SIZE << " random "
          << " values took " << elapsed << "s\n";
      }
    }
  };

  struct BenchSortByKey {
    template<typename Value>
    VTKM_CONT_EXPORT void operator()(const Value vtkmNotUsed(v)) const {
      typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

      Timer timer;
      boost::mt19937 rng;
      // We benchmark 5% to 30% of ARRAY_SIZE keys in 5% increments
      for (size_t p = 5; p <= 30; p += 5){
        size_t n_keys = (ARRAY_SIZE * p) / 100;
        std::vector<Value> values(ARRAY_SIZE, Value());
        std::vector<vtkm::Id> keys(ARRAY_SIZE, 0);
        for (size_t i = 0; i < values.size(); ++i){
          values[i] = TestValue(vtkm::Id(rng()), Value());
          keys[i] = vtkm::Id(i % n_keys);
        }
        ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
        IdArrayHandle key_handle = vtkm::cont::make_ArrayHandle(keys);
        timer.Reset();
        Algorithm::SortByKey(value_handle, key_handle);
        vtkm::Float64 elapsed = timer.GetElapsedTime();
        std::cout << "SortByKey on " << ARRAY_SIZE
          << " random values with " << n_keys << " different vtkm::Id keys took "
          << elapsed << "s\n";
      }
    }
  };

  struct BenchStreamCompact {
    template<typename Value>
    VTKM_CONT_EXPORT void operator()(const Value vtkmNotUsed(v)) const {
      typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

      Timer timer;
      // We benchmark 5% to 30% valid values in 5% increments
      for (size_t p = 5; p <= 30; p += 5){
        size_t n_valid = (ARRAY_SIZE * p) / 100;
        size_t modulo = ARRAY_SIZE / n_valid;
        std::vector<Value> values(ARRAY_SIZE, Value());
        for (size_t i = 0; i < values.size(); ++i){
          values[i] = i % modulo == 0 ? TestValue(1, Value()) : Value();
        }
        ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
        IdArrayHandle out_handle;
        timer.Reset();
        Algorithm::StreamCompact(value_handle, out_handle);
        vtkm::Float64 elapsed = timer.GetElapsedTime();
        std::cout << "StreamCompact on " << ARRAY_SIZE << " "
          << " values with " << out_handle.GetNumberOfValues()
          << " valid values took " << elapsed << "s\n";

        std::vector<vtkm::Id> stencil(ARRAY_SIZE, 0);
        for (size_t i = 0; i < stencil.size(); ++i){
          stencil[i] = i % modulo == 0 ? 1 : vtkm::Id();
        }
        IdArrayHandle stencil_handle = vtkm::cont::make_ArrayHandle(stencil);
        ValueArrayHandle out_val_handle;
        timer.Reset();
        Algorithm::StreamCompact(value_handle, stencil_handle, out_val_handle);
        elapsed = timer.GetElapsedTime();
        std::cout << "StreamCompact with stencil on " << ARRAY_SIZE
          << " values with " << out_val_handle.GetNumberOfValues()
          << " valid values took " << elapsed << "s\n";
      }
    }
  };

  struct BenchUnique {
    template<typename Value>
    VTKM_CONT_EXPORT void operator()(const Value vtkmNotUsed(v)) const {
      typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

      Timer timer;
      // We benchmark 5% to 30% valid values in 5% increments
      for (size_t p = 5; p <= 30; p += 5){
        size_t n_valid = (ARRAY_SIZE * p) / 100;
        std::vector<Value> values(ARRAY_SIZE, Value());
        for (size_t i = 0; i < values.size(); ++i){
          values[i] = TestValue(vtkm::Id(i % n_valid), Value());
        }
        ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
        Algorithm::Sort(value_handle);
        timer.Reset();
        Algorithm::Unique(value_handle);
        vtkm::Float64 elapsed = timer.GetElapsedTime();
        std::cout << "Unique on " << ARRAY_SIZE << " values with "
          << value_handle.GetNumberOfValues() << " valid values took "
          << elapsed << "s\n";
      }
    }
  };

  struct BenchUpperBounds {
    template<typename Value>
    VTKM_CONT_EXPORT void operator()(const Value vtkmNotUsed(v)) const {
      typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

      Timer timer;
      std::vector<Value> input(ARRAY_SIZE, Value());
      for (size_t i = 0; i < input.size(); ++i){
        input[i] = TestValue(vtkm::Id(i), Value());
      }
      ValueArrayHandle input_handle = vtkm::cont::make_ArrayHandle(input);

      // We benchmark finding indices for the elements using various
      // ratios of values to input from 5-30% of # of elements in input
      for (size_t p = 5; p <= 30; p += 5){
        size_t n_vals = (ARRAY_SIZE * p) / 100;
        std::vector<Value> values(n_vals, Value());
        for (size_t i = 0; i < values.size(); ++i){
          values[i] = TestValue(vtkm::Id(2 * i), Value());
        }
        ValueArrayHandle value_handle = vtkm::cont::make_ArrayHandle(values);
        IdArrayHandle out_handle;
        timer.Reset();
        Algorithm::UpperBounds(input_handle, value_handle, out_handle);
        vtkm::Float64 elapsed = timer.GetElapsedTime();
        std::cout << "UpperBounds on " << ARRAY_SIZE << " input and "
          << n_vals << " values took " << elapsed << "s\n";
      }
    }
  };

public:

  struct ValueTypes : vtkm::ListTagBase<vtkm::UInt8, vtkm::UInt32, vtkm::Int32,
                                        vtkm::Int64, vtkm::Vec<vtkm::Int32, 2>,
                                        vtkm::Vec<vtkm::UInt8, 4>, vtkm::Float32,
                                        vtkm::Float64, vtkm::Vec<vtkm::Float64, 3>,
                                        vtkm::Vec<vtkm::Float32, 4> >{};


  static VTKM_CONT_EXPORT int Run(){
      std::cout << DIVIDER << "\nRunning DeviceAdapter benchmarks\n";

      std::cout << DIVIDER << "\nBenchmarking LowerBounds\n";
      vtkm::testing::Testing::TryTypes(BenchLowerBounds(), ValueTypes());

      std::cout << "\n" << DIVIDER << "\nBenchmarking Reduce\n";
      vtkm::testing::Testing::TryTypes(BenchReduce(), ValueTypes());

      std::cout << "\n" << DIVIDER << "\nBenchmarking ReduceByKey\n";
      vtkm::testing::Testing::TryTypes(BenchReduceByKey(), ValueTypes());

      std::cout << "\n" << DIVIDER << "\nBenchmarking ScanInclusive\n";
      vtkm::testing::Testing::TryTypes(BenchScanInclusive(), ValueTypes());

      std::cout << "\n" << DIVIDER << "\nBenchmarking ScanExclusive\n";
      vtkm::testing::Testing::TryTypes(BenchScanExclusive(), ValueTypes());

      std::cout << "\n" << DIVIDER << "\nBenchmarking Sort\n";
      vtkm::testing::Testing::TryTypes(BenchSort(), ValueTypes());

      std::cout << "\n" << DIVIDER << "\nBenchmarking SortByKey\n";
      vtkm::testing::Testing::TryTypes(BenchSortByKey(), ValueTypes());

      std::cout << "\n" << DIVIDER << "\nBenchmarking StreamCompact\n";
      vtkm::testing::Testing::TryTypes(BenchStreamCompact(), ValueTypes());

      std::cout << "\n" << DIVIDER << "\nBenchmarking Unique\n";
      vtkm::testing::Testing::TryTypes(BenchUnique(), ValueTypes());

      std::cout << "\n" << DIVIDER << "\nBenchmarking UpperBounds\n";
      vtkm::testing::Testing::TryTypes(BenchUpperBounds(), ValueTypes());
      return 0;
  }
};

#undef ARRAY_SIZE

}
} // namespace vtkm::benchmarking

#endif

