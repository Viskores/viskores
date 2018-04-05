//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
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
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/exec/FunctorBase.h>

#include <sstream>
#include <string>
#include <vector>

// 256 MB of floats:
const vtkm::Id ARRAY_SIZE = 256 * 1024 * 1024 / 4;

namespace vtkm
{
namespace benchmarking
{

template <typename DeviceAdapter>
struct BenchmarkArrayTransfer
{
  using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
  using StorageTag = vtkm::cont::StorageTagBasic;
  using Timer = vtkm::cont::Timer<DeviceAdapter>;

  //------------- Functors for benchmarks --------------------------------------

  // Reads all values in ArrayHandle.
  template <typename PortalType>
  struct ReadValues : vtkm::exec::FunctorBase
  {
    using ValueType = typename PortalType::ValueType;
    using ArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;

    PortalType Portal;
    const ValueType MinValue;

    VTKM_CONT
    ReadValues(const PortalType& portal, const ValueType& minValue)
      : Portal(portal)
      , MinValue(minValue)
    {
    }

    VTKM_EXEC
    void operator()(vtkm::Id i) const
    {
      if (this->Portal.Get(i) < this->MinValue)
      {
        // We don't really do anything with this, we just need to do *something*
        // to prevent the compiler from optimizing out the array accesses.
        this->RaiseError("Unexpected value.");
      }
    }

    // unused int argument is simply to distinguish this method from the
    // VTKM_EXEC overload (VTKM_EXEC_CONT won't work here because of the
    // RaiseError call).
    VTKM_CONT
    void operator()(vtkm::Id i, int) const
    {
      if (this->Portal.Get(i) < this->MinValue)
      {
        // We don't really do anything with this, we just need to do *something*
        // to prevent the compiler from optimizing out the array accesses.
        std::cerr << "Unexpected value.\n";
      }
    }
  };

  // Writes values to ArrayHandle.
  template <typename PortalType>
  struct WriteValues : vtkm::exec::FunctorBase
  {
    using ValueType = typename PortalType::ValueType;
    using ArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;

    PortalType Portal;

    VTKM_CONT
    WriteValues(const PortalType& portal)
      : Portal(portal)
    {
    }

    VTKM_EXEC_CONT
    void operator()(vtkm::Id i) const { this->Portal.Set(i, static_cast<ValueType>(i)); }
  };

  // Reads and writes values to ArrayHandle.
  template <typename PortalType>
  struct ReadWriteValues : vtkm::exec::FunctorBase
  {
    using ValueType = typename PortalType::ValueType;
    using ArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;

    PortalType Portal;

    VTKM_CONT
    ReadWriteValues(const PortalType& portal)
      : Portal(portal)
    {
    }

    VTKM_EXEC_CONT
    void operator()(vtkm::Id i) const
    {
      ValueType val = this->Portal.Get(i);
      val += static_cast<ValueType>(i);
      this->Portal.Set(i, val);
    }
  };

  //------------- Benchmark functors -------------------------------------------

  // Copies NumValues from control environment to execution environment and
  // accesses them as read-only.
  template <typename ValueType>
  struct BenchContToExecRead
  {
    using ArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
    using PortalType = typename ArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;
    using ValueTypeTraits = vtkm::TypeTraits<ValueType>;

    vtkm::Id NumValues;

    VTKM_CONT
    BenchContToExecRead(vtkm::Id numValues)
      : NumValues(numValues)
    {
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream out;
      out << "Copying from Control --> Execution (read-only): " << this->NumValues << " values ("
          << (this->NumValues * static_cast<vtkm::Id>(sizeof(ValueType))) << " bytes)";
      return out.str();
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      std::vector<ValueType> vec(static_cast<std::size_t>(this->NumValues),
                                 ValueTypeTraits::ZeroInitialization());
      ArrayType array = vtkm::cont::make_ArrayHandle(vec);

      // Time the copy:
      Timer timer;
      ReadValues<PortalType> functor(array.PrepareForInput(DeviceAdapter()),
                                     ValueTypeTraits::ZeroInitialization());
      Algo::Schedule(functor, this->NumValues);
      return timer.GetElapsedTime();
    }
  };
  VTKM_MAKE_BENCHMARK(ContToExecRead, BenchContToExecRead, ARRAY_SIZE);

