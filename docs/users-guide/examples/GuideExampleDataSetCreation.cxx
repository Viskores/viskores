//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/FieldRangeCompute.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/cont/UncertainCellSet.h>

#include <vtkm/filter/field_conversion/CellAverage.h>

#include <vtkm/Math.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace DataSetCreationNamespace
{

namespace can_convert_example
{
////
//// BEGIN-EXAMPLE UnknownCellSetCanConvert
////
VTKM_CONT vtkm::Id3 Get3DPointDimensions(
  const vtkm::cont::UnknownCellSet& unknownCellSet)
{
  if (unknownCellSet.CanConvert<vtkm::cont::CellSetStructured<3>>())
  {
    vtkm::cont::CellSetStructured<3> cellSet;
    unknownCellSet.AsCellSet(cellSet);
    return cellSet.GetPointDimensions();
  }
  else if (unknownCellSet.CanConvert<vtkm::cont::CellSetStructured<2>>())
  {
    vtkm::cont::CellSetStructured<2> cellSet;
    unknownCellSet.AsCellSet(cellSet);
    vtkm::Id2 dims = cellSet.GetPointDimensions();
    return vtkm::Id3{ dims[0], dims[1], 1 };
  }
  else
  {
    return vtkm::Id3{ unknownCellSet.GetNumberOfPoints(), 1, 1 };
  }
}
////
//// END-EXAMPLE UnknownCellSetCanConvert
////
} // namespace can_convert_example

namespace cast_and_call_for_types_example
{

////
//// BEGIN-EXAMPLE UnknownCellSetCastAndCallForTypes
////
struct Get3DPointDimensionsFunctor
{
  template<vtkm::IdComponent Dims>
  VTKM_CONT void operator()(const vtkm::cont::CellSetStructured<Dims>& cellSet,
                            vtkm::Id3& outDims) const
  {
    vtkm::Vec<vtkm::Id, Dims> pointDims = cellSet.GetPointDimensions();
    for (vtkm::IdComponent d = 0; d < Dims; ++d)
    {
      outDims[d] = pointDims[d];
    }
  }

  VTKM_CONT void operator()(const vtkm::cont::CellSet& cellSet, vtkm::Id3& outDims) const
  {
    outDims[0] = cellSet.GetNumberOfPoints();
  }
};

VTKM_CONT vtkm::Id3 Get3DPointDimensions(
  const vtkm::cont::UnknownCellSet& unknownCellSet)
{
  vtkm::Id3 dims(1);
  unknownCellSet.CastAndCallForTypes<VTKM_DEFAULT_CELL_SET_LIST>(
    Get3DPointDimensionsFunctor{}, dims);
  return dims;
}
////
//// END-EXAMPLE UnknownCellSetCastAndCallForTypes
////

VTKM_CONT vtkm::Id3 Get3DStructuredPointDimensions(
  const vtkm::cont::UnknownCellSet& unknownCellSet)
{
  vtkm::Id3 dims;
  ////
  //// BEGIN-EXAMPLE UncertainCellSet
  ////
  using StructuredCellSetList = vtkm::List<vtkm::cont::CellSetStructured<1>,
                                           vtkm::cont::CellSetStructured<2>,
                                           vtkm::cont::CellSetStructured<3>>;
  vtkm::cont::UncertainCellSet<StructuredCellSetList> uncertainCellSet(unknownCellSet);
  uncertainCellSet.CastAndCall(Get3DPointDimensionsFunctor{}, dims);
  ////
  //// END-EXAMPLE UncertainCellSet
  ////
  return dims;
}

} // namespace cast_and_call_for_types_example

struct MyWorklet : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn, FieldOutCell);
  using ExecutionSignature = _2(IncidentElementCount);

  VTKM_EXEC vtkm::IdComponent operator()(vtkm::IdComponent pointCount) const
  {
    return pointCount;
  }
};

