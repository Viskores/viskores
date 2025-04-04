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

#include <viskores/Math.h>
#include <viskores/Range.h>
#include <viskores/VecTraits.h>
#include <viskores/VectorAnalysis.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/ErrorInternal.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/Timer.h>

#include <viskores/cont/internal/OptionParser.h>

#include <viskores/filter/FieldSelection.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/entity_extraction/ExternalFaces.h>
#include <viskores/filter/entity_extraction/Threshold.h>
#include <viskores/filter/entity_extraction/ThresholdPoints.h>
#include <viskores/filter/field_conversion/CellAverage.h>
#include <viskores/filter/field_conversion/PointAverage.h>
#include <viskores/filter/field_transform/Warp.h>
#include <viskores/filter/geometry_refinement/Tetrahedralize.h>
#include <viskores/filter/geometry_refinement/Triangulate.h>
#include <viskores/filter/geometry_refinement/VertexClustering.h>
#include <viskores/filter/vector_analysis/Gradient.h>
#include <viskores/filter/vector_analysis/VectorMagnitude.h>

#include <viskores/io/VTKDataSetReader.h>

#include <viskores/source/Wavelet.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <cctype> // for std::tolower
#include <sstream>
#include <type_traits>

#ifdef VISKORES_ENABLE_OPENMP
#include <omp.h>
#endif

// A specific vtk dataset can be used during benchmarking using the
// "Filename [filename]" argument.

// Otherwise a wavelet dataset will be used. The size of the wavelet can be
// specified via "WaveletDim [integer]" argument. The default is 256, resulting
// in a 256x256x256 (cell extent) dataset.

// Passing in the "Tetra" option will pass the input dataset through the
// Tetrahedralize filter to generate an unstructured, single cell type dataset.

// For the filters that require fields, the desired fields may be specified
// using these arguments:
//
// PointScalars [fieldname]
// CellScalars [fieldname]
// PointVectors [fieldname]
//
// If the fields are not specified, the first field with the correct association
// is used. If no such field exists, one will be generated from the data.

namespace
{

// Hold configuration state (e.g. active device):
viskores::cont::InitializeResult Config;

// The input dataset we'll use on the filters:
viskores::cont::DataSet* InputDataSet;
viskores::cont::DataSet* UnstructuredInputDataSet;
viskores::cont::DataSet& GetInputDataSet()
{
  return *InputDataSet;
}

viskores::cont::DataSet& GetUnstructuredInputDataSet()
{
  return *UnstructuredInputDataSet;
}

viskores::cont::PartitionedDataSet* InputPartitionedData;
viskores::cont::PartitionedDataSet* UnstructuredInputPartitionedData;
viskores::cont::PartitionedDataSet& GetInputPartitionedData()
{
  return *InputPartitionedData;
}
viskores::cont::PartitionedDataSet& GetUnstructuredInputPartitionedData()
{
  return *UnstructuredInputPartitionedData;
}

// The point scalars to use:
static std::string PointScalarsName;
// The cell scalars to use:
static std::string CellScalarsName;
// The point vectors to use:
static std::string PointVectorsName;
// Whether the input is a file or is generated
bool FileAsInput = false;

bool InputIsStructured()
{
  return GetInputDataSet().GetCellSet().IsType<viskores::cont::CellSetStructured<3>>() ||
    GetInputDataSet().GetCellSet().IsType<viskores::cont::CellSetStructured<2>>() ||
    GetInputDataSet().GetCellSet().IsType<viskores::cont::CellSetStructured<1>>();
}

enum GradOpts : int
{
  Gradient = 1,
  PointGradient = 1 << 1,
  Divergence = 1 << 2,
  Vorticity = 1 << 3,
  QCriterion = 1 << 4,
  RowOrdering = 1 << 5,
  ScalarInput = 1 << 6,
  PartitionedInput = 1 << 7
};

void BenchGradient(::benchmark::State& state, int options)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;

  viskores::filter::vector_analysis::Gradient filter;

  if (options & ScalarInput)
  {
    // Some outputs require vectors:
    if (options & Divergence || options & Vorticity || options & QCriterion)
    {
      throw viskores::cont::ErrorInternal("A requested gradient output is "
                                          "incompatible with scalar input.");
    }
    filter.SetActiveField(PointScalarsName, viskores::cont::Field::Association::Points);
  }
  else
  {
    filter.SetActiveField(PointVectorsName, viskores::cont::Field::Association::Points);
  }

