//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/Math.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include "Benchmarker.h"
#include <vtkm/cont/testing/Testing.h>

#include <cctype>
#include <random>
#include <string>

namespace vtkm
{
namespace benchmarking
{

#define CUBE_SIZE 256
static const std::string DIVIDER(40, '-');

enum BenchmarkName
{
  CELL_TO_POINT = 1 << 1,
  POINT_TO_CELL = 1 << 2,
  MC_CLASSIFY = 1 << 3,
  ALL = CELL_TO_POINT | POINT_TO_CELL | MC_CLASSIFY
};

class AveragePointToCell : public vtkm::worklet::WorkletMapPointToCell
{
public:
  using ControlSignature = void(FieldInPoint<> inPoints,
                                CellSetIn cellset,
                                FieldOutCell<> outCells);
  using ExecutionSignature = void(_1, PointCount, _3);
  using InputDomain = _2;

  template <typename PointValueVecType, typename OutType>
  VTKM_EXEC void operator()(const PointValueVecType& pointValues,
                            const vtkm::IdComponent& numPoints,
                            OutType& average) const
  {
    OutType sum = static_cast<OutType>(pointValues[0]);
    for (vtkm::IdComponent pointIndex = 1; pointIndex < numPoints; ++pointIndex)
    {
      sum = sum + static_cast<OutType>(pointValues[pointIndex]);
    }

    average = sum / static_cast<OutType>(numPoints);
  }
};

class AverageCellToPoint : public vtkm::worklet::WorkletMapCellToPoint
{
public:
  using ControlSignature = void(FieldInCell<> inCells, CellSetIn topology, FieldOut<> outPoints);
  using ExecutionSignature = void(_1, _3, CellCount);
  using InputDomain = _2;

  template <typename CellVecType, typename OutType>
  VTKM_EXEC void operator()(const CellVecType& cellValues,
                            OutType& avgVal,
                            const vtkm::IdComponent& numCellIDs) const
  {
    //simple functor that returns the average cell Value.
    avgVal = vtkm::TypeTraits<OutType>::ZeroInitialization();
    if (numCellIDs != 0)
    {
      for (vtkm::IdComponent cellIndex = 0; cellIndex < numCellIDs; ++cellIndex)
      {
        avgVal += static_cast<OutType>(cellValues[cellIndex]);
      }
      avgVal = avgVal / static_cast<OutType>(numCellIDs);
    }
  }
};

// -----------------------------------------------------------------------------
template <typename T>
class Classification : public vtkm::worklet::WorkletMapPointToCell
{
public:
  using ControlSignature = void(FieldInPoint<> inNodes,
                                CellSetIn cellset,
                                FieldOutCell<IdComponentType> outCaseId);
  using ExecutionSignature = void(_1, _3);
  using InputDomain = _2;

  T IsoValue;

  VTKM_CONT
  Classification(T isovalue)
    : IsoValue(isovalue)
  {
  }

  template <typename FieldInType>
  VTKM_EXEC void operator()(const FieldInType& fieldIn, vtkm::IdComponent& caseNumber) const
  {
    using FieldType = typename vtkm::VecTraits<FieldInType>::ComponentType;
    const FieldType iso = static_cast<FieldType>(this->IsoValue);

    caseNumber = ((fieldIn[0] > iso) | (fieldIn[1] > iso) << 1 | (fieldIn[2] > iso) << 2 |
                  (fieldIn[3] > iso) << 3 | (fieldIn[4] > iso) << 4 | (fieldIn[5] > iso) << 5 |
                  (fieldIn[6] > iso) << 6 | (fieldIn[7] > iso) << 7);
  }
};

struct ValueTypes
  : vtkm::ListTagBase<vtkm::UInt32, vtkm::Int32, vtkm::Int64, vtkm::Float32, vtkm::Float64>
{
};
using StorageListTag = ::vtkm::cont::StorageListTagBasic;

/// This class runs a series of micro-benchmarks to measure
/// performance of different field operations
template <class DeviceAdapterTag>
class BenchmarkTopologyAlgorithms
{
  using StorageTag = vtkm::cont::StorageTagBasic;

  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;

  using Timer = vtkm::cont::Timer<DeviceAdapterTag>;

  using ValueDynamicHandle = vtkm::cont::DynamicArrayHandleBase<ValueTypes, StorageListTag>;

private:
  template <typename T, typename Enable = void>
  struct NumberGenerator
  {
  };

  template <typename T>
  struct NumberGenerator<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
  {
    std::mt19937 rng;
    std::uniform_real_distribution<T> distribution;
    NumberGenerator(T low, T high)
      : rng()
      , distribution(low, high)
    {
    }
    T next() { return distribution(rng); }
  };