void CreateUniformGrid()
{
  std::cout << "Creating uniform grid." << std::endl;

  ////
  //// BEGIN-EXAMPLE CreateUniformGrid
  ////
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;

  vtkm::cont::DataSet dataSet = dataSetBuilder.Create(vtkm::Id3(101, 101, 26));
  ////
  //// END-EXAMPLE CreateUniformGrid
  ////

  vtkm::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();
  std::cout << bounds << std::endl;

  VTKM_TEST_ASSERT(test_equal(bounds, vtkm::Bounds(0, 100, 0, 100, 0, 25)),
                   "Bad bounds");
  vtkm::cont::UnknownCellSet unknownCellSet = dataSet.GetCellSet();
  VTKM_TEST_ASSERT(can_convert_example::Get3DPointDimensions(unknownCellSet) ==
                   vtkm::Id3(101, 101, 26));
  VTKM_TEST_ASSERT(cast_and_call_for_types_example::Get3DPointDimensions(
                     unknownCellSet) == vtkm::Id3(101, 101, 26));
  VTKM_TEST_ASSERT(cast_and_call_for_types_example::Get3DStructuredPointDimensions(
                     unknownCellSet) == vtkm::Id3(101, 101, 26));

  vtkm::cont::ArrayHandle<vtkm::IdComponent> outArray;
  ////
  //// BEGIN-EXAMPLE UnknownCellSetResetCellSetList
  ////
  using StructuredCellSetList = vtkm::List<vtkm::cont::CellSetStructured<1>,
                                           vtkm::cont::CellSetStructured<2>,
                                           vtkm::cont::CellSetStructured<3>>;
  vtkm::cont::Invoker invoke;
  invoke(
    MyWorklet{}, unknownCellSet.ResetCellSetList<StructuredCellSetList>(), outArray);
  ////
  //// END-EXAMPLE UnknownCellSetResetCellSetList
  ////
}

void CreateUniformGridCustomOriginSpacing()
{
  std::cout << "Creating uniform grid with custom origin and spacing." << std::endl;

  ////
  //// BEGIN-EXAMPLE CreateUniformGridCustomOriginSpacing
  ////
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;

  vtkm::cont::DataSet dataSet = dataSetBuilder.Create(vtkm::Id3(101, 101, 26),
                                                      vtkm::Vec3f(-50.0, -50.0, -50.0),
                                                      vtkm::Vec3f(1.0, 1.0, 4.0));
  ////
  //// END-EXAMPLE CreateUniformGridCustomOriginSpacing
  ////

  vtkm::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();
  std::cout << bounds << std::endl;

  VTKM_TEST_ASSERT(test_equal(bounds, vtkm::Bounds(-50, 50, -50, 50, -50, 50)),
                   "Bad bounds");
}

void CreateRectilinearGrid()
{
  std::cout << "Create rectilinear grid." << std::endl;

  ////
  //// BEGIN-EXAMPLE CreateRectilinearGrid
  ////
  // Make x coordinates range from -4 to 4 with tighter spacing near 0.
  std::vector<vtkm::Float32> xCoordinates;
  for (vtkm::Float32 x = -2.0f; x <= 2.0f; x += 0.02f)
  {
    xCoordinates.push_back(vtkm::CopySign(x * x, x));
  }

  // Make y coordinates range from 0 to 2 with tighter spacing near 2.
  std::vector<vtkm::Float32> yCoordinates;
  for (vtkm::Float32 y = 0.0f; y <= 4.0f; y += 0.02f)
  {
    yCoordinates.push_back(vtkm::Sqrt(y));
  }

  // Make z coordinates rangefrom -1 to 1 with even spacing.
  std::vector<vtkm::Float32> zCoordinates;
  for (vtkm::Float32 z = -1.0f; z <= 1.0f; z += 0.02f)
  {
    zCoordinates.push_back(z);
  }

  vtkm::cont::DataSetBuilderRectilinear dataSetBuilder;

  vtkm::cont::DataSet dataSet =
    dataSetBuilder.Create(xCoordinates, yCoordinates, zCoordinates);
  ////
  //// END-EXAMPLE CreateRectilinearGrid
  ////

  vtkm::Id numPoints = dataSet.GetCellSet().GetNumberOfPoints();
  std::cout << "Num points: " << numPoints << std::endl;
  VTKM_TEST_ASSERT(numPoints == 4080501, "Got wrong number of points.");

  vtkm::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();
  std::cout << bounds << std::endl;

  VTKM_TEST_ASSERT(test_equal(bounds, vtkm::Bounds(-4, 4, 0, 2, -1, 1)), "Bad bounds");
}

