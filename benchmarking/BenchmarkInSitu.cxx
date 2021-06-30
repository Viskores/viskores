#include "Benchmarker.h"

#include <random>
#include <sstream>

#include <vtkm/ImplicitFunction.h>

#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/FieldRangeCompute.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/cont/internal/OptionParser.h>

#include <vtkm/filter/Contour.h>
#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/Gradient.h>
#include <vtkm/filter/Slice.h>
#include <vtkm/filter/Streamline.h>
#include <vtkm/filter/Tetrahedralize.h>
#include <vtkm/filter/Tube.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>

namespace
{

const uint32_t DEFAULT_NUM_CYCLES = 20;
const vtkm::Id DEFAULT_DATASET_DIM = 128;
const vtkm::FloatDefault DEFAULT_SPACING = 0.1f;

// Hold configuration state (e.g. active device):
vtkm::cont::InitializeResult Config;
// Input dataset dimensions:
static vtkm::Id DataSetDim;
// The input datasets we'll use on the filters:
static vtkm::cont::DataSet InputDataSet;
static vtkm::cont::PartitionedDataSet PartitionedInputDataSet;
// The point scalars to use:
static std::string PointScalarsName;
// The point vectors to use:
static std::string PointVectorsName;

enum class RenderingMode
{
  None = 0,
  Mesh = 1,
  RayTrace = 2,
  Volume = 3,
};

struct PerlinNoise3DWorklet : public vtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn, FieldInPoint, WholeArrayIn, FieldOut noise);
  using ExecutionSignature = void(_2, _3, _4);

  VTKM_CONT PerlinNoise3DWorklet(vtkm::Id repeat)
    : Repeat(repeat)
  {
  }

  // Adapted from https://adrianb.io/2014/08/09/perlinnoise.html
  // Archive link: https://web.archive.org/web/20210329174559/https://adrianb.io/2014/08/09/perlinnoise.html
  template <typename PointVecType, typename PermsPortal, typename OutType>
  VTKM_EXEC void operator()(const PointVecType& pos, const PermsPortal& perms, OutType& noise) const
  {
    vtkm::Id xi = static_cast<vtkm::Id>(pos[0]) % this->Repeat;
    vtkm::Id yi = static_cast<vtkm::Id>(pos[1]) % this->Repeat;
    vtkm::Id zi = static_cast<vtkm::Id>(pos[2]) % this->Repeat;
    vtkm::FloatDefault xf = pos[0] - xi;
    vtkm::FloatDefault yf = pos[1] - yi;
    vtkm::FloatDefault zf = pos[2] - zi;
    vtkm::FloatDefault u = this->Fade(xf);
    vtkm::FloatDefault v = this->Fade(yf);
    vtkm::FloatDefault w = this->Fade(zf);

    vtkm::Id aaa, aba, aab, abb, baa, bba, bab, bbb;
    aaa = perms[perms[perms[xi] + yi] + zi];
    aba = perms[perms[perms[xi] + this->Increment(yi)] + zi];
    aab = perms[perms[perms[xi] + yi] + this->Increment(zi)];
    abb = perms[perms[perms[xi] + this->Increment(yi)] + this->Increment(zi)];
    baa = perms[perms[perms[this->Increment(xi)] + yi] + zi];
    bba = perms[perms[perms[this->Increment(xi)] + this->Increment(yi)] + zi];
    bab = perms[perms[perms[this->Increment(xi)] + yi] + this->Increment(zi)];
    bbb = perms[perms[perms[this->Increment(xi)] + this->Increment(yi)] + this->Increment(zi)];

    vtkm::FloatDefault x1, x2, y1, y2;
    x1 = vtkm::Lerp(this->Gradient(aaa, xf, yf, zf), this->Gradient(baa, xf - 1, yf, zf), u);
    x2 =
      vtkm::Lerp(this->Gradient(aba, xf, yf - 1, zf), this->Gradient(bba, xf - 1, yf - 1, zf), u);
    y1 = vtkm::Lerp(x1, x2, v);

    x1 =
      vtkm::Lerp(this->Gradient(aab, xf, yf, zf - 1), this->Gradient(bab, xf - 1, yf, zf - 1), u);
    x2 = vtkm::Lerp(
      this->Gradient(abb, xf, yf - 1, zf - 1), this->Gradient(bbb, xf - 1, yf - 1, zf - 1), u);
    y2 = vtkm::Lerp(x1, x2, v);

    noise = (vtkm::Lerp(y1, y2, w) + OutType(1.0f)) * OutType(0.5f);
  }

  VTKM_EXEC vtkm::FloatDefault Fade(vtkm::FloatDefault t) const
  {
    return t * t * t * (t * (t * 6 - 15) + 10);
  }

  VTKM_EXEC vtkm::Id Increment(vtkm::Id n) const { return (n + 1) % this->Repeat; }

  VTKM_EXEC vtkm::FloatDefault Gradient(vtkm::Id hash,
                                        vtkm::FloatDefault x,
                                        vtkm::FloatDefault y,
                                        vtkm::FloatDefault z) const
  {
    switch (hash & 0xF)
    {
      case 0x0:
        return x + y;
      case 0x1:
        return -x + y;
      case 0x2:
        return x - y;
      case 0x3:
        return -x - y;
      case 0x4:
        return x + z;
      case 0x5:
        return -x + z;
      case 0x6:
        return x - z;
      case 0x7:
        return -x - z;
      case 0x8:
        return y + z;
      case 0x9:
        return -y + z;
      case 0xA:
        return y - z;
      case 0xB:
        return -y - z;
      case 0xC:
        return y + x;
      case 0xD:
        return -y + z;
      case 0xE:
        return y - x;
      case 0xF:
        return -y - z;
      default:
        return 0; // never happens
    }
  }

  vtkm::Id Repeat;
};