  filter.SetComputeGradient(static_cast<bool>(options & Gradient));
  filter.SetComputePointGradient(static_cast<bool>(options & PointGradient));
  filter.SetComputeDivergence(static_cast<bool>(options & Divergence));
  filter.SetComputeVorticity(static_cast<bool>(options & Vorticity));
  filter.SetComputeQCriterion(static_cast<bool>(options & QCriterion));

  if (options & RowOrdering)
  {
    filter.SetRowMajorOrdering();
  }
  else
  {
    filter.SetColumnMajorOrdering();
  }

  viskores::cont::Timer timer{ device };
  //viskores::cont::DataSet input = static_cast<bool>(options & Structured) ? GetInputDataSet() : GetUnstructuredInputDataSet();

  viskores::cont::PartitionedDataSet input;
  if (options & PartitionedInput)
  {
    input = GetInputPartitionedData();
  }
  else
  {
    input = GetInputDataSet();
  }

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(input);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}

#define VISKORES_PRIVATE_GRADIENT_BENCHMARK(Name, Opts) \
  void BenchGradient##Name(::benchmark::State& state)   \
  {                                                     \
    BenchGradient(state, Opts);                         \
  }                                                     \
  VISKORES_BENCHMARK(BenchGradient##Name)

VISKORES_PRIVATE_GRADIENT_BENCHMARK(Scalar, Gradient | ScalarInput);
VISKORES_PRIVATE_GRADIENT_BENCHMARK(ScalarPartitionedData,
                                    Gradient | ScalarInput | PartitionedInput);
VISKORES_PRIVATE_GRADIENT_BENCHMARK(Vector, Gradient);
VISKORES_PRIVATE_GRADIENT_BENCHMARK(VectorPartitionedData, Gradient | PartitionedInput);
VISKORES_PRIVATE_GRADIENT_BENCHMARK(VectorRow, Gradient | RowOrdering);
VISKORES_PRIVATE_GRADIENT_BENCHMARK(Point, PointGradient);
VISKORES_PRIVATE_GRADIENT_BENCHMARK(Divergence, Divergence);
VISKORES_PRIVATE_GRADIENT_BENCHMARK(Vorticity, Vorticity);
VISKORES_PRIVATE_GRADIENT_BENCHMARK(QCriterion, QCriterion);
VISKORES_PRIVATE_GRADIENT_BENCHMARK(All,
                                    Gradient | PointGradient | Divergence | Vorticity | QCriterion);

#undef VISKORES_PRIVATE_GRADIENT_BENCHMARK

void BenchThreshold(::benchmark::State& state, bool partitionedInput)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;

  // Lookup the point scalar range
  const auto range = []() -> viskores::Range
  {
    auto ptScalarField =
      GetInputDataSet().GetField(PointScalarsName, viskores::cont::Field::Association::Points);
    return ptScalarField.GetRange().ReadPortal().Get(0);
  }();

  // Extract points with values between 25-75% of the range
  viskores::Float64 quarter = range.Length() / 4.;
  viskores::Float64 mid = range.Center();

  viskores::filter::entity_extraction::Threshold filter;
  filter.SetActiveField(PointScalarsName, viskores::cont::Field::Association::Points);
  filter.SetLowerThreshold(mid - quarter);
  filter.SetUpperThreshold(mid + quarter);

  auto input = partitionedInput ? GetInputPartitionedData() : GetInputDataSet();

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(input);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}

