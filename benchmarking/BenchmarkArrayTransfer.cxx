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

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/Timer.h>

#include <viskores/worklet/WorkletMapField.h>

#include <sstream>
#include <string>
#include <vector>

namespace
{

// Make this global so benchmarks can access the current device id:
viskores::cont::InitializeResult Config;

const viskores::UInt64 COPY_SIZE_MIN = (1 << 10); // 1 KiB
const viskores::UInt64 COPY_SIZE_MAX = (1 << 30); // 1 GiB

using TestTypes = viskores::List<viskores::Float32>;

//------------- Functors for benchmarks --------------------------------------

// Reads all values in ArrayHandle.
struct ReadValues : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn);

  template <typename T>
  VISKORES_EXEC void operator()(const T& val) const
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
struct WriteValues : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldOut);
  using ExecutionSignature = void(_1, InputIndex);

  template <typename T>
  VISKORES_EXEC void operator()(T& val, viskores::Id idx) const
  {
    val = static_cast<T>(idx);
  }
};

// Reads and writes values to ArrayHandle.
struct ReadWriteValues : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1, InputIndex);

  template <typename T>
  VISKORES_EXEC void operator()(T& val, viskores::Id idx) const
  {
    val += static_cast<T>(idx);
  }
};

// Takes a vector of data and creates a fresh ArrayHandle with memory just allocated
// in the control environment.
template <typename T>
viskores::cont::ArrayHandle<T> CreateFreshArrayHandle(const std::vector<T>& vec)
{
  return viskores::cont::make_ArrayHandleMove(std::vector<T>(vec));
}

//------------- Benchmark functors -------------------------------------------

