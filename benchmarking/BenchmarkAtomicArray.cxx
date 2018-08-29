//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include "Benchmarker.h"

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/AtomicArray.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/exec/FunctorBase.h>

#include <iomanip>
#include <sstream>
#include <string>

namespace vtkm
{
namespace benchmarking
{

// This is 32x larger than the largest array size.
static constexpr vtkm::Id NumWrites = 33554432; // 2^25

#define MAKE_ATOMIC_BENCHMARKS(Name, Class)                                                        \
  VTKM_MAKE_BENCHMARK(Name##1, Class, 1);                                                          \
  VTKM_MAKE_BENCHMARK(Name##8, Class, 8);                                                          \
  VTKM_MAKE_BENCHMARK(Name##32, Class, 32);                                                        \
  VTKM_MAKE_BENCHMARK(Name##512, Class, 512);                                                      \
  VTKM_MAKE_BENCHMARK(Name##2048, Class, 2048);                                                    \
  VTKM_MAKE_BENCHMARK(Name##32768, Class, 32768);                                                  \
  VTKM_MAKE_BENCHMARK(Name##1048576, Class, 1048576)

#define RUN_ATOMIC_BENCHMARKS(Name)                                                                \
  VTKM_RUN_BENCHMARK(Name##1, vtkm::cont::AtomicArrayTypeListTag{});                               \
  VTKM_RUN_BENCHMARK(Name##8, vtkm::cont::AtomicArrayTypeListTag{});                               \
  VTKM_RUN_BENCHMARK(Name##32, vtkm::cont::AtomicArrayTypeListTag{});                              \
  VTKM_RUN_BENCHMARK(Name##512, vtkm::cont::AtomicArrayTypeListTag{});                             \
  VTKM_RUN_BENCHMARK(Name##2048, vtkm::cont::AtomicArrayTypeListTag{});                            \
  VTKM_RUN_BENCHMARK(Name##32768, vtkm::cont::AtomicArrayTypeListTag{});                           \
  VTKM_RUN_BENCHMARK(Name##1048576, vtkm::cont::AtomicArrayTypeListTag{})

template <class Device>
class BenchmarkAtomicArray
{
public:
  using Algo = vtkm::cont::DeviceAdapterAlgorithm<Device>;
  using Timer = vtkm::cont::Timer<Device>;

  // Benchmarks AtomicArray::Add such that each work index writes to adjacent
  // indices.
  template <typename ValueType>
  struct BenchAddSeq
  {
    vtkm::Id ArraySize;
    vtkm::cont::ArrayHandle<ValueType> Data;

    template <typename PortalType>
    struct Worker : public vtkm::exec::FunctorBase
    {
      vtkm::Id ArraySize;
      PortalType Portal;

      VTKM_CONT
      Worker(vtkm::Id arraySize, PortalType portal)
        : ArraySize(arraySize)
        , Portal(portal)
      {
      }

      VTKM_EXEC
      void operator()(vtkm::Id i) const { this->Portal.Add(i % this->ArraySize, 1); }
    };

    BenchAddSeq(vtkm::Id arraySize)
      : ArraySize(arraySize)
    {
      this->Data.PrepareForOutput(this->ArraySize, Device{});
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::AtomicArray<ValueType> array(this->Data);
      auto portal = array.PrepareForExecution(Device{});
      Worker<decltype(portal)> worker{ this->ArraySize, portal };

      Timer timer;
      Algo::Schedule(worker, NumWrites);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "Add (Seq, Atomic, " << std::setw(7) << std::setfill('0') << this->ArraySize << ")";
      return desc.str();
    }
  };
  MAKE_ATOMIC_BENCHMARKS(AddSeq, BenchAddSeq);

  // Provides a non-atomic baseline for BenchAddSeq
  template <typename ValueType>
  struct BenchAddSeqBaseline
  {
    vtkm::Id ArraySize;
    vtkm::cont::ArrayHandle<ValueType> Data;

    template <typename PortalType>
    struct Worker : public vtkm::exec::FunctorBase
    {
      vtkm::Id ArraySize;
      PortalType Portal;

      VTKM_CONT
      Worker(vtkm::Id arraySize, PortalType portal)
        : ArraySize(arraySize)
        , Portal(portal)
      {
      }

      VTKM_EXEC
      void operator()(vtkm::Id i) const
      {
        vtkm::Id idx = i % this->ArraySize;
        this->Portal.Set(idx, this->Portal.Get(idx) + 1);
      }
    };

    BenchAddSeqBaseline(vtkm::Id arraySize)
      : ArraySize(arraySize)
    {
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      auto portal = this->Data.PrepareForOutput(this->ArraySize, Device{});
      Worker<decltype(portal)> worker{ this->ArraySize, portal };

      Timer timer;
      Algo::Schedule(worker, NumWrites);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "Add (Seq, Baseline, " << std::setw(7) << std::setfill('0') << this->ArraySize << ")";
      return desc.str();
    }
  };
  MAKE_ATOMIC_BENCHMARKS(AddSeqBase, BenchAddSeqBaseline);

  // Benchmarks AtomicArray::Add such that each work index writes to a strided
  // index ( floor(i / stride) + stride * (i % stride)
  template <typename ValueType>
  struct BenchAddStride
  {
    vtkm::Id ArraySize;
    vtkm::Id Stride;
    vtkm::cont::ArrayHandle<ValueType> Data;

    template <typename PortalType>
    struct Worker : public vtkm::exec::FunctorBase
    {
      vtkm::Id ArraySize;
      vtkm::Id Stride;
      PortalType Portal;

      VTKM_CONT
      Worker(vtkm::Id arraySize, vtkm::Id stride, PortalType portal)
        : ArraySize(arraySize)
        , Stride(stride)
        , Portal(portal)
      {
      }

      VTKM_EXEC
      void operator()(vtkm::Id i) const
      {
        vtkm::Id idx = (i / this->Stride + this->Stride * (i % this->Stride)) % this->ArraySize;
        this->Portal.Add(idx % this->ArraySize, 1);
      }
    };

    BenchAddStride(vtkm::Id arraySize, vtkm::Id stride = 32)
      : ArraySize(arraySize)
      , Stride(stride)
    {
      this->Data.PrepareForOutput(this->ArraySize, Device{});
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::AtomicArray<ValueType> array(this->Data);
      auto portal = array.PrepareForExecution(Device{});
      Worker<decltype(portal)> worker{ this->ArraySize, this->Stride, portal };

      Timer timer;
      Algo::Schedule(worker, NumWrites);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "Add (Stride=" << this->Stride << ", Atomic, " << std::setw(7) << std::setfill('0')
           << this->ArraySize << ")";
      return desc.str();
    }
  };
  MAKE_ATOMIC_BENCHMARKS(AddStride, BenchAddStride);

  // Non-atomic baseline for AddStride
  template <typename ValueType>
  struct BenchAddStrideBaseline
  {
    vtkm::Id ArraySize;
    vtkm::Id Stride;
    vtkm::cont::ArrayHandle<ValueType> Data;

    template <typename PortalType>
    struct Worker : public vtkm::exec::FunctorBase
    {
      vtkm::Id ArraySize;
      vtkm::Id Stride;
      PortalType Portal;

      VTKM_CONT
      Worker(vtkm::Id arraySize, vtkm::Id stride, PortalType portal)
        : ArraySize(arraySize)
        , Stride(stride)
        , Portal(portal)
      {
      }

      VTKM_EXEC
      void operator()(vtkm::Id i) const
      {
        vtkm::Id idx = (i / this->Stride + this->Stride * (i % this->Stride)) % this->ArraySize;
        this->Portal.Set(idx, this->Portal.Get(idx) + 1);
      }
    };

    BenchAddStrideBaseline(vtkm::Id arraySize, vtkm::Id stride = 32)
      : ArraySize(arraySize)
      , Stride(stride)
    {
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      auto portal = this->Data.PrepareForOutput(this->ArraySize, Device{});
      Worker<decltype(portal)> worker{ this->ArraySize, this->Stride, portal };

      Timer timer;
      Algo::Schedule(worker, NumWrites);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "Add (Stride=" << this->Stride << ", Baseline, " << std::setw(7) << std::setfill('0')
           << this->ArraySize << ")";
      return desc.str();
    }
  };
  MAKE_ATOMIC_BENCHMARKS(AddStrideBase, BenchAddStrideBaseline);

  // Benchmarks AtomicArray::CompareAndSwap such that each work index writes to adjacent
  // indices.
  template <typename ValueType>
  struct BenchCASSeq
  {
    vtkm::Id ArraySize;
    vtkm::cont::ArrayHandle<ValueType> Data;

    template <typename PortalType>
    struct Worker : public vtkm::exec::FunctorBase
    {
      vtkm::Id ArraySize;
      PortalType Portal;

      VTKM_CONT
      Worker(vtkm::Id arraySize, PortalType portal)
        : ArraySize(arraySize)
        , Portal(portal)
      {
      }

      VTKM_EXEC
      void operator()(vtkm::Id i) const
      {
        vtkm::Id idx = i % this->ArraySize;
        ValueType val = static_cast<ValueType>(i);
        // Get the old val with a no-op
        ValueType oldVal = this->Portal.Add(idx, static_cast<ValueType>(0));
        ValueType assumed = static_cast<ValueType>(0);
        do
        {
          assumed = oldVal;
          oldVal = this->Portal.CompareAndSwap(idx, assumed + val, assumed);
        } while (assumed != oldVal);
      }
    };

    BenchCASSeq(vtkm::Id arraySize)
      : ArraySize(arraySize)
    {
      this->Data.PrepareForOutput(this->ArraySize, Device{});
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::AtomicArray<ValueType> array(this->Data);
      auto portal = array.PrepareForExecution(Device{});
      Worker<decltype(portal)> worker{ this->ArraySize, portal };

      Timer timer;
      Algo::Schedule(worker, NumWrites);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "CAS (Seq, Atomic, " << std::setw(7) << std::setfill('0') << this->ArraySize << ")";
      return desc.str();
    }
  };
  MAKE_ATOMIC_BENCHMARKS(CASSeq, BenchCASSeq);

  // Provides a non-atomic baseline for BenchCASSeq
  template <typename ValueType>
  struct BenchCASSeqBaseline
  {
    vtkm::Id ArraySize;
    vtkm::cont::ArrayHandle<ValueType> Data;

    template <typename PortalType>
    struct Worker : public vtkm::exec::FunctorBase
    {
      vtkm::Id ArraySize;
      PortalType Portal;

      VTKM_CONT
      Worker(vtkm::Id arraySize, PortalType portal)
        : ArraySize(arraySize)
        , Portal(portal)
      {
      }

      VTKM_EXEC
      void operator()(vtkm::Id i) const
      {
        vtkm::Id idx = i % this->ArraySize;
        ValueType val = static_cast<ValueType>(i);
        ValueType oldVal = this->Portal.Get(idx);
        this->Portal.Set(idx, oldVal + val);
      }
    };

    BenchCASSeqBaseline(vtkm::Id arraySize)
      : ArraySize(arraySize)
    {
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      auto portal = this->Data.PrepareForOutput(this->ArraySize, Device{});
      Worker<decltype(portal)> worker{ this->ArraySize, portal };

      Timer timer;
      Algo::Schedule(worker, NumWrites);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "CAS (Seq, Baseline, " << std::setw(7) << std::setfill('0') << this->ArraySize << ")";
      return desc.str();
    }
  };
  MAKE_ATOMIC_BENCHMARKS(CASSeqBase, BenchCASSeqBaseline);

  // Benchmarks AtomicArray::CompareAndSwap such that each work index writes to
  // a strided index:
  // ( floor(i / stride) + stride * (i % stride)
  template <typename ValueType>
  struct BenchCASStride
  {
    vtkm::Id ArraySize;
    vtkm::Id Stride;
    vtkm::cont::ArrayHandle<ValueType> Data;

    template <typename PortalType>
    struct Worker : public vtkm::exec::FunctorBase
    {
      vtkm::Id ArraySize;
      vtkm::Id Stride;
      PortalType Portal;

      VTKM_CONT
      Worker(vtkm::Id arraySize, vtkm::Id stride, PortalType portal)
        : ArraySize(arraySize)
        , Stride(stride)
        , Portal(portal)
      {
      }

      VTKM_EXEC
      void operator()(vtkm::Id i) const
      {
        vtkm::Id idx = (i / this->Stride + this->Stride * (i % this->Stride)) % this->ArraySize;
        ValueType val = static_cast<ValueType>(i);
        // Get the old val with a no-op
        ValueType oldVal = this->Portal.Add(idx, static_cast<ValueType>(0));
        ValueType assumed = static_cast<ValueType>(0);
        do
        {
          assumed = oldVal;
          oldVal = this->Portal.CompareAndSwap(idx, assumed + val, assumed);
        } while (assumed != oldVal);
      }
    };

    BenchCASStride(vtkm::Id arraySize, vtkm::Id stride = 32)
      : ArraySize(arraySize)
      , Stride(stride)
    {
      this->Data.PrepareForOutput(this->ArraySize, Device{});
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::AtomicArray<ValueType> array(this->Data);
      auto portal = array.PrepareForExecution(Device{});
      Worker<decltype(portal)> worker{ this->ArraySize, this->Stride, portal };

      Timer timer;
      Algo::Schedule(worker, NumWrites);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "CAS (Stride=" << this->Stride << ", Atomic, " << std::setw(7) << std::setfill('0')
           << this->ArraySize << ")";
      return desc.str();
    }
  };
  MAKE_ATOMIC_BENCHMARKS(CASStride, BenchCASStride);

  // Non-atomic baseline for CASStride
  template <typename ValueType>
  struct BenchCASStrideBaseline
  {
    vtkm::Id ArraySize;
    vtkm::Id Stride;
    vtkm::cont::ArrayHandle<ValueType> Data;

    template <typename PortalType>
    struct Worker : public vtkm::exec::FunctorBase
    {
      vtkm::Id ArraySize;
      vtkm::Id Stride;
      PortalType Portal;

      VTKM_CONT
      Worker(vtkm::Id arraySize, vtkm::Id stride, PortalType portal)
        : ArraySize(arraySize)
        , Stride(stride)
        , Portal(portal)
      {
      }

      VTKM_EXEC
      void operator()(vtkm::Id i) const
      {
        vtkm::Id idx = (i / this->Stride + this->Stride * (i % this->Stride)) % this->ArraySize;
        ValueType val = static_cast<ValueType>(i);
        ValueType oldVal = this->Portal.Get(idx);
        this->Portal.Set(idx, oldVal + val);
      }
    };

    BenchCASStrideBaseline(vtkm::Id arraySize, vtkm::Id stride = 32)
      : ArraySize(arraySize)
      , Stride(stride)
    {
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      auto portal = this->Data.PrepareForOutput(this->ArraySize, Device{});
      Worker<decltype(portal)> worker{ this->ArraySize, this->Stride, portal };

      Timer timer;
      Algo::Schedule(worker, NumWrites);
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "CAS (Stride=" << this->Stride << ", Baseline, " << std::setw(7) << std::setfill('0')
           << this->ArraySize << ")";
      return desc.str();
    }
  };
  MAKE_ATOMIC_BENCHMARKS(CASStrideBase, BenchCASStrideBaseline);

  static void Run()
  {
    RUN_ATOMIC_BENCHMARKS(AddSeq);
    RUN_ATOMIC_BENCHMARKS(AddSeqBase);
    RUN_ATOMIC_BENCHMARKS(AddStride);
    RUN_ATOMIC_BENCHMARKS(AddStrideBase);

    RUN_ATOMIC_BENCHMARKS(CASSeq);
    RUN_ATOMIC_BENCHMARKS(CASSeqBase);
    RUN_ATOMIC_BENCHMARKS(CASStride);
    RUN_ATOMIC_BENCHMARKS(CASStrideBase);
  }
};
}
} // end namespace vtkm::benchmarking

int main(int, char* [])
{
  using Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  auto tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker();
  tracker.ForceDevice(Device{});

  try
  {
    vtkm::benchmarking::BenchmarkAtomicArray<Device>::Run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Benchmark encountered an exception: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
