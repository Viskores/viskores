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

#include <vtkm/ListTag.h>
#include <vtkm/Math.h>
#include <vtkm/Range.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorInternal.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/StorageBasic.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/cont/internal/OptionParser.h>

#include <vtkm/filter/CellAverage.h>
#include <vtkm/filter/ExternalFaces.h>
#include <vtkm/filter/FieldSelection.h>
#include <vtkm/filter/Gradient.h>
#include <vtkm/filter/MarchingCubes.h>
#include <vtkm/filter/PointAverage.h>
#include <vtkm/filter/PolicyBase.h>
#include <vtkm/filter/Tetrahedralize.h>
#include <vtkm/filter/Threshold.h>
#include <vtkm/filter/ThresholdPoints.h>
#include <vtkm/filter/VectorMagnitude.h>
#include <vtkm/filter/VertexClustering.h>
#include <vtkm/filter/WarpScalar.h>
#include <vtkm/filter/WarpVector.h>

#include <vtkm/io/reader/VTKDataSetReader.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WaveletGenerator.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <cctype> // for std::tolower
#include <sstream>
#include <type_traits>

#ifdef VTKM_ENABLE_TBB
#include <tbb/task_scheduler_init.h>
#endif
#ifdef VTKM_ENABLE_OPENMP
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

// The number of benchmarks can be reduced using the ReducedOptions argument.
// All filters will be tested, but fewer variants will be used.

// For the TBB/OpenMP implementations, the number of threads can be customized
// using a "NumThreads [numThreads]" argument.

namespace
{


// unscoped enum so we can use bitwise ops without a lot of hassle:
enum BenchmarkName
{
  NONE = 0,
  GRADIENT = 1,
  THRESHOLD = 1 << 1,
  THRESHOLD_POINTS = 1 << 2,
  CELL_AVERAGE = 1 << 3,
  POINT_AVERAGE = 1 << 4,
  WARP_SCALAR = 1 << 5,
  WARP_VECTOR = 1 << 6,
  MARCHING_CUBES = 1 << 7,
  EXTERNAL_FACES = 1 << 8,
  TETRAHEDRALIZE = 1 << 9,
  VERTEX_CLUSTERING = 1 << 10,
  CELL_TO_POINT = 1 << 11,

  ALL = GRADIENT | THRESHOLD | THRESHOLD_POINTS | CELL_AVERAGE | POINT_AVERAGE | WARP_SCALAR |
    WARP_VECTOR |
    MARCHING_CUBES |
    EXTERNAL_FACES |
    TETRAHEDRALIZE |
    VERTEX_CLUSTERING |
    CELL_TO_POINT
};

static const std::string DIVIDER(40, '-');

// The input dataset we'll use on the filters (must be global, as we can't
// pass args to the benchmark functors).
static vtkm::cont::DataSet InputDataSet;
// The point scalars to use:
static std::string PointScalarsName;
// The cell scalars to use:
static std::string CellScalarsName;
// The point vectors to use:
static std::string PointVectorsName;
// Use fewer variants of each benchmark
static bool ReducedOptions;

// Limit the filter executions to only consider the following types, otherwise
// compile times and binary sizes are nuts.
using FieldTypes = vtkm::ListTagBase<vtkm::Float32,
                                     vtkm::Float64,
                                     vtkm::Vec<vtkm::Float32, 3>,
                                     vtkm::Vec<vtkm::Float64, 3>>;

using StructuredCellList = vtkm::ListTagBase<vtkm::cont::CellSetStructured<3>>;

using UnstructuredCellList =
  vtkm::ListTagBase<vtkm::cont::CellSetExplicit<>, vtkm::cont::CellSetSingleType<>>;

using AllCellList = vtkm::ListTagJoin<StructuredCellList, UnstructuredCellList>;

using CoordinateList = vtkm::ListTagBase<vtkm::Vec<vtkm::Float32, 3>, vtkm::Vec<vtkm::Float64, 3>>;

struct WaveletGeneratorDataFunctor
{
  template <typename DeviceAdapter>
  bool operator()(DeviceAdapter, vtkm::worklet::WaveletGenerator& gen)
  {
    InputDataSet = gen.GenerateDataSet<DeviceAdapter>();
    return true;
  }
};

class BenchmarkFilterPolicy : public vtkm::filter::PolicyBase<BenchmarkFilterPolicy>
{
public:
  using FieldTypeList = FieldTypes;

  using StructuredCellSetList = StructuredCellList;
  using UnstructuredCellSetList = UnstructuredCellList;
  using AllCellSetList = AllCellList;

  using CoordinateTypeList = CoordinateList;
};

// Class implementing all filter benchmarks:
class BenchmarkFilters
{
  using Timer = vtkm::cont::Timer;

  enum GradOpts
  {
    Gradient = 1,
    PointGradient = 1 << 1,
    Divergence = 1 << 2,
    Vorticity = 1 << 3,
    QCriterion = 1 << 4,
    RowOrdering = 1 << 5,
    ScalarInput = 1 << 6
  };

  template <typename, typename DeviceAdapter>
  struct BenchGradient
  {
    vtkm::filter::Gradient Filter;
    int Options;