// Copies NumValues from control environment to execution environment and
// accesses them as read-only.
template <typename ValueType>
void BenchContToExecRead(benchmark::State& state)
{
  using ArrayType = viskores::cont::ArrayHandle<ValueType>;

  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::UInt64 numBytes = static_cast<viskores::UInt64>(state.range(0));
  const viskores::Id numValues = static_cast<viskores::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Control --> Execution (read-only): " << numValues << " values ("
         << viskores::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues), 2);

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;

    // Make a fresh array each iteration to force a copy from control to execution each time.
    // (Prevents unified memory devices from caching data.)
    ArrayType array = CreateFreshArrayHandle(vec);

    timer.Start();
    invoker(ReadValues{}, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchContToExecRead,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Writes values to ArrayHandle in execution environment. There is no actual
// copy between control/execution in this case.
template <typename ValueType>
void BenchContToExecWrite(benchmark::State& state)
{
  using ArrayType = viskores::cont::ArrayHandle<ValueType>;

  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::UInt64 numBytes = static_cast<viskores::UInt64>(state.range(0));
  const viskores::Id numValues = static_cast<viskores::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Control --> Execution (write-only): " << numValues << " values ("
         << viskores::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  ArrayType array;
  array.Allocate(numValues);

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
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
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchContToExecWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Copies NumValues from control environment to execution environment and
// both reads and writes them.
template <typename ValueType>
void BenchContToExecReadWrite(benchmark::State& state)
{
  using ArrayType = viskores::cont::ArrayHandle<ValueType>;

  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::UInt64 numBytes = static_cast<viskores::UInt64>(state.range(0));
  const viskores::Id numValues = static_cast<viskores::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Control --> Execution (read-write): " << numValues << " values ("
         << viskores::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues), 2);

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;

    // Make a fresh array each iteration to force a copy from control to execution each time.
    // (Prevents unified memory devices from caching data.)
    ArrayType array = CreateFreshArrayHandle(vec);

    timer.Start();
    invoker(ReadWriteValues{}, array);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());

    // Remove data from execution environment so it has to be transferred again.
    array.ReleaseResourcesExecution();
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchContToExecReadWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Copies NumValues from control environment to execution environment and
// back, then accesses them as read-only.
template <typename ValueType>
void BenchRoundTripRead(benchmark::State& state)
{
  using ArrayType = viskores::cont::ArrayHandle<ValueType>;

  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::UInt64 numBytes = static_cast<viskores::UInt64>(state.range(0));
  const viskores::Id numValues = static_cast<viskores::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Control --> Execution --> Control (read-only): " << numValues
         << " values (" << viskores::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues), 2);

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;

    // Make a fresh array each iteration to force a copy from control to execution each time.
    // (Prevents unified memory devices from caching data.)
    ArrayType array = CreateFreshArrayHandle(vec);

    timer.Start();
    invoker(ReadValues{}, array);

    // Copy back to host and read:
    // (Note, this probably does not copy. The array exists in both control and execution for read.)
    auto portal = array.ReadPortal();
    for (viskores::Id i = 0; i < numValues; ++i)
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
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchRoundTripRead,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Copies NumValues from control environment to execution environment and
// back, then reads and writes them in-place.
template <typename ValueType>
void BenchRoundTripReadWrite(benchmark::State& state)
{
  using ArrayType = viskores::cont::ArrayHandle<ValueType>;

  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::UInt64 numBytes = static_cast<viskores::UInt64>(state.range(0));
  const viskores::Id numValues = static_cast<viskores::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Control --> Execution --> Control (read-write): " << numValues
         << " values (" << viskores::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  std::vector<ValueType> vec(static_cast<std::size_t>(numValues));

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;

    // Make a fresh array each iteration to force a copy from control to execution each time.
    // (Prevents unified memory devices from caching data.)
    ArrayType array = CreateFreshArrayHandle(vec);

    timer.Start();

    // Do work on device:
    invoker(ReadWriteValues{}, array);

    // Copy back to host and read/write:
    auto portal = array.WritePortal();
    for (viskores::Id i = 0; i < numValues; ++i)
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
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchRoundTripReadWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Write NumValues to device allocated memory and copies them back to control
// for reading.
template <typename ValueType>
void BenchExecToContRead(benchmark::State& state)
{
  using ArrayType = viskores::cont::ArrayHandle<ValueType>;

  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::UInt64 numBytes = static_cast<viskores::UInt64>(state.range(0));
  const viskores::Id numValues = static_cast<viskores::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Execution --> Control (read-only on control): " << numValues
         << " values (" << viskores::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    ArrayType array;
    array.Allocate(numValues);

    // Time the copy:
    timer.Start();

    // Allocate/write data on device
    invoker(WriteValues{}, array);

    // Read back on host:
    auto portal = array.WritePortal();
    for (viskores::Id i = 0; i < numValues; ++i)
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
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchExecToContRead,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Write NumValues to device allocated memory and copies them back to control
// and overwrites them.
template <typename ValueType>
void BenchExecToContWrite(benchmark::State& state)
{
  using ArrayType = viskores::cont::ArrayHandle<ValueType>;

  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::UInt64 numBytes = static_cast<viskores::UInt64>(state.range(0));
  const viskores::Id numValues = static_cast<viskores::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Execution --> Control (write-only on control): " << numValues
         << " values (" << viskores::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    ArrayType array;
    array.Allocate(numValues);

    timer.Start();

    // Allocate/write data on device
    invoker(WriteValues{}, array);

    // Read back on host:
    auto portal = array.WritePortal();
    for (viskores::Id i = 0; i < numValues; ++i)
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
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchExecToContWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

// Write NumValues to device allocated memory and copies them back to control
// for reading and writing.
template <typename ValueType>
void BenchExecToContReadWrite(benchmark::State& state)
{
  using ArrayType = viskores::cont::ArrayHandle<ValueType>;

  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::UInt64 numBytes = static_cast<viskores::UInt64>(state.range(0));
  const viskores::Id numValues = static_cast<viskores::Id>(numBytes / sizeof(ValueType));

  {
    std::ostringstream desc;
    desc << "Copying from Execution --> Control (read-write on control): " << numValues
         << " values (" << viskores::cont::GetHumanReadableSize(numBytes) << ")";
    state.SetLabel(desc.str());
  }

  viskores::cont::Invoker invoker{ device };
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    ArrayType array;
    array.Allocate(numValues);

    timer.Start();

    // Allocate/write data on device
    invoker(WriteValues{}, array);

    // Read back on host:
    auto portal = array.WritePortal();
    for (viskores::Id i = 0; i < numValues; ++i)
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
VISKORES_BENCHMARK_TEMPLATES_OPTS(BenchExecToContReadWrite,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TestTypes);

} // end anon namespace

int main(int argc, char* argv[])
{
  auto opts = viskores::cont::InitializeOptions::RequireDevice;

  // Initialize command line args
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
