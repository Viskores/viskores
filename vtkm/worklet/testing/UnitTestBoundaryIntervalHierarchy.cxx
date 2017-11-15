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
#include <vtkm/cont/testing/Testing.h>
//#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/worklet/spatialstructure/BoundaryIntervalHierarchy.h>
#include <vtkm/worklet/spatialstructure/BoundaryIntervalHierarchyBuilder.h>

namespace
{
/*
const char* TETS_ONLY_FILE = "tets_only.vtk";
const char* GLOBE_FILE = "globe.vtk";
const char* UCD2D_FILE = "ucd2d.vtk";
const char* UCD3D_FILE = "ucd3d.vtk";
const char* BUOYANCY_FILE = "buoyancy.vtk";
*/

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

struct BoundaryIntervalHierarchyTester : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>,
                                ExecObject,
                                WholeCellSetIn<>,
                                WholeArrayIn<>,
                                FieldIn<>,
                                FieldOut<>);
  typedef _6 ExecutionSignature(_1, _2, _3, _4, _5);

  template <typename Point,
            typename BoundaryIntervalHierarchyExecObject,
            typename CellSet,
            typename CoordsPortal>
  VTKM_EXEC vtkm::IdComponent operator()(const Point& point,
                                         const BoundaryIntervalHierarchyExecObject& bih,
                                         const CellSet& cellSet,
                                         const CoordsPortal& coords,
                                         const vtkm::Id expectedId) const
  {
    vtkm::Id cellId = bih.Find(point, cellSet, coords, *this);
    return (1 - static_cast<vtkm::IdComponent>(expectedId == cellId));
  }
}; // struct BoundaryIntervalHierarchyTester

vtkm::cont::DataSet ConstructDataSet(vtkm::Id size)
{
  return vtkm::cont::DataSetBuilderUniform().Create(vtkm::Id3(size, size, size));
}

void TestBoundaryIntervalHierarchy()
{
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  using Algorithms = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
  namespace spatial = vtkm::worklet::spatialstructure;

  const vtkm::cont::DataSet dataSet = ConstructDataSet(101);
  vtkm::cont::DynamicCellSet cellSet = dataSet.GetCellSet();
  vtkm::cont::DynamicArrayHandleCoordinateSystem vertices = dataSet.GetCoordinateSystem().GetData();

  spatial::BoundaryIntervalHierarchy bih =
    spatial::BoundaryIntervalHierarchyBuilder().Build(cellSet, vertices, DeviceAdapter());

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> centroids;
  vtkm::worklet::DispatcherMapTopology<CellCentroidCalculator>().Invoke(
    cellSet, vertices, centroids);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> negativePoints;
  negativePoints.Allocate(4);
  negativePoints.GetPortalControl().Set(0, vtkm::make_Vec<vtkm::FloatDefault>(-100, -100, -100));
  negativePoints.GetPortalControl().Set(1, vtkm::make_Vec<vtkm::FloatDefault>(100, -100, -100));
  negativePoints.GetPortalControl().Set(2, vtkm::make_Vec<vtkm::FloatDefault>(-100, 100, -100));
  negativePoints.GetPortalControl().Set(3, vtkm::make_Vec<vtkm::FloatDefault>(-100, -100, 100));
  auto points = vtkm::cont::make_ArrayHandleConcatenate(centroids, negativePoints);

  vtkm::worklet::spatialstructure::BoundaryIntervalHierarchyExecutionObject<DeviceAdapter>
    bihExecObject = bih.PrepareForInput<DeviceAdapter>();

  vtkm::cont::ArrayHandleCounting<vtkm::Id> expectedPositiveCellIds(
    0, 1, cellSet.GetNumberOfCells());
  vtkm::cont::ArrayHandleConstant<vtkm::Id> expectedNegativeCellIds(-1, 4);
  auto expectedCellIds =
    vtkm::cont::make_ArrayHandleConcatenate(expectedPositiveCellIds, expectedNegativeCellIds);

  vtkm::cont::ArrayHandle<vtkm::IdComponent> results;
  vtkm::worklet::DispatcherMapField<BoundaryIntervalHierarchyTester>().Invoke(
    points, bihExecObject, cellSet, vertices, expectedCellIds, results);
  vtkm::Id numDiffs = Algorithms::Reduce(results, 0, vtkm::Add());

  VTKM_TEST_ASSERT(test_equal(numDiffs, 0), "Wrong cell id for BoundaryIntervalHierarchy");
}

/*
vtkm::cont::DataSet LoadFromFile(const char* file)
{
  vtkm::io::reader::VTKDataSetReader reader(file);
  return reader.ReadDataSet();
}

void TestBoundaryIntervalHierarchyFromFile(const char* file)
{
  TestBoundaryIntervalHierarchy(LoadFromFile(file));
}
*/

void RunTest()
{
  TestBoundaryIntervalHierarchy();
}

} // anonymous namespace

int UnitTestBoundaryIntervalHierarchy(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(RunTest);
}