#define VISKORES_PRIVATE_THRESHOLD_BENCHMARK(Name, Opts) \
  void BenchThreshold##Name(::benchmark::State& state)   \
  {                                                      \
    BenchThreshold(state, Opts);                         \
  }                                                      \
  VISKORES_BENCHMARK(BenchThreshold##Name)

VISKORES_PRIVATE_THRESHOLD_BENCHMARK(BenchThreshold, false);
VISKORES_PRIVATE_THRESHOLD_BENCHMARK(BenchThresholdPartitioned, true);

void BenchThresholdPoints(::benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const bool compactPoints = static_cast<bool>(state.range(0));
  const bool partitionedInput = static_cast<bool>(state.range(1));

  // Lookup the point scalar range
  const auto range = []() -> viskores::Range
  {
    auto ptScalarField =
      GetInputDataSet().GetField(PointScalarsName, viskores::cont::Field::Association::Points);
    return ptScalarField.GetRange().ReadPortal().Get(0);
  }();

  // Extract points with values between 25-75% of the range
  viskores::Float64 quarter = range.Length() / 4.;
  viskores::Float64 mid = range.Center();

  viskores::filter::entity_extraction::ThresholdPoints filter;
  filter.SetActiveField(PointScalarsName, viskores::cont::Field::Association::Points);
  filter.SetLowerThreshold(mid - quarter);
  filter.SetUpperThreshold(mid + quarter);
  filter.SetCompactPoints(compactPoints);

  viskores::cont::PartitionedDataSet input;
  input = partitionedInput ? GetInputPartitionedData() : GetInputDataSet();

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(input);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}

void BenchThresholdPointsGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "CompactPts", "PartitionedInput" });

  bm->Args({ 0, 0 });
  bm->Args({ 1, 0 });
  bm->Args({ 0, 1 });
  bm->Args({ 1, 1 });
}

VISKORES_BENCHMARK_APPLY(BenchThresholdPoints, BenchThresholdPointsGenerator);