    VTKM_CONT
    BenchGradient(int options)
      : Options(options)
    {
      if (options & ScalarInput)
      {
        // Some outputs require vectors:
        if (options & Divergence || options & Vorticity || options & QCriterion)
        {
          throw vtkm::cont::ErrorInternal("A requested gradient output is "
                                          "incompatible with scalar input.");
        }
        this->Filter.SetActiveField(PointScalarsName, vtkm::cont::Field::Association::POINTS);
      }
      else
      {
        this->Filter.SetActiveField(PointVectorsName, vtkm::cont::Field::Association::POINTS);
      }

      this->Filter.SetComputeGradient(static_cast<bool>(options & Gradient));
      this->Filter.SetComputePointGradient(static_cast<bool>(options & PointGradient));
      this->Filter.SetComputeDivergence(static_cast<bool>(options & Divergence));
      this->Filter.SetComputeVorticity(static_cast<bool>(options & Vorticity));
      this->Filter.SetComputeQCriterion(static_cast<bool>(options & QCriterion));

      if (options & RowOrdering)
      {
        this->Filter.SetRowMajorOrdering();
      }
      else
      {
        this->Filter.SetColumnMajorOrdering();
      }
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      auto result = this->Filter.Execute(InputDataSet, BenchmarkFilterPolicy());
      (void)result;
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "Gradient (";
      if (this->Options & Gradient)
      {
        desc << "Gradient,";
      }
      if (this->Options & PointGradient)
      {
        desc << "PointGradient,";
      }
      if (this->Options & Divergence)
      {
        desc << "Divergence,";
      }
      if (this->Options & Vorticity)
      {
        desc << "Vorticity,";
      }
      if (this->Options & QCriterion)
      {
        desc << "QCriterion,";
      }
      if (this->Options & RowOrdering)
      {
        desc << "RowOrdering,";
      }
      else
      {
        desc << "ColumnOrdering,";
      }
      if (this->Options & ScalarInput)
      {
        desc << "ScalarInput";
      }
      else
      {
        desc << "VectorInput";
      }

      desc << ")";

      return desc.str();
    }
  };
  VTKM_MAKE_BENCHMARK(GradientScalar, BenchGradient, Gradient | ScalarInput);
  VTKM_MAKE_BENCHMARK(GradientVector, BenchGradient, Gradient);
  VTKM_MAKE_BENCHMARK(GradientVectorRow, BenchGradient, Gradient | RowOrdering);
  VTKM_MAKE_BENCHMARK(GradientPoint, BenchGradient, PointGradient);
  VTKM_MAKE_BENCHMARK(GradientDivergence, BenchGradient, Divergence);
  VTKM_MAKE_BENCHMARK(GradientVorticity, BenchGradient, Vorticity);
  VTKM_MAKE_BENCHMARK(GradientQCriterion, BenchGradient, QCriterion);
  VTKM_MAKE_BENCHMARK(GradientKitchenSink,
                      BenchGradient,
                      Gradient | PointGradient | Divergence | Vorticity | QCriterion);

  template <typename, typename DeviceAdapter>
  struct BenchThreshold
  {
    vtkm::filter::Threshold Filter;

    VTKM_CONT
    BenchThreshold()
    {
      auto field = InputDataSet.GetField(PointScalarsName, vtkm::cont::Field::Association::POINTS);
      auto rangeHandle = field.GetRange();
      auto range = rangeHandle.GetPortalConstControl().Get(0);

      // Extract points with values between 25-75% of the range
      vtkm::Float64 quarter = range.Length() / 4.;
      vtkm::Float64 mid = range.Center();

      this->Filter.SetActiveField(PointScalarsName, vtkm::cont::Field::Association::POINTS);
      this->Filter.SetLowerThreshold(mid - quarter);
      this->Filter.SetUpperThreshold(mid + quarter);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      auto result = this->Filter.Execute(InputDataSet, BenchmarkFilterPolicy());
      (void)result;
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const { return "Threshold"; }
  };
  VTKM_MAKE_BENCHMARK(Threshold, BenchThreshold);

  template <typename, typename DeviceAdapter>
  struct BenchThresholdPoints
  {
    bool CompactPoints;
    vtkm::filter::ThresholdPoints Filter;

    VTKM_CONT
    BenchThresholdPoints(bool compactPoints)
      : CompactPoints(compactPoints)
    {
      auto field = InputDataSet.GetField(PointScalarsName, vtkm::cont::Field::Association::POINTS);
      auto rangeHandle = field.GetRange();
      auto range = rangeHandle.GetPortalConstControl().Get(0);

      // Extract points with values between 25-75% of the range
      vtkm::Float64 quarter = range.Length() / 4.;
      vtkm::Float64 mid = range.Center();

      this->Filter.SetActiveField(PointScalarsName, vtkm::cont::Field::Association::POINTS);
      this->Filter.SetLowerThreshold(mid - quarter);
      this->Filter.SetUpperThreshold(mid + quarter);

      this->Filter.SetCompactPoints(this->CompactPoints);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      auto result = this->Filter.Execute(InputDataSet, BenchmarkFilterPolicy());
      (void)result;
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const { return "ThresholdPoints"; }
  };
  VTKM_MAKE_BENCHMARK(ThresholdPoints, BenchThresholdPoints, false);
  VTKM_MAKE_BENCHMARK(ThresholdPointsCompact, BenchThresholdPoints, true);

  template <typename, typename DeviceAdapter>
  struct BenchCellAverage
  {
    vtkm::filter::CellAverage Filter;

    VTKM_CONT
    BenchCellAverage()
    {
      this->Filter.SetActiveField(PointScalarsName, vtkm::cont::Field::Association::POINTS);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      auto result = this->Filter.Execute(InputDataSet, BenchmarkFilterPolicy());
      (void)result;
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const { return "CellAverage"; }
  };
  VTKM_MAKE_BENCHMARK(CellAverage, BenchCellAverage);