  // Writes values to ArrayHandle in execution environment. There is no actual
  // copy between control/execution in this case.
  template <typename ValueType>
  struct BenchContToExecWrite
  {
    using ArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
    using PortalType = typename ArrayType::template ExecutionTypes<DeviceAdapter>::Portal;
    using ValueTypeTraits = vtkm::TypeTraits<ValueType>;

    vtkm::Id NumValues;

    VTKM_CONT
    BenchContToExecWrite(vtkm::Id numValues)
      : NumValues(numValues)
    {
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream out;
      out << "Copying from Control --> Execution (write-only): " << this->NumValues << " values ("
          << (this->NumValues * static_cast<vtkm::Id>(sizeof(ValueType))) << " bytes)";
      return out.str();
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      ArrayType array;

      // Time the write:
      Timer timer;
      WriteValues<PortalType> functor(array.PrepareForOutput(this->NumValues, DeviceAdapter()));
      Algo::Schedule(functor, this->NumValues);
      return timer.GetElapsedTime();
    }
  };
  VTKM_MAKE_BENCHMARK(ContToExecWrite, BenchContToExecWrite, ARRAY_SIZE);

  // Copies NumValues from control environment to execution environment and
  // both reads and writes them.
  template <typename ValueType>
  struct BenchContToExecReadWrite
  {
    using ArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
    using PortalType = typename ArrayType::template ExecutionTypes<DeviceAdapter>::Portal;
    using ValueTypeTraits = vtkm::TypeTraits<ValueType>;

    vtkm::Id NumValues;

    VTKM_CONT
    BenchContToExecReadWrite(vtkm::Id numValues)
      : NumValues(numValues)
    {
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream out;
      out << "Copying from Control --> Execution (read-write): " << this->NumValues << " values ("
          << (this->NumValues * static_cast<vtkm::Id>(sizeof(ValueType))) << " bytes)";
      return out.str();
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      std::vector<ValueType> vec(static_cast<std::size_t>(this->NumValues),
                                 ValueTypeTraits::ZeroInitialization());
      ArrayType array = vtkm::cont::make_ArrayHandle(vec);

      // Time the copy:
      Timer timer;
      ReadWriteValues<PortalType> functor(array.PrepareForInPlace(DeviceAdapter()));
      Algo::Schedule(functor, this->NumValues);
      return timer.GetElapsedTime();
    }
  };
  VTKM_MAKE_BENCHMARK(ContToExecReadWrite, BenchContToExecReadWrite, ARRAY_SIZE);

  // Copies NumValues from control environment to execution environment and
  // back, then accesses them as read-only.
  template <typename ValueType>
  struct BenchRoundTripRead
  {
    using ArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
    using PortalContType = typename ArrayType::PortalConstControl;
    using PortalExecType = typename ArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;
    using ValueTypeTraits = vtkm::TypeTraits<ValueType>;

    vtkm::Id NumValues;

    VTKM_CONT
    BenchRoundTripRead(vtkm::Id numValues)
      : NumValues(numValues)
    {
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream out;
      out << "Copying from Control --> Execution --> Control (read-only): " << this->NumValues
          << " values (" << (this->NumValues * static_cast<vtkm::Id>(sizeof(ValueType)))
          << " bytes)";
      return out.str();
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      std::vector<ValueType> vec(static_cast<std::size_t>(this->NumValues),
                                 ValueTypeTraits::ZeroInitialization());
      ArrayType array = vtkm::cont::make_ArrayHandle(vec);

      // Ensure data is in control before we start:
      array.ReleaseResourcesExecution();

      // Time the copy:
      Timer timer;

      // Copy to device:
      ReadValues<PortalExecType> functor(array.PrepareForInput(DeviceAdapter()),
                                         ValueTypeTraits::ZeroInitialization());
      Algo::Schedule(functor, this->NumValues);

      // Copy back to host and read:
      ReadValues<PortalContType> cFunctor(array.GetPortalConstControl(),
                                          ValueTypeTraits::ZeroInitialization());
      for (vtkm::Id i = 0; i < this->NumValues; ++i)
      {
        cFunctor(i, 0);
      }

      return timer.GetElapsedTime();
    }
  };
  VTKM_MAKE_BENCHMARK(RoundTripRead, BenchRoundTripRead, ARRAY_SIZE);