class PerlinNoise3DGenerator : public vtkm::filter::FilterField<PerlinNoise3DGenerator>
{
public:
  VTKM_CONT PerlinNoise3DGenerator(vtkm::IdComponent tableSize, vtkm::Id seed)
    : TableSize(tableSize)
    , Seed(seed)
  {
    this->GeneratePermutations();
    this->SetUseCoordinateSystemAsField(true);
  }

  template <typename FieldType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const FieldType&,
                                          const vtkm::filter::FieldMetadata& fieldMetadata,
                                          vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> noiseArray;
    PerlinNoise3DWorklet worklet{ this->TableSize };
    this->Invoke(worklet, input.GetCellSet(), input.GetCoordinateSystem(), this->P, noiseArray);

    return vtkm::filter::CreateResult(input, noiseArray, "PerlinNoise3D", fieldMetadata);
  }

protected:
  VTKM_CONT void GeneratePermutations()
  {
    std::mt19937_64 rng;
    rng.seed(this->Seed);
    std::uniform_int_distribution<vtkm::Id> distribution(0, this->TableSize - 1);

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> perms;
    perms.Allocate(this->TableSize);
    auto permsPortal = perms.WritePortal();
    for (auto i = 0; i < permsPortal.GetNumberOfValues(); ++i)
    {
      permsPortal.Set(i, distribution(rng));
    }
    this->P.Allocate(2 * this->TableSize);
    auto pPortal = this->P.WritePortal();
    for (auto i = 0; i < pPortal.GetNumberOfValues(); ++i)
    {
      pPortal.Set(i, permsPortal.Get(i % this->TableSize));
    }
  }


private:
  vtkm::IdComponent TableSize;
  vtkm::Id Seed;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> P;
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