  template <typename T>
  struct NumberGenerator<T, typename std::enable_if<!std::is_floating_point<T>::value>::type>
  {
    std::mt19937 rng;
    std::uniform_int_distribution<T> distribution;

    NumberGenerator(T low, T high)
      : rng()
      , distribution(low, high)
    {
    }
    T next() { return distribution(rng); }
  };

  template <typename Value>
  struct BenchCellToPointAvg
  {
    std::vector<Value> input;
    vtkm::cont::ArrayHandle<Value, StorageTag> InputHandle;
    std::size_t DomainSize;

    VTKM_CONT
    BenchCellToPointAvg()
    {
      NumberGenerator<Value> generator(static_cast<Value>(1.0), static_cast<Value>(100.0));
      //cube size is points in each dim
      this->DomainSize = (CUBE_SIZE - 1) * (CUBE_SIZE - 1) * (CUBE_SIZE - 1);
      this->input.resize(DomainSize);
      for (std::size_t i = 0; i < DomainSize; ++i)
      {
        this->input[i] = generator.next();
      }
      this->InputHandle = vtkm::cont::make_ArrayHandle(this->input);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(vtkm::Id3(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE));
      vtkm::cont::ArrayHandle<Value, StorageTag> result;

      Timer timer;

      vtkm::worklet::DispatcherMapTopology<AverageCellToPoint> dispatcher;
      dispatcher.Invoke(this->InputHandle, cellSet, result);
      //result.SyncControlArray();

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Static"); }

    VTKM_CONT
    std::string Description() const
    {

      std::stringstream description;
      description << "Computing Cell To Point Average "
                  << "[" << this->Type() << "] "
                  << "with a domain size of: " << this->DomainSize;
      return description.str();
    }
  };

  template <typename Value>
  struct BenchCellToPointAvgDynamic : public BenchCellToPointAvg<Value>
  {

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(vtkm::Id3(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE));

      ValueDynamicHandle dinput(this->InputHandle);
      vtkm::cont::ArrayHandle<Value, StorageTag> result;

      Timer timer;

      vtkm::worklet::DispatcherMapTopology<AverageCellToPoint> dispatcher;

      dispatcher.Invoke(dinput, cellSet, result);
      //result.SyncControlArray();

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Dynamic"); }
  };

  VTKM_MAKE_BENCHMARK(CellToPointAvg, BenchCellToPointAvg);
  VTKM_MAKE_BENCHMARK(CellToPointAvgDynamic, BenchCellToPointAvgDynamic);

  template <typename Value>
  struct BenchPointToCellAvg
  {
    std::vector<Value> input;
    vtkm::cont::ArrayHandle<Value, StorageTag> InputHandle;
    std::size_t DomainSize;

    VTKM_CONT
    BenchPointToCellAvg()
    {
      NumberGenerator<Value> generator(static_cast<Value>(1.0), static_cast<Value>(100.0));

      this->DomainSize = (CUBE_SIZE) * (CUBE_SIZE) * (CUBE_SIZE);
      this->input.resize(DomainSize);
      for (std::size_t i = 0; i < DomainSize; ++i)
      {
        this->input[i] = generator.next();
      }
      this->InputHandle = vtkm::cont::make_ArrayHandle(this->input);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(vtkm::Id3(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE));
      vtkm::cont::ArrayHandle<Value, StorageTag> result;

      Timer timer;

      vtkm::worklet::DispatcherMapTopology<AveragePointToCell> dispatcher;
      dispatcher.Invoke(this->InputHandle, cellSet, result);
      //result.SyncControlArray();

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Static"); }

    VTKM_CONT
    std::string Description() const
    {

      std::stringstream description;
      description << "Computing Point To Cell Average "
                  << "[" << this->Type() << "] "
                  << "with a domain size of: " << this->DomainSize;
      return description.str();
    }
  };

  template <typename Value>
  struct BenchPointToCellAvgDynamic : public BenchPointToCellAvg<Value>
  {

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(vtkm::Id3(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE));

      ValueDynamicHandle dinput(this->InputHandle);
      vtkm::cont::ArrayHandle<Value, StorageTag> result;

      Timer timer;

      vtkm::worklet::DispatcherMapTopology<AveragePointToCell> dispatcher;
      dispatcher.Invoke(dinput, cellSet, result);
      //result.SyncControlArray();

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Dynamic"); }
  };

  VTKM_MAKE_BENCHMARK(PointToCellAvg, BenchPointToCellAvg);
  VTKM_MAKE_BENCHMARK(PointToCellAvgDynamic, BenchPointToCellAvgDynamic);

