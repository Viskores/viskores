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

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/internal/Configure.h>

#include <vtkm/testing/Testing.h>

#include <vtkm/List.h>

#include <sstream>

#ifdef VTKM_ENABLE_TBB
#include <tbb/task_scheduler_init.h>
#endif // TBB

// For the TBB implementation, the number of threads can be customized using a
// "NumThreads [numThreads]" argument.

namespace
{

// Make this global so benchmarks can access the current device id:
vtkm::cont::InitializeResult Config;

const vtkm::UInt64 COPY_SIZE_MIN = (1 << 10); // 1 KiB
const vtkm::UInt64 COPY_SIZE_MAX = (1 << 30); // 1 GiB

using TypeList = vtkm::List<vtkm::UInt8,
                            vtkm::Vec2ui_8,
                            vtkm::Vec3ui_8,
                            vtkm::Vec4ui_8,
                            vtkm::UInt32,
                            vtkm::Vec2ui_32,
                            vtkm::UInt64,
                            vtkm::Vec2ui_64,
                            vtkm::Float32,
                            vtkm::Vec2f_32,
                            vtkm::Float64,
                            vtkm::Vec2f_64,
                            vtkm::Pair<vtkm::UInt32, vtkm::Float32>,
                            vtkm::Pair<vtkm::UInt32, vtkm::Float64>,
                            vtkm::Pair<vtkm::UInt64, vtkm::Float32>,
                            vtkm::Pair<vtkm::UInt64, vtkm::Float64>>;

template <typename ValueType>
void CopySpeed(benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::UInt64 numBytes = static_cast<vtkm::UInt64>(state.range(0));
  const vtkm::Id numValues = static_cast<vtkm::Id>(numBytes / sizeof(ValueType));

  state.SetLabel(vtkm::cont::GetHumanReadableSize(numBytes));

  vtkm::cont::ArrayHandle<ValueType> src;
  vtkm::cont::ArrayHandle<ValueType> dst;
  src.Allocate(numValues);
  dst.Allocate(numValues);

  vtkm::cont::Timer timer(device);
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    vtkm::cont::Algorithm::Copy(device, src, dst);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  const int64_t iterations = static_cast<int64_t>(state.iterations());
  state.SetBytesProcessed(static_cast<int64_t>(numBytes) * iterations);
  state.SetItemsProcessed(static_cast<int64_t>(numValues) * iterations);
}
VTKM_BENCHMARK_TEMPLATES_OPTS(CopySpeed,
                                ->Range(COPY_SIZE_MIN, COPY_SIZE_MAX)
                                ->ArgName("Bytes"),
                              TypeList);

} // end anon namespace

int main(int argc, char* argv[])
{
  // Parse VTK-m options:
  auto opts = vtkm::cont::InitializeOptions::RequireDevice | vtkm::cont::InitializeOptions::AddHelp;
  Config = vtkm::cont::Initialize(argc, argv, opts);

  // Setup device:
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);

// Handle NumThreads command-line arg:
#ifdef VTKM_ENABLE_TBB
  int numThreads = tbb::task_scheduler_init::automatic;
#endif // TBB

  if (argc == 3)
  {
    if (std::string(argv[1]) == "NumThreads")
    {
#ifdef VTKM_ENABLE_TBB
      std::istringstream parse(argv[2]);
      parse >> numThreads;
      std::cout << "Selected " << numThreads << " TBB threads." << std::endl;
#else
      std::cerr << "NumThreads valid only on TBB. Ignoring." << std::endl;
#endif // TBB
    }
  }

#ifdef VTKM_ENABLE_TBB
  // Must not be destroyed as long as benchmarks are running:
  tbb::task_scheduler_init init(numThreads);
#endif // TBB

  // handle benchmarking related args and run benchmarks:
  VTKM_EXECUTE_BENCHMARKS(argc, argv);
}