void BenchCellAverage(::benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;

  viskores::filter::field_conversion::CellAverage filter;
  filter.SetActiveField(PointScalarsName, viskores::cont::Field::Association::Points);

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(GetInputDataSet());
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VISKORES_BENCHMARK(BenchCellAverage);

void BenchPointAverage(::benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const bool isPartitioned = static_cast<bool>(state.range(0));

  viskores::filter::field_conversion::PointAverage filter;
  filter.SetActiveField(CellScalarsName, viskores::cont::Field::Association::Cells);

  viskores::cont::PartitionedDataSet input;
  input = isPartitioned ? GetInputPartitionedData() : GetInputDataSet();
  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(input);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VISKORES_BENCHMARK_OPTS(BenchPointAverage, ->ArgName("PartitionedInput")->DenseRange(0, 1));

void BenchWarpScalar(::benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const bool isPartitioned = static_cast<bool>(state.range(0));

  viskores::filter::field_transform::Warp filter;
  filter.SetScaleFactor(2.0f);
  filter.SetUseCoordinateSystemAsField(true);
  filter.SetDirectionField(PointVectorsName);
  filter.SetScaleField(PointScalarsName);

  viskores::cont::PartitionedDataSet input;
  input = isPartitioned ? GetInputPartitionedData() : GetInputDataSet();

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(input);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VISKORES_BENCHMARK_OPTS(BenchWarpScalar, ->ArgName("PartitionedInput")->DenseRange(0, 1));

void BenchWarpVector(::benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const bool isPartitioned = static_cast<bool>(state.range(0));

  viskores::filter::field_transform::Warp filter;
  filter.SetScaleFactor(2.0f);
  filter.SetUseCoordinateSystemAsField(true);
  filter.SetDirectionField(PointVectorsName);

  viskores::cont::PartitionedDataSet input;
  input = isPartitioned ? GetInputPartitionedData() : GetInputDataSet();

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(input);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VISKORES_BENCHMARK_OPTS(BenchWarpVector, ->ArgName("PartitionedInput")->DenseRange(0, 1));

void BenchContour(::benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;

  const bool isStructured = static_cast<viskores::Id>(state.range(0));
  const viskores::Id numIsoVals = static_cast<viskores::Id>(state.range(1));
  const bool mergePoints = static_cast<bool>(state.range(2));
  const bool normals = static_cast<bool>(state.range(3));
  const bool fastNormals = static_cast<bool>(state.range(4));
  const bool isPartitioned = static_cast<bool>(state.range(5));

  viskores::filter::contour::Contour filter;
  filter.SetActiveField(PointScalarsName, viskores::cont::Field::Association::Points);

  // Set up some equally spaced contours, with the min/max slightly inside the
  // scalar range:
  const viskores::Range scalarRange = []() -> viskores::Range
  {
    auto field =
      GetInputDataSet().GetField(PointScalarsName, viskores::cont::Field::Association::Points);
    return field.GetRange().ReadPortal().Get(0);
  }();
  const auto step = scalarRange.Length() / static_cast<viskores::Float64>(numIsoVals + 1);
  const auto minIsoVal = scalarRange.Min + (step / 2.);

  filter.SetNumberOfIsoValues(numIsoVals);
  for (viskores::Id i = 0; i < numIsoVals; ++i)
  {
    filter.SetIsoValue(i, minIsoVal + (step * static_cast<viskores::Float64>(i)));
  }

  filter.SetMergeDuplicatePoints(mergePoints);
  filter.SetGenerateNormals(normals);
  filter.SetComputeFastNormals(fastNormals);

  viskores::cont::Timer timer{ device };

  viskores::cont::PartitionedDataSet input;
  if (isPartitioned)
  {
    input = isStructured ? GetInputPartitionedData() : GetUnstructuredInputPartitionedData();
  }
  else
  {
    input = isStructured ? GetInputDataSet() : GetUnstructuredInputDataSet();
  }

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(input);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}

void BenchContourGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "IsStructuredDataSet",
                 "NIsoVals",
                 "MergePts",
                 "GenNormals",
                 "FastNormals",
                 "MultiPartitioned" });

  auto helper = [&](const viskores::Id numIsoVals)
  {
    bm->Args({ 0, numIsoVals, 0, 0, 0, 0 });
    bm->Args({ 0, numIsoVals, 1, 0, 0, 0 });
    bm->Args({ 0, numIsoVals, 0, 1, 0, 0 });
    bm->Args({ 0, numIsoVals, 0, 1, 1, 0 });
    bm->Args({ 1, numIsoVals, 0, 0, 0, 0 });
    bm->Args({ 1, numIsoVals, 1, 0, 0, 0 });
    bm->Args({ 1, numIsoVals, 0, 1, 0, 0 });
    bm->Args({ 1, numIsoVals, 0, 1, 1, 0 });

    bm->Args({ 0, numIsoVals, 0, 0, 0, 1 });
    bm->Args({ 0, numIsoVals, 1, 0, 0, 1 });
    bm->Args({ 0, numIsoVals, 0, 1, 0, 1 });
    bm->Args({ 0, numIsoVals, 0, 1, 1, 1 });
    bm->Args({ 1, numIsoVals, 0, 0, 0, 1 });
    bm->Args({ 1, numIsoVals, 1, 0, 0, 1 });
    bm->Args({ 1, numIsoVals, 0, 1, 0, 1 });
    bm->Args({ 1, numIsoVals, 0, 1, 1, 1 });
  };

  helper(1);
  helper(3);
  helper(12);
}

VISKORES_BENCHMARK_APPLY(BenchContour, BenchContourGenerator);

void BenchExternalFaces(::benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const bool compactPoints = static_cast<bool>(state.range(0));
  const bool isPartitioned = false; //static_cast<bool>(state.range(1));

  viskores::filter::entity_extraction::ExternalFaces filter;
  filter.SetCompactPoints(compactPoints);

  viskores::cont::PartitionedDataSet input;
  input = isPartitioned ? GetInputPartitionedData() : GetInputDataSet();

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(input);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}

void BenchExternalFacesGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "Compact", "PartitionedInput" });

  bm->Args({ 0, 0 });
  bm->Args({ 1, 0 });
  bm->Args({ 0, 1 });
  bm->Args({ 1, 1 });
}
VISKORES_BENCHMARK_APPLY(BenchExternalFaces, BenchExternalFacesGenerator);

void BenchTetrahedralize(::benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const bool isPartitioned = static_cast<bool>(state.range(0));

  // This filter only supports structured datasets:
  if (FileAsInput && !InputIsStructured())
  {
    state.SkipWithError("Tetrahedralize Filter requires structured data.");
  }

  viskores::filter::geometry_refinement::Tetrahedralize filter;
  viskores::cont::PartitionedDataSet input;
  input = isPartitioned ? GetInputPartitionedData() : GetInputDataSet();

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(input);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}

VISKORES_BENCHMARK_OPTS(BenchTetrahedralize, ->ArgName("PartitionedInput")->DenseRange(0, 1));

