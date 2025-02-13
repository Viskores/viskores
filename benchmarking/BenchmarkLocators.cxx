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
#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/CellLocatorTwoLevel.h>
#include <viskores/cont/CellLocatorUniformBins.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/Timer.h>
#include <viskores/cont/internal/OptionParser.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/geometry_refinement/Triangulate.h>

#include <random>

namespace
{
// Hold configuration state (e.g. active device):
viskores::cont::InitializeResult Config;

class RandomPointGenerator
{
public:
  RandomPointGenerator(const viskores::Bounds& bounds, const viskores::UInt32& seed)
    : Bounds(bounds)
    , Seed(seed)
  {
    using DistType = std::uniform_real_distribution<viskores::FloatDefault>;
    this->Generator = std::default_random_engine(this->Seed);

    this->Distributions.resize(3);
    this->Distributions[0] = DistType(this->Bounds.X.Min, this->Bounds.X.Max);
    this->Distributions[1] = DistType(this->Bounds.Y.Min, this->Bounds.Y.Max);
    this->Distributions[2] = DistType(this->Bounds.Z.Min, this->Bounds.Z.Max);
  }

  viskores::Vec3f GetPt()
  {
    return viskores::Vec3f(this->Distributions[0](this->Generator),
                           this->Distributions[1](this->Generator),
                           this->Distributions[2](this->Generator));
  }

private:
  viskores::Bounds Bounds;
  std::default_random_engine Generator;
  std::vector<std::uniform_real_distribution<viskores::FloatDefault>> Distributions;
  viskores::UInt32 Seed = 0;
};

class FindCellWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points, ExecObject locator);
  using ExecutionSignature = void(_1, _2);

  template <typename LocatorType>
  VISKORES_EXEC void operator()(const viskores::Vec3f& point, const LocatorType& locator) const
  {
    viskores::Id cellId;
    viskores::Vec3f pcoords;
    locator.FindCell(point, cellId, pcoords);
  }
};

class IterateFindCellWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points,
                                ExecObject locator,
                                WholeArrayIn dx,
                                WholeArrayIn dy);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename LocatorType, typename DeltaArrayType>
  VISKORES_EXEC void operator()(const viskores::Vec3f& inPoint,
                                const LocatorType& locator,
                                const DeltaArrayType& dx,
                                const DeltaArrayType& dy) const
  {
    viskores::Id cellId;
    viskores::Vec3f pcoords;
    viskores::Vec3f pt = inPoint;
    typename LocatorType::LastCell lastCell;
    for (viskores::Id i = 0; i < this->NumIters; i++)
    {
      if (this->UseLastCell)
        locator.FindCell(pt, cellId, pcoords, lastCell);
      else
        locator.FindCell(pt, cellId, pcoords);

      // shift each value to -1, 1, then multiply by len;
      pt[0] += (dx.Get(i) * 2 - 1.) * this->LenX;
      pt[1] += (dy.Get(i) * 2 - 1.) * this->LenY;
    }
  }

  viskores::FloatDefault LenX = 0.0025;
  viskores::FloatDefault LenY = 0.0025;
  viskores::Id NumIters;
  bool UseLastCell = true;
};