  template <typename Value>
  struct BenchClassification
  {
    std::vector<Value> input;
    vtkm::cont::ArrayHandle<Value, StorageTag> InputHandle;
    Value IsoValue;
    size_t DomainSize;

    VTKM_CONT
    BenchClassification()
    {
      NumberGenerator<Value> generator(static_cast<Value>(1.0), static_cast<Value>(100.0));

      this->DomainSize = (CUBE_SIZE) * (CUBE_SIZE) * (CUBE_SIZE);
      this->input.resize(DomainSize);
      for (std::size_t i = 0; i < DomainSize; ++i)
      {
        this->input[i] = generator.next();
      }
      this->InputHandle = vtkm::cont::make_ArrayHandle(this->input);
      this->IsoValue = generator.next();
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(vtkm::Id3(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE));
      vtkm::cont::ArrayHandle<vtkm::IdComponent, StorageTag> result;

      ValueDynamicHandle dinput(this->InputHandle);

      Timer timer;

      Classification<Value> worklet(this->IsoValue);
      vtkm::worklet::DispatcherMapTopology<Classification<Value>> dispatcher(worklet);
      dispatcher.Invoke(dinput, cellSet, result);
      //result.SyncControlArray();

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Static"); }

    VTKM_CONT
    std::string Description() const
    {

      std::stringstream description;
      description << "Computing Marching Cubes Classification "
                  << "[" << this->Type() << "] "
                  << "with a domain size of: " << this->DomainSize;
      return description.str();
    }
  };

  template <typename Value>
  struct BenchClassificationDynamic : public BenchClassification<Value>
  {
    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(vtkm::Id3(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE));
      vtkm::cont::ArrayHandle<vtkm::IdComponent, StorageTag> result;

      Timer timer;

      Classification<Value> worklet(this->IsoValue);
      vtkm::worklet::DispatcherMapTopology<Classification<Value>> dispatcher(worklet);
      dispatcher.Invoke(this->InputHandle, cellSet, result);
      //result.SyncControlArray();

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Dynamic"); }
  };

  VTKM_MAKE_BENCHMARK(Classification, BenchClassification);
  VTKM_MAKE_BENCHMARK(ClassificationDynamic, BenchClassificationDynamic);

public:
  static VTKM_CONT int Run(int benchmarks)
  {
    std::cout << DIVIDER << "\nRunning Topology Algorithm benchmarks\n";

    if (benchmarks & CELL_TO_POINT)
    {
      std::cout << DIVIDER << "\nBenchmarking Cell To Point Average\n";
      VTKM_RUN_BENCHMARK(CellToPointAvg, ValueTypes());
      VTKM_RUN_BENCHMARK(CellToPointAvgDynamic, ValueTypes());
    }

    if (benchmarks & POINT_TO_CELL)
    {
      std::cout << DIVIDER << "\nBenchmarking Point to Cell Average\n";
      VTKM_RUN_BENCHMARK(PointToCellAvg, ValueTypes());
      VTKM_RUN_BENCHMARK(PointToCellAvgDynamic, ValueTypes());
    }

    if (benchmarks & MC_CLASSIFY)
    {
      std::cout << DIVIDER << "\nBenchmarking Hex/Voxel MC Classification\n";
      VTKM_RUN_BENCHMARK(Classification, ValueTypes());
      VTKM_RUN_BENCHMARK(ClassificationDynamic, ValueTypes());
    }

    return 0;
  }
};

#undef ARRAY_SIZE
}
} // namespace vtkm::benchmarking

int main(int argc, char* argv[])
{
  int benchmarks = 0;
  if (argc < 2)
  {
    benchmarks = vtkm::benchmarking::ALL;
  }
  else
  {
    for (int i = 1; i < argc; ++i)
    {
      std::string arg = argv[i];
      std::transform(arg.begin(), arg.end(), arg.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      });
      if (arg == "celltopoint")
      {
        benchmarks |= vtkm::benchmarking::CELL_TO_POINT;
      }
      else if (arg == "pointtocell")
      {
        benchmarks |= vtkm::benchmarking::POINT_TO_CELL;
      }
      else if (arg == "classify")
      {
        benchmarks |= vtkm::benchmarking::MC_CLASSIFY;
      }
      else
      {
        std::cout << "Unrecognized benchmark: " << argv[i] << std::endl;
        return 1;
      }
    }
  }

  //now actually execute the benchmarks
  return vtkm::benchmarking::BenchmarkTopologyAlgorithms<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Run(
    benchmarks);
}
