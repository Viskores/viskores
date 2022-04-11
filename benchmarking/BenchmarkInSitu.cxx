//==========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//==========================================================================

#include "Benchmarker.h"

#include <random>
#include <sstream>

#include <vtkm/ImplicitFunction.h>

#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/FieldRangeCompute.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/cont/internal/OptionParser.h>

#include <vtkm/filter/Streamline.h>
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/filter/contour/Slice.h>
#include <vtkm/filter/geometry_refinement/Tetrahedralize.h>
#include <vtkm/filter/geometry_refinement/Tube.h>
#include <vtkm/filter/vector_analysis/Gradient.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>

#include <vtkm/source/PerlinNoise.h>

namespace
{

const uint32_t DEFAULT_NUM_CYCLES = 20;
const vtkm::Id DEFAULT_DATASET_DIM = 128;
const vtkm::Id DEFAULT_IMAGE_SIZE = 1024;

// Hold configuration state (e.g. active device):
vtkm::cont::InitializeResult Config;
// Input dataset dimensions:
vtkm::Id DataSetDim;
// Image size:
vtkm::Id ImageSize;
// The input datasets we'll use on the filters:
vtkm::cont::DataSet InputDataSet;
vtkm::cont::PartitionedDataSet PartitionedInputDataSet;
// The point scalars to use:
std::string PointScalarsName;
// The point vectors to use:
std::string PointVectorsName;

enum class RenderingMode
{
  None = 0,
  Mesh = 1,
  RayTrace = 2,
  Volume = 3,
};

std::vector<vtkm::cont::DataSet> ExtractDataSets(const vtkm::cont::PartitionedDataSet& partitions)
{
  return partitions.GetPartitions();
}

// Mirrors ExtractDataSet(ParitionedDataSet), to keep code simple at use sites
std::vector<vtkm::cont::DataSet> ExtractDataSets(vtkm::cont::DataSet& dataSet)
{
  return std::vector<vtkm::cont::DataSet>{ dataSet };
}

void BuildInputDataSet(uint32_t cycle, bool isStructured, bool isMultiBlock, vtkm::Id dim)
{
  vtkm::cont::PartitionedDataSet partitionedInputDataSet;
  vtkm::cont::DataSet inputDataSet;

  PointScalarsName = "perlinnoise";
  PointVectorsName = "perlinnoisegrad";

  // Generate uniform dataset(s)
  const vtkm::Id3 dims{ dim, dim, dim };
  if (isMultiBlock)
  {
    for (auto i = 0; i < 2; ++i)
    {
      for (auto j = 0; j < 2; ++j)
      {
        for (auto k = 0; k < 2; ++k)
        {
          const vtkm::Vec3f origin{ static_cast<vtkm::FloatDefault>(i),
                                    static_cast<vtkm::FloatDefault>(j),
                                    static_cast<vtkm::FloatDefault>(k) };
          const vtkm::source::PerlinNoise noise{ dims,
                                                 origin,
                                                 static_cast<vtkm::IdComponent>(cycle) };
          const auto dataset = noise.Execute();
          partitionedInputDataSet.AppendPartition(dataset);
        }
      }
    }
  }
  else
  {
    const vtkm::source::PerlinNoise noise{ dims, static_cast<vtkm::IdComponent>(cycle) };
    inputDataSet = noise.Execute();
  }

  // Generate Perln Noise Gradient point vector field
  vtkm::filter::vector_analysis::Gradient gradientFilter;
  gradientFilter.SetActiveField(PointScalarsName, vtkm::cont::Field::Association::Points);
  gradientFilter.SetComputePointGradient(true);
  gradientFilter.SetOutputFieldName(PointVectorsName);
  gradientFilter.SetFieldsToPass(
    vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::Mode::All));
  if (isMultiBlock)
  {
    partitionedInputDataSet = gradientFilter.Execute(partitionedInputDataSet);
  }
  else
  {
    inputDataSet = gradientFilter.Execute(inputDataSet);
  }