  // Copies NumValues from control environment to execution environment and
  // back, then reads and writes them in-place.
  template <typename ValueType>
  struct BenchRoundTripReadWrite
  {
    using ArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
    using PortalContType = typename ArrayType::PortalControl;
    using PortalExecType = typename ArrayType::template ExecutionTypes<DeviceAdapter>::Portal;
    using ValueTypeTraits = vtkm::TypeTraits<ValueType>;

    vtkm::Id NumValues;

    VTKM_CONT
    BenchRoundTripReadWrite(vtkm::Id numValues)
      : NumValues(numValues)
    {
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream out;
      out << "Copying from Control --> Execution --> Control (read-write): " << this->NumValues
          << " values (" << (this->NumValues * static_cast<vtkm::Id>(sizeof(ValueType)))
          << " bytes)";
      return out.str();
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      std::vector<ValueType> vec(static_cast<std::size_t>(this->NumValues),
                                 ValueTypeTraits::ZeroInitialization());
      ArrayType array = vtkm::cont::make_ArrayHandle(vec);

      // Ensure data is in control before we start:
      array.ReleaseResourcesExecution();

      // Time the copy:
      Timer timer;

      // Do work on device:
      ReadWriteValues<PortalExecType> functor(array.PrepareForInPlace(DeviceAdapter()));
      Algo::Schedule(functor, this->NumValues);

      ReadWriteValues<PortalContType> cFunctor(array.GetPortalControl());
      for (vtkm::Id i = 0; i < this->NumValues; ++i)
      {
        cFunctor(i);
      }

      return timer.GetElapsedTime();
    }
  };
  VTKM_MAKE_BENCHMARK(RoundTripReadWrite, BenchRoundTripReadWrite, ARRAY_SIZE);

  // Write NumValues to device allocated memory and copies them back to control
  // for reading.
  template <typename ValueType>
  struct BenchExecToContRead
  {
    using ArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
    using PortalContType = typename ArrayType::PortalConstControl;
    using PortalExecType = typename ArrayType::template ExecutionTypes<DeviceAdapter>::Portal;
    using ValueTypeTraits = vtkm::TypeTraits<ValueType>;

    vtkm::Id NumValues;

    VTKM_CONT
    BenchExecToContRead(vtkm::Id numValues)
      : NumValues(numValues)
    {
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream out;
      out << "Copying from Execution --> Control (read-only on control): " << this->NumValues
          << " values (" << (this->NumValues * static_cast<vtkm::Id>(sizeof(ValueType)))
          << " bytes)";
      return out.str();
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      ArrayType array;

      // Time the copy:
      Timer timer;

      // Allocate/write data on device
      WriteValues<PortalExecType> functor(array.PrepareForOutput(this->NumValues, DeviceAdapter()));
      Algo::Schedule(functor, this->NumValues);

      // Read back on host:
      ReadValues<PortalContType> cFunctor(array.GetPortalConstControl(),
                                          ValueTypeTraits::ZeroInitialization());
      for (vtkm::Id i = 0; i < this->NumValues; ++i)
      {
        cFunctor(i, 0);
      }

      return timer.GetElapsedTime();
    }
  };
  VTKM_MAKE_BENCHMARK(ExecToContRead, BenchExecToContRead, ARRAY_SIZE);

