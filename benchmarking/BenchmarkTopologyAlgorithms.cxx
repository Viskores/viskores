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

#include <vtkm/Math.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/testing/Testing.h>

#include <cctype>
#include <random>
#include <string>

namespace
{

#define CUBE_SIZE 256

using ValueTypes = vtkm::List<vtkm::UInt32, vtkm::Int32, vtkm::Int64, vtkm::Float32, vtkm::Float64>;

using ValueVariantHandle = vtkm::cont::VariantArrayHandleBase<ValueTypes>;

// Hold configuration state (e.g. active device)
vtkm::cont::InitializeResult Config;

class AveragePointToCell : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(FieldInPoint inPoints, CellSetIn cellset, FieldOutCell outCells);
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

class AverageCellToPoint : public vtkm::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(FieldInCell inCells, CellSetIn topology, FieldOut outPoints);
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
class Classification : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(FieldInPoint inNodes, CellSetIn cellset, FieldOutCell outCaseId);
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

// Returns an extra random value.
// Like, an additional random value.
// Not a random value that's somehow "extra random".
template <typename ArrayT>
VTKM_CONT typename ArrayT::ValueType FillRandomValues(ArrayT& array,
                                                      vtkm::Id size,
                                                      vtkm::Float64 min,
                                                      vtkm::Float64 max)
{
  using ValueType = typename ArrayT::ValueType;

  NumberGenerator<ValueType> generator{ static_cast<ValueType>(min), static_cast<ValueType>(max) };
  array.Allocate(size);
  auto portal = array.GetPortalControl();
  for (vtkm::Id i = 0; i < size; ++i)
  {
    portal.Set(i, generator.next());
  }
  return generator.next();
}

template <typename Value>
struct BenchCellToPointAvgImpl
{
  vtkm::cont::ArrayHandle<Value> Input;

  ::benchmark::State& State;
  vtkm::Id CubeSize;
  vtkm::Id NumCells;

  vtkm::cont::Timer Timer;
  vtkm::cont::Invoker Invoker;

  VTKM_CONT
  BenchCellToPointAvgImpl(::benchmark::State& state)
    : State{ state }
    , CubeSize{ CUBE_SIZE }
    , NumCells{ (this->CubeSize - 1) * (this->CubeSize - 1) * (this->CubeSize - 1) }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {
    FillRandomValues(this->Input, this->NumCells, 1., 100.);

    { // Configure label:
      std::ostringstream desc;
      desc << "CubeSize:" << this->CubeSize;
      this->State.SetLabel(desc.str());
    }
  }

  template <typename BenchArrayType>
  VTKM_CONT void Run(const BenchArrayType& input)
  {
    vtkm::cont::CellSetStructured<3> cellSet;
    cellSet.SetPointDimensions(vtkm::Id3{ this->CubeSize, this->CubeSize, this->CubeSize });
    vtkm::cont::ArrayHandle<Value> result;

    for (auto _ : this->State)
    {
      (void)_;
      this->Timer.Start();
      this->Invoker(AverageCellToPoint{}, input, cellSet, result);
      this->Timer.Stop();

      this->State.SetIterationTime(this->Timer.GetElapsedTime());
    }

    // #items = #points
    const int64_t iterations = static_cast<int64_t>(this->State.iterations());
    this->State.SetItemsProcessed(static_cast<int64_t>(cellSet.GetNumberOfPoints()) * iterations);
  }
};

template <typename ValueType>
void BenchCellToPointAvgStatic(::benchmark::State& state)
{
  BenchCellToPointAvgImpl<ValueType> impl{ state };
  impl.Run(impl.Input);
};
VTKM_BENCHMARK_TEMPLATES(BenchCellToPointAvgStatic, ValueTypes);

template <typename ValueType>
void BenchCellToPointAvgDynamic(::benchmark::State& state)
{
  BenchCellToPointAvgImpl<ValueType> impl{ state };
  impl.Run(ValueVariantHandle{ impl.Input });
};
VTKM_BENCHMARK_TEMPLATES(BenchCellToPointAvgDynamic, ValueTypes);

template <typename Value>
struct BenchPointToCellAvgImpl
{
  vtkm::cont::ArrayHandle<Value> Input;

  ::benchmark::State& State;
  vtkm::Id CubeSize;
  vtkm::Id NumPoints;

  vtkm::cont::Timer Timer;
  vtkm::cont::Invoker Invoker;