  // Run Tetrahedralize filter to convert uniform dataset(s) into unstructured ones
  if (!isStructured)
  {
    vtkm::filter::geometry_refinement::Tetrahedralize destructizer;
    destructizer.SetFieldsToPass(
      vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::Mode::All));
    if (isMultiBlock)
    {
      partitionedInputDataSet = destructizer.Execute(partitionedInputDataSet);
    }
    else
    {
      inputDataSet = destructizer.Execute(inputDataSet);
    }
  }

  // Release execution resources to simulate in-situ workload, where the data is
  // not present in the execution environment
  std::vector<vtkm::cont::DataSet> dataSets =
    isMultiBlock ? ExtractDataSets(partitionedInputDataSet) : ExtractDataSets(inputDataSet);
  for (auto& dataSet : dataSets)
  {
    dataSet.GetCellSet().ReleaseResourcesExecution();
    dataSet.GetCoordinateSystem().ReleaseResourcesExecution();
    dataSet.GetField(PointScalarsName).ReleaseResourcesExecution();
    dataSet.GetField(PointVectorsName).ReleaseResourcesExecution();
  }

  PartitionedInputDataSet = partitionedInputDataSet;
  InputDataSet = inputDataSet;
}

vtkm::rendering::Canvas* RenderDataSets(const std::vector<vtkm::cont::DataSet>& dataSets,
                                        RenderingMode mode,
                                        std::string fieldName)
{
  vtkm::rendering::Scene scene;
  vtkm::cont::ColorTable colorTable("inferno");
  if (mode == RenderingMode::Volume)
  {
    colorTable.AddPointAlpha(0.0f, 0.03f);
    colorTable.AddPointAlpha(1.0f, 0.01f);
  }

  for (auto& dataSet : dataSets)
  {
    scene.AddActor(vtkm::rendering::Actor(dataSet.GetCellSet(),
                                          dataSet.GetCoordinateSystem(),
                                          dataSet.GetField(fieldName),
                                          colorTable));
  }

  auto bounds = std::accumulate(dataSets.begin(),
                                dataSets.end(),
                                vtkm::Bounds(),
                                [=](const vtkm::Bounds& val, const vtkm::cont::DataSet& partition) {
                                  return val + vtkm::cont::BoundsCompute(partition);
                                });
  vtkm::Vec3f_64 totalExtent(bounds.X.Length(), bounds.Y.Length(), bounds.Z.Length());
  vtkm::Float64 mag = vtkm::Magnitude(totalExtent);
  vtkm::Normalize(totalExtent);

  // setup a camera and point it to towards the center of the input data
  vtkm::rendering::Camera camera;
  camera.SetFieldOfView(60.f);
  camera.ResetToBounds(bounds);
  camera.SetLookAt(totalExtent * (mag * .5f));
  camera.SetViewUp(vtkm::make_Vec(0.f, 1.f, 0.f));
  camera.SetPosition(totalExtent * (mag * 1.5f));

  vtkm::rendering::CanvasRayTracer canvas(ImageSize, ImageSize);

  auto mapper = [=]() -> std::unique_ptr<vtkm::rendering::Mapper> {
    switch (mode)
    {
      case RenderingMode::Mesh:
      {
        return std::unique_ptr<vtkm::rendering::Mapper>(new vtkm::rendering::MapperWireframer());
      }
      case RenderingMode::RayTrace:
      {
        return std::unique_ptr<vtkm::rendering::Mapper>(new vtkm::rendering::MapperRayTracer());
      }
      case RenderingMode::Volume:
      {
        return std::unique_ptr<vtkm::rendering::Mapper>(new vtkm::rendering::MapperVolume());
      }
      case RenderingMode::None:
      default:
      {
        return std::unique_ptr<vtkm::rendering::Mapper>(new vtkm::rendering::MapperRayTracer());
      }
    }
  }();

  vtkm::rendering::View3D view(scene,
                               *mapper,
                               canvas,
                               camera,
                               vtkm::rendering::Color(0.8f, 0.8f, 0.6f),
                               vtkm::rendering::Color(0.2f, 0.4f, 0.2f));
  view.Paint();

  return view.GetCanvas().NewCopy();
}

void WriteToDisk(const vtkm::rendering::Canvas& canvas,
                 RenderingMode mode,
                 std::string bench,
                 bool isStructured,
                 bool isMultiBlock,
                 uint32_t cycle)
{
  std::ostringstream nameBuilder;
  nameBuilder << "insitu_" << bench << "_"
              << "cycle_" << cycle << "_" << (isStructured ? "structured_" : "unstructured_")
              << (isMultiBlock ? "multi_" : "single_")
              << (mode == RenderingMode::Mesh ? "mesh"
                                              : (mode == RenderingMode::Volume ? "volume" : "ray"))
              << ".png";
  canvas.SaveAs(nameBuilder.str());
}