  template <typename, typename DeviceAdapter>
  struct BenchPointAverage
  {
    vtkm::filter::PointAverage Filter;

    VTKM_CONT
    BenchPointAverage()
    {
      this->Filter.SetActiveField(CellScalarsName, vtkm::cont::Field::Association::CELL_SET);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      auto result = this->Filter.Execute(InputDataSet, BenchmarkFilterPolicy());
      (void)result;
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const { return "PointAverage"; }
  };
  VTKM_MAKE_BENCHMARK(PointAverage, BenchPointAverage);

  template <typename, typename DeviceAdapter>
  struct BenchWarpScalar
  {
    vtkm::filter::WarpScalar Filter;

    VTKM_CONT
    BenchWarpScalar()
      : Filter(2.)
    {
      this->Filter.SetUseCoordinateSystemAsPrimaryField(true);
      this->Filter.SetNormalField(PointVectorsName, vtkm::cont::Field::Association::POINTS);
      this->Filter.SetScalarFactorField(PointScalarsName, vtkm::cont::Field::Association::POINTS);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      auto result = this->Filter.Execute(InputDataSet, BenchmarkFilterPolicy());
      (void)result;
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const { return "WarpScalar"; }
  };
  VTKM_MAKE_BENCHMARK(WarpScalar, BenchWarpScalar);

  template <typename, typename DeviceAdapter>
  struct BenchWarpVector
  {
    vtkm::filter::WarpVector Filter;

    VTKM_CONT
    BenchWarpVector()
      : Filter(2.)
    {
      this->Filter.SetUseCoordinateSystemAsField(true);
      this->Filter.SetVectorField(PointVectorsName, vtkm::cont::Field::Association::POINTS);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      auto result = this->Filter.Execute(InputDataSet, BenchmarkFilterPolicy());
      (void)result;
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const { return "WarpVector"; }
  };
  VTKM_MAKE_BENCHMARK(WarpVector, BenchWarpVector);

  template <typename, typename DeviceAdapter>
  struct BenchMarchingCubes
  {
    vtkm::filter::MarchingCubes Filter;

    VTKM_CONT
    BenchMarchingCubes(vtkm::Id numIsoVals, bool mergePoints, bool normals, bool fastNormals)
      : Filter()
    {
      this->Filter.SetActiveField(PointScalarsName, vtkm::cont::Field::Association::POINTS);

      // Set up some equally spaced contours:
      auto field = InputDataSet.GetField(PointScalarsName, vtkm::cont::Field::Association::POINTS);
      auto range = field.GetRange().GetPortalConstControl().Get(0);
      auto step = range.Length() / static_cast<vtkm::Float64>(numIsoVals + 1);
      this->Filter.SetNumberOfIsoValues(numIsoVals);
      auto val = range.Min + step;
      for (vtkm::Id i = 0; i < numIsoVals; ++i)
      {
        this->Filter.SetIsoValue(i, val);
        val += step;
      }

      this->Filter.SetMergeDuplicatePoints(mergePoints);
      this->Filter.SetGenerateNormals(normals);
      this->Filter.SetComputeFastNormalsForStructured(fastNormals);
      this->Filter.SetComputeFastNormalsForUnstructured(fastNormals);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      auto result = this->Filter.Execute(InputDataSet, BenchmarkFilterPolicy());
      (void)result;
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "MarchingCubes numIsoVal=" << this->Filter.GetNumberOfIsoValues() << " "
           << "mergePoints=" << this->Filter.GetMergeDuplicatePoints() << " "
           << "normals=" << this->Filter.GetGenerateNormals() << " "
           << "fastNormals=" << this->Filter.GetComputeFastNormalsForStructured();
      return desc.str();
    }
  };
  VTKM_MAKE_BENCHMARK(MarchingCubes1FFF, BenchMarchingCubes, 1, false, false, false);
  VTKM_MAKE_BENCHMARK(MarchingCubes3FFF, BenchMarchingCubes, 3, false, false, false);
  VTKM_MAKE_BENCHMARK(MarchingCubes12FFF, BenchMarchingCubes, 12, false, false, false);
  VTKM_MAKE_BENCHMARK(MarchingCubes1TFF, BenchMarchingCubes, 1, true, false, false);
  VTKM_MAKE_BENCHMARK(MarchingCubes3TFF, BenchMarchingCubes, 3, true, false, false);
  VTKM_MAKE_BENCHMARK(MarchingCubes12TFF, BenchMarchingCubes, 12, true, false, false);
  VTKM_MAKE_BENCHMARK(MarchingCubes1FTF, BenchMarchingCubes, 1, false, true, false);
  VTKM_MAKE_BENCHMARK(MarchingCubes3FTF, BenchMarchingCubes, 3, false, true, false);
  VTKM_MAKE_BENCHMARK(MarchingCubes12FTF, BenchMarchingCubes, 12, false, true, false);
  VTKM_MAKE_BENCHMARK(MarchingCubes1FTT, BenchMarchingCubes, 1, false, true, true);
  VTKM_MAKE_BENCHMARK(MarchingCubes3FTT, BenchMarchingCubes, 3, false, true, true);
  VTKM_MAKE_BENCHMARK(MarchingCubes12FTT, BenchMarchingCubes, 12, false, true, true);

  template <typename, typename DeviceAdapter>
  struct BenchExternalFaces
  {
    vtkm::filter::ExternalFaces Filter;