void BenchVertexClustering(::benchmark::State& state)
{
  const viskores::cont::DeviceAdapterId device = Config.Device;
  const viskores::Id numDivs = static_cast<viskores::Id>(state.range(0));

  // This filter only supports unstructured datasets:
  if (FileAsInput && InputIsStructured())
  {
    state.SkipWithError("VertexClustering Filter requires unstructured data (use --tetra).");
  }

  viskores::filter::geometry_refinement::VertexClustering filter;
  filter.SetNumberOfDivisions({ numDivs });

  viskores::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;

    timer.Start();
    auto result = filter.Execute(GetUnstructuredInputDataSet());
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VISKORES_BENCHMARK_OPTS(BenchVertexClustering,
                          ->RangeMultiplier(2)
                          ->Range(32, 1024)
                          ->ArgName("NumDivs"));

// Helper for resetting the reverse connectivity table:
struct PrepareForInput
{
  mutable viskores::cont::Timer Timer;

  PrepareForInput()
    : Timer{ Config.Device }
  {
  }

  void operator()(const viskores::cont::CellSet& cellSet) const
  {
    static bool warned{ false };
    if (!warned)
    {
      std::cerr << "Invalid cellset type for benchmark.\n";
      cellSet.PrintSummary(std::cerr);
      warned = true;
    }
  }

  template <typename T1, typename T2, typename T3>
  VISKORES_CONT void operator()(const viskores::cont::CellSetExplicit<T1, T2, T3>& cellSet) const
  {
    viskores::cont::TryExecuteOnDevice(Config.Device, *this, cellSet);
  }

  template <typename T1, typename T2, typename T3, typename DeviceTag>
  VISKORES_CONT bool operator()(DeviceTag,
                                const viskores::cont::CellSetExplicit<T1, T2, T3>& cellSet) const
  {
    // Why does CastAndCall insist on making the cellset const?
    using CellSetT = viskores::cont::CellSetExplicit<T1, T2, T3>;
    CellSetT& mcellSet = const_cast<CellSetT&>(cellSet);
    mcellSet.ResetConnectivity(viskores::TopologyElementTagPoint{},
                               viskores::TopologyElementTagCell{});

    viskores::cont::Token token;
    this->Timer.Start();
    auto result = cellSet.PrepareForInput(
      DeviceTag{}, viskores::TopologyElementTagPoint{}, viskores::TopologyElementTagCell{}, token);
    ::benchmark::DoNotOptimize(result);
    this->Timer.Stop();

    return true;
  }
};

void BenchReverseConnectivityGen(::benchmark::State& state)
{
  if (FileAsInput && InputIsStructured())
  {
    state.SkipWithError("ReverseConnectivityGen requires unstructured data (--use tetra).");
  }

  auto cellset = GetUnstructuredInputDataSet().GetCellSet();
  PrepareForInput functor;
  for (auto _ : state)
  {
    (void)_;
    viskores::cont::CastAndCall(cellset, functor);
    state.SetIterationTime(functor.Timer.GetElapsedTime());
  }
}
VISKORES_BENCHMARK(BenchReverseConnectivityGen);

// Generates a Vec3 field from point coordinates.
struct PointVectorGenerator : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  viskores::Bounds Bounds;
  viskores::Vec3f_64 Center;
  viskores::Vec3f_64 Scale;

  VISKORES_CONT
  PointVectorGenerator(const viskores::Bounds& bounds)
    : Bounds(bounds)
    , Center(bounds.Center())
    , Scale((6. * viskores::Pi()) / bounds.X.Length(),
            (2. * viskores::Pi()) / bounds.Y.Length(),
            (7. * viskores::Pi()) / bounds.Z.Length())
  {
  }

  template <typename T>
  VISKORES_EXEC viskores::Vec<T, 3> operator()(viskores::Vec<T, 3> val) const
  {
    using Vec3T = viskores::Vec<T, 3>;
    using Vec3F64 = viskores::Vec3f_64;

    Vec3F64 valF64{ val };
    Vec3F64 periodic = (valF64 - this->Center) * this->Scale;
    periodic[0] = viskores::Sin(periodic[0]);
    periodic[1] = viskores::Sin(periodic[1]);
    periodic[2] = viskores::Cos(periodic[2]);

    if (viskores::MagnitudeSquared(periodic) > 0.)
    {
      viskores::Normalize(periodic);
    }
    if (viskores::MagnitudeSquared(valF64) > 0.)
    {
      viskores::Normalize(valF64);
    }
    return Vec3T{ viskores::Normal(periodic + valF64) };
  }
};