template <typename DataSetType>
DataSetType RunContourHelper(vtkm::filter::contour::Contour& filter,
                             vtkm::Id numIsoVals,
                             const DataSetType& input)
{
  // Set up some equally spaced contours, with the min/max slightly inside the
  // scalar range:
  const vtkm::Range scalarRange =
    vtkm::cont::FieldRangeCompute(input, PointScalarsName).ReadPortal().Get(0);
  const auto step = scalarRange.Length() / static_cast<vtkm::Float64>(numIsoVals + 1);
  const auto minIsoVal = scalarRange.Min + (step * 0.5f);
  filter.SetNumberOfIsoValues(numIsoVals);
  for (vtkm::Id i = 0; i < numIsoVals; ++i)
  {
    filter.SetIsoValue(i, minIsoVal + (step * static_cast<vtkm::Float64>(i)));
  }

  return filter.Execute(input);
}

void BenchContour(::benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;

  const uint32_t cycle = static_cast<uint32_t>(state.range(0));
  const vtkm::Id numIsoVals = static_cast<vtkm::Id>(state.range(1));
  const bool isStructured = static_cast<bool>(state.range(2));
  const bool isMultiBlock = static_cast<bool>(state.range(3));
  const RenderingMode renderAlgo = static_cast<RenderingMode>(state.range(4));

  vtkm::cont::Timer inputGenTimer{ device };
  inputGenTimer.Start();
  BuildInputDataSet(cycle, isStructured, isMultiBlock, DataSetDim);
  inputGenTimer.Stop();

  vtkm::filter::contour::Contour filter;
  filter.SetActiveField(PointScalarsName, vtkm::cont::Field::Association::Points);
  filter.SetMergeDuplicatePoints(true);
  filter.SetGenerateNormals(true);
  filter.SetComputeFastNormalsForStructured(true);
  filter.SetComputeFastNormalsForUnstructured(true);

  vtkm::cont::Timer totalTimer{ device };
  vtkm::cont::Timer filterTimer{ device };
  vtkm::cont::Timer renderTimer{ device };
  vtkm::cont::Timer writeTimer{ device };

  for (auto _ : state)
  {
    (void)_;
    totalTimer.Start();
    filterTimer.Start();
    std::vector<vtkm::cont::DataSet> dataSets;
    if (isMultiBlock)
    {
      auto input = PartitionedInputDataSet;
      auto result = RunContourHelper(filter, numIsoVals, input);
      dataSets = ExtractDataSets(result);
    }
    else
    {
      auto input = InputDataSet;
      auto result = RunContourHelper(filter, numIsoVals, input);
      dataSets = ExtractDataSets(result);
    }
    filterTimer.Stop();

    renderTimer.Start();
    auto canvas = RenderDataSets(dataSets, renderAlgo, PointScalarsName);
    renderTimer.Stop();

    writeTimer.Start();
    WriteToDisk(*canvas, renderAlgo, "contour", isStructured, isMultiBlock, cycle);
    writeTimer.Stop();

    totalTimer.Stop();

    state.SetIterationTime(totalTimer.GetElapsedTime());
    state.counters.insert(
      { { "InputGenTime", static_cast<uint32_t>(inputGenTimer.GetElapsedTime() * 1000) },
        { "FilterTime", static_cast<uint32_t>(filterTimer.GetElapsedTime() * 1000) },
        { "RenderTime", static_cast<uint32_t>(renderTimer.GetElapsedTime() * 1000) },
        { "WriteTime", static_cast<uint32_t>(writeTimer.GetElapsedTime() * 1000) } });
  }
}

void BenchContourGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "Cycle", "NIsos", "IsStructured", "IsMultiBlock", "RenderingMode" });

  std::vector<uint32_t> isStructureds{ false, true };
  std::vector<uint32_t> isMultiBlocks{ false, true };
  std::vector<RenderingMode> renderingModes{ RenderingMode::RayTrace };
  for (uint32_t cycle = 1; cycle <= DEFAULT_NUM_CYCLES; ++cycle)
  {
    for (auto& isStructured : isStructureds)
    {
      for (auto& isMultiBlock : isMultiBlocks)
      {
        for (auto& mode : renderingModes)
        {
          bm->Args({ cycle, 10, isStructured, isMultiBlock, static_cast<int>(mode) });
        }
      }
    }
  }
}

VTKM_BENCHMARK_APPLY(BenchContour, BenchContourGenerator);