    VTKM_CONT
    BenchExternalFaces(bool compactPoints)
      : Filter()
    {
      this->Filter.SetCompactPoints(compactPoints);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      auto result = this->Filter.Execute(InputDataSet, BenchmarkFilterPolicy());
      (void)result;
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "ExternalFaces";
      if (this->Filter.GetCompactPoints())
      {
        desc << " (compact points)";
      }
      return desc.str();
    }
  };
  VTKM_MAKE_BENCHMARK(ExternalFaces, BenchExternalFaces, false);
  VTKM_MAKE_BENCHMARK(ExternalFacesCompact, BenchExternalFaces, true);

  template <typename, typename DeviceAdapter>
  struct BenchTetrahedralize
  {
    vtkm::filter::Tetrahedralize Filter;

    VTKM_CONT
    BenchTetrahedralize()
      : Filter()
    {
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      auto result = this->Filter.Execute(InputDataSet, BenchmarkFilterPolicy());
      (void)result;
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const { return "Tetrahedralize"; }
  };
  VTKM_MAKE_BENCHMARK(Tetrahedralize, BenchTetrahedralize);

  template <typename, typename DeviceAdapter>
  struct BenchVertexClustering
  {
    vtkm::filter::VertexClustering Filter;

    VTKM_CONT
    BenchVertexClustering(vtkm::Id ndims)
      : Filter()
    {
      this->Filter.SetNumberOfDivisions({ ndims });
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      Timer timer{ DeviceAdapter() };
      timer.Start();
      auto result = this->Filter.Execute(InputDataSet, BenchmarkFilterPolicy());
      (void)result;
      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      vtkm::Id dims = this->Filter.GetNumberOfDivisions()[0];
      std::ostringstream desc;
      desc << "VertexClustering filter (" << dims << "x" << dims << "x" << dims << ")";
      return desc.str();
    }
  };
  VTKM_MAKE_BENCHMARK(VertexClustering32, BenchVertexClustering, 32);
  VTKM_MAKE_BENCHMARK(VertexClustering64, BenchVertexClustering, 64);
  VTKM_MAKE_BENCHMARK(VertexClustering128, BenchVertexClustering, 128);
  VTKM_MAKE_BENCHMARK(VertexClustering256, BenchVertexClustering, 256);
  VTKM_MAKE_BENCHMARK(VertexClustering512, BenchVertexClustering, 512);
  VTKM_MAKE_BENCHMARK(VertexClustering1024, BenchVertexClustering, 1024);

  template <typename, typename DeviceAdapter>
  struct BenchCellToPoint
  {
    struct PrepareForInput
    {
      mutable double Time{ 0 };

      void operator()(const vtkm::cont::CellSet& cellSet) const
      {
        static bool warned{ false };
        if (!warned)
        {
          std::cerr << "Invalid cellset type for benchmark.\n";
          cellSet.PrintSummary(std::cerr);
          warned = true;
        }
      }

      template <typename T1, typename T2, typename T3, typename T4>
      VTKM_CONT void operator()(const vtkm::cont::CellSetExplicit<T1, T2, T3, T4>& cellSet) const
      {
        { // Why does CastAndCall insist on making the cellset const?
          using CellSetT = vtkm::cont::CellSetExplicit<T1, T2, T3, T4>;
          CellSetT& mcellSet = const_cast<CellSetT&>(cellSet);
          mcellSet.ResetConnectivity(vtkm::TopologyElementTagCell{},
                                     vtkm::TopologyElementTagPoint{});
        }

        Timer timer{ DeviceAdapter() };
        timer.Start();
        cellSet.PrepareForInput(
          DeviceAdapter(), vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
        this->Time = timer.GetElapsedTime();
      }
    };

    VTKM_CONT
    BenchCellToPoint() {}

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      auto cellset = InputDataSet.GetCellSet();
      PrepareForInput functor;
      cellset.CastAndCall(functor);
      return functor.Time;
    }