  VTKM_CONT
  BenchPointToCellAvgImpl(::benchmark::State& state)
    : State{ state }
    , CubeSize{ CUBE_SIZE }
    , NumPoints{ (this->CubeSize) * (this->CubeSize) * (this->CubeSize) }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {
    FillRandomValues(this->Input, this->NumPoints, 1., 100.);

    { // Configure label:
      std::ostringstream desc;
      desc << "CubeSize:" << this->CubeSize;
      this->State.SetLabel(desc.str());
    }
  }

  template <typename BenchArrayType>
  VTKM_CONT void Run(const BenchArrayType& input)
  {
    vtkm::cont::CellSetStructured<3> cellSet;
    cellSet.SetPointDimensions(vtkm::Id3{ this->CubeSize, this->CubeSize, this->CubeSize });
    vtkm::cont::ArrayHandle<Value> result;

    for (auto _ : this->State)
    {
      (void)_;
      this->Timer.Start();
      this->Invoker(AveragePointToCell{}, input, cellSet, result);
      this->Timer.Stop();

      this->State.SetIterationTime(this->Timer.GetElapsedTime());
    }

    // #items = #cells
    const int64_t iterations = static_cast<int64_t>(this->State.iterations());
    this->State.SetItemsProcessed(static_cast<int64_t>(cellSet.GetNumberOfCells()) * iterations);
  }
};

template <typename ValueType>
void BenchPointToCellAvgStatic(::benchmark::State& state)
{
  BenchPointToCellAvgImpl<ValueType> impl{ state };
  impl.Run(impl.Input);
};
VTKM_BENCHMARK_TEMPLATES(BenchPointToCellAvgStatic, ValueTypes);

template <typename ValueType>
void BenchPointToCellAvgDynamic(::benchmark::State& state)
{
  BenchPointToCellAvgImpl<ValueType> impl{ state };
  impl.Run(ValueVariantHandle{ impl.Input });
};
VTKM_BENCHMARK_TEMPLATES(BenchPointToCellAvgDynamic, ValueTypes);

template <typename Value>
struct BenchClassificationImpl
{
  vtkm::cont::ArrayHandle<Value> Input;

  ::benchmark::State& State;
  vtkm::Id CubeSize;
  vtkm::Id DomainSize;
  Value IsoValue;

  vtkm::cont::Timer Timer;
  vtkm::cont::Invoker Invoker;

  VTKM_CONT
  BenchClassificationImpl(::benchmark::State& state)
    : State{ state }
    , CubeSize{ CUBE_SIZE }
    , DomainSize{ this->CubeSize * this->CubeSize * this->CubeSize }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {
    this->IsoValue = FillRandomValues(this->Input, this->DomainSize, 1., 100.);

    { // Configure label:
      std::ostringstream desc;
      desc << "CubeSize:" << this->CubeSize;
      this->State.SetLabel(desc.str());
    }
  }

  template <typename BenchArrayType>
  VTKM_CONT void Run(const BenchArrayType& input)
  {
    vtkm::cont::CellSetStructured<3> cellSet;
    cellSet.SetPointDimensions(vtkm::Id3{ this->CubeSize, this->CubeSize, this->CubeSize });
    vtkm::cont::ArrayHandle<vtkm::IdComponent> result;

    Classification<Value> worklet(this->IsoValue);

    for (auto _ : this->State)
    {
      (void)_;
      this->Timer.Start();
      this->Invoker(worklet, input, cellSet, result);
      this->Timer.Stop();

      this->State.SetIterationTime(this->Timer.GetElapsedTime());
    }

    // #items = #cells
    const int64_t iterations = static_cast<int64_t>(this->State.iterations());
    this->State.SetItemsProcessed(static_cast<int64_t>(cellSet.GetNumberOfCells()) * iterations);
  }
};

template <typename ValueType>
void BenchClassificationStatic(::benchmark::State& state)
{
  BenchClassificationImpl<ValueType> impl{ state };
  impl.Run(impl.Input);
};
VTKM_BENCHMARK_TEMPLATES(BenchClassificationStatic, ValueTypes);

template <typename ValueType>
void BenchClassificationDynamic(::benchmark::State& state)
{
  BenchClassificationImpl<ValueType> impl{ state };
  impl.Run(ValueVariantHandle{ impl.Input });
};
VTKM_BENCHMARK_TEMPLATES(BenchClassificationDynamic, ValueTypes);

} // end anon namespace

int main(int argc, char* argv[])
{
  // Parse VTK-m options:
  auto opts = vtkm::cont::InitializeOptions::RequireDevice | vtkm::cont::InitializeOptions::AddHelp;
  Config = vtkm::cont::Initialize(argc, argv, opts);

  // Setup device:
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);

  // handle benchmarking related args and run benchmarks:
  VTKM_EXECUTE_BENCHMARKS(argc, argv);
}