void MakeRandomSeeds(vtkm::Id seedCount,
                     vtkm::Bounds& bounds,
                     vtkm::cont::ArrayHandle<vtkm::Particle>& seeds)
{
  std::default_random_engine generator(static_cast<vtkm::UInt32>(255));
  vtkm::FloatDefault zero(0), one(1);
  std::uniform_real_distribution<vtkm::FloatDefault> distribution(zero, one);
  std::vector<vtkm::Particle> points;
  points.resize(0);
  for (vtkm::Id i = 0; i < seedCount; i++)
  {
    vtkm::FloatDefault rx = distribution(generator);
    vtkm::FloatDefault ry = distribution(generator);
    vtkm::FloatDefault rz = distribution(generator);
    vtkm::Vec3f p;
    p[0] = static_cast<vtkm::FloatDefault>(bounds.X.Min + rx * bounds.X.Length());
    p[1] = static_cast<vtkm::FloatDefault>(bounds.Y.Min + ry * bounds.Y.Length());
    p[2] = static_cast<vtkm::FloatDefault>(bounds.Z.Min + rz * bounds.Z.Length());
    points.push_back(vtkm::Particle(p, static_cast<vtkm::Id>(i)));
  }
  vtkm::cont::ArrayHandle<vtkm::Particle> tmp =
    vtkm::cont::make_ArrayHandle(points, vtkm::CopyFlag::Off);
  vtkm::cont::ArrayCopy(tmp, seeds);
}

vtkm::Id GetNumberOfPoints(const vtkm::cont::DataSet& input)
{
  return input.GetCoordinateSystem().GetNumberOfPoints();
}

vtkm::Id GetNumberOfPoints(const vtkm::cont::PartitionedDataSet& input)
{
  return input.GetPartition(0).GetCoordinateSystem().GetNumberOfPoints();
}

void AddField(vtkm::cont::DataSet& input,
              std::string fieldName,
              std::vector<vtkm::FloatDefault>& field)
{
  input.AddPointField(fieldName, field);
}

void AddField(vtkm::cont::PartitionedDataSet& input,
              std::string fieldName,
              std::vector<vtkm::FloatDefault>& field)
{
  for (auto i = 0; i < input.GetNumberOfPartitions(); ++i)
  {
    auto partition = input.GetPartition(i);
    AddField(partition, fieldName, field);
    input.ReplacePartition(i, partition);
  }
}

template <typename DataSetType>
DataSetType RunStreamlinesHelper(vtkm::filter::Streamline& filter, const DataSetType& input)
{
  auto dataSetBounds = vtkm::cont::BoundsCompute(input);
  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  MakeRandomSeeds(100, dataSetBounds, seedArray);
  filter.SetSeeds(seedArray);

  auto result = filter.Execute(input);
  auto numPoints = GetNumberOfPoints(result);
  std::vector<vtkm::FloatDefault> colorMap(
    static_cast<std::vector<vtkm::FloatDefault>::size_type>(numPoints));
  for (std::vector<vtkm::FloatDefault>::size_type i = 0; i < colorMap.size(); i++)
  {
    colorMap[i] = static_cast<vtkm::FloatDefault>(i);
  }

  AddField(result, "pointvar", colorMap);
  return result;
}

void BenchStreamlines(::benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;

  const uint32_t cycle = static_cast<uint32_t>(state.range(0));
  const bool isStructured = static_cast<bool>(state.range(1));
  const bool isMultiBlock = static_cast<bool>(state.range(2));
  const RenderingMode renderAlgo = static_cast<RenderingMode>(state.range(3));

  vtkm::cont::Timer inputGenTimer{ device };
  inputGenTimer.Start();
  BuildInputDataSet(cycle, isStructured, isMultiBlock, DataSetDim);
  inputGenTimer.Stop();

  vtkm::filter::Streamline streamline;
  streamline.SetStepSize(0.1f);
  streamline.SetNumberOfSteps(1000);
  streamline.SetActiveField(PointVectorsName);

  vtkm::cont::Timer totalTimer{ device };
  vtkm::cont::Timer filterTimer{ device };
  vtkm::cont::Timer renderTimer{ device };
  vtkm::cont::Timer writeTimer{ device };

  for (auto _ : state)
  {
    (void)_;
    totalTimer.Start();
    filterTimer.Start();

    std::vector<vtkm::cont::DataSet> dataSets;
    if (isMultiBlock)
    {
      auto input = PartitionedInputDataSet;
      auto result = RunStreamlinesHelper(streamline, input);
      dataSets = ExtractDataSets(result);
    }
    else
    {
      auto input = InputDataSet;
      auto result = RunStreamlinesHelper(streamline, input);
      dataSets = ExtractDataSets(result);
    }
    filterTimer.Stop();

    renderTimer.Start();
    auto canvas = RenderDataSets(dataSets, renderAlgo, "pointvar");
    renderTimer.Stop();

    writeTimer.Start();
    WriteToDisk(*canvas, renderAlgo, "streamlines", isStructured, isMultiBlock, cycle);
    writeTimer.Stop();

    totalTimer.Stop();

    state.SetIterationTime(totalTimer.GetElapsedTime());
    state.counters.insert(
      { { "InputGenTime", static_cast<uint32_t>(inputGenTimer.GetElapsedTime() * 1000) },
        { "FilterTime", static_cast<uint32_t>(filterTimer.GetElapsedTime() * 1000) },
        { "RenderTime", static_cast<uint32_t>(renderTimer.GetElapsedTime() * 1000) },
        { "WriteTime", static_cast<uint32_t>(writeTimer.GetElapsedTime() * 1000) } });
  }
}

void BenchStreamlinesGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "Cycle", "IsStructured", "IsMultiBlock", "RenderingMode" });

  std::vector<uint32_t> isStructureds{ false, true };
  std::vector<uint32_t> isMultiBlocks{ false, true };
  std::vector<RenderingMode> renderingModes{ RenderingMode::Mesh };
  for (uint32_t cycle = 1; cycle <= DEFAULT_NUM_CYCLES; ++cycle)
  {
    for (auto& isStructured : isStructureds)
    {
      for (auto& isMultiBlock : isMultiBlocks)
      {
        for (auto& mode : renderingModes)
        {
          bm->Args({ cycle, isStructured, isMultiBlock, static_cast<int>(mode) });
        }
      }
    }
  }
}

VTKM_BENCHMARK_APPLY(BenchStreamlines, BenchStreamlinesGenerator);

vtkm::Vec3f GetSlicePlaneOrigin(const bool isMultiBlock)
{
  if (isMultiBlock)
  {
    auto data = PartitionedInputDataSet;
    vtkm::Bounds global;
    global = data.GetPartition(0).GetCoordinateSystem().GetBounds();
    for (auto i = 1; i < data.GetNumberOfPartitions(); ++i)
    {
      auto dataset = data.GetPartition(i);
      vtkm::Bounds bounds = dataset.GetCoordinateSystem().GetBounds();
      global.X.Min = vtkm::Min(global.X.Min, bounds.X.Min);
      global.Y.Min = vtkm::Min(global.Y.Min, bounds.Y.Min);
      global.Z.Min = vtkm::Min(global.Z.Min, bounds.Z.Min);
      global.X.Max = vtkm::Min(global.X.Max, bounds.X.Max);
      global.Y.Max = vtkm::Min(global.Y.Max, bounds.Y.Max);
      global.Z.Max = vtkm::Min(global.Z.Max, bounds.Z.Max);
    }
    return vtkm::Vec3f{ static_cast<vtkm::FloatDefault>((global.X.Max - global.X.Min) / 2.),
                        static_cast<vtkm::FloatDefault>((global.Y.Max - global.Y.Min) / 2.),
                        static_cast<vtkm::FloatDefault>((global.Z.Max - global.Z.Min) / 2.) };
  }
  else
  {
    auto data = InputDataSet;
    vtkm::Bounds bounds = data.GetCoordinateSystem().GetBounds();
    return vtkm::Vec3f{ static_cast<vtkm::FloatDefault>((bounds.X.Max - bounds.X.Min) / 2.),
                        static_cast<vtkm::FloatDefault>((bounds.Y.Max - bounds.Y.Min) / 2.),
                        static_cast<vtkm::FloatDefault>((bounds.Z.Max - bounds.Z.Min) / 2.) };
  }
}