void CreateExplicitGrid()
{
  std::cout << "Creating explicit grid." << std::endl;

  ////
  //// BEGIN-EXAMPLE CreateExplicitGrid
  ////
  // Array of point coordinates.
  std::vector<vtkm::Vec3f_32> pointCoordinates;
  pointCoordinates.push_back(vtkm::Vec3f_32(1.1f, 0.0f, 0.0f));
  pointCoordinates.push_back(vtkm::Vec3f_32(0.2f, 0.4f, 0.0f));
  pointCoordinates.push_back(vtkm::Vec3f_32(0.9f, 0.6f, 0.0f));
  pointCoordinates.push_back(vtkm::Vec3f_32(1.4f, 0.5f, 0.0f));
  pointCoordinates.push_back(vtkm::Vec3f_32(1.8f, 0.3f, 0.0f));
  pointCoordinates.push_back(vtkm::Vec3f_32(0.4f, 1.0f, 0.0f));
  pointCoordinates.push_back(vtkm::Vec3f_32(1.0f, 1.2f, 0.0f));
  pointCoordinates.push_back(vtkm::Vec3f_32(1.5f, 0.9f, 0.0f));

  // Array of shapes.
  std::vector<vtkm::UInt8> shapes;
  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  shapes.push_back(vtkm::CELL_SHAPE_POLYGON);
  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);

  // Array of number of indices per cell.
  std::vector<vtkm::IdComponent> numIndices;
  numIndices.push_back(3);
  numIndices.push_back(4);
  numIndices.push_back(3);
  numIndices.push_back(5);
  numIndices.push_back(3);

  // Connectivity array.
  std::vector<vtkm::Id> connectivity;
  connectivity.push_back(0); // Cell 0
  connectivity.push_back(2);
  connectivity.push_back(1);
  connectivity.push_back(0); // Cell 1
  connectivity.push_back(4);
  connectivity.push_back(3);
  connectivity.push_back(2);
  connectivity.push_back(1); // Cell 2
  connectivity.push_back(2);
  connectivity.push_back(5);
  connectivity.push_back(2); // Cell 3
  connectivity.push_back(3);
  connectivity.push_back(7);
  connectivity.push_back(6);
  connectivity.push_back(5);
  connectivity.push_back(3); // Cell 4
  connectivity.push_back(4);
  connectivity.push_back(7);

  // Copy these arrays into a DataSet.
  vtkm::cont::DataSetBuilderExplicit dataSetBuilder;

  vtkm::cont::DataSet dataSet =
    dataSetBuilder.Create(pointCoordinates, shapes, numIndices, connectivity);
  ////
  //// END-EXAMPLE CreateExplicitGrid
  ////

  vtkm::cont::CellSetExplicit<> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);
  VTKM_TEST_ASSERT(test_equal(cellSet.GetNumberOfPoints(), 8),
                   "Data set has wrong number of points.");
  VTKM_TEST_ASSERT(test_equal(cellSet.GetNumberOfCells(), 5),
                   "Data set has wrong number of cells.");

  vtkm::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();
  std::cout << bounds << std::endl;

  VTKM_TEST_ASSERT(test_equal(bounds, vtkm::Bounds(0.2, 1.8, 0.0, 1.2, 0.0, 0.0)),
                   "Bad bounds");

  // Do a simple check of the connectivity by getting the number of cells
  // incident on each point. This array is unlikely to be correct if the
  // topology got screwed up.
  auto numCellsPerPoint = cellSet.GetNumIndicesArray(vtkm::TopologyElementTagPoint(),
                                                     vtkm::TopologyElementTagCell());

  vtkm::cont::printSummary_ArrayHandle(numCellsPerPoint, std::cout);
  std::cout << std::endl;
  auto numCellsPortal = numCellsPerPoint.ReadPortal();
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(0), 2),
                   "Wrong number of cells on point 0");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(1), 2),
                   "Wrong number of cells on point 1");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(2), 4),
                   "Wrong number of cells on point 2");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(3), 3),
                   "Wrong number of cells on point 3");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(4), 2),
                   "Wrong number of cells on point 4");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(5), 2),
                   "Wrong number of cells on point 5");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(6), 1),
                   "Wrong number of cells on point 6");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(7), 2),
                   "Wrong number of cells on point 7");
}

