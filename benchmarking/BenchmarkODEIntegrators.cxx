//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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

#include <viskores/Particle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/Timer.h>
#include <viskores/cont/internal/OptionParser.h>
#include <viskores/filter/flow/ParticleAdvection.h>

namespace
{
// Hold configuration state (e.g. active device):
viskores::cont::InitializeResult Config;

// Wrapper around RK4:
void BenchParticleAdvection(::benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id3 dims(5, 5, 5);
  const viskores::Vec3f vecX(1, 0, 0);

  viskores::Id numPoints = dims[0] * dims[1] * dims[2];

  std::vector<viskores::Vec3f> vectorField(static_cast<std::size_t>(numPoints)); // 3D
  for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    vectorField[i] = vecX;

  viskores::cont::DataSetBuilderUniform dataSetBuilder;

  viskores::cont::DataSet ds = dataSetBuilder.Create(dims);
  ds.AddPointField("vector", vectorField);

  viskores::cont::ArrayHandle<viskores::Particle> seedArray =
    viskores::cont::make_ArrayHandle({ viskores::Particle(viskores::Vec3f(.2f, 1.0f, .2f), 0),
                                       viskores::Particle(viskores::Vec3f(.2f, 2.0f, .2f), 1),
                                       viskores::Particle(viskores::Vec3f(.2f, 3.0f, .2f), 2) });

  viskores::filter::flow::ParticleAdvection particleAdvection;

  particleAdvection.SetStepSize(viskores::FloatDefault(1) / state.range(0));
  particleAdvection.SetNumberOfSteps(static_cast<viskores::Id>(state.range(0)));
  particleAdvection.SetSeeds(seedArray);
  particleAdvection.SetActiveField("vector");
  viskores::cont::Timer timer{ device };
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
VISKORES_BENCHMARK_OPTS(BenchParticleAdvection,
                          ->RangeMultiplier(2)
                          ->Range(32, 4096)
                          ->ArgName("Steps")
                          ->Complexity());

} // end anon namespace

int main(int argc, char* argv[])
{
  auto opts = viskores::cont::InitializeOptions::DefaultAnyDevice;
  std::vector<char*> args(argv, argv + argc);
  viskores::bench::detail::InitializeArgs(&argc, args, opts);
  Config = viskores::cont::Initialize(argc, args.data(), opts);
  if (opts != viskores::cont::InitializeOptions::None)
  {
    viskores::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);
  }
  VISKORES_EXECUTE_BENCHMARKS(argc, args.data());
}