void BenchSlice(::benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;

  const uint32_t cycle = static_cast<uint32_t>(state.range(0));
  const bool isStructured = static_cast<bool>(state.range(1));
  const bool isMultiBlock = static_cast<bool>(state.range(2));
  const RenderingMode renderAlgo = static_cast<RenderingMode>(state.range(3));

  vtkm::cont::Timer inputGenTimer{ device };
  inputGenTimer.Start();
  BuildInputDataSet(cycle, isStructured, isMultiBlock, DataSetDim);
  inputGenTimer.Stop();

  vtkm::filter::contour::Slice filter;

  vtkm::cont::Timer totalTimer{ device };
  vtkm::cont::Timer filterTimer{ device };
  vtkm::cont::Timer renderTimer{ device };
  vtkm::cont::Timer writeTimer{ device };

  for (auto _ : state)
  {
    (void)_;
    totalTimer.Start();
    filterTimer.Start();
    std::vector<vtkm::cont::DataSet> dataSets;
    if (isMultiBlock)
    {
      auto input = PartitionedInputDataSet;
      vtkm::Vec3f origin = GetSlicePlaneOrigin(isMultiBlock);
      // Set-up implicit function
      vtkm::Plane plane(origin, vtkm::Plane::Vector{ 1, 1, 1 });
      filter.SetImplicitFunction(plane);
      auto result = filter.Execute(input);
      dataSets = ExtractDataSets(result);
    }
    else
    {
      auto input = InputDataSet;
      vtkm::Vec3f origin = GetSlicePlaneOrigin(isMultiBlock);
      // Set-up implicit function
      vtkm::Plane plane(origin, vtkm::Plane::Vector{ 1, 1, 1 });
      filter.SetImplicitFunction(plane);
      auto result = filter.Execute(input);
      dataSets = ExtractDataSets(result);
    }
    filterTimer.Stop();

    renderTimer.Start();
    auto canvas = RenderDataSets(dataSets, renderAlgo, PointScalarsName);
    renderTimer.Stop();

    writeTimer.Start();
    WriteToDisk(*canvas, renderAlgo, "slice", isStructured, isMultiBlock, cycle);
    writeTimer.Stop();

    totalTimer.Stop();

    state.SetIterationTime(totalTimer.GetElapsedTime());
    state.counters.insert(
      { { "InputGenTime", static_cast<uint32_t>(inputGenTimer.GetElapsedTime() * 1000) },
        { "FilterTime", static_cast<uint32_t>(filterTimer.GetElapsedTime() * 1000) },
        { "RenderTime", static_cast<uint32_t>(renderTimer.GetElapsedTime() * 1000) },
        { "WriteTime", static_cast<uint32_t>(writeTimer.GetElapsedTime() * 1000) } });
  }
}

void BenchSliceGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "Cycle", "IsStructured", "IsMultiBlock", "RenderingMode" });

  std::vector<uint32_t> isStructureds{ false, true };
  std::vector<uint32_t> isMultiBlocks{ false, true };
  std::vector<RenderingMode> renderingModes{ RenderingMode::RayTrace };
  for (uint32_t cycle = 1; cycle <= DEFAULT_NUM_CYCLES; ++cycle)
  {
    for (auto& isStructured : isStructureds)
    {
      for (auto& isMultiBlock : isMultiBlocks)
      {
        for (auto& mode : renderingModes)
        {
          bm->Args({ cycle, isStructured, isMultiBlock, static_cast<int>(mode) });
        }
      }
    }
  }
}

VTKM_BENCHMARK_APPLY(BenchSlice, BenchSliceGenerator);

void BenchMeshRendering(::benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;

  const uint32_t cycle = static_cast<uint32_t>(state.range(0));
  const bool isStructured = static_cast<bool>(state.range(1));
  const bool isMultiBlock = static_cast<bool>(state.range(2));

  vtkm::cont::Timer inputGenTimer{ device };
  vtkm::cont::Timer renderTimer{ device };
  vtkm::cont::Timer writeTimer{ device };

  inputGenTimer.Start();
  BuildInputDataSet(cycle, isStructured, isMultiBlock, DataSetDim);
  inputGenTimer.Stop();

  vtkm::cont::Timer totalTimer{ device };

  for (auto _ : state)
  {
    (void)_;

    totalTimer.Start();

    std::vector<vtkm::cont::DataSet> dataSets =
      isMultiBlock ? ExtractDataSets(PartitionedInputDataSet) : ExtractDataSets(InputDataSet);

    renderTimer.Start();
    auto canvas = RenderDataSets(dataSets, RenderingMode::Mesh, PointScalarsName);
    renderTimer.Stop();

    writeTimer.Start();
    WriteToDisk(*canvas, RenderingMode::Mesh, "mesh", isStructured, isMultiBlock, cycle);
    writeTimer.Stop();

    totalTimer.Stop();

    state.SetIterationTime(totalTimer.GetElapsedTime());
    state.counters.insert(
      { { "InputGenTime", static_cast<uint32_t>(inputGenTimer.GetElapsedTime() * 1000) },
        { "FilterTime", 0 },
        { "RenderTime", static_cast<uint32_t>(renderTimer.GetElapsedTime() * 1000) },
        { "WriteTime", static_cast<uint32_t>(writeTimer.GetElapsedTime() * 1000) } });
  }
}

void BenchMeshRenderingGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "Cycle", "IsStructured", "IsMultiBlock" });

  std::vector<uint32_t> isStructureds{ false, true };
  std::vector<uint32_t> isMultiBlocks{ false, true };
  for (uint32_t cycle = 1; cycle <= DEFAULT_NUM_CYCLES; ++cycle)
  {
    for (auto& isStructured : isStructureds)
    {
      for (auto& isMultiBlock : isMultiBlocks)
      {
        bm->Args({ cycle, isStructured, isMultiBlock });
      }
    }
  }
}

VTKM_BENCHMARK_APPLY(BenchMeshRendering, BenchMeshRenderingGenerator);

void BenchVolumeRendering(::benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;

  const uint32_t cycle = static_cast<uint32_t>(state.range(0));
  const bool isStructured = true;
  const bool isMultiBlock = static_cast<bool>(state.range(1));

  vtkm::cont::Timer inputGenTimer{ device };
  inputGenTimer.Start();
  BuildInputDataSet(cycle, isStructured, isMultiBlock, DataSetDim);
  inputGenTimer.Stop();

  vtkm::cont::Timer totalTimer{ device };
  vtkm::cont::Timer renderTimer{ device };
  vtkm::cont::Timer writeTimer{ device };

  for (auto _ : state)
  {
    (void)_;
    totalTimer.Start();

    renderTimer.Start();
    std::vector<vtkm::cont::DataSet> dataSets =
      isMultiBlock ? ExtractDataSets(PartitionedInputDataSet) : ExtractDataSets(InputDataSet);
    auto canvas = RenderDataSets(dataSets, RenderingMode::Volume, PointScalarsName);
    renderTimer.Stop();

    writeTimer.Start();
    WriteToDisk(*canvas, RenderingMode::Volume, "volume", isStructured, isMultiBlock, cycle);
    writeTimer.Stop();

    totalTimer.Stop();

    state.SetIterationTime(totalTimer.GetElapsedTime());
    state.counters.insert(
      { { "InputGenTime", static_cast<uint32_t>(inputGenTimer.GetElapsedTime() * 1000) },
        { "FilterTime", 0 },
        { "RenderTime", static_cast<uint32_t>(renderTimer.GetElapsedTime() * 1000) },
        { "WriteTime", static_cast<uint32_t>(writeTimer.GetElapsedTime() * 1000) } });
  }
}

void BenchVolumeRenderingGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "Cycle", "IsMultiBlock" });

  std::vector<uint32_t> isMultiBlocks{ false };
  for (uint32_t cycle = 1; cycle <= DEFAULT_NUM_CYCLES; ++cycle)
  {
    for (auto& isMultiBlock : isMultiBlocks)
    {
      bm->Args({ cycle, isMultiBlock });
    }
  }
}

VTKM_BENCHMARK_APPLY(BenchVolumeRendering, BenchVolumeRenderingGenerator);

struct Arg : vtkm::cont::internal::option::Arg
{
  static vtkm::cont::internal::option::ArgStatus Number(
    const vtkm::cont::internal::option::Option& option,
    bool msg)
  {
    bool argIsNum = ((option.arg != nullptr) && (option.arg[0] != '\0'));
    const char* c = option.arg;
    while (argIsNum && (*c != '\0'))
    {
      argIsNum &= static_cast<bool>(std::isdigit(*c));
      ++c;
    }

    if (argIsNum)
    {
      return vtkm::cont::internal::option::ARG_OK;
    }
    else
    {
      if (msg)
      {
        std::cerr << "Option " << option.name << " requires a numeric argument." << std::endl;
      }

      return vtkm::cont::internal::option::ARG_ILLEGAL;
    }
  }
};

enum OptionIndex
{
  UNKNOWN,
  HELP,
  DATASET_DIM,
  IMAGE_SIZE,
};

