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

#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/worklet/spatialstructure/BoundingIntervalHierarchy.h>
#include <vtkm/worklet/spatialstructure/BoundingIntervalHierarchyBuilder.h>

namespace
{
struct CellCentroidCalculator : public vtkm::worklet::WorkletMapPointToCell
{
  typedef void ControlSignature(CellSetIn, FieldInPoint<>, FieldOut<>);
  typedef _3 ExecutionSignature(_1, PointCount, _2);

  template <typename CellShape, typename InputPointField>
  VTKM_EXEC typename InputPointField::ComponentType operator()(
    CellShape shape,
    vtkm::IdComponent numPoints,
    const InputPointField& inputPointField) const
  {
    vtkm::Vec<vtkm::FloatDefault, 3> parametricCenter =
      vtkm::exec::ParametricCoordinatesCenter(numPoints, shape, *this);
    return vtkm::exec::CellInterpolate(inputPointField, parametricCenter, shape, *this);
  }
}; // struct CellCentroidCalculator

struct BoundingIntervalHierarchyTester : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>,
                                ExecObject,
                                WholeCellSetIn<>,
                                WholeArrayIn<>,
                                FieldIn<>,
                                FieldOut<>);
  typedef _6 ExecutionSignature(_1, _2, _3, _4, _5);

  template <typename Point,
            typename BoundingIntervalHierarchyExecObject,
            typename CellSet,
            typename CoordsPortal>
  VTKM_EXEC vtkm::IdComponent operator()(const Point& point,
                                         const BoundingIntervalHierarchyExecObject& bih,
                                         const CellSet& cellSet,
                                         const CoordsPortal& coords,
                                         const vtkm::Id expectedId) const
  {
    vtkm::Id cellId = bih.Find(point, cellSet, coords, *this);
    return (1 - static_cast<vtkm::IdComponent>(expectedId == cellId));
  }
}; // struct BoundingIntervalHierarchyTester

vtkm::cont::DataSet ConstructDataSet(vtkm::Id size)
{
  return vtkm::cont::DataSetBuilderUniform().Create(vtkm::Id3(size, size, size));
}

void TestBoundingIntervalHierarchy(vtkm::cont::DataSet dataSet, vtkm::IdComponent numPlanes)
{
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  using Algorithms = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
  using Timer = vtkm::cont::Timer<DeviceAdapter>;
  namespace spatial = vtkm::worklet::spatialstructure;

  vtkm::cont::DynamicCellSet cellSet = dataSet.GetCellSet();
  vtkm::cont::DynamicArrayHandleCoordinateSystem vertices = dataSet.GetCoordinateSystem().GetData();

  std::cout << "Using numPlanes: " << numPlanes << "\n";
  spatial::BoundingIntervalHierarchy bih = spatial::BoundingIntervalHierarchyBuilder(numPlanes, 5)
                                             .Build(cellSet, vertices, DeviceAdapter());

  Timer centroidsTimer;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> centroids;
  vtkm::worklet::DispatcherMapTopology<CellCentroidCalculator>().Invoke(
    cellSet, vertices, centroids);
  std::cout << "Centroids calculation time: " << centroidsTimer.GetElapsedTime() << "\n";

  vtkm::worklet::spatialstructure::BoundingIntervalHierarchyExecutionObject<DeviceAdapter>
    bihExecObject = bih.PrepareForInput<DeviceAdapter>();

  vtkm::cont::ArrayHandleCounting<vtkm::Id> expectedCellIds(0, 1, cellSet.GetNumberOfCells());

  Timer interpolationTimer;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> results;
#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_CUDA
  //set up stack size for cuda envinroment
  size_t stackSizeBackup;
  cudaDeviceGetLimit(&stackSizeBackup, cudaLimitStackSize);
  cudaDeviceSetLimit(cudaLimitStackSize, 1024 * 200);
#endif
  vtkm::worklet::DispatcherMapField<BoundingIntervalHierarchyTester>().Invoke(
    centroids, bihExecObject, cellSet, vertices, expectedCellIds, results);
#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_CUDA
  cudaDeviceSetLimit(cudaLimitStackSize, stackSizeBackup);
#endif
  vtkm::Id numDiffs = Algorithms::Reduce(results, 0, vtkm::Add());
  vtkm::Float64 timeDiff = interpolationTimer.GetElapsedTime();
  std::cout << "No of interpolations: " << results.GetNumberOfValues() << "\n";
  std::cout << "Interpolation time: " << timeDiff << "\n";
  std::cout << "Average interpolation rate: "
            << (static_cast<vtkm::Float64>(results.GetNumberOfValues()) / timeDiff) << "\n";
  std::cout << "No of diffs: " << numDiffs << "\n";
}

vtkm::cont::DataSet LoadFromFile(const char* file)
{
  vtkm::io::reader::VTKDataSetReader reader(file);
  return reader.ReadDataSet();
}

void TestBoundingIntervalHierarchyFromFile(const char* file, vtkm::IdComponent numPlanes)
{
  TestBoundingIntervalHierarchy(LoadFromFile(file), numPlanes);
}

void RunTest()
{
  TestBoundingIntervalHierarchy(ConstructDataSet(145), 3);
  TestBoundingIntervalHierarchy(ConstructDataSet(145), 4);
  TestBoundingIntervalHierarchy(ConstructDataSet(145), 6);
  TestBoundingIntervalHierarchy(ConstructDataSet(145), 9);
  TestBoundingIntervalHierarchyFromFile("buoyancy.vtk", 3);
  TestBoundingIntervalHierarchyFromFile("buoyancy.vtk", 4);
  TestBoundingIntervalHierarchyFromFile("buoyancy.vtk", 6);
  TestBoundingIntervalHierarchyFromFile("buoyancy.vtk", 9);
}

} // anonymous namespace

int UnitTestBoundingIntervalHierarchy(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(RunTest);
}