  // Write NumValues to device allocated memory and copies them back to control
  // and overwrites them.
  template <typename ValueType>
  struct BenchExecToContWrite
  {
    using ArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
    using PortalContType = typename ArrayType::PortalControl;
    using PortalExecType = typename ArrayType::template ExecutionTypes<DeviceAdapter>::Portal;
    using ValueTypeTraits = vtkm::TypeTraits<ValueType>;

    vtkm::Id NumValues;

    VTKM_CONT
    BenchExecToContWrite(vtkm::Id numValues)
      : NumValues(numValues)
    {
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream out;
      out << "Copying from Execution --> Control (write-only on control): " << this->NumValues
          << " values (" << (this->NumValues * static_cast<vtkm::Id>(sizeof(ValueType)))
          << " bytes)";
      return out.str();
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      ArrayType array;

      // Time the copy:
      Timer timer;

      // Allocate/write data on device
      WriteValues<PortalExecType> functor(array.PrepareForOutput(this->NumValues, DeviceAdapter()));
      Algo::Schedule(functor, this->NumValues);

      // Read back on host:
      WriteValues<PortalContType> cFunctor(array.GetPortalControl());
      for (vtkm::Id i = 0; i < this->NumValues; ++i)
      {
        cFunctor(i);
      }

      return timer.GetElapsedTime();
    }
  };
  VTKM_MAKE_BENCHMARK(ExecToContWrite, BenchExecToContWrite, ARRAY_SIZE);

  // Write NumValues to device allocated memory and copies them back to control
  // for reading and writing.
  template <typename ValueType>
  struct BenchExecToContReadWrite
  {
    using ArrayType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
    using PortalContType = typename ArrayType::PortalControl;
    using PortalExecType = typename ArrayType::template ExecutionTypes<DeviceAdapter>::Portal;
    using ValueTypeTraits = vtkm::TypeTraits<ValueType>;

    vtkm::Id NumValues;

    VTKM_CONT
    BenchExecToContReadWrite(vtkm::Id numValues)
      : NumValues(numValues)
    {
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream out;
      out << "Copying from Execution --> Control (read-write on control): " << this->NumValues
          << " values (" << (this->NumValues * static_cast<vtkm::Id>(sizeof(ValueType)))
          << " bytes)";
      return out.str();
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      ArrayType array;

      // Time the copy:
      Timer timer;

      // Allocate/write data on device
      WriteValues<PortalExecType> functor(array.PrepareForOutput(this->NumValues, DeviceAdapter()));
      Algo::Schedule(functor, this->NumValues);

      // Read back on host:
      ReadWriteValues<PortalContType> cFunctor(array.GetPortalControl());
      for (vtkm::Id i = 0; i < this->NumValues; ++i)
      {
        cFunctor(i);
      }

      return timer.GetElapsedTime();
    }
  };
  VTKM_MAKE_BENCHMARK(ExecToContReadWrite, BenchExecToContReadWrite, ARRAY_SIZE);

  //----------- Benchmark caller -----------------------------------------------

  using TestTypes = vtkm::ListTagBase<vtkm::Float32>;

  static VTKM_CONT bool Run()
  {
    VTKM_RUN_BENCHMARK(ContToExecRead, TestTypes());
    VTKM_RUN_BENCHMARK(ContToExecWrite, TestTypes());
    VTKM_RUN_BENCHMARK(ContToExecReadWrite, TestTypes());
    VTKM_RUN_BENCHMARK(RoundTripRead, TestTypes());
    VTKM_RUN_BENCHMARK(RoundTripReadWrite, TestTypes());
    VTKM_RUN_BENCHMARK(ExecToContRead, TestTypes());
    VTKM_RUN_BENCHMARK(ExecToContWrite, TestTypes());
    VTKM_RUN_BENCHMARK(ExecToContReadWrite, TestTypes());

    return true;
  }
};
}
} // end namespace vtkm::benchmarking

int main(int, char* [])
{
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  using Benchmarks = vtkm::benchmarking::BenchmarkArrayTransfer<DeviceAdapter>;
  bool result = Benchmarks::Run();
  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
