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
#include <vtkm/cont/BoundingIntervalHierarchy.hxx>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/reader/VTKDataSetReader.h>

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
  typedef void ControlSignature(FieldIn<>, ExecObject, FieldIn<>, FieldOut<>);
  typedef _4 ExecutionSignature(_1, _2, _3);

  template <typename Point, typename BoundingIntervalHierarchyExecObject>
  VTKM_EXEC vtkm::IdComponent operator()(const Point& point,
                                         const BoundingIntervalHierarchyExecObject& bih,
                                         const vtkm::Id expectedId) const
  {
    vtkm::Vec<vtkm::FloatDefault, 3> parametric;
    vtkm::Id cellId;
    bih->FindCell(point, cellId, parametric, *this);
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

  vtkm::cont::DynamicCellSet cellSet = dataSet.GetCellSet();
  vtkm::cont::ArrayHandleVirtualCoordinates vertices = dataSet.GetCoordinateSystem().GetData();

  std::cout << "Using numPlanes: " << numPlanes << "\n";
  std::cout << "Building Bounding Interval Hierarchy Tree" << std::endl;
  vtkm::cont::BoundingIntervalHierarchy bih = vtkm::cont::BoundingIntervalHierarchy(numPlanes, 5);
  bih.SetCellSet(cellSet);
  bih.SetCoordinates(dataSet.GetCoordinateSystem());
  bih.Update();
  std::cout << "Built Bounding Interval Hierarchy Tree" << std::endl;

  Timer centroidsTimer;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> centroids;
  vtkm::worklet::DispatcherMapTopology<CellCentroidCalculator>().Invoke(
    cellSet, vertices, centroids);
  //std::cout << "Centroids calculation time: " << centroidsTimer.GetElapsedTime() << "\n";

  vtkm::cont::ArrayHandleCounting<vtkm::Id> expectedCellIds(0, 1, cellSet.GetNumberOfCells());

  Timer interpolationTimer;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> results;
#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_CUDA
  //set up stack size for cuda envinroment
  size_t stackSizeBackup;
  cudaDeviceGetLimit(&stackSizeBackup, cudaLimitStackSize);

  std::cout << "Default stack size " << stackSizeBackup << "\n";

  cudaDeviceSetLimit(cudaLimitStackSize, 1024 * 50);
#endif

  vtkm::worklet::DispatcherMapField<BoundingIntervalHierarchyTester>().Invoke(
    centroids, bih, expectedCellIds, results);

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
  VTKM_TEST_ASSERT(numDiffs == 0, "Calculated cell Ids not the same as expected cell Ids");
}

void RunTest()
{
  TestBoundingIntervalHierarchy(ConstructDataSet(16), 3);
  TestBoundingIntervalHierarchy(ConstructDataSet(16), 4);
  TestBoundingIntervalHierarchy(ConstructDataSet(16), 6);
  TestBoundingIntervalHierarchy(ConstructDataSet(16), 9);
}

} // anonymous namespace

int UnitTestBoundingIntervalHierarchy(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(RunTest);
}
