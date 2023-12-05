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

#include <vtkm/Particle.h>
#include <vtkm/cont/CellLocatorTwoLevel.h>
#include <vtkm/cont/CellLocatorUniformBins.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/internal/OptionParser.h>
#include <vtkm/io/VTKDataSetWriter.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/filter/clean_grid/CleanGrid.h>
#include <vtkm/filter/geometry_refinement/Triangulate.h>

#include <random>

namespace
{
// Hold configuration state (e.g. active device):
vtkm::cont::InitializeResult Config;

class RandomPointGenerator
{
public:
  RandomPointGenerator(const vtkm::Bounds& bounds, const vtkm::UInt32& seed = 0)
    : Bounds(bounds)
    , Seed(seed)
  {
    using DistType = std::uniform_real_distribution<vtkm::FloatDefault>;
    this->Generator = std::default_random_engine(this->Seed);

    this->Distributions.resize(3);
    this->Distributions[0] = DistType(this->Bounds.X.Min, this->Bounds.X.Max);
    this->Distributions[1] = DistType(this->Bounds.Y.Min, this->Bounds.Y.Max);
    this->Distributions[2] = DistType(this->Bounds.Z.Min, this->Bounds.Z.Max);
  }

  vtkm::Vec3f GetPt()
  {
    return vtkm::Vec3f(this->Distributions[0](this->Generator),
                       this->Distributions[1](this->Generator),
                       this->Distributions[2](this->Generator));
  }

private:
  vtkm::Bounds Bounds;
  std::default_random_engine Generator;
  std::vector<std::uniform_real_distribution<vtkm::FloatDefault>> Distributions;
  vtkm::UInt32 Seed = 0;
};

class FindCellWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points,
                                ExecObject locator,
                                FieldOut cellIds,
                                FieldOut pcoords);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename LocatorType>
  VTKM_EXEC void operator()(const vtkm::Vec3f& point,
                            const LocatorType& locator,
                            vtkm::Id& cellId,
                            vtkm::Vec3f& pcoords) const
  {
    locator.FindCell(point, cellId, pcoords);
    /*
    vtkm::ErrorCode status = locator.FindCell(point, cellId, pcoords);
    if (status != vtkm::ErrorCode::Success)
    {
      std::cout<<"Missing pt: "<<point<<std::endl;
      //this->RaiseError(vtkm::ErrorString(status));
    }
    */
  }
};

vtkm::cont::DataSet CreateExplicitDataSet2D(vtkm::Id Nx, vtkm::Id Ny)
{
  vtkm::Id3 dims(Nx, Ny, 1);
  const vtkm::Vec3f origin(0, 0, 0);
  vtkm::Vec3f spacing(
    1 / static_cast<vtkm::FloatDefault>(Nx - 1), 1 / static_cast<vtkm::FloatDefault>(Ny - 1), 0);
  auto ds = vtkm::cont::DataSetBuilderUniform::Create(dims, origin, spacing);

  //Turn the grid into an explicit triangle grid.
  vtkm::filter::geometry_refinement::Triangulate triangulator;
  vtkm::filter::clean_grid::CleanGrid cleanGrid;
  auto triDS = cleanGrid.Execute(triangulator.Execute(ds));
  //triDS.PrintSummary(std::cout);

  //Randomly tweak each vertex.
  auto coords =
    triDS.GetCoordinateSystem().GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>();
  auto coordsPortal = coords.WritePortal();
  vtkm::Id nCoords = coordsPortal.GetNumberOfValues();

  vtkm::FloatDefault dx = spacing[0] * 0.33, dy = spacing[1] * 0.33;
  std::default_random_engine dre;
  std::uniform_real_distribution<vtkm::FloatDefault> rangeX(-dx, dy);
  std::uniform_real_distribution<vtkm::FloatDefault> rangeY(-dy, dy);
  for (vtkm::Id i = 0; i < nCoords; i++)
  {
    auto pt = coordsPortal.Get(i);
    pt[0] += rangeX(dre);
    pt[1] += rangeY(dre);
    coordsPortal.Set(i, pt);
  }

  /*
  vtkm::io::VTKDataSetWriter writer("triDS.vtk");
  writer.SetFileTypeToBinary();
  writer.WriteDataSet(triDS);
  */

  return triDS;
}

