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

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/ErrorInternal.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/internal/OptionParser.h>
#include <vtkm/filter/ParticleAdvection.h>
#include <vtkm/worklet/particleadvection/EulerIntegrator.h>
#include <vtkm/worklet/particleadvection/RK4Integrator.h>
#ifdef VTKM_ENABLE_TBB
#include <tbb/task_scheduler_init.h>
#endif
#ifdef VTKM_ENABLE_OPENMP
#include <omp.h>
#endif


namespace
{
// Hold configuration state (e.g. active device):
vtkm::cont::InitializeResult Config;

// Wrapper around RK4:
void BenchParticleAdvection(::benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;
  const vtkm::Id3 dims(5, 5, 5);
  const vtkm::Vec3f vecX(1, 0, 0);

  vtkm::Id numPoints = dims[0] * dims[1] * dims[2];

  std::vector<vtkm::Vec3f> vectorField(static_cast<std::size_t>(numPoints));
  for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    vectorField[i] = vecX;

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;

  vtkm::cont::DataSet ds = dataSetBuilder.Create(dims);
  ds.AddPointField("vector", vectorField);

  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray =
    vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                   vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1),
                                   vtkm::Particle(vtkm::Vec3f(.2f, 3.0f, .2f), 2) });

  vtkm::filter::ParticleAdvection particleAdvection;

  particleAdvection.SetStepSize(vtkm::FloatDefault(1) / state.range(0));
  particleAdvection.SetNumberOfSteps(static_cast<vtkm::Id>(state.range(0)));
  particleAdvection.SetSeeds(seedArray);
  particleAdvection.SetActiveField("vector");
  vtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto output = particleAdvection.Execute(ds);
    ::benchmark::DoNotOptimize(output);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
  state.SetComplexityN(state.range(0));
}
VTKM_BENCHMARK_OPTS(BenchParticleAdvection,
                      ->RangeMultiplier(2)
                      ->Range(32, 4096)
                      ->ArgName("Steps")
                      ->Complexity());

} // end anon namespace

int main(int argc, char* argv[])
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  std::vector<char*> args(argv, argv + argc);
  vtkm::bench::detail::InitializeArgs(&argc, args, opts);
  Config = vtkm::cont::Initialize(argc, args.data(), opts);
  if (opts != vtkm::cont::InitializeOptions::None)
  {
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);
  }
  VTKM_EXECUTE_BENCHMARKS(argc, args.data());
}