void ParseBenchmarkOptions(int& argc, char** argv)
{
  namespace option = vtkm::cont::internal::option;

  std::vector<option::Descriptor> usage;
  std::string usageHeader{ "Usage: " };
  usageHeader.append(argv[0]);
  usageHeader.append("[input data options] [benchmark options]");
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, usageHeader.c_str() });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, "Input data options are:" });
  usage.push_back({ HELP, 0, "h", "help", Arg::None, "  -h, --help\tDisplay this help." });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, Config.Usage.c_str() });
  usage.push_back({ DATASET_DIM,
                    0,
                    "s",
                    "size",
                    Arg::Number,
                    "  -s, --size <N> \tSpecify dataset dimension and "
                    "dataset with NxNxN dimensions is created. "
                    "If not specified, N=128" });
  usage.push_back({ IMAGE_SIZE,
                    0,
                    "i",
                    "image-size",
                    Arg::Number,
                    "  -i, --image-size <N> \tSpecify size of the rendered image."
                    " The image is rendered as a square of size NxN. "
                    "If not specified, N=1024" });
  usage.push_back({ 0, 0, nullptr, nullptr, nullptr, nullptr });

  option::Stats stats(usage.data(), argc - 1, argv + 1);
  std::unique_ptr<option::Option[]> options{ new option::Option[stats.options_max] };
  std::unique_ptr<option::Option[]> buffer{ new option::Option[stats.buffer_max] };
  option::Parser commandLineParse(usage.data(), argc - 1, argv + 1, options.get(), buffer.get());

  if (options[HELP])
  {
    option::printUsage(std::cout, usage.data());
    // Print google benchmark usage too
    const char* helpstr = "--help";
    char* tmpargv[] = { argv[0], const_cast<char*>(helpstr), nullptr };
    int tmpargc = 2;
    VTKM_EXECUTE_BENCHMARKS(tmpargc, tmpargv);
    exit(0);
  }
  if (options[DATASET_DIM])
  {
    std::istringstream parse(options[DATASET_DIM].arg);
    parse >> DataSetDim;
  }
  else
  {
    DataSetDim = DEFAULT_DATASET_DIM;
  }
  if (options[IMAGE_SIZE])
  {
    std::istringstream parse(options[IMAGE_SIZE].arg);
    parse >> ImageSize;
  }
  else
  {
    ImageSize = DEFAULT_IMAGE_SIZE;
  }

  std::cerr << "Using data set dimensions = " << DataSetDim << std::endl;
  std::cerr << "Using image size = " << ImageSize << "x" << ImageSize << std::endl;

  // Now go back through the arg list and remove anything that is not in the list of
  // unknown options or non-option arguments.
  int destArg = 1;
  // This is copy/pasted from vtkm::cont::Initialize(), should probably be abstracted eventually:
  for (int srcArg = 1; srcArg < argc; ++srcArg)
  {
    std::string thisArg{ argv[srcArg] };
    bool copyArg = false;

    // Special case: "--" gets removed by optionparser but should be passed.
    if (thisArg == "--")
    {
      copyArg = true;
    }
    for (const option::Option* opt = options[UNKNOWN]; !copyArg && opt != nullptr;
         opt = opt->next())
    {
      if (thisArg == opt->name)
      {
        copyArg = true;
      }
      if ((opt->arg != nullptr) && (thisArg == opt->arg))
      {
        copyArg = true;
      }
      // Special case: optionparser sometimes removes a single "-" from an option
      if (thisArg.substr(1) == opt->name)
      {
        copyArg = true;
      }
    }
    for (int nonOpt = 0; !copyArg && nonOpt < commandLineParse.nonOptionsCount(); ++nonOpt)
    {
      if (thisArg == commandLineParse.nonOption(nonOpt))
      {
        copyArg = true;
      }
    }
    if (copyArg)
    {
      if (destArg != srcArg)
      {
        argv[destArg] = argv[srcArg];
      }
      ++destArg;
    }
  }
  argc = destArg;
}

} // end anon namespace

int main(int argc, char* argv[])
{
  auto opts = vtkm::cont::InitializeOptions::RequireDevice;

  std::vector<char*> args(argv, argv + argc);
  vtkm::bench::detail::InitializeArgs(&argc, args, opts);

  // Parse VTK-m options
  Config = vtkm::cont::Initialize(argc, args.data(), opts);
  ParseBenchmarkOptions(argc, args.data());

  // This opts chances when it is help
  if (opts != vtkm::cont::InitializeOptions::None)
  {
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);
  }

  VTKM_EXECUTE_BENCHMARKS(argc, args.data());
}
