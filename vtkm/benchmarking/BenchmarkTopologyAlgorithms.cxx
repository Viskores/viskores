//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/benchmarking/Benchmarker.h>

#ifdef VTKM_MSVC
#pragma warning(disable:4723)
#endif

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/random.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#ifdef VTKM_MSVC
#pragma warning(default:4723)
#endif

#include <string>

namespace vtkm {
namespace benchmarking {

#define CUBE_SIZE 256
const static std::string DIVIDER(40, '-');

enum BenchmarkName {
  CELL_TO_POINT = 1 << 1,
  POINT_TO_CELL = 1 << 2,
  MC_CLASSIFY = 1 << 3,
  ALL = CELL_TO_POINT |
        POINT_TO_CELL |
        MC_CLASSIFY
};

class AveragePointToCell : public vtkm::worklet::WorkletMapPointToCell
{
public:
  typedef void ControlSignature(FieldInPoint<> inPoints,
                                CellSetIn cellset,
                                FieldOutCell<> outCells);
  typedef void ExecutionSignature(_1, PointCount, _3);
  typedef _2 InputDomain;

  template<typename PointValueVecType, typename OutType>
  VTKM_EXEC_EXPORT
  void operator()(const PointValueVecType &pointValues,
                  const vtkm::IdComponent &numPoints,
                  OutType &average) const
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
  typedef void ControlSignature(FieldInCell<> inCells,
                                CellSetIn topology,
                                FieldOut<> outPoints);
  typedef void ExecutionSignature(_1, _3, CellCount);
  typedef _2 InputDomain;

  template<typename CellVecType, typename OutType>
  VTKM_EXEC_EXPORT
  void operator()(const CellVecType &cellValues,
                  OutType &avgVal,
                  const vtkm::IdComponent &numCellIDs) const
  {
    //simple functor that returns the average cell Value.
    avgVal = static_cast<OutType>(cellValues[0]);
    for (vtkm::IdComponent cellIndex = 1; cellIndex < numCellIDs; ++cellIndex)
    {
      avgVal += static_cast<OutType>(cellValues[cellIndex]);
    }
    avgVal = avgVal / static_cast<OutType>(numCellIDs);
  }
};

// -----------------------------------------------------------------------------
template<typename T>
class Classification : public vtkm::worklet::WorkletMapPointToCell
{
public:
  typedef void ControlSignature(
      FieldInPoint<> inNodes,
      CellSetIn cellset,
      FieldOutCell< IdComponentType > outCaseId);
  typedef void ExecutionSignature(_1, _3);
  typedef _2 InputDomain;

  T IsoValue;

  VTKM_CONT_EXPORT
  Classification(T isovalue) :
    IsoValue(isovalue)
  {
  }

  template<typename FieldInType>
  VTKM_EXEC_EXPORT
  void operator()(const FieldInType &fieldIn,
                  vtkm::IdComponent &caseNumber) const
  {
    typedef typename vtkm::VecTraits<FieldInType>::ComponentType FieldType;
    const FieldType iso = static_cast<FieldType>(this->IsoValue);

    caseNumber= ((fieldIn[0] > iso)      |
                 (fieldIn[1] > iso) << 1 |
                 (fieldIn[2] > iso) << 2 |
                 (fieldIn[3] > iso) << 3 |
                 (fieldIn[4] > iso) << 4 |
                 (fieldIn[5] > iso) << 5 |
                 (fieldIn[6] > iso) << 6 |
                 (fieldIn[7] > iso) << 7);
  }
};



/// This class runs a series of micro-benchmarks to measure
/// performance of different field operations
template<class DeviceAdapterTag>
class BenchmarkTopologyAlgorithms {
  typedef vtkm::cont::StorageTagBasic StorageTag;

  typedef vtkm::cont::ArrayHandle<vtkm::Id, StorageTag> IdArrayHandle;

  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag> Algorithm;

  typedef vtkm::cont::Timer<DeviceAdapterTag> Timer;

private:

  template<typename Value>
  struct BenchCellToPointAvg {
    std::vector< Value > input;
    vtkm::cont::ArrayHandle< Value, StorageTag> InputHandle;

    VTKM_CONT_EXPORT
    BenchCellToPointAvg()
    {
      typedef boost::uniform_real<Value> ValueRange;
      typedef boost::mt19937 MTGenerator;
      typedef boost::variate_generator<MTGenerator&, ValueRange> Generator;

      boost::mt19937 rng;
      boost::uniform_real<Value> range(static_cast<Value>(1.0),
                                       static_cast<Value>(100.0));
      Generator generator(rng, range);

      //cube size is points in each dim
      const std::size_t csize = (CUBE_SIZE-1)*(CUBE_SIZE-1)*(CUBE_SIZE-1);
      this->input.resize( csize );
      for(std::size_t i=0; i < csize; ++i )
      {
        this->input[i] = generator();
      }
      this->InputHandle = vtkm::cont::make_ArrayHandle(this->input);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()()
    {
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions( vtkm::Id3(CUBE_SIZE,CUBE_SIZE,CUBE_SIZE) );
      vtkm::cont::ArrayHandle<Value,StorageTag> result;

      Timer timer;

      vtkm::worklet::DispatcherMapTopology< AverageCellToPoint > dispatcher;
      dispatcher.Invoke( this->InputHandle,
                         cellSet,
                         result);

      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      const std::size_t csize = (CUBE_SIZE-1)*(CUBE_SIZE-1)*(CUBE_SIZE-1);
      std::stringstream description;
      description << "Computing Cell To Point Average for cell " << csize << " values";
      return description.str();
    }
  };