    VTKM_CONT
    std::string Description() const
    {
      std::ostringstream desc;
      desc << "CellToPoint table construction";
      return desc.str();
    }
  };
  VTKM_MAKE_BENCHMARK(CellToPoint, BenchCellToPoint);

public:
  static VTKM_CONT int Run(int benches, vtkm::cont::DeviceAdapterId id)
  {
    // This has no influence on the benchmarks. See issue #286.
    auto dummyTypes = vtkm::ListTagBase<vtkm::Int32>{};

    std::cout << DIVIDER << "\nRunning Filter benchmarks\n";

    if (benches & BenchmarkName::GRADIENT)
    {
      if (ReducedOptions)
      {
        VTKM_RUN_BENCHMARK(GradientScalar, dummyTypes, id);
        VTKM_RUN_BENCHMARK(GradientVector, dummyTypes, id);
        VTKM_RUN_BENCHMARK(GradientVectorRow, dummyTypes, id);
        VTKM_RUN_BENCHMARK(GradientKitchenSink, dummyTypes, id);
      }
      else
      {
        VTKM_RUN_BENCHMARK(GradientScalar, dummyTypes, id);
        VTKM_RUN_BENCHMARK(GradientVector, dummyTypes, id);
        VTKM_RUN_BENCHMARK(GradientVectorRow, dummyTypes, id);
        VTKM_RUN_BENCHMARK(GradientPoint, dummyTypes, id);
        VTKM_RUN_BENCHMARK(GradientDivergence, dummyTypes, id);
        VTKM_RUN_BENCHMARK(GradientVorticity, dummyTypes, id);
        VTKM_RUN_BENCHMARK(GradientQCriterion, dummyTypes, id);
        VTKM_RUN_BENCHMARK(GradientKitchenSink, dummyTypes, id);
      }
    }
    if (benches & BenchmarkName::THRESHOLD)
    {
      VTKM_RUN_BENCHMARK(Threshold, dummyTypes, id);
    }
    if (benches & BenchmarkName::THRESHOLD_POINTS)
    {
      VTKM_RUN_BENCHMARK(ThresholdPoints, dummyTypes, id);
      VTKM_RUN_BENCHMARK(ThresholdPointsCompact, dummyTypes, id);
    }
    if (benches & BenchmarkName::CELL_AVERAGE)
    {
      VTKM_RUN_BENCHMARK(CellAverage, dummyTypes, id);
    }
    if (benches & BenchmarkName::POINT_AVERAGE)
    {
      VTKM_RUN_BENCHMARK(PointAverage, dummyTypes, id);
    }
    if (benches & BenchmarkName::WARP_SCALAR)
    {
      VTKM_RUN_BENCHMARK(WarpScalar, dummyTypes, id);
    }
    if (benches & BenchmarkName::WARP_VECTOR)
    {
      VTKM_RUN_BENCHMARK(WarpVector, dummyTypes, id);
    }
    if (benches & BenchmarkName::MARCHING_CUBES)
    {
      if (ReducedOptions)
      {
        VTKM_RUN_BENCHMARK(MarchingCubes1FFF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes12FFF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes12TFF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes12FTF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes12FTT, dummyTypes, id);
      }
      else
      {
        VTKM_RUN_BENCHMARK(MarchingCubes1FFF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes3FFF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes12FFF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes1TFF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes3TFF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes12TFF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes1FTF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes3FTF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes12FTF, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes1FTT, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes3FTT, dummyTypes, id);
        VTKM_RUN_BENCHMARK(MarchingCubes12FTT, dummyTypes, id);
      }
    }
    if (benches & BenchmarkName::EXTERNAL_FACES)
    {
      VTKM_RUN_BENCHMARK(ExternalFaces, dummyTypes, id);
      VTKM_RUN_BENCHMARK(ExternalFacesCompact, dummyTypes, id);
    }
    if (benches & BenchmarkName::TETRAHEDRALIZE)
    {
      VTKM_RUN_BENCHMARK(Tetrahedralize, dummyTypes, id);
    }
    if (benches & BenchmarkName::VERTEX_CLUSTERING)
    {
      if (ReducedOptions)
      {
        VTKM_RUN_BENCHMARK(VertexClustering32, dummyTypes, id);
        VTKM_RUN_BENCHMARK(VertexClustering256, dummyTypes, id);
        VTKM_RUN_BENCHMARK(VertexClustering1024, dummyTypes, id);
      }
      else
      {
        VTKM_RUN_BENCHMARK(VertexClustering32, dummyTypes, id);
        VTKM_RUN_BENCHMARK(VertexClustering64, dummyTypes, id);
        VTKM_RUN_BENCHMARK(VertexClustering128, dummyTypes, id);
        VTKM_RUN_BENCHMARK(VertexClustering256, dummyTypes, id);
        VTKM_RUN_BENCHMARK(VertexClustering512, dummyTypes, id);
        VTKM_RUN_BENCHMARK(VertexClustering1024, dummyTypes, id);
      }
    }
    if (benches & BenchmarkName::CELL_TO_POINT)
    {
      VTKM_RUN_BENCHMARK(CellToPoint, dummyTypes, id);
    }

    return 0;
  }
};

// Generates a Vec3 field from point coordinates.
struct PointVectorGenerator : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  vtkm::Bounds Bounds;
  vtkm::Vec<vtkm::Float64, 3> Center;
  vtkm::Vec<vtkm::Float64, 3> Scale;

  VTKM_CONT
  PointVectorGenerator(const vtkm::Bounds& bounds)
    : Bounds(bounds)
    , Center(bounds.Center())
    , Scale((6. * vtkm::Pi()) / bounds.X.Length(),
            (2. * vtkm::Pi()) / bounds.Y.Length(),
            (7. * vtkm::Pi()) / bounds.Z.Length())
  {
  }

  template <typename T>
  VTKM_EXEC vtkm::Vec<T, 3> operator()(vtkm::Vec<T, 3> val) const
  {
    using Vec3T = vtkm::Vec<T, 3>;
    using Vec3F64 = vtkm::Vec<vtkm::Float64, 3>;

    Vec3F64 valF64{ val };
    Vec3F64 periodic = (valF64 - this->Center) * this->Scale;
    periodic[0] = vtkm::Sin(periodic[0]);
    periodic[1] = vtkm::Sin(periodic[1]);
    periodic[2] = vtkm::Cos(periodic[2]);

    if (vtkm::MagnitudeSquared(periodic) > 0.)
    {
      vtkm::Normalize(periodic);
    }
    if (vtkm::MagnitudeSquared(valF64) > 0.)
    {
      vtkm::Normalize(valF64);
    }
    return Vec3T{ vtkm::Normal(periodic + valF64) };
  }
};

// Get the number of components in a VariantArrayHandle, ArrayHandle, or Field's
// ValueType.
struct NumberOfComponents
{
  vtkm::IdComponent NumComponents;

  template <typename ArrayHandleT>
  VTKM_CONT void operator()(const ArrayHandleT&)
  {
    using ValueType = typename ArrayHandleT::ValueType;
    using Traits = vtkm::VecTraits<ValueType>;
    this->NumComponents = Traits::NUM_COMPONENTS;
  }

