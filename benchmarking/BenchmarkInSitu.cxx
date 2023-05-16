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

#include <cstddef>
#include <random>
#include <sstream>
#include <unordered_map>

#include <vtkm/ImplicitFunction.h>
#include <vtkm/Particle.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/FieldRangeCompute.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/cont/internal/OptionParser.h>

#include <vtkm/filter/contour/Contour.h>
#include <vtkm/filter/contour/Slice.h>
#include <vtkm/filter/flow/Streamline.h>
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

// Global counters for number of cycles
// These are globals because google benchmarks restarts the test for every
// repetition when using --benchmark_repetitions

// Additionlly, we need this global flag for when not doing repetitions,
// as benchmark will repeatedly drop in and out of the measured function
// and report the number of iterations for the last run of the function.
// Thus, we'll have way more output images than what the iteration number
// would lead you to believe (maybe fixed in > 1.7 with warmup time specifier)
bool benchmark_repetitions = false;

inline void hash_combine(std::size_t& vtkmNotUsed(seed)) {}

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  hash_combine(seed, rest...);
}

std::unordered_map<size_t, int> bench_cycles;

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
  vtkm::source::PerlinNoise noise;
  noise.SetPointDimensions({ dim, dim, dim });
  noise.SetSeed(static_cast<vtkm::IdComponent>(cycle));
  if (isMultiBlock)
  {
    for (auto i = 0; i < 2; ++i)
    {
      for (auto j = 0; j < 2; ++j)
      {
        for (auto k = 0; k < 2; ++k)
        {
          noise.SetOrigin({ static_cast<vtkm::FloatDefault>(i),
                            static_cast<vtkm::FloatDefault>(j),
                            static_cast<vtkm::FloatDefault>(k) });
          const auto dataset = noise.Execute();
          partitionedInputDataSet.AppendPartition(dataset);
        }
      }
    }
  }
  else
  {
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

  const vtkm::Id numIsoVals = static_cast<vtkm::Id>(state.range(0));
  const bool isStructured = static_cast<bool>(state.range(1));
  const bool isMultiBlock = static_cast<bool>(state.range(2));
  const RenderingMode renderAlgo = static_cast<RenderingMode>(state.range(3));

  vtkm::cont::Timer totalTimer{ device };
  vtkm::cont::Timer filterTimer{ device };
  vtkm::cont::Timer renderTimer{ device };
  vtkm::cont::Timer writeTimer{ device };

  size_t hash_val = 0;
  hash_combine(hash_val, std::string("BenchContour"), isStructured, isMultiBlock, renderAlgo);

  int* cycles = &(bench_cycles[hash_val]);
  if (!benchmark_repetitions)
    *cycles = 0;

  for (auto _ : state)
  {
    (void)_;
    totalTimer.Start();

    // Disable the benchmark timers for the updating/creation of the datasets
    state.PauseTiming(); // Stop timers.
    vtkm::cont::Timer inputGenTimer{ device };
    inputGenTimer.Start();
    BuildInputDataSet(*cycles, isStructured, isMultiBlock, DataSetDim);
    inputGenTimer.Stop();

    vtkm::filter::contour::Contour filter;
    filter.SetActiveField(PointScalarsName, vtkm::cont::Field::Association::Points);
    filter.SetMergeDuplicatePoints(true);
    filter.SetGenerateNormals(true);
    filter.SetComputeFastNormals(true);
    state.ResumeTiming(); // And resume timers.

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
    WriteToDisk(*canvas, renderAlgo, "contour", isStructured, isMultiBlock, *cycles);
    writeTimer.Stop();

    totalTimer.Stop();

    (*cycles)++;

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
  bm->ArgNames({ "NIsos", "IsStructured", "IsMultiBlock", "RenderingMode" });

  std::vector<uint32_t> isStructureds{ false, true };
  std::vector<uint32_t> isMultiBlocks{ false, true };
  std::vector<RenderingMode> renderingModes{ RenderingMode::RayTrace };
  for (auto& isStructured : isStructureds)
  {
    for (auto& isMultiBlock : isMultiBlocks)
    {
      for (auto& mode : renderingModes)
      {
        size_t hash_val = 0;
        hash_combine(hash_val, std::string("BenchContour"), isStructured, isMultiBlock, mode);
        auto search = bench_cycles.find(hash_val);
        // If we can't find the hash, or we're not doing repetitions, reset to 0
        if (search == bench_cycles.end())
          bench_cycles[hash_val] = 0;
        bm->Args({ 10, isStructured, isMultiBlock, static_cast<int>(mode) });
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
DataSetType RunStreamlinesHelper(vtkm::filter::flow::Streamline& filter, const DataSetType& input)
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

  const bool isStructured = static_cast<bool>(state.range(0));
  const bool isMultiBlock = static_cast<bool>(state.range(1));
  const RenderingMode renderAlgo = static_cast<RenderingMode>(state.range(2));

  vtkm::cont::Timer totalTimer{ device };
  vtkm::cont::Timer filterTimer{ device };
  vtkm::cont::Timer renderTimer{ device };
  vtkm::cont::Timer writeTimer{ device };

  size_t hash_val = 0;
  hash_combine(hash_val, std::string("BenchStreamlines"), isStructured, isMultiBlock, renderAlgo);

  int* cycles = &(bench_cycles[hash_val]);
  if (!benchmark_repetitions)
    *cycles = 0;

  for (auto _ : state)
  {
    (void)_;
    totalTimer.Start();

    // Disable the benchmark timers for the updating/creation of the datasets
    state.PauseTiming(); // Stop timers.
    vtkm::cont::Timer inputGenTimer{ device };
    inputGenTimer.Start();
    BuildInputDataSet(*cycles, isStructured, isMultiBlock, DataSetDim);
    inputGenTimer.Stop();

    vtkm::filter::flow::Streamline streamline;
    streamline.SetStepSize(0.1f);
    streamline.SetNumberOfSteps(1000);
    streamline.SetActiveField(PointVectorsName);
    state.ResumeTiming(); // And resume timers.

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
    WriteToDisk(*canvas, renderAlgo, "streamlines", isStructured, isMultiBlock, *cycles);
    writeTimer.Stop();

    totalTimer.Stop();

    (*cycles)++;

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
  bm->ArgNames({ "IsStructured", "IsMultiBlock", "RenderingMode" });

  std::vector<uint32_t> isStructureds{ false, true };
  std::vector<uint32_t> isMultiBlocks{ false, true };
  std::vector<RenderingMode> renderingModes{ RenderingMode::Mesh };
  for (auto& isStructured : isStructureds)
  {
    for (auto& isMultiBlock : isMultiBlocks)
    {
      for (auto& mode : renderingModes)
      {
        size_t hash_val = 0;
        hash_combine(hash_val, std::string("BenchStreamlines"), isStructured, isMultiBlock, mode);
        auto search = bench_cycles.find(hash_val);
        // If we can't find the hash, or we're not doing repetitions, reset to 0
        if (search == bench_cycles.end())
          bench_cycles[hash_val] = 0;
        bm->Args({ isStructured, isMultiBlock, static_cast<int>(mode) });
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

  const bool isStructured = static_cast<bool>(state.range(0));
  const bool isMultiBlock = static_cast<bool>(state.range(1));
  const RenderingMode renderAlgo = static_cast<RenderingMode>(state.range(2));

  vtkm::filter::contour::Slice filter;

  vtkm::cont::Timer totalTimer{ device };
  vtkm::cont::Timer filterTimer{ device };
  vtkm::cont::Timer renderTimer{ device };
  vtkm::cont::Timer writeTimer{ device };

  size_t hash_val = 0;
  hash_combine(hash_val, std::string("BenchSlice"), isStructured, isMultiBlock, renderAlgo);

  int* cycles = &(bench_cycles[hash_val]);
  if (!benchmark_repetitions)
    *cycles = 0;

  for (auto _ : state)
  {
    (void)_;
    totalTimer.Start();

    // Disable the benchmark timers for the updating/creation of the datasets
    state.PauseTiming(); // Stop timers.
    vtkm::cont::Timer inputGenTimer{ device };
    inputGenTimer.Start();
    BuildInputDataSet(*cycles, isStructured, isMultiBlock, DataSetDim);
    inputGenTimer.Stop();
    state.ResumeTiming(); // And resume timers.

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
    WriteToDisk(*canvas, renderAlgo, "slice", isStructured, isMultiBlock, *cycles);
    writeTimer.Stop();

    totalTimer.Stop();

    (*cycles)++;

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
  bm->ArgNames({ "IsStructured", "IsMultiBlock", "RenderingMode" });

  std::vector<uint32_t> isStructureds{ false, true };
  std::vector<uint32_t> isMultiBlocks{ false, true };
  std::vector<RenderingMode> renderingModes{ RenderingMode::RayTrace };
  for (auto& isStructured : isStructureds)
  {
    for (auto& isMultiBlock : isMultiBlocks)
    {
      for (auto& mode : renderingModes)
      {
        size_t hash_val = 0;
        hash_combine(hash_val, std::string("BenchSlice"), isStructured, isMultiBlock, mode);
        auto search = bench_cycles.find(hash_val);
        // If we can't find the hash, or we're not doing repetitions, reset to 0
        if (search == bench_cycles.end())
          bench_cycles[hash_val] = 0;
        bm->Args({ isStructured, isMultiBlock, static_cast<int>(mode) });
      }
    }
  }
}

VTKM_BENCHMARK_APPLY(BenchSlice, BenchSliceGenerator);

void BenchMeshRendering(::benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;

  const bool isStructured = static_cast<bool>(state.range(0));
  const bool isMultiBlock = static_cast<bool>(state.range(1));

  vtkm::cont::Timer inputGenTimer{ device };
  vtkm::cont::Timer renderTimer{ device };
  vtkm::cont::Timer writeTimer{ device };

  vtkm::cont::Timer totalTimer{ device };

  size_t hash_val = 0;
  hash_combine(hash_val, std::string("BenchMeshRendering"), isStructured, isMultiBlock);

  int* cycles = &(bench_cycles[hash_val]);
  if (!benchmark_repetitions)
    *cycles = 0;

  for (auto _ : state)
  {
    (void)_;

    totalTimer.Start();

    // Disable the benchmark timers for the updating/creation of the datasets
    state.PauseTiming(); // Stop timers.
    inputGenTimer.Start();
    BuildInputDataSet(*cycles, isStructured, isMultiBlock, DataSetDim);
    inputGenTimer.Stop();
    state.ResumeTiming(); // And resume timers.

    std::vector<vtkm::cont::DataSet> dataSets =
      isMultiBlock ? ExtractDataSets(PartitionedInputDataSet) : ExtractDataSets(InputDataSet);

    renderTimer.Start();
    auto canvas = RenderDataSets(dataSets, RenderingMode::Mesh, PointScalarsName);
    renderTimer.Stop();

    writeTimer.Start();
    WriteToDisk(*canvas, RenderingMode::Mesh, "mesh", isStructured, isMultiBlock, *cycles);
    writeTimer.Stop();

    totalTimer.Stop();

    (*cycles)++;

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
  bm->ArgNames({ "IsStructured", "IsMultiBlock" });

  std::vector<uint32_t> isStructureds{ false, true };
  std::vector<uint32_t> isMultiBlocks{ false, true };
  for (auto& isStructured : isStructureds)
  {
    for (auto& isMultiBlock : isMultiBlocks)
    {
      size_t hash_val = 0;
      hash_combine(hash_val, std::string("BenchMeshRendering"), isStructured, isMultiBlock);
      auto search = bench_cycles.find(hash_val);
      // If we can't find the hash, or we're not doing repetitions, reset to 0
      if (search == bench_cycles.end())
        bench_cycles[hash_val] = 0;
      bm->Args({ isStructured, isMultiBlock });
    }
  }
}

VTKM_BENCHMARK_APPLY(BenchMeshRendering, BenchMeshRenderingGenerator);

void BenchVolumeRendering(::benchmark::State& state)
{
  const vtkm::cont::DeviceAdapterId device = Config.Device;

  const bool isStructured = true;
  const bool isMultiBlock = static_cast<bool>(state.range(0));

  vtkm::cont::Timer totalTimer{ device };
  vtkm::cont::Timer renderTimer{ device };
  vtkm::cont::Timer writeTimer{ device };

  size_t hash_val = 0;
  hash_combine(hash_val, std::string("BenchVolumeRendering"), isMultiBlock);

  int* cycles = &(bench_cycles[hash_val]);
  if (!benchmark_repetitions)
    *cycles = 0;

  for (auto _ : state)
  {
    (void)_;
    totalTimer.Start();

    // Disable the benchmark timers for the updating/creation of the datasets
    state.PauseTiming(); // Stop timers.
    vtkm::cont::Timer inputGenTimer{ device };
    inputGenTimer.Start();
    BuildInputDataSet(*cycles, isStructured, isMultiBlock, DataSetDim);
    inputGenTimer.Stop();
    state.ResumeTiming(); // And resume timers.

    renderTimer.Start();
    std::vector<vtkm::cont::DataSet> dataSets =
      isMultiBlock ? ExtractDataSets(PartitionedInputDataSet) : ExtractDataSets(InputDataSet);
    auto canvas = RenderDataSets(dataSets, RenderingMode::Volume, PointScalarsName);
    renderTimer.Stop();

    writeTimer.Start();
    WriteToDisk(*canvas, RenderingMode::Volume, "volume", isStructured, isMultiBlock, *cycles);
    writeTimer.Stop();

    totalTimer.Stop();

    (*cycles)++;

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
  bm->ArgNames({ "IsMultiBlock" });

  std::vector<uint32_t> isMultiBlocks{ false };
  for (auto& isMultiBlock : isMultiBlocks)
  {
    size_t hash_val = 0;
    hash_combine(hash_val, std::string("BenchVolumeRendering"), isMultiBlock);
    auto search = bench_cycles.find(hash_val);
    // If we can't find the hash, or we're not doing repetitions, reset to 0
    if (search == bench_cycles.end())
      bench_cycles[hash_val] = 0;
    bm->Args({ isMultiBlock });
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

// Adding a const char* or std::string to a vector of char* is harder than it sounds.
void AddArg(int& argc, std::vector<char*>& args, const std::string newArg)
{
  // This object will be deleted when the program exits
  static std::vector<std::vector<char>> stringPool;

  // Add a new vector of chars to the back of stringPool
  stringPool.emplace_back();
  std::vector<char>& newArgData = stringPool.back();

  // Copy the string to the std::vector.
  // Yes, something like malloc or strdup would be easier. But that would technically create
  // a memory leak that could be reported by a memory analyzer. By copying this way, the
  // memory will be deleted on program exit and no leak will be reported.
  newArgData.resize(newArg.length() + 1);
  std::copy(newArg.begin(), newArg.end(), newArgData.begin());
  newArgData.back() = '\0'; // Don't forget the terminating null character.

  // Add the argument to the list.
  if (args.size() <= static_cast<std::size_t>(argc))
  {
    args.resize(static_cast<std::size_t>(argc + 1));
  }
  args[static_cast<std::size_t>(argc)] = newArgData.data();

  ++argc;
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

  bool benchmark_min_time = false;
  bool benchmark_report_aggregates_only = false;
  for (auto i = 0; i <= argc; i++)
    if (!strncmp(args[i], "--benchmark_repetitions", 23))
      benchmark_repetitions = true;
    else if (!strncmp(args[i], "--benchmark_min_time", 20))
      benchmark_min_time = true;
    else if (!strncmp(args[i], "--benchmark_report_aggregates_only", 34))
      benchmark_report_aggregates_only = true;

  // If repetitions are explicitly set without also specifying a minimum_time,
  // force the minimum time to be fairly small so that in all likelyhood, benchmarks
  // will only run 1 iteration for each test
  //
  // And, for good measure, only output the accumulated statistics
  if (benchmark_repetitions)
  {
    if (!benchmark_min_time)
    {
      AddArg(argc, args, "--benchmark_min_time=0.00000001");
    }

    if (!benchmark_report_aggregates_only)
    {
      AddArg(argc, args, "--benchmark_report_aggregates_only=true");
    }
  }

  VTKM_EXECUTE_BENCHMARKS(argc, args.data());
}