  VTKM_MAKE_BENCHMARK(CellToPointAvg, BenchCellToPointAvg);

  template<typename Value>
  struct BenchPointToCellAvg {
    std::vector< Value > input;
    vtkm::cont::ArrayHandle< Value, StorageTag> InputHandle;

    VTKM_CONT_EXPORT
    BenchPointToCellAvg()
    {
      typedef boost::uniform_real<Value> ValueRange;
      typedef boost::mt19937 MTGenerator;
      typedef boost::variate_generator<MTGenerator&, ValueRange> Generator;

      boost::mt19937 rng;
      boost::uniform_real<Value> range(static_cast<Value>(1.0),
                                       static_cast<Value>(100.0));
      Generator generator(rng, range);

      const std::size_t psize = (CUBE_SIZE)*(CUBE_SIZE)*(CUBE_SIZE);
      this->input.resize( psize );
      for(std::size_t i=0; i < psize; ++i )
      {
        this->input[i] = generator();
      }
      this->InputHandle = vtkm::cont::make_ArrayHandle(this->input);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()()
    {
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions( vtkm::Id3(CUBE_SIZE,CUBE_SIZE,CUBE_SIZE) );
      vtkm::cont::ArrayHandle<Value,StorageTag> result;

      Timer timer;

      vtkm::worklet::DispatcherMapTopology< AveragePointToCell > dispatcher;
      dispatcher.Invoke( this->InputHandle,
                         cellSet,
                         result);

      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      const std::size_t psize = (CUBE_SIZE)*(CUBE_SIZE)*(CUBE_SIZE);
      std::stringstream description;
      description << "Computing Point To Cell Average for point " << psize << " values";
      return description.str();
    }
  };

  VTKM_MAKE_BENCHMARK(PointToCellAvg, BenchPointToCellAvg);

  template<typename Value>
  struct BenchClassification {
    std::vector< Value > input;
    vtkm::cont::ArrayHandle< Value, StorageTag> InputHandle;
    Value IsoValue;

    VTKM_CONT_EXPORT
    BenchClassification()
    {
      typedef boost::uniform_real<Value> ValueRange;
      typedef boost::mt19937 MTGenerator;
      typedef boost::variate_generator<MTGenerator&, ValueRange> Generator;

      boost::mt19937 rng;
      boost::uniform_real<Value> range(static_cast<Value>(1.0),
                                       static_cast<Value>(100.0));
      Generator generator(rng, range);

      const std::size_t psize = (CUBE_SIZE)*(CUBE_SIZE)*(CUBE_SIZE);
      this->input.resize( psize );
      for(std::size_t i=0; i < psize; ++i )
      {
        this->input[i] = generator();
      }
      this->InputHandle = vtkm::cont::make_ArrayHandle(this->input);
      this->IsoValue = generator();
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()()
    {
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions( vtkm::Id3(CUBE_SIZE,CUBE_SIZE,CUBE_SIZE) );
      vtkm::cont::ArrayHandle< vtkm::IdComponent, StorageTag> result;

      Timer timer;

      Classification<Value> worklet(this->IsoValue);
      vtkm::worklet::DispatcherMapTopology< Classification<Value> > dispatcher(worklet);
      dispatcher.Invoke( this->InputHandle,
                         cellSet,
                         result);

      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      const std::size_t psize = (CUBE_SIZE)*(CUBE_SIZE)*(CUBE_SIZE);
      std::stringstream description;
      description << "Computing Marching Cubes Classification for point " << psize << " values";
      return description.str();
    }
  };

  VTKM_MAKE_BENCHMARK(Classification, BenchClassification);

public:
  struct ValueTypes : vtkm::ListTagBase<vtkm::UInt32, vtkm::Int32, vtkm::Int64,
                                        vtkm::Float32, vtkm::Float64  >{};


  static VTKM_CONT_EXPORT int Run(int benchmarks){
    std::cout << DIVIDER << "\nRunning Topology Algorithm benchmarks\n";

    if (benchmarks & CELL_TO_POINT) {
      std::cout << DIVIDER << "\nBenchmarking Cell To Point Average\n";
      VTKM_RUN_BENCHMARK(CellToPointAvg, ValueTypes());
    }

    if (benchmarks & POINT_TO_CELL){
      std::cout << DIVIDER << "\nBenchmarking Point to Cell Average\n";
      VTKM_RUN_BENCHMARK(PointToCellAvg, ValueTypes());
    }

    if (benchmarks & MC_CLASSIFY){
      std::cout << DIVIDER << "\nBenchmarking Hex/Voxel MC Classification\n";
      VTKM_RUN_BENCHMARK(Classification, ValueTypes());
    }

    return 0;
  }
};


#undef ARRAY_SIZE

}
} // namespace vtkm::benchmarking

int main(int argc, char *argv[])
{
  int benchmarks = 0;
  if (argc < 2){
    benchmarks = vtkm::benchmarking::ALL;
  }
  else {
    for (int i = 1; i < argc; ++i){
      std::string arg = argv[i];
      std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
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
      else {
        std::cout << "Unrecognized benchmark: " << argv[i] << std::endl;
        return 1;
      }
    }
  }

  //now actually execute the benchmarks
  return vtkm::benchmarking::BenchmarkTopologyAlgorithms
    <VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Run(benchmarks);
}