  template <typename DynamicType>
  VTKM_CONT static vtkm::IdComponent Check(const DynamicType& obj)
  {
    NumberOfComponents functor;
    vtkm::cont::CastAndCall(obj, functor);
    return functor.NumComponents;
  }
};

void FindFields(bool needPointScalars, bool needCellScalars, bool needPointVectors)
{
  if (needPointScalars && PointScalarsName.empty())
  {
    for (vtkm::Id i = 0; i < InputDataSet.GetNumberOfFields(); ++i)
    {
      auto field = InputDataSet.GetField(i);
      if (field.GetAssociation() == vtkm::cont::Field::Association::POINTS &&
          NumberOfComponents::Check(field) == 1)
      {
        PointScalarsName = field.GetName();
        std::cout << "Found PointScalars: " << PointScalarsName << "\n";
        break;
      }
    }
  }

  if (needCellScalars && CellScalarsName.empty())
  {
    for (vtkm::Id i = 0; i < InputDataSet.GetNumberOfFields(); ++i)
    {
      auto field = InputDataSet.GetField(i);
      if (field.GetAssociation() == vtkm::cont::Field::Association::CELL_SET &&
          NumberOfComponents::Check(field) == 1)
      {
        CellScalarsName = field.GetName();
        std::cout << "Found CellScalars: " << CellScalarsName << "\n";
        break;
      }
    }
  }

  if (needPointVectors && PointVectorsName.empty())
  {
    for (vtkm::Id i = 0; i < InputDataSet.GetNumberOfFields(); ++i)
    {
      auto field = InputDataSet.GetField(i);
      if (field.GetAssociation() == vtkm::cont::Field::Association::POINTS &&
          NumberOfComponents::Check(field) == 3)
      {
        PointVectorsName = field.GetName();
        std::cout << "Found CellVectors: " << PointVectorsName << "\n";
        break;
      }
    }
  }
}

void CreateFields(bool needPointScalars, bool needCellScalars, bool needPointVectors)
{
  // Do point vectors first, so we can generate the scalars from them if needed
  if (needPointVectors && PointVectorsName.empty())
  {
    // Construct them from the coordinates:
    auto coords = InputDataSet.GetCoordinateSystem();
    auto bounds = coords.GetBounds();
    auto points = coords.GetData();
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> pvecs;

    PointVectorGenerator worklet(bounds);
    vtkm::worklet::DispatcherMapField<PointVectorGenerator> dispatch(worklet);
    dispatch.Invoke(points, pvecs);
    InputDataSet.AddField(
      vtkm::cont::Field("GeneratedPointVectors", vtkm::cont::Field::Association::POINTS, pvecs));
    PointVectorsName = "GeneratedPointVectors";
    std::cout << "Generated point vectors '" << PointVectorsName << "' from "
                                                                    "coordinate data.\n";
  }

  if (needPointScalars && PointScalarsName.empty())
  {
    // Attempt to construct them from a cell field:
    if (CellScalarsName.empty())
    { // attempt to find a set of cell scalars in the input:
      FindFields(false, true, false);
    }
    if (!CellScalarsName.empty())
    { // Generate from found cell field:
      vtkm::filter::PointAverage avg;
      avg.SetActiveField(CellScalarsName, vtkm::cont::Field::Association::CELL_SET);
      avg.SetOutputFieldName("GeneratedPointScalars");
      auto outds = avg.Execute(InputDataSet, BenchmarkFilterPolicy());
      InputDataSet.AddField(
        outds.GetField("GeneratedPointScalars", vtkm::cont::Field::Association::POINTS));
      PointScalarsName = "GeneratedPointScalars";
      std::cout << "Generated point scalars '" << PointScalarsName << "' from "
                                                                      "cell scalars, '"
                << CellScalarsName << "'.\n";
    }
    else
    { // Attempt to construct them from point vectors:
      if (PointVectorsName.empty())
      {
        FindFields(false, false, true);
      }
      if (PointVectorsName.empty())
      {
        CreateFields(false, false, true); // cannot fail
      }

      // Compute the magnitude of the vectors:
      vtkm::filter::VectorMagnitude mag;
      mag.SetActiveField(PointVectorsName, vtkm::cont::Field::Association::POINTS);
      mag.SetOutputFieldName("GeneratedPointScalars");
      auto outds = mag.Execute(InputDataSet, BenchmarkFilterPolicy());
      InputDataSet.AddField(
        outds.GetField("GeneratedPointScalars", vtkm::cont::Field::Association::POINTS));
      PointScalarsName = "GeneratedPointScalars";
      std::cout << "Generated point scalars '" << PointScalarsName << "' from "
                                                                      "point vectors, '"
                << PointVectorsName << "'.\n";
    }
  }

  if (needCellScalars && CellScalarsName.empty())
  {
    // Attempt to construct them from a point field:
    if (PointScalarsName.empty())
    { // attempt to find a set of point scalars in the input:
      FindFields(true, false, false);
    }
    if (!PointScalarsName.empty())
    { // Generate from found point field:
      vtkm::filter::CellAverage avg;
      avg.SetActiveField(PointScalarsName, vtkm::cont::Field::Association::POINTS);
      avg.SetOutputFieldName("GeneratedCellScalars");
      auto outds = avg.Execute(InputDataSet, BenchmarkFilterPolicy());
      InputDataSet.AddField(
        outds.GetField("GeneratedCellScalars", vtkm::cont::Field::Association::CELL_SET));
      CellScalarsName = "GeneratedCellScalars";
      std::cout << "Generated cell scalars '" << CellScalarsName << "' from "
                                                                    "point scalars, '"
                << PointScalarsName << "'.\n";
    }
  }
}

void AssertFields(bool needPointScalars, bool needCellScalars, bool needPointVectors)
{
  if (needPointScalars)
  {
    if (PointScalarsName.empty())
    {
      throw vtkm::cont::ErrorInternal("PointScalarsName not set!");
    }
    if (!InputDataSet.HasField(PointScalarsName, vtkm::cont::Field::Association::POINTS))
    {
      throw vtkm::cont::ErrorInternal("PointScalars field not in dataset!");
    }
  }
  if (needCellScalars)
  {
    if (CellScalarsName.empty())
    {
      throw vtkm::cont::ErrorInternal("CellScalarsName not set!");
    }
    if (!InputDataSet.HasField(CellScalarsName, vtkm::cont::Field::Association::CELL_SET))
    {
      throw vtkm::cont::ErrorInternal("CellScalars field not in dataset!");
    }
  }
  if (needPointVectors)
  {
    if (PointVectorsName.empty())
    {
      throw vtkm::cont::ErrorInternal("PointVectorsName not set!");
    }
    if (!InputDataSet.HasField(PointVectorsName, vtkm::cont::Field::Association::POINTS))
    {
      throw vtkm::cont::ErrorInternal("PointVectors field not in dataset!");
    }
  }
}

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
      if (msg)
      {
        std::cerr << "Option " << option.name << " requires an argument." << std::endl;
      }
      return vtkm::cont::internal::option::ARG_ILLEGAL;
    }
    else
    {
      return vtkm::cont::internal::option::ARG_OK;
    }
  }
};

enum optionIndex
{
  UNKNOWN,
  HELP,
  NUM_THREADS,
  FILENAME,
  POINT_SCALARS,
  CELL_SCALARS,
  POINT_VECTORS,
  WAVELET_DIM,
  TETRA,
  REDUCED_OPTIONS
};

int BenchmarkBody(int argc, char** argv, const vtkm::cont::InitializeResult& config)
{
  int numThreads = 0;
  int benches = BenchmarkName::NONE;
  std::string filename;
  vtkm::Id waveletDim = 256;
  bool tetra = false;
  bool needPointScalars = false;
  bool needCellScalars = false;
  bool needPointVectors = false;

  ReducedOptions = false;

  namespace option = vtkm::cont::internal::option;

  std::vector<option::Descriptor> usage;
  std::string usageHeader{ "Usage: " };
  usageHeader.append(argv[0]);
  usageHeader.append(" [options] [benchmarks]");
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, usageHeader.c_str() });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, "Options are:" });
  usage.push_back({ HELP, 0, "h", "help", Arg::None, "  -h, --help\tDisplay this help." });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, config.Usage.c_str() });
  usage.push_back({ NUM_THREADS,
                    0,
                    "",
                    "num-threads",
                    Arg::Number,
                    "  --num-threads <N> \tSpecify the number of threads to use." });
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
  usage.push_back({ TETRA,
                    0,
                    "",
                    "tetra",
                    Arg::None,
                    "  --tetra \tTetrahedralize data set before running benchmark." });
  usage.push_back({ REDUCED_OPTIONS,
                    0,
                    "",
                    "reduced-options",
                    Arg::None,
                    "  --reduced-options \tRun fewer variants of each filter. " });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, "Benchmarks are one or more of:" });
  usage.push_back({ UNKNOWN,
                    0,
                    "",
                    "",
                    Arg::None,
                    "\tgradient, threshold, threshold_points, cell_average, point_average, "
                    "warp_scalar, warp_vector, marching_cubes, external_faces, "
                    "tetrahedralize, cell_to_point" });
  usage.push_back(
    { UNKNOWN, 0, "", "", Arg::None, "If no benchmarks are listed, all will be run." });
  usage.push_back({ 0, 0, nullptr, nullptr, nullptr, nullptr });


  vtkm::cont::internal::option::Stats stats(usage.data(), argc - 1, argv + 1);
  std::unique_ptr<option::Option[]> options{ new option::Option[stats.options_max] };
  std::unique_ptr<option::Option[]> buffer{ new option::Option[stats.buffer_max] };
  option::Parser commandLineParse(usage.data(), argc - 1, argv + 1, options.get(), buffer.get());

  if (options[UNKNOWN])
  {
    std::cerr << "Unknown option: " << options[UNKNOWN].name << std::endl;
    option::printUsage(std::cerr, usage.data());
    exit(1);
  }

  if (options[HELP])
  {
    option::printUsage(std::cerr, usage.data());
    exit(0);
  }

  if (options[NUM_THREADS])
  {
    std::istringstream parse(options[NUM_THREADS].arg);
    parse >> numThreads;
    if (config.Device == vtkm::cont::DeviceAdapterTagTBB() ||
        config.Device == vtkm::cont::DeviceAdapterTagOpenMP())
    {
      std::cout << "Selected " << numThreads << " " << config.Device.GetName() << " threads."
                << std::endl;
    }
    else
    {
      std::cerr << options[NUM_THREADS].name << " not valid on this device. Ignoring." << std::endl;
    }
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

  tetra = (options[TETRA] != nullptr);
  ReducedOptions = (options[REDUCED_OPTIONS] != nullptr);

  for (int i = 0; i < commandLineParse.nonOptionsCount(); ++i)
  {
    std::string arg = commandLineParse.nonOption(i);
    std::transform(arg.begin(), arg.end(), arg.begin(), [](char c) {
      return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
    if (arg == "gradient")
    {
      benches |= BenchmarkName::GRADIENT;
      needPointScalars = true;
      needPointVectors = true;
    }
    else if (arg == "threshold")
    {
      benches |= BenchmarkName::THRESHOLD;
      needPointScalars = true;
    }
    else if (arg == "threshold_points")
    {
      benches |= BenchmarkName::THRESHOLD_POINTS;
      needPointScalars = true;
    }
    else if (arg == "cell_average")
    {
      benches |= BenchmarkName::CELL_AVERAGE;
      needPointScalars = true;
    }
    else if (arg == "point_average")
    {
      benches |= BenchmarkName::POINT_AVERAGE;
      needCellScalars = true;
    }
    else if (arg == "warp_scalar")
    {
      benches |= BenchmarkName::WARP_SCALAR;
      needPointScalars = true;
      needPointVectors = true;
    }
    else if (arg == "warp_vector")
    {
      benches |= BenchmarkName::WARP_VECTOR;
      needPointVectors = true;
    }
    else if (arg == "marching_cubes")
    {
      benches |= BenchmarkName::MARCHING_CUBES;
      needPointScalars = true;
    }
    else if (arg == "external_faces")
    {
      benches |= BenchmarkName::EXTERNAL_FACES;
    }
    else if (arg == "tetrahedralize")
    {
      benches |= BenchmarkName::TETRAHEDRALIZE;
    }
    else if (arg == "vertex_clustering")
    {
      benches |= BenchmarkName::VERTEX_CLUSTERING;
    }
    else if (arg == "cell_to_point")
    {
      benches |= BenchmarkName::CELL_TO_POINT;
    }
    else
    {
      std::cerr << "Unrecognized benchmark: " << arg << std::endl;
      option::printUsage(std::cerr, usage.data());
      return 1;
    }
  }

#ifdef VTKM_ENABLE_TBB
  // Must not be destroyed as long as benchmarks are running:
  tbb::task_scheduler_init init((numThreads > 0) ? numThreads
                                                 : tbb::task_scheduler_init::automatic);
#endif
#ifdef VTKM_ENABLE_OPENMP
  omp_set_num_threads((numThreads > 0) ? numThreads : omp_get_max_threads());
#endif

  if (benches == BenchmarkName::NONE)
  {
    benches = BenchmarkName::ALL;
    needPointScalars = true;
    needCellScalars = true;
    needPointVectors = true;
  }

  // Load / generate the dataset
  if (!filename.empty())
  {
    std::cout << "Loading file: " << filename << "\n";
    vtkm::io::reader::VTKDataSetReader reader(filename);
    InputDataSet = reader.ReadDataSet();
  }
  else
  {
    std::cout << "Generating " << waveletDim << "x" << waveletDim << "x" << waveletDim
              << " wavelet...\n";
    vtkm::worklet::WaveletGenerator gen;
    gen.SetExtent({ 0 }, { waveletDim });

    // WaveletGenerator needs a template device argument not a id to deduce the portal type.
    WaveletGeneratorDataFunctor genFunctor;
    vtkm::cont::TryExecuteOnDevice(config.Device, genFunctor, gen);
  }

  if (tetra)
  {
    std::cout << "Tetrahedralizing dataset...\n";
    vtkm::filter::Tetrahedralize tet;
    tet.SetFieldsToPass(vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::MODE_ALL));
    InputDataSet = tet.Execute(InputDataSet);
  }

  bool isStructured = InputDataSet.GetCellSet().IsType<vtkm::cont::CellSetStructured<3>>() ||
    InputDataSet.GetCellSet().IsType<vtkm::cont::CellSetStructured<2>>() ||
    InputDataSet.GetCellSet().IsType<vtkm::cont::CellSetStructured<1>>();

  // Check for incompatible options
  if (benches & BenchmarkName::TETRAHEDRALIZE && !isStructured)
  {
    std::cout << "Warning: Cannot benchmark vtkm::filter::Tetrahedralize on "
                 "unstructured datasets. Removing from options.\n";
    benches = benches ^ BenchmarkName::TETRAHEDRALIZE;
  }
  if (benches & BenchmarkName::VERTEX_CLUSTERING && isStructured)
  {
    std::cout << "Warning: Cannot benchmark vtkm::filter::VertexClustering on "
                 "structured dataset. Removing from options.\n";
    benches = benches ^ BenchmarkName::VERTEX_CLUSTERING;
  }
  if (benches & BenchmarkName::CELL_TO_POINT && isStructured)
  {
    std::cout << "Info: CellToPoint benchmark is trivial on structured datasets. "
                 "Removing from options.\n";
    benches = benches ^ BenchmarkName::CELL_TO_POINT;
  }

  // Check to see what fields already exist in the input:
  FindFields(needPointScalars, needCellScalars, needPointVectors);

  // Create any missing fields:
  CreateFields(needPointScalars, needCellScalars, needPointVectors);

  // Assert that required fields exist:
  AssertFields(needPointScalars, needCellScalars, needPointVectors);

  std::cout << "\nDataSet Summary:\n";
  InputDataSet.PrintSummary(std::cout);
  std::cout << "\n";

  //now actually execute the benchmarks
  int result = BenchmarkFilters::Run(benches, config.Device);

  // Explicitly free resources before exit.
  InputDataSet.Clear();

  return result;
}

} // end anon namespace

int main(int argc, char* argv[])
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  int retval = 1;
  try
  {
    retval = BenchmarkBody(argc, argv, config);
  }
  catch (std::exception& e)
  {
    std::cerr << "Benchmark encountered an exception: " << e.what() << "\n";
    return 1;
  }

  return retval;
}