viskores::cont::DataSet CreateExplicitDataSet2D(viskores::Id Nx, viskores::Id Ny)
{
  viskores::Id3 dims(Nx, Ny, 1);
  const viskores::Vec3f origin(0, 0, 0);
  viskores::Vec3f spacing(1 / static_cast<viskores::FloatDefault>(Nx - 1),
                          1 / static_cast<viskores::FloatDefault>(Ny - 1),
                          0);
  auto ds = viskores::cont::DataSetBuilderUniform::Create(dims, origin, spacing);

  //Turn the grid into an explicit triangle grid.
  viskores::filter::geometry_refinement::Triangulate triangulator;
  viskores::filter::clean_grid::CleanGrid cleanGrid;
  auto triDS = cleanGrid.Execute(triangulator.Execute(ds));
  //triDS.PrintSummary(std::cout);

  //Randomly tweak each vertex.
  auto coords = triDS.GetCoordinateSystem()
                  .GetData()
                  .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>();
  auto coordsPortal = coords.WritePortal();
  viskores::Id nCoords = coordsPortal.GetNumberOfValues();

  viskores::FloatDefault dx = spacing[0] * 0.33, dy = spacing[1] * 0.33;
  std::default_random_engine dre;
  std::uniform_real_distribution<viskores::FloatDefault> rangeX(-dx, dy);
  std::uniform_real_distribution<viskores::FloatDefault> rangeY(-dy, dy);
  for (viskores::Id i = 0; i < nCoords; i++)
  {
    auto pt = coordsPortal.Get(i);
    pt[0] += rangeX(dre);
    pt[1] += rangeY(dre);
    coordsPortal.Set(i, pt);
  }

  return triDS;
}

viskores::cont::ArrayHandle<viskores::Vec3f> CreateRandomPoints(viskores::Id numPoints,
                                                                const viskores::cont::DataSet& ds,
                                                                viskores::Id seed)
{
  RandomPointGenerator rpg(ds.GetCoordinateSystem().GetBounds(), seed);

  std::vector<viskores::Vec3f> pts(numPoints);
  for (auto& pt : pts)
    pt = rpg.GetPt();

  return viskores::cont::make_ArrayHandle(pts, viskores::CopyFlag::On);
}

template <typename LocatorType>
void RunLocatorBenchmark(const viskores::cont::ArrayHandle<viskores::Vec3f>& points,
                         LocatorType& locator)
{
  //Call find cell on each point.
  viskores::cont::Invoker invoker;

  invoker(FindCellWorklet{}, points, locator);
}

template <typename LocatorType>
void RunLocatorIterateBenchmark(
  const viskores::cont::ArrayHandle<viskores::Vec3f>& points,
  viskores::Id numIters,
  LocatorType& locator,
  const viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault>& dx,
  const viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault>& dy,
  bool useLastCell)
{
  //Call find cell on each point.
  viskores::cont::Invoker invoker;

  IterateFindCellWorklet worklet;
  worklet.NumIters = numIters;
  worklet.UseLastCell = useLastCell;
  invoker(worklet, points, locator, dx, dy);
}

void Bench2DCellLocatorTwoLevel(::benchmark::State& state)
{
  viskores::Id numPoints = static_cast<viskores::Id>(state.range(0));
  viskores::Id Nx = static_cast<viskores::Id>(state.range(1));
  viskores::Id Ny = static_cast<viskores::Id>(state.range(2));
  viskores::FloatDefault L1Param = static_cast<viskores::FloatDefault>(state.range(3));
  viskores::FloatDefault L2Param = static_cast<viskores::FloatDefault>(state.range(4));

  auto triDS = CreateExplicitDataSet2D(Nx, Ny);

  const viskores::cont::DeviceAdapterId device = Config.Device;
  viskores::cont::Timer timer{ device };

  viskores::cont::CellLocatorTwoLevel locator2L;
  locator2L.SetDensityL1(L1Param);
  locator2L.SetDensityL2(L2Param);

  locator2L.SetCellSet(triDS.GetCellSet());
  locator2L.SetCoordinates(triDS.GetCoordinateSystem());
  locator2L.Update();

  //Random number seed. Modify it during the loop to ensure different random numbers.
  viskores::Id seed = 0;
  for (auto _ : state)
  {
    (void)_;

    auto points = CreateRandomPoints(numPoints, triDS, seed++);

    timer.Start();
    RunLocatorBenchmark(points, locator2L);
    timer.Stop();
    state.SetIterationTime(timer.GetElapsedTime());
  }
}