void CreateExplicitGridIterative()
{
  std::cout << "Creating explicit grid iteratively." << std::endl;

  ////
  //// BEGIN-EXAMPLE CreateExplicitGridIterative
  ////
  vtkm::cont::DataSetBuilderExplicitIterative dataSetBuilder;

  dataSetBuilder.AddPoint(1.1, 0.0, 0.0);
  dataSetBuilder.AddPoint(0.2, 0.4, 0.0);
  dataSetBuilder.AddPoint(0.9, 0.6, 0.0);
  dataSetBuilder.AddPoint(1.4, 0.5, 0.0);
  dataSetBuilder.AddPoint(1.8, 0.3, 0.0);
  dataSetBuilder.AddPoint(0.4, 1.0, 0.0);
  dataSetBuilder.AddPoint(1.0, 1.2, 0.0);
  dataSetBuilder.AddPoint(1.5, 0.9, 0.0);

  dataSetBuilder.AddCell(vtkm::CELL_SHAPE_TRIANGLE);
  dataSetBuilder.AddCellPoint(0);
  dataSetBuilder.AddCellPoint(2);
  dataSetBuilder.AddCellPoint(1);

  dataSetBuilder.AddCell(vtkm::CELL_SHAPE_QUAD);
  dataSetBuilder.AddCellPoint(0);
  dataSetBuilder.AddCellPoint(4);
  dataSetBuilder.AddCellPoint(3);
  dataSetBuilder.AddCellPoint(2);

  dataSetBuilder.AddCell(vtkm::CELL_SHAPE_TRIANGLE);
  dataSetBuilder.AddCellPoint(1);
  dataSetBuilder.AddCellPoint(2);
  dataSetBuilder.AddCellPoint(5);

  dataSetBuilder.AddCell(vtkm::CELL_SHAPE_POLYGON);
  dataSetBuilder.AddCellPoint(2);
  dataSetBuilder.AddCellPoint(3);
  dataSetBuilder.AddCellPoint(7);
  dataSetBuilder.AddCellPoint(6);
  dataSetBuilder.AddCellPoint(5);

  dataSetBuilder.AddCell(vtkm::CELL_SHAPE_TRIANGLE);
  dataSetBuilder.AddCellPoint(3);
  dataSetBuilder.AddCellPoint(4);
  dataSetBuilder.AddCellPoint(7);

  vtkm::cont::DataSet dataSet = dataSetBuilder.Create();
  ////
  //// END-EXAMPLE CreateExplicitGridIterative
  ////

  vtkm::cont::UnknownCellSet unknownCells = dataSet.GetCellSet();

  ////
  //// BEGIN-EXAMPLE UnknownCellSetAsCellSet
  ////
  vtkm::cont::CellSetExplicit<> cellSet;
  unknownCells.AsCellSet(cellSet);

  // This is an equivalent way to get the cell set.
  auto cellSet2 = unknownCells.AsCellSet<vtkm::cont::CellSetExplicit<>>();
  ////
  //// END-EXAMPLE UnknownCellSetAsCellSet
  ////

  VTKM_STATIC_ASSERT((std::is_same<decltype(cellSet), decltype(cellSet2)>::value));
  VTKM_TEST_ASSERT(cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell{},
                                                vtkm::TopologyElementTagPoint{}) ==
                   cellSet2.GetConnectivityArray(vtkm::TopologyElementTagCell{},
                                                 vtkm::TopologyElementTagPoint{}));

  VTKM_TEST_ASSERT(test_equal(cellSet.GetNumberOfPoints(), 8),
                   "Data set has wrong number of points.");
  VTKM_TEST_ASSERT(test_equal(cellSet.GetNumberOfCells(), 5),
                   "Data set has wrong number of cells.");

  vtkm::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();
  std::cout << bounds << std::endl;

  VTKM_TEST_ASSERT(test_equal(bounds, vtkm::Bounds(0.2, 1.8, 0.0, 1.2, 0.0, 0.0)),
                   "Bad bounds");

  // Do a simple check of the connectivity by getting the number of cells
  // incident on each point. This array is unlikely to be correct if the
  // topology got screwed up.
  auto numCellsPerPoint = cellSet.GetNumIndicesArray(vtkm::TopologyElementTagPoint(),
                                                     vtkm::TopologyElementTagCell());

  vtkm::cont::printSummary_ArrayHandle(numCellsPerPoint, std::cout);
  std::cout << std::endl;
  auto numCellsPortal = numCellsPerPoint.ReadPortal();
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(0), 2),
                   "Wrong number of cells on point 0");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(1), 2),
                   "Wrong number of cells on point 1");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(2), 4),
                   "Wrong number of cells on point 2");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(3), 3),
                   "Wrong number of cells on point 3");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(4), 2),
                   "Wrong number of cells on point 4");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(5), 2),
                   "Wrong number of cells on point 5");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(6), 1),
                   "Wrong number of cells on point 6");
  VTKM_TEST_ASSERT(test_equal(numCellsPortal.Get(7), 2),
                   "Wrong number of cells on point 7");
}