void BuildInputDataSet(uint32_t cycle,
                       bool isStructured,
                       bool isMultiBlock,
                       vtkm::Id dims,
                       vtkm::FloatDefault spacing = 0.1f)
{
  vtkm::cont::PartitionedDataSet partitionedInputDataSet;
  vtkm::cont::DataSet inputDataSet;

  PointScalarsName = "PerlinNoise3D";
  PointVectorsName = "PerlinNoise3DGradient";

  // Generate uniform dataset(s)
  const vtkm::Id3 dataSetDims{ dims, dims, dims };
  const vtkm::Vec3f dataSetSpacing{ spacing, spacing, spacing };
  if (isMultiBlock)
  {
    for (auto i = 0; i < 2; ++i)
    {
      for (auto j = 0; j < 2; ++j)
      {
        for (auto k = 0; k < 2; ++k)
        {
          const vtkm::Vec3f dataSetOrigin{ (dims - 1) * spacing * i,
                                           (dims - 1) * spacing * j,
                                           (dims - 1) * spacing * k };
          vtkm::cont::DataSetBuilderUniform dataSetBuilder;
          vtkm::cont::DataSet uniformDataSet =
            dataSetBuilder.Create(dataSetDims, dataSetOrigin, dataSetSpacing);
          partitionedInputDataSet.AppendPartition(uniformDataSet);
        }
      }
    }
  }
  else
  {
    const vtkm::Vec3f dataSetOrigin{ 0.0f, 0.0f, 0.0f };
    vtkm::cont::DataSetBuilderUniform dataSetBuilder;
    inputDataSet = dataSetBuilder.Create(dataSetDims, dataSetOrigin, dataSetSpacing);
  }

  // Generate Perlin Noise point scalar field
  PerlinNoise3DGenerator fieldGenerator(dims, cycle);
  if (isMultiBlock)
  {
    partitionedInputDataSet = fieldGenerator.Execute(partitionedInputDataSet);
  }
  else
  {
    inputDataSet = fieldGenerator.Execute(inputDataSet);
  }

  // Generate Perln Noise Gradient point vector field
  vtkm::filter::Gradient gradientFilter;
  gradientFilter.SetActiveField(PointScalarsName, vtkm::cont::Field::Association::POINTS);
  gradientFilter.SetComputePointGradient(true);
  gradientFilter.SetOutputFieldName(PointVectorsName);
  gradientFilter.SetFieldsToPass(
    vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::MODE_ALL));
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
    vtkm::filter::Tetrahedralize destructizer;
    destructizer.SetFieldsToPass(
      vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::MODE_ALL));
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

void RenderDataSets(const std::vector<vtkm::cont::DataSet>& dataSets,
                    RenderingMode mode,
                    std::string fieldName,
                    std::string bench,
                    bool isStructured,
                    uint32_t cycle)
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

  vtkm::rendering::CanvasRayTracer canvas(1920, 1080);

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

  // TODO: Remove this later, once the various benchmark quirks are fixed
  (void)cycle;
  (void)isStructured;
  (void)bench;
  /*
  std::ostringstream p;
  p << "output_"
    << "c" << cycle << "_" << (isStructured ? "structured" : "unstructured") << "_"
    << (dataSets.size() == 1 ? "single_" : "multi_") << bench << "_"
    << (mode == RenderingMode::Mesh ? "mesh" : (mode == RenderingMode::Volume ? "volume" : "ray"))
    << ".png";
  view.SaveAs(p.str());
  */
}

