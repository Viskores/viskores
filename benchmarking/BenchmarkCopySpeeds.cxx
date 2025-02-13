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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/Timer.h>

#include <viskores/internal/Configure.h>

#include <viskores/List.h>

#include <sstream>

namespace
{

// Make this global so benchmarks can access the current device id:
viskores::cont::InitializeResult Config;

const viskores::UInt64 COPY_SIZE_MIN = (1 << 10); // 1 KiB
const viskores::UInt64 COPY_SIZE_MAX = (1 << 30); // 1 GiB

using TypeList = viskores::List<viskores::UInt8,
                                viskores::Vec2ui_8,
                                viskores::Vec3ui_8,
                                viskores::Vec4ui_8,
                                viskores::UInt32,
                                viskores::Vec2ui_32,
                                viskores::UInt64,
                                viskores::Vec2ui_64,
                                viskores::Float32,
                                viskores::Vec2f_32,
                                viskores::Float64,
                                viskores::Vec2f_64,
                                viskores::Pair<viskores::UInt32, viskores::Float32>,
                                viskores::Pair<viskores::UInt32, viskores::Float64>,
                                viskores::Pair<viskores::UInt64, viskores::Float32>,
                                viskores::Pair<viskores::UInt64, viskores::Float64>>;

template <typename ValueType>
void CopySpeed(benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::UInt64 numBytes = static_cast<viskores::UInt64>(state.range(0));
  const viskores::Id numValues = static_cast<viskores::Id>(numBytes / sizeof(ValueType));

  state.SetLabel(viskores::cont::GetHumanReadableSize(numBytes));

  viskores::cont::ArrayHandle<ValueType> src;
  viskores::cont::ArrayHandle<ValueType> dst;
  src.Allocate(numValues);
  dst.Allocate(numValues);

  viskores::cont::Timer timer(device);
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
}
VISKORES_BENCHMARK_TEMPLATES_OPTS(CopySpeed,
                                    ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                    ->ArgName("Bytes"),
                                  TypeList);

} // end anon namespace

int main(int argc, char* argv[])
{
  // Parse Viskores options:
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