void AddFieldData()
{
  std::cout << "Add field data." << std::endl;

  ////
  //// BEGIN-EXAMPLE AddFieldData
  ////
  // Make a simple structured data set.
  const vtkm::Id3 pointDimensions(20, 20, 10);
  const vtkm::Id3 cellDimensions = pointDimensions - vtkm::Id3(1, 1, 1);
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet dataSet = dataSetBuilder.Create(pointDimensions);

  // Create a field that identifies points on the boundary.
  std::vector<vtkm::UInt8> boundaryPoints;
  for (vtkm::Id zIndex = 0; zIndex < pointDimensions[2]; zIndex++)
  {
    for (vtkm::Id yIndex = 0; yIndex < pointDimensions[1]; yIndex++)
    {
      for (vtkm::Id xIndex = 0; xIndex < pointDimensions[0]; xIndex++)
      {
        if ((xIndex == 0) || (xIndex == pointDimensions[0] - 1) || (yIndex == 0) ||
            (yIndex == pointDimensions[1] - 1) || (zIndex == 0) ||
            (zIndex == pointDimensions[2] - 1))
        {
          boundaryPoints.push_back(1);
        }
        else
        {
          boundaryPoints.push_back(0);
        }
      }
    }
  }

  dataSet.AddPointField("boundary_points", boundaryPoints);

  // Create a field that identifies cells on the boundary.
  std::vector<vtkm::UInt8> boundaryCells;
  for (vtkm::Id zIndex = 0; zIndex < cellDimensions[2]; zIndex++)
  {
    for (vtkm::Id yIndex = 0; yIndex < cellDimensions[1]; yIndex++)
    {
      for (vtkm::Id xIndex = 0; xIndex < cellDimensions[0]; xIndex++)
      {
        if ((xIndex == 0) || (xIndex == cellDimensions[0] - 1) || (yIndex == 0) ||
            (yIndex == cellDimensions[1] - 1) || (zIndex == 0) ||
            (zIndex == cellDimensions[2] - 1))
        {
          boundaryCells.push_back(1);
        }
        else
        {
          boundaryCells.push_back(0);
        }
      }
    }
  }

  dataSet.AddCellField("boundary_cells", boundaryCells);
  ////
  //// END-EXAMPLE AddFieldData
  ////
}

void CreateCellSetPermutation()
{
  std::cout << "Create a cell set permutation" << std::endl;

  ////
  //// BEGIN-EXAMPLE CreateCellSetPermutation
  ////
  // Create a simple data set.
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet originalDataSet = dataSetBuilder.Create(vtkm::Id3(33, 33, 26));
  vtkm::cont::CellSetStructured<3> originalCellSet;
  originalDataSet.GetCellSet().AsCellSet(originalCellSet);

  // Create a permutation array for the cells. Each value in the array refers
  // to a cell in the original cell set. This particular array selects every
  // 10th cell.
  vtkm::cont::ArrayHandleCounting<vtkm::Id> permutationArray(0, 10, 2560);

  // Create a permutation of that cell set containing only every 10th cell.
  vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<3>,
                                 vtkm::cont::ArrayHandleCounting<vtkm::Id>>
    permutedCellSet(permutationArray, originalCellSet);
  ////
  //// END-EXAMPLE CreateCellSetPermutation
  ////

  std::cout << "Num points: " << permutedCellSet.GetNumberOfPoints() << std::endl;
  VTKM_TEST_ASSERT(permutedCellSet.GetNumberOfPoints() == 28314,
                   "Wrong number of points.");
  std::cout << "Num cells: " << permutedCellSet.GetNumberOfCells() << std::endl;
  VTKM_TEST_ASSERT(permutedCellSet.GetNumberOfCells() == 2560, "Wrong number of cells.");
}