void Bench2DCellLocatorUniformBins(::benchmark::State& state)
{
  viskores::Id numPoints = static_cast<viskores::Id>(state.range(0));
  viskores::Id Nx = static_cast<viskores::Id>(state.range(1));
  viskores::Id Ny = static_cast<viskores::Id>(state.range(2));
  viskores::Id UGNx = static_cast<viskores::Id>(state.range(3));
  viskores::Id UGNy = static_cast<viskores::Id>(state.range(4));

  auto triDS = CreateExplicitDataSet2D(Nx, Ny);

  const viskores::cont::DeviceAdapterId device = Config.Device;
  viskores::cont::Timer timer{ device };

  viskores::cont::CellLocatorUniformBins locatorUB;
  locatorUB.SetDims({ UGNx, UGNy, 1 });
  locatorUB.SetCellSet(triDS.GetCellSet());
  locatorUB.SetCoordinates(triDS.GetCoordinateSystem());
  locatorUB.Update();

  //Random number seed. Modify it during the loop to ensure different random numbers.
  viskores::Id seed = 0;
  for (auto _ : state)
  {
    (void)_;

    auto points = CreateRandomPoints(numPoints, triDS, seed++);

    timer.Start();
    RunLocatorBenchmark(points, locatorUB);
    timer.Stop();
    state.SetIterationTime(timer.GetElapsedTime());
  }
}

void Bench2DCellLocatorTwoLevelIterate(::benchmark::State& state)
{
  viskores::Id numPoints = static_cast<viskores::Id>(state.range(0));
  viskores::Id numIters = static_cast<viskores::Id>(state.range(1));
  viskores::Id Nx = static_cast<viskores::Id>(state.range(2));
  viskores::Id Ny = static_cast<viskores::Id>(state.range(3));
  viskores::FloatDefault L1Param = static_cast<viskores::FloatDefault>(state.range(4));
  viskores::FloatDefault L2Param = static_cast<viskores::FloatDefault>(state.range(5));
  bool useLastCell = static_cast<bool>(state.range(6));

  auto triDS = CreateExplicitDataSet2D(Nx, Ny);

  const viskores::cont::DeviceAdapterId device = Config.Device;
  viskores::cont::Timer timer{ device };

  viskores::cont::CellLocatorTwoLevel locator2L;
  locator2L.SetDensityL1(L1Param);
  locator2L.SetDensityL2(L2Param);

  locator2L.SetCellSet(triDS.GetCellSet());
  locator2L.SetCoordinates(triDS.GetCoordinateSystem());
  locator2L.Update();

  //Random number seed. Modify it during the loop to ensure different random numbers.
  viskores::Id seed = 0;
  for (auto _ : state)
  {
    (void)_;

    auto points = CreateRandomPoints(numPoints, triDS, seed++);
    auto dx =
      viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault>(numIters, seed++);
    auto dy =
      viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault>(numIters, seed++);

    timer.Start();
    RunLocatorIterateBenchmark(points, numIters, locator2L, dx, dy, useLastCell);
    timer.Stop();
    state.SetIterationTime(timer.GetElapsedTime());
  }
}

void Bench2DCellLocatorUniformBinsIterate(::benchmark::State& state)
{
  viskores::Id numPoints = static_cast<viskores::Id>(state.range(0));
  viskores::Id numIters = static_cast<viskores::Id>(state.range(1));
  viskores::Id Nx = static_cast<viskores::Id>(state.range(2));
  viskores::Id Ny = static_cast<viskores::Id>(state.range(3));
  viskores::Id UGNx = static_cast<viskores::Id>(state.range(4));
  viskores::Id UGNy = static_cast<viskores::Id>(state.range(5));
  bool useLastCell = static_cast<bool>(state.range(6));

  auto triDS = CreateExplicitDataSet2D(Nx, Ny);

  const viskores::cont::DeviceAdapterId device = Config.Device;
  viskores::cont::Timer timer{ device };

  viskores::cont::CellLocatorUniformBins locatorUB;
  locatorUB.SetDims({ UGNx, UGNy, 1 });
  locatorUB.SetCellSet(triDS.GetCellSet());
  locatorUB.SetCoordinates(triDS.GetCoordinateSystem());
  locatorUB.Update();

  locatorUB.SetCellSet(triDS.GetCellSet());
  locatorUB.SetCoordinates(triDS.GetCoordinateSystem());
  locatorUB.Update();

  //Random number seed. Modify it during the loop to ensure different random numbers.
  viskores::Id seed = 0;
  for (auto _ : state)
  {
    (void)_;

    auto points = CreateRandomPoints(numPoints, triDS, seed++);
    auto dx =
      viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault>(numIters, seed++);
    auto dy =
      viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault>(numIters, seed++);

    timer.Start();
    RunLocatorIterateBenchmark(points, numIters, locatorUB, dx, dy, useLastCell);
    timer.Stop();
    state.SetIterationTime(timer.GetElapsedTime());
  }
}

void Bench2DCellLocatorTwoLevelGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "NumPoints", "DSNx", "DSNy", "LocL1Param", "LocL2Param" });

  auto numPts = { 5000, 10000 };
  auto DSdims = { 100, 1000 };
  auto L1Param = { 64 };
  auto L2Param = { 1 };

  for (auto& DSDimx : DSdims)
    for (auto& DSDimy : DSdims)
      for (auto& np : numPts)
        for (auto& l1p : L1Param)
          for (auto& l2p : L2Param)
          {
            bm->Args({ np, DSDimx, DSDimy, l1p, l2p });
          }
}

void Bench2DCellLocatorUniformBinsGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "NumPoints", "DSNx", "DSNy", "LocNx", "LocNy" });

  auto numPts = { 5000, 10000 };
  auto DSdims = { 100, 1000 };
  auto numBins = { 100, 500, 1000 };

  for (auto& DSDimx : DSdims)
    for (auto& DSDimy : DSdims)
      for (auto& np : numPts)
        for (auto& nb : numBins)
        {
          bm->Args({ np, DSDimx, DSDimy, nb, nb });
        }
}

void Bench2DCellLocatorTwoLevelIterateGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "NumPoints", "NumIters", "DSNx", "DSNy", "LocL1Param", "LocL2Param", "LastCell" });

  auto numPts = { 1000, 5000 };
  auto numIters = { 100, 500 };
  auto DSdims = { 1000 };
  auto L1Param = { 64 };
  auto L2Param = { 1 };
  auto lastCell = { 0, 1 };

  for (auto& DSDimx : DSdims)
    for (auto& DSDimy : DSdims)
      for (auto& np : numPts)
        for (auto& ni : numIters)
          for (auto& l1p : L1Param)
            for (auto& l2p : L2Param)
              for (auto& lc : lastCell)
              {
                bm->Args({ np, ni, DSDimx, DSDimy, l1p, l2p, lc });
              }
}

void Bench2DCellLocatorUniformBinsIterateGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "NumPoints", "NumIters", "DSNx", "DSNy", "LocNx", "LocNY", "LastCell" });

  auto numPts = { 1000, 5000 };
  auto numIters = { 100, 500 };
  auto DSdims = { 1000 };
  auto numBins = { 128 };
  auto lastCell = { 0, 1 };

  for (auto& DSDimx : DSdims)
    for (auto& DSDimy : DSdims)
      for (auto& np : numPts)
        for (auto& ni : numIters)
          for (auto& nb : numBins)
            for (auto& lc : lastCell)
            {
              bm->Args({ np, ni, DSDimx, DSDimy, nb, nb, lc });
            }
}


VISKORES_BENCHMARK_APPLY(Bench2DCellLocatorTwoLevel, Bench2DCellLocatorTwoLevelGenerator);
VISKORES_BENCHMARK_APPLY(Bench2DCellLocatorUniformBins, Bench2DCellLocatorUniformBinsGenerator);

VISKORES_BENCHMARK_APPLY(Bench2DCellLocatorTwoLevelIterate,
                         Bench2DCellLocatorTwoLevelIterateGenerator);
VISKORES_BENCHMARK_APPLY(Bench2DCellLocatorUniformBinsIterate,
                         Bench2DCellLocatorUniformBinsIterateGenerator);

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