void FindFields()
{
  if (PointScalarsName.empty())
  {
    for (viskores::Id i = 0; i < GetInputDataSet().GetNumberOfFields(); ++i)
    {
      auto field = GetInputDataSet().GetField(i);
      if (field.GetAssociation() == viskores::cont::Field::Association::Points &&
          field.GetData().GetNumberOfComponentsFlat() == 1)
      {
        PointScalarsName = field.GetName();
        std::cerr << "[FindFields] Found PointScalars: " << PointScalarsName << "\n";
        break;
      }
    }
  }

  if (CellScalarsName.empty())
  {
    for (viskores::Id i = 0; i < GetInputDataSet().GetNumberOfFields(); ++i)
    {
      auto field = GetInputDataSet().GetField(i);
      if (field.GetAssociation() == viskores::cont::Field::Association::Cells &&
          field.GetData().GetNumberOfComponentsFlat() == 1)
      {
        CellScalarsName = field.GetName();
        std::cerr << "[FindFields] CellScalars: " << CellScalarsName << "\n";
        break;
      }
    }
  }

  if (PointVectorsName.empty())
  {
    for (viskores::Id i = 0; i < GetInputDataSet().GetNumberOfFields(); ++i)
    {
      auto field = GetInputDataSet().GetField(i);
      if (field.GetAssociation() == viskores::cont::Field::Association::Points &&
          field.GetData().GetNumberOfComponentsFlat() == 3)
      {
        PointVectorsName = field.GetName();
        std::cerr << "[FindFields] Found PointVectors: " << PointVectorsName << "\n";
        break;
      }
    }
  }
}

void CreateMissingFields()
{
  // Do point vectors first, so we can generate the scalars from them if needed
  if (PointVectorsName.empty())
  {
    // Construct them from the coordinates:
    auto coords = GetInputDataSet().GetCoordinateSystem();
    auto bounds = coords.GetBounds();
    auto points = coords.GetData();
    viskores::cont::ArrayHandle<viskores::Vec3f> pvecs;

    PointVectorGenerator worklet(bounds);
    viskores::worklet::DispatcherMapField<PointVectorGenerator> dispatch(worklet);
    dispatch.Invoke(points, pvecs);
    GetInputDataSet().AddField(viskores::cont::Field(
      "GeneratedPointVectors", viskores::cont::Field::Association::Points, pvecs));
    PointVectorsName = "GeneratedPointVectors";
    std::cerr << "[CreateFields] Generated point vectors '" << PointVectorsName
              << "' from coordinate data.\n";
  }

  if (PointScalarsName.empty())
  {
    if (!CellScalarsName.empty())
    { // Generate from found cell field:
      viskores::filter::field_conversion::PointAverage avg;
      avg.SetActiveField(CellScalarsName, viskores::cont::Field::Association::Cells);
      avg.SetOutputFieldName("GeneratedPointScalars");
      auto outds = avg.Execute(GetInputDataSet());
      GetInputDataSet().AddField(
        outds.GetField("GeneratedPointScalars", viskores::cont::Field::Association::Points));
      PointScalarsName = "GeneratedPointScalars";
      std::cerr << "[CreateFields] Generated point scalars '" << PointScalarsName
                << "' from cell scalars, '" << CellScalarsName << "'.\n";
    }
    else
    {
      // Compute the magnitude of the vectors:
      VISKORES_ASSERT(!PointVectorsName.empty());
      viskores::filter::vector_analysis::VectorMagnitude mag;
      mag.SetActiveField(PointVectorsName, viskores::cont::Field::Association::Points);
      mag.SetOutputFieldName("GeneratedPointScalars");
      auto outds = mag.Execute(GetInputDataSet());
      GetInputDataSet().AddField(
        outds.GetField("GeneratedPointScalars", viskores::cont::Field::Association::Points));
      PointScalarsName = "GeneratedPointScalars";
      std::cerr << "[CreateFields] Generated point scalars '" << PointScalarsName
                << "' from point vectors, '" << PointVectorsName << "'.\n";
    }
  }

  if (CellScalarsName.empty())
  { // Attempt to construct them from a point field:
    VISKORES_ASSERT(!PointScalarsName.empty());
    viskores::filter::field_conversion::CellAverage avg;
    avg.SetActiveField(PointScalarsName, viskores::cont::Field::Association::Points);
    avg.SetOutputFieldName("GeneratedCellScalars");
    auto outds = avg.Execute(GetInputDataSet());
    GetInputDataSet().AddField(
      outds.GetField("GeneratedCellScalars", viskores::cont::Field::Association::Cells));
    CellScalarsName = "GeneratedCellScalars";
    std::cerr << "[CreateFields] Generated cell scalars '" << CellScalarsName
              << "' from point scalars, '" << PointScalarsName << "'.\n";
  }
}

