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
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <sstream>
#include <string>
#include <vector>

namespace
{

// Make this global so benchmarks can access the current device id:
vtkm::cont::InitializeResult Config;

const vtkm::UInt64 COPY_SIZE_MIN = (1 << 10); // 1 KiB
const vtkm::UInt64 COPY_SIZE_MAX = (1 << 30); // 1 GiB

using TestTypes = vtkm::List<vtkm::Float32>;

//------------- Functors for benchmarks --------------------------------------

// Reads all values in ArrayHandle.
struct ReadValues : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn);

  template <typename T>
  VTKM_EXEC void operator()(const T& val) const
  {
    if (val < 0)
    {
      // We don't really do anything with this, we just need to do *something*
      // to prevent the compiler from optimizing out the array accesses.
      this->RaiseError("Unexpected value.");
    }
  }
};

// Writes values to ArrayHandle.
struct WriteValues : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldOut);
  using ExecutionSignature = void(_1, InputIndex);

  template <typename T>
  VTKM_EXEC void operator()(T& val, vtkm::Id idx) const
  {
    val = static_cast<T>(idx);
  }
};

// Reads and writes values to ArrayHandle.
struct ReadWriteValues : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1, InputIndex);

  template <typename T>
  VTKM_EXEC void operator()(T& val, vtkm::Id idx) const
  {
    val += static_cast<T>(idx);
  }
};

//------------- Benchmark functors -------------------------------------------

// Copies NumValues from control environment to execution environment and
// accesses them as read-only.
template <typename ValueType>
void BenchContToExecRead(benchmark::State& state)
{
  using ArrayType = vtkm::cont::ArrayHandle<ValueType>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::UInt64 numBytes = static_cast<vtkm::UInt64>(state.range(0));
  const vtkm::Id numValues = static_cast<vtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Control --> Execution (read-only): " << numValues << " values ("
         << vtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues));
  ArrayType array = vtkm::cont::make_ArrayHandle(vec);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(ReadValues{}, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchContToExecRead,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Writes values to ArrayHandle in execution environment. There is no actual
// copy between control/execution in this case.
template <typename ValueType>
void BenchContToExecWrite(benchmark::State& state)
{
  using ArrayType = vtkm::cont::ArrayHandle<ValueType>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::UInt64 numBytes = static_cast<vtkm::UInt64>(state.range(0));
  const vtkm::Id numValues = static_cast<vtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Control --> Execution (write-only): " << numValues << " values ("
         << vtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  ArrayType array;
  array.Allocate(numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(WriteValues{}, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchContToExecWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Copies NumValues from control environment to execution environment and
// both reads and writes them.
template <typename ValueType>
void BenchContToExecReadWrite(benchmark::State& state)
{
  using ArrayType = vtkm::cont::ArrayHandle<ValueType>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::UInt64 numBytes = static_cast<vtkm::UInt64>(state.range(0));
  const vtkm::Id numValues = static_cast<vtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Control --> Execution (read-write): " << numValues << " values ("
         << vtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues));
  ArrayType array = vtkm::cont::make_ArrayHandle(vec);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(ReadWriteValues{}, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchContToExecReadWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Copies NumValues from control environment to execution environment and
// back, then accesses them as read-only.
template <typename ValueType>
void BenchRoundTripRead(benchmark::State& state)
{
  using ArrayType = vtkm::cont::ArrayHandle<ValueType>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::UInt64 numBytes = static_cast<vtkm::UInt64>(state.range(0));
  const vtkm::Id numValues = static_cast<vtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Control --> Execution --> Control (read-only): " << numValues
         << " values (" << vtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues));
  ArrayType array = vtkm::cont::make_ArrayHandle(vec);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Ensure data is in control before we start:
    array.ReleaseResourcesExecution();

    timer.Start();
    invoker(ReadValues{}, array);

    // Copy back to host and read:
    auto portal = array.GetPortalConstControl();
    for (vtkm::Id i = 0; i < numValues; ++i)
    {
      benchmark::DoNotOptimize(portal.Get(i));
    }

    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchRoundTripRead,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Copies NumValues from control environment to execution environment and
// back, then reads and writes them in-place.
template <typename ValueType>
void BenchRoundTripReadWrite(benchmark::State& state)
{
  using ArrayType = vtkm::cont::ArrayHandle<ValueType>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::UInt64 numBytes = static_cast<vtkm::UInt64>(state.range(0));
  const vtkm::Id numValues = static_cast<vtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Control --> Execution --> Control (read-write): " << numValues
         << " values (" << vtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues));
  ArrayType array = vtkm::cont::make_ArrayHandle(vec);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Ensure data is in control before we start:
    array.ReleaseResourcesExecution();

    timer.Start();

    // Do work on device:
    invoker(ReadWriteValues{}, array);

    auto portal = array.GetPortalControl();
    for (vtkm::Id i = 0; i < numValues; ++i)
    {
      portal.Set(i, portal.Get(i) - static_cast<ValueType>(i));
    }

    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchRoundTripReadWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Write NumValues to device allocated memory and copies them back to control
// for reading.
template <typename ValueType>
void BenchExecToContRead(benchmark::State& state)
{
  using ArrayType = vtkm::cont::ArrayHandle<ValueType>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::UInt64 numBytes = static_cast<vtkm::UInt64>(state.range(0));
  const vtkm::Id numValues = static_cast<vtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Execution --> Control (read-only on control): " << numValues
         << " values (" << vtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  ArrayType array;
  array.Allocate(numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    // Time the copy:
    timer.Start();

    // Allocate/write data on device
    invoker(WriteValues{}, array);

    // Read back on host:
    auto portal = array.GetPortalControl();
    for (vtkm::Id i = 0; i < numValues; ++i)
    {
      benchmark::DoNotOptimize(portal.Get(i));
    }

    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
};
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchExecToContRead,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Write NumValues to device allocated memory and copies them back to control
// and overwrites them.
template <typename ValueType>
void BenchExecToContWrite(benchmark::State& state)
{
  using ArrayType = vtkm::cont::ArrayHandle<ValueType>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::UInt64 numBytes = static_cast<vtkm::UInt64>(state.range(0));
  const vtkm::Id numValues = static_cast<vtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Execution --> Control (write-only on control): " << numValues
         << " values (" << vtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  ArrayType array;
  array.Allocate(numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();

    // Allocate/write data on device
    invoker(WriteValues{}, array);

    // Read back on host:
    auto portal = array.GetPortalControl();
    for (vtkm::Id i = 0; i < numValues; ++i)
    {
      portal.Set(i, portal.Get(i) - static_cast<ValueType>(i));
    }

    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchExecToContWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Write NumValues to device allocated memory and copies them back to control
// for reading and writing.
template <typename ValueType>
void BenchExecToContReadWrite(benchmark::State& state)
{
  using ArrayType = vtkm::cont::ArrayHandle<ValueType>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::UInt64 numBytes = static_cast<vtkm::UInt64>(state.range(0));
  const vtkm::Id numValues = static_cast<vtkm::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Execution --> Control (read-write on control): " << numValues
         << " values (" << vtkm::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  ArrayType array;
  array.Allocate(numValues);

  vtkm::cont::Invoker invoker{ device };
  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();

    // Allocate/write data on device
    invoker(WriteValues{}, array);

    // Read back on host:
    auto portal = array.GetPortalControl();
    for (vtkm::Id i = 0; i < numValues; ++i)
    {
      benchmark::DoNotOptimize(portal.Get(i));
    }

    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
VTKM_BENCHMARK_TEMPLATES_OPTS(BenchExecToContReadWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

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