vtkm::cont::ArrayHandle<vtkm::Vec3f> CreateRandomPoints(vtkm::Id numPoints,
                                                        const vtkm::cont::DataSet& ds)
{
  RandomPointGenerator rpg(ds.GetCoordinateSystem().GetBounds());

  std::vector<vtkm::Vec3f> pts(numPoints);
  for (auto& pt : pts)
    pt = rpg.GetPt();

  return vtkm::cont::make_ArrayHandle(pts, vtkm::CopyFlag::Off);
}

template <typename LocatorType>
void RunBenchmark(const vtkm::cont::ArrayHandle<vtkm::Vec3f>& points, LocatorType& locator)
{
  //Call find cell on each point.
  vtkm::cont::Invoker invoker;
  vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> pcoords;

  invoker(FindCellWorklet{}, points, locator, cellIds, pcoords);
}

void BenchLocator2D2L(::benchmark::State& state)
{
  vtkm::Id numPoints = static_cast<vtkm::Id>(state.range(0));
  vtkm::Id Nx = static_cast<vtkm::Id>(state.range(1));
  vtkm::Id Ny = static_cast<vtkm::Id>(state.range(2));
  vtkm::FloatDefault L1Param = static_cast<vtkm::FloatDefault>(state.range(3));
  vtkm::FloatDefault L2Param = static_cast<vtkm::FloatDefault>(state.range(4));

  auto triDS = CreateExplicitDataSet2D(Nx, Ny);

  const vtkm::cont::DeviceAdapterId device = Config.Device;
  vtkm::cont::Timer timer{ device };

  vtkm::cont::CellLocatorTwoLevel locator2L;
  locator2L.SetDensityL1(L1Param);
  locator2L.SetDensityL2(L2Param);

  locator2L.SetCellSet(triDS.GetCellSet());
  locator2L.SetCoordinates(triDS.GetCoordinateSystem());
  locator2L.Update();

  for (auto _ : state)
  {
    (void)_;

    auto points = CreateRandomPoints(numPoints, triDS);

    timer.Start();
    RunBenchmark(points, locator2L);
    timer.Stop();
    state.SetIterationTime(timer.GetElapsedTime());
  }
}

void BenchLocator2DUB(::benchmark::State& state)
{
  vtkm::Id numPoints = static_cast<vtkm::Id>(state.range(0));
  vtkm::Id Nx = static_cast<vtkm::Id>(state.range(1));
  vtkm::Id Ny = static_cast<vtkm::Id>(state.range(2));
  vtkm::Id UGNx = static_cast<vtkm::Id>(state.range(3));
  vtkm::Id UGNy = static_cast<vtkm::Id>(state.range(4));

  auto triDS = CreateExplicitDataSet2D(Nx, Ny);

  const vtkm::cont::DeviceAdapterId device = Config.Device;
  vtkm::cont::Timer timer{ device };

  vtkm::cont::CellLocatorUniformBins locatorUB;
  locatorUB.SetDims({ UGNx, UGNy, 1 });
  locatorUB.SetCellSet(triDS.GetCellSet());
  locatorUB.SetCoordinates(triDS.GetCoordinateSystem());
  locatorUB.Update();

  for (auto _ : state)
  {
    (void)_;

    auto points = CreateRandomPoints(numPoints, triDS);

    timer.Start();
    RunBenchmark(points, locatorUB);
    timer.Stop();
    state.SetIterationTime(timer.GetElapsedTime());
  }
}

void BenchLocator2DGenerator2L(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "NumPoints", "DSNx", "DSNy", "LocL1Param", "LocL2Param" });

  auto numPts = { 100, 500 };
  auto DSdims = { 100, 200 };
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

void BenchLocator2DGeneratorUB(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "NumPoints", "DSNx", "DSNy", "LocNx", "LocNy" });

  auto numPts = { 1000, 5000, 10000 };
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

VTKM_BENCHMARK_APPLY(BenchLocator2D2L, BenchLocator2DGenerator2L);
//VTKM_BENCHMARK_APPLY(BenchLocator2DUB, BenchLocator2DGeneratorUB);

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