struct Arg : viskores::cont::internal::option::Arg
{
  static viskores::cont::internal::option::ArgStatus Number(
    const viskores::cont::internal::option::Option& option,
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
      return viskores::cont::internal::option::ARG_OK;
    }
    else
    {
      if (msg)
      {
        std::cerr << "Option " << option.name << " requires a numeric argument." << std::endl;
      }

      return viskores::cont::internal::option::ARG_ILLEGAL;
    }
  }

  static viskores::cont::internal::option::ArgStatus Required(
    const viskores::cont::internal::option::Option& option,
    bool msg)
  {
    if ((option.arg != nullptr) && (option.arg[0] != '\0'))
    {
      return viskores::cont::internal::option::ARG_OK;
    }
    else
    {
      if (msg)
      {
        std::cerr << "Option " << option.name << " requires an argument." << std::endl;
      }
      return viskores::cont::internal::option::ARG_ILLEGAL;
    }
  }
};

enum optionIndex
{
  UNKNOWN,
  HELP,
  FILENAME,
  POINT_SCALARS,
  CELL_SCALARS,
  POINT_VECTORS,
  WAVELET_DIM,
  NUM_PARTITIONS,
  TETRA
};

void InitDataSet(int& argc, char** argv)
{
  std::string filename;
  viskores::Id waveletDim = 256;
  viskores::Id numPartitions = 1;
  bool tetra = false;

  namespace option = viskores::cont::internal::option;

  std::vector<option::Descriptor> usage;
  std::string usageHeader{ "Usage: " };
  usageHeader.append(argv[0]);
  usageHeader.append(" [input data options] [benchmark options]");
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, usageHeader.c_str() });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, "Input data options are:" });
  usage.push_back({ HELP, 0, "h", "help", Arg::None, "  -h, --help\tDisplay this help." });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, Config.Usage.c_str() });
  usage.push_back({ FILENAME,
                    0,
                    "",
                    "file",
                    Arg::Required,
                    "  --file <filename> \tFile (in legacy vtk format) to read as input. "
                    "If not specified, a wavelet source is generated." });
  usage.push_back({ POINT_SCALARS,
                    0,
                    "",
                    "point-scalars",
                    Arg::Required,
                    "  --point-scalars <name> \tName of the point scalar field to operate on." });
  usage.push_back({ CELL_SCALARS,
                    0,
                    "",
                    "cell-scalars",
                    Arg::Required,
                    "  --cell-scalars <name> \tName of the cell scalar field to operate on." });
  usage.push_back({ POINT_VECTORS,
                    0,
                    "",
                    "point-vectors",
                    Arg::Required,
                    "  --point-vectors <name> \tName of the point vector field to operate on." });
  usage.push_back({ WAVELET_DIM,
                    0,
                    "",
                    "wavelet-dim",
                    Arg::Number,
                    "  --wavelet-dim <N> \tThe size in each dimension of the wavelet grid "
                    "(if generated)." });
  usage.push_back({ NUM_PARTITIONS,
                    0,
                    "",
                    "num-partitions",
                    Arg::Number,
                    "  --num-partitions <N> \tThe number of partitions to create" });
  usage.push_back({ TETRA,
                    0,
                    "",
                    "tetra",
                    Arg::None,
                    "  --tetra \tTetrahedralize data set before running benchmark." });
  usage.push_back({ 0, 0, nullptr, nullptr, nullptr, nullptr });


  viskores::cont::internal::option::Stats stats(usage.data(), argc - 1, argv + 1);
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
    VISKORES_EXECUTE_BENCHMARKS(tmpargc, tmpargv);
    exit(0);
  }

  if (options[FILENAME])
  {
    filename = options[FILENAME].arg;
  }

  if (options[POINT_SCALARS])
  {
    PointScalarsName = options[POINT_SCALARS].arg;
  }
  if (options[CELL_SCALARS])
  {
    CellScalarsName = options[CELL_SCALARS].arg;
  }
  if (options[POINT_VECTORS])
  {
    PointVectorsName = options[POINT_VECTORS].arg;
  }

  if (options[WAVELET_DIM])
  {
    std::istringstream parse(options[WAVELET_DIM].arg);
    parse >> waveletDim;
  }

  if (options[NUM_PARTITIONS])
  {
    std::istringstream parse(options[NUM_PARTITIONS].arg);
    parse >> numPartitions;
  }

  tetra = (options[TETRA] != nullptr);

  // Now go back through the arg list and remove anything that is not in the list of
  // unknown options or non-option arguments.
  int destArg = 1;
  // This is copy/pasted from viskores::cont::Initialize(), should probably be abstracted eventually:
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

  // Load / generate the dataset
  viskores::cont::Timer inputGenTimer{ Config.Device };
  inputGenTimer.Start();

  if (!filename.empty())
  {
    std::cerr << "[InitDataSet] Loading file: " << filename << "\n";
    viskores::io::VTKDataSetReader reader(filename);
    InputDataSet = new viskores::cont::DataSet;
    *InputDataSet = reader.ReadDataSet();
    FileAsInput = true;
  }
  else
  {
    std::cerr << "[InitDataSet] Generating " << waveletDim << "x" << waveletDim << "x" << waveletDim
              << " wavelet...\n";
    viskores::source::Wavelet source;
    source.SetExtent({ 0 }, { waveletDim - 1 });

    InputDataSet = new viskores::cont::DataSet;
    *InputDataSet = source.Execute();
  }

  FindFields();
  CreateMissingFields();

  std::cerr
    << "[InitDataSet] Create UnstructuredInputDataSet from Tetrahedralized InputDataSet...\n";
  viskores::filter::geometry_refinement::Tetrahedralize tet;
  tet.SetFieldsToPass(
    viskores::filter::FieldSelection(viskores::filter::FieldSelection::Mode::All));
  UnstructuredInputDataSet = new viskores::cont::DataSet;
  *UnstructuredInputDataSet = tet.Execute(GetInputDataSet());

  if (tetra)
  {
    GetInputDataSet() = GetUnstructuredInputDataSet();
  }

  //Create partitioned data.
  if (numPartitions > 0)
  {
    std::cerr << "[InitDataSet] Creating " << numPartitions << " partitions." << std::endl;
    InputPartitionedData = new viskores::cont::PartitionedDataSet;
    UnstructuredInputPartitionedData = new viskores::cont::PartitionedDataSet;
    for (viskores::Id i = 0; i < numPartitions; i++)
    {
      GetInputPartitionedData().AppendPartition(GetInputDataSet());
      GetUnstructuredInputPartitionedData().AppendPartition(GetUnstructuredInputDataSet());
    }
  }

  inputGenTimer.Stop();

  std::cerr << "[InitDataSet] DataSet initialization took " << inputGenTimer.GetElapsedTime()
            << " seconds.\n\n-----------------";
}

} // end anon namespace

int main(int argc, char* argv[])
{
  auto opts = viskores::cont::InitializeOptions::RequireDevice;

  std::vector<char*> args(argv, argv + argc);
  viskores::bench::detail::InitializeArgs(&argc, args, opts);

  // Parse Viskores options:
  Config = viskores::cont::Initialize(argc, args.data(), opts);

  // This opts changes when it is help
  if (opts != viskores::cont::InitializeOptions::None)
  {
    viskores::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);
  }
  InitDataSet(argc, args.data());

  const std::string dataSetSummary = []() -> std::string
  {
    std::ostringstream out;
    GetInputDataSet().PrintSummary(out);
    return out.str();
  }();

  // handle benchmarking related args and run benchmarks:
  VISKORES_EXECUTE_BENCHMARKS_PREAMBLE(argc, args.data(), dataSetSummary);
  delete InputDataSet;
  delete UnstructuredInputDataSet;
  delete InputPartitionedData;
  delete UnstructuredInputPartitionedData;
}