void CreatePartitionedDataSet()
{
  std::cout << "Creating partitioned data." << std::endl;

  ////
  //// BEGIN-EXAMPLE CreatePartitionedDataSet
  ////
  // Create two uniform data sets
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;

  vtkm::cont::DataSet dataSet1 = dataSetBuilder.Create(vtkm::Id3(10, 10, 10));
  vtkm::cont::DataSet dataSet2 = dataSetBuilder.Create(vtkm::Id3(30, 30, 30));

  // Add the datasets to a multi block
  vtkm::cont::PartitionedDataSet partitionedData;
  partitionedData.AppendPartitions({ dataSet1, dataSet2 });
  ////
  //// END-EXAMPLE CreatePartitionedDataSet
  ////

  VTKM_TEST_ASSERT(partitionedData.GetNumberOfPartitions() == 2,
                   "Incorrect number of blocks");
}

void QueryPartitionedDataSet()
{
  std::cout << "Query on a partitioned data." << std::endl;

  vtkm::cont::testing::MakeTestDataSet makeData;

  vtkm::cont::PartitionedDataSet partitionedData;
  partitionedData.AppendPartitions(
    { makeData.Make2DExplicitDataSet0(), makeData.Make3DExplicitDataSet5() });

  ////
  //// BEGIN-EXAMPLE QueryPartitionedDataSet
  ////
  // Get the bounds of a multi-block data set
  vtkm::Bounds bounds = vtkm::cont::BoundsCompute(partitionedData);

  // Get the overall min/max of a field named "cellvar"
  vtkm::cont::ArrayHandle<vtkm::Range> cellvarRanges =
    vtkm::cont::FieldRangeCompute(partitionedData, "cellvar");

  // Assuming the "cellvar" field has scalar values, then cellvarRanges has one entry
  vtkm::Range cellvarRange = cellvarRanges.ReadPortal().Get(0);
  ////
  //// END-EXAMPLE QueryPartitionedDataSet
  ////

  std::cout << bounds << std::endl;
  VTKM_TEST_ASSERT(test_equal(bounds, vtkm::Bounds(0.0, 3.0, 0.0, 4.0, 0.0, 1.0)),
                   "Bad bounds");

  std::cout << cellvarRange << std::endl;
  VTKM_TEST_ASSERT(test_equal(cellvarRange, vtkm::Range(0, 130.5)), "Bad range");
}

void FilterPartitionedDataSet()
{
  std::cout << "Filter on a partitioned data." << std::endl;

  vtkm::cont::testing::MakeTestDataSet makeData;

  vtkm::cont::PartitionedDataSet partitionedData;
  partitionedData.AppendPartitions(
    { makeData.Make3DUniformDataSet0(), makeData.Make3DUniformDataSet1() });

  ////
  //// BEGIN-EXAMPLE FilterPartitionedDataSet
  ////
  vtkm::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetActiveField("pointvar", vtkm::cont::Field::Association::Points);

  vtkm::cont::PartitionedDataSet results = cellAverage.Execute(partitionedData);
  ////
  //// END-EXAMPLE FilterPartitionedDataSet
  ////

  VTKM_TEST_ASSERT(results.GetNumberOfPartitions() == 2, "Incorrect number of blocks.");
}

void Test()
{
  CreateUniformGrid();
  CreateUniformGridCustomOriginSpacing();
  CreateRectilinearGrid();
  CreateExplicitGrid();
  CreateExplicitGridIterative();
  AddFieldData();
  CreateCellSetPermutation();
  CreatePartitionedDataSet();
  QueryPartitionedDataSet();
  FilterPartitionedDataSet();
}

} // namespace DataSetCreationNamespace

int GuideExampleDataSetCreation(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DataSetCreationNamespace::Test, argc, argv);
}