template <typename DataSetType>
DataSetType RunContourHelper(vtkm::filter::Contour& filter,
                             vtkm::Id numIsoVals,
                             const DataSetType& input)
{
  // Set up some equally spaced contours, with the min/max slightly inside the
  // scalar range:
  const vtkm::Range scalarRange =
    vtkm::cont::ArrayGetValue(0, vtkm::cont::FieldRangeCompute(input, PointScalarsName));
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
  BuildInputDataSet(cycle, isStructured, isMultiBlock, DataSetDim, DEFAULT_SPACING);
  inputGenTimer.Stop();

  vtkm::filter::Contour filter;
  filter.SetActiveField(PointScalarsName, vtkm::cont::Field::Association::POINTS);
  filter.SetMergeDuplicatePoints(true);
  filter.SetGenerateNormals(true);
  filter.SetComputeFastNormalsForStructured(true);
  filter.SetComputeFastNormalsForUnstructured(true);

  vtkm::cont::Timer totalTimer{ device };
  vtkm::cont::Timer filterTimer{ device };
  vtkm::cont::Timer renderTimer{ device };

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
    RenderDataSets(dataSets, renderAlgo, PointScalarsName, "contour", isStructured, cycle);
    renderTimer.Stop();

    totalTimer.Stop();

    state.SetIterationTime(totalTimer.GetElapsedTime());
    state.counters.insert(
      { { "InputGenTime", static_cast<uint32_t>(inputGenTimer.GetElapsedTime() * 1000) },
        { "FilterTime", static_cast<uint32_t>(filterTimer.GetElapsedTime() * 1000) },
        { "RenderTime", static_cast<uint32_t>(renderTimer.GetElapsedTime() * 1000) } });
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
  BuildInputDataSet(cycle, isStructured, isMultiBlock, DataSetDim, DEFAULT_SPACING);
  inputGenTimer.Stop();

  vtkm::filter::Streamline streamline;
  streamline.SetStepSize(0.1f);
  streamline.SetNumberOfSteps(1000);
  streamline.SetActiveField(PointVectorsName);

  vtkm::cont::Timer totalTimer{ device };
  vtkm::cont::Timer filterTimer{ device };
  vtkm::cont::Timer renderTimer{ device };

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
    RenderDataSets(dataSets, renderAlgo, "pointvar", "streamlines", isStructured, cycle);
    renderTimer.Stop();

    totalTimer.Stop();

    state.SetIterationTime(totalTimer.GetElapsedTime());
    state.counters.insert(
      { { "InputGenTime", static_cast<uint32_t>(inputGenTimer.GetElapsedTime() * 1000) },
        { "FilterTime", static_cast<uint32_t>(filterTimer.GetElapsedTime() * 1000) },
        { "RenderTime", static_cast<uint32_t>(renderTimer.GetElapsedTime() * 1000) } });
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
  BuildInputDataSet(cycle, isStructured, isMultiBlock, DataSetDim, DEFAULT_SPACING);
  inputGenTimer.Stop();

  vtkm::filter::Slice filter;

  vtkm::cont::Timer totalTimer{ device };
  vtkm::cont::Timer filterTimer{ device };
  vtkm::cont::Timer renderTimer{ device };

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
    RenderDataSets(dataSets, renderAlgo, PointScalarsName, "slice", isStructured, cycle);
    renderTimer.Stop();

    totalTimer.Stop();

    state.SetIterationTime(totalTimer.GetElapsedTime());
    state.counters.insert(
      { { "InputGenTime", static_cast<uint32_t>(inputGenTimer.GetElapsedTime() * 1000) },
        { "FilterTime", static_cast<uint32_t>(filterTimer.GetElapsedTime() * 1000) },
        { "RenderTime", static_cast<uint32_t>(renderTimer.GetElapsedTime() * 1000) } });
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
  inputGenTimer.Start();
  BuildInputDataSet(cycle, isStructured, isMultiBlock, DataSetDim, DEFAULT_SPACING);
  inputGenTimer.Stop();

  vtkm::cont::Timer totalTimer{ device };

  for (auto _ : state)
  {
    (void)_;

    totalTimer.Start();
    std::vector<vtkm::cont::DataSet> dataSets =
      isMultiBlock ? ExtractDataSets(PartitionedInputDataSet) : ExtractDataSets(InputDataSet);
    RenderDataSets(dataSets, RenderingMode::Mesh, PointScalarsName, "mesh", isStructured, cycle);
    totalTimer.Stop();

    state.SetIterationTime(totalTimer.GetElapsedTime());
    state.counters.insert(
      { { "InputGenTime", static_cast<uint32_t>(inputGenTimer.GetElapsedTime() * 1000) } });
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
  BuildInputDataSet(cycle, isStructured, isMultiBlock, DataSetDim, DEFAULT_SPACING);
  inputGenTimer.Stop();

  vtkm::cont::Timer totalTimer{ device };

  for (auto _ : state)
  {
    (void)_;
    vtkm::rendering::Scene scene;

    totalTimer.Start();
    std::vector<vtkm::cont::DataSet> dataSets =
      isMultiBlock ? ExtractDataSets(PartitionedInputDataSet) : ExtractDataSets(InputDataSet);
    RenderDataSets(
      dataSets, RenderingMode::Volume, PointScalarsName, "volume", isStructured, cycle);
    totalTimer.Stop();

    state.SetIterationTime(totalTimer.GetElapsedTime());
    state.counters.insert(
      { { "InputGenTime", static_cast<uint32_t>(inputGenTimer.GetElapsedTime() * 1000) } });
  }
}

void BenchVolumeRenderingGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "Cycle", "IsMultiBlock" });

  std::vector<uint32_t> isMultiBlocks{ false, /*true*/ };
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

  static vtkm::cont::internal::option::ArgStatus Required(
    const vtkm::cont::internal::option::Option& option,
    bool msg)
  {
    if ((option.arg != nullptr) && (option.arg[0] != '\0'))
    {
      return vtkm::cont::internal::option::ARG_OK;
    }
    else
    {
      if (msg)
      {
        std::cerr << "Option " << option.name << " requires an argument." << std::endl;
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
                    "dataset with NxNxN dimensions and 0.1 spacing is created. "
                    "If not specified, N=128" });
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
    DataSetDim = 128;
  }

  std::cerr << "Using data set dimensions(N) = " << DataSetDim << std::endl;

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
