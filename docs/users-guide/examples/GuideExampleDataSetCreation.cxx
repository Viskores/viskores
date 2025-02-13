//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/BoundsCompute.h>
#include <viskores/cont/CellSetPermutation.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/FieldRangeCompute.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/UncertainCellSet.h>

#include <viskores/filter/field_conversion/CellAverage.h>

#include <viskores/Math.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace DataSetCreationNamespace
{

namespace can_convert_example
{
////
//// BEGIN-EXAMPLE UnknownCellSetCanConvert
////
VISKORES_CONT viskores::Id3 Get3DPointDimensions(
  const viskores::cont::UnknownCellSet& unknownCellSet)
{
  if (unknownCellSet.CanConvert<viskores::cont::CellSetStructured<3>>())
  {
    viskores::cont::CellSetStructured<3> cellSet;
    unknownCellSet.AsCellSet(cellSet);
    return cellSet.GetPointDimensions();
  }
  else if (unknownCellSet.CanConvert<viskores::cont::CellSetStructured<2>>())
  {
    viskores::cont::CellSetStructured<2> cellSet;
    unknownCellSet.AsCellSet(cellSet);
    viskores::Id2 dims = cellSet.GetPointDimensions();
    return viskores::Id3{ dims[0], dims[1], 1 };
  }
  else
  {
    return viskores::Id3{ unknownCellSet.GetNumberOfPoints(), 1, 1 };
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
  template<viskores::IdComponent Dims>
  VISKORES_CONT void operator()(const viskores::cont::CellSetStructured<Dims>& cellSet,
                                viskores::Id3& outDims) const
  {
    viskores::Vec<viskores::Id, Dims> pointDims = cellSet.GetPointDimensions();
    for (viskores::IdComponent d = 0; d < Dims; ++d)
    {
      outDims[d] = pointDims[d];
    }
  }

  VISKORES_CONT void operator()(const viskores::cont::CellSet& cellSet,
                                viskores::Id3& outDims) const
  {
    outDims[0] = cellSet.GetNumberOfPoints();
  }
};

VISKORES_CONT viskores::Id3 Get3DPointDimensions(
  const viskores::cont::UnknownCellSet& unknownCellSet)
{
  viskores::Id3 dims(1);
  unknownCellSet.CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST>(
    Get3DPointDimensionsFunctor{}, dims);
  return dims;
}
////
//// END-EXAMPLE UnknownCellSetCastAndCallForTypes
////

VISKORES_CONT viskores::Id3 Get3DStructuredPointDimensions(
  const viskores::cont::UnknownCellSet& unknownCellSet)
{
  viskores::Id3 dims;
  ////
  //// BEGIN-EXAMPLE UncertainCellSet
  ////
  using StructuredCellSetList = viskores::List<viskores::cont::CellSetStructured<1>,
                                               viskores::cont::CellSetStructured<2>,
                                               viskores::cont::CellSetStructured<3>>;
  viskores::cont::UncertainCellSet<StructuredCellSetList> uncertainCellSet(
    unknownCellSet);
  uncertainCellSet.CastAndCall(Get3DPointDimensionsFunctor{}, dims);
  ////
  //// END-EXAMPLE UncertainCellSet
  ////
  return dims;
}

} // namespace cast_and_call_for_types_example

struct MyWorklet : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn, FieldOutCell);
  using ExecutionSignature = _2(IncidentElementCount);

  VISKORES_EXEC viskores::IdComponent operator()(viskores::IdComponent pointCount) const
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
  viskores::cont::DataSetBuilderUniform dataSetBuilder;

  viskores::cont::DataSet dataSet = dataSetBuilder.Create(viskores::Id3(101, 101, 26));
  ////
  //// END-EXAMPLE CreateUniformGrid
  ////

  viskores::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();
  std::cout << bounds << std::endl;

  VISKORES_TEST_ASSERT(test_equal(bounds, viskores::Bounds(0, 100, 0, 100, 0, 25)),
                       "Bad bounds");
  viskores::cont::UnknownCellSet unknownCellSet = dataSet.GetCellSet();
  VISKORES_TEST_ASSERT(can_convert_example::Get3DPointDimensions(unknownCellSet) ==
                       viskores::Id3(101, 101, 26));
  VISKORES_TEST_ASSERT(cast_and_call_for_types_example::Get3DPointDimensions(
                         unknownCellSet) == viskores::Id3(101, 101, 26));
  VISKORES_TEST_ASSERT(cast_and_call_for_types_example::Get3DStructuredPointDimensions(
                         unknownCellSet) == viskores::Id3(101, 101, 26));

  viskores::cont::ArrayHandle<viskores::IdComponent> outArray;
  ////
  //// BEGIN-EXAMPLE UnknownCellSetResetCellSetList
  ////
  using StructuredCellSetList = viskores::List<viskores::cont::CellSetStructured<1>,
                                               viskores::cont::CellSetStructured<2>,
                                               viskores::cont::CellSetStructured<3>>;
  viskores::cont::Invoker invoke;
  invoke(
    MyWorklet{}, unknownCellSet.ResetCellSetList<StructuredCellSetList>(), outArray);
  ////
  //// END-EXAMPLE UnknownCellSetResetCellSetList
  ////

  ////
  //// BEGIN-EXAMPLE DataSetPrintSummary
  ////
  dataSet.PrintSummary(std::cout);
  ////
  //// END-EXAMPLE DataSetPrintSummary
  ////
}

void CreateUniformGridCustomOriginSpacing()
{
  std::cout << "Creating uniform grid with custom origin and spacing." << std::endl;

  ////
  //// BEGIN-EXAMPLE CreateUniformGridCustomOriginSpacing
  ////
  viskores::cont::DataSetBuilderUniform dataSetBuilder;

  viskores::cont::DataSet dataSet =
    dataSetBuilder.Create(viskores::Id3(101, 101, 26),
                          viskores::Vec3f(-50.0, -50.0, -50.0),
                          viskores::Vec3f(1.0, 1.0, 4.0));
  ////
  //// END-EXAMPLE CreateUniformGridCustomOriginSpacing
  ////

  viskores::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();
  std::cout << bounds << std::endl;

  VISKORES_TEST_ASSERT(test_equal(bounds, viskores::Bounds(-50, 50, -50, 50, -50, 50)),
                       "Bad bounds");
}

void CreateRectilinearGrid()
{
  std::cout << "Create rectilinear grid." << std::endl;

  ////
  //// BEGIN-EXAMPLE CreateRectilinearGrid
  ////
  // Make x coordinates range from -4 to 4 with tighter spacing near 0.
  std::vector<viskores::Float32> xCoordinates;
  for (viskores::Float32 x = -2.0f; x <= 2.0f; x += 0.02f)
  {
    xCoordinates.push_back(viskores::CopySign(x * x, x));
  }

  // Make y coordinates range from 0 to 2 with tighter spacing near 2.
  std::vector<viskores::Float32> yCoordinates;
  for (viskores::Float32 y = 0.0f; y <= 4.0f; y += 0.02f)
  {
    yCoordinates.push_back(viskores::Sqrt(y));
  }

  // Make z coordinates rangefrom -1 to 1 with even spacing.
  std::vector<viskores::Float32> zCoordinates;
  for (viskores::Float32 z = -1.0f; z <= 1.0f; z += 0.02f)
  {
    zCoordinates.push_back(z);
  }

  viskores::cont::DataSetBuilderRectilinear dataSetBuilder;

  viskores::cont::DataSet dataSet =
    dataSetBuilder.Create(xCoordinates, yCoordinates, zCoordinates);
  ////
  //// END-EXAMPLE CreateRectilinearGrid
  ////

  viskores::Id numPoints = dataSet.GetCellSet().GetNumberOfPoints();
  std::cout << "Num points: " << numPoints << std::endl;
  VISKORES_TEST_ASSERT(numPoints == 4080501, "Got wrong number of points.");

  viskores::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();
  std::cout << bounds << std::endl;

  VISKORES_TEST_ASSERT(test_equal(bounds, viskores::Bounds(-4, 4, 0, 2, -1, 1)),
                       "Bad bounds");
}

void CreateExplicitGrid()
{
  std::cout << "Creating explicit grid." << std::endl;

  ////
  //// BEGIN-EXAMPLE CreateExplicitGrid
  ////
  // Array of point coordinates.
  std::vector<viskores::Vec3f_32> pointCoordinates;
  pointCoordinates.push_back(viskores::Vec3f_32(1.1f, 0.0f, 0.0f));
  pointCoordinates.push_back(viskores::Vec3f_32(0.2f, 0.4f, 0.0f));
  pointCoordinates.push_back(viskores::Vec3f_32(0.9f, 0.6f, 0.0f));
  pointCoordinates.push_back(viskores::Vec3f_32(1.4f, 0.5f, 0.0f));
  pointCoordinates.push_back(viskores::Vec3f_32(1.8f, 0.3f, 0.0f));
  pointCoordinates.push_back(viskores::Vec3f_32(0.4f, 1.0f, 0.0f));
  pointCoordinates.push_back(viskores::Vec3f_32(1.0f, 1.2f, 0.0f));
  pointCoordinates.push_back(viskores::Vec3f_32(1.5f, 0.9f, 0.0f));

  // Array of shapes.
  std::vector<viskores::UInt8> shapes;
  shapes.push_back(viskores::CELL_SHAPE_TRIANGLE);
  shapes.push_back(viskores::CELL_SHAPE_QUAD);
  shapes.push_back(viskores::CELL_SHAPE_TRIANGLE);
  shapes.push_back(viskores::CELL_SHAPE_POLYGON);
  shapes.push_back(viskores::CELL_SHAPE_TRIANGLE);

  // Array of number of indices per cell.
  std::vector<viskores::IdComponent> numIndices;
  numIndices.push_back(3);
  numIndices.push_back(4);
  numIndices.push_back(3);
  numIndices.push_back(5);
  numIndices.push_back(3);

  // Connectivity array.
  std::vector<viskores::Id> connectivity;
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
  viskores::cont::DataSetBuilderExplicit dataSetBuilder;

  viskores::cont::DataSet dataSet =
    dataSetBuilder.Create(pointCoordinates, shapes, numIndices, connectivity);
  ////
  //// END-EXAMPLE CreateExplicitGrid
  ////

  viskores::cont::CellSetExplicit<> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);
  VISKORES_TEST_ASSERT(test_equal(cellSet.GetNumberOfPoints(), 8),
                       "Data set has wrong number of points.");
  VISKORES_TEST_ASSERT(test_equal(cellSet.GetNumberOfCells(), 5),
                       "Data set has wrong number of cells.");

  viskores::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();
  std::cout << bounds << std::endl;

  VISKORES_TEST_ASSERT(
    test_equal(bounds, viskores::Bounds(0.2, 1.8, 0.0, 1.2, 0.0, 0.0)), "Bad bounds");

  // Do a simple check of the connectivity by getting the number of cells
  // incident on each point. This array is unlikely to be correct if the
  // topology got screwed up.
  auto numCellsPerPoint = cellSet.GetNumIndicesArray(viskores::TopologyElementTagPoint(),
                                                     viskores::TopologyElementTagCell());

  viskores::cont::printSummary_ArrayHandle(numCellsPerPoint, std::cout);
  std::cout << std::endl;
  auto numCellsPortal = numCellsPerPoint.ReadPortal();
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(0), 2),
                       "Wrong number of cells on point 0");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(1), 2),
                       "Wrong number of cells on point 1");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(2), 4),
                       "Wrong number of cells on point 2");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(3), 3),
                       "Wrong number of cells on point 3");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(4), 2),
                       "Wrong number of cells on point 4");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(5), 2),
                       "Wrong number of cells on point 5");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(6), 1),
                       "Wrong number of cells on point 6");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(7), 2),
                       "Wrong number of cells on point 7");
}

void CreateExplicitGridIterative()
{
  std::cout << "Creating explicit grid iteratively." << std::endl;

  ////
  //// BEGIN-EXAMPLE CreateExplicitGridIterative
  ////
  viskores::cont::DataSetBuilderExplicitIterative dataSetBuilder;

  dataSetBuilder.AddPoint(1.1, 0.0, 0.0);
  dataSetBuilder.AddPoint(0.2, 0.4, 0.0);
  dataSetBuilder.AddPoint(0.9, 0.6, 0.0);
  dataSetBuilder.AddPoint(1.4, 0.5, 0.0);
  dataSetBuilder.AddPoint(1.8, 0.3, 0.0);
  dataSetBuilder.AddPoint(0.4, 1.0, 0.0);
  dataSetBuilder.AddPoint(1.0, 1.2, 0.0);
  dataSetBuilder.AddPoint(1.5, 0.9, 0.0);

  dataSetBuilder.AddCell(viskores::CELL_SHAPE_TRIANGLE);
  dataSetBuilder.AddCellPoint(0);
  dataSetBuilder.AddCellPoint(2);
  dataSetBuilder.AddCellPoint(1);

  dataSetBuilder.AddCell(viskores::CELL_SHAPE_QUAD);
  dataSetBuilder.AddCellPoint(0);
  dataSetBuilder.AddCellPoint(4);
  dataSetBuilder.AddCellPoint(3);
  dataSetBuilder.AddCellPoint(2);

  dataSetBuilder.AddCell(viskores::CELL_SHAPE_TRIANGLE);
  dataSetBuilder.AddCellPoint(1);
  dataSetBuilder.AddCellPoint(2);
  dataSetBuilder.AddCellPoint(5);

  dataSetBuilder.AddCell(viskores::CELL_SHAPE_POLYGON);
  dataSetBuilder.AddCellPoint(2);
  dataSetBuilder.AddCellPoint(3);
  dataSetBuilder.AddCellPoint(7);
  dataSetBuilder.AddCellPoint(6);
  dataSetBuilder.AddCellPoint(5);

  dataSetBuilder.AddCell(viskores::CELL_SHAPE_TRIANGLE);
  dataSetBuilder.AddCellPoint(3);
  dataSetBuilder.AddCellPoint(4);
  dataSetBuilder.AddCellPoint(7);

  viskores::cont::DataSet dataSet = dataSetBuilder.Create();
  ////
  //// END-EXAMPLE CreateExplicitGridIterative
  ////

  viskores::cont::UnknownCellSet unknownCells = dataSet.GetCellSet();

  ////
  //// BEGIN-EXAMPLE UnknownCellSetAsCellSet
  ////
  viskores::cont::CellSetExplicit<> cellSet;
  unknownCells.AsCellSet(cellSet);

  // This is an equivalent way to get the cell set.
  auto cellSet2 = unknownCells.AsCellSet<viskores::cont::CellSetExplicit<>>();
  ////
  //// END-EXAMPLE UnknownCellSetAsCellSet
  ////

  VISKORES_STATIC_ASSERT((std::is_same<decltype(cellSet), decltype(cellSet2)>::value));
  VISKORES_TEST_ASSERT(
    cellSet.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                 viskores::TopologyElementTagPoint{}) ==
    cellSet2.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                  viskores::TopologyElementTagPoint{}));

  VISKORES_TEST_ASSERT(test_equal(cellSet.GetNumberOfPoints(), 8),
                       "Data set has wrong number of points.");
  VISKORES_TEST_ASSERT(test_equal(cellSet.GetNumberOfCells(), 5),
                       "Data set has wrong number of cells.");

  viskores::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();
  std::cout << bounds << std::endl;

  VISKORES_TEST_ASSERT(
    test_equal(bounds, viskores::Bounds(0.2, 1.8, 0.0, 1.2, 0.0, 0.0)), "Bad bounds");

  // Do a simple check of the connectivity by getting the number of cells
  // incident on each point. This array is unlikely to be correct if the
  // topology got screwed up.
  auto numCellsPerPoint = cellSet.GetNumIndicesArray(viskores::TopologyElementTagPoint(),
                                                     viskores::TopologyElementTagCell());

  viskores::cont::printSummary_ArrayHandle(numCellsPerPoint, std::cout);
  std::cout << std::endl;
  auto numCellsPortal = numCellsPerPoint.ReadPortal();
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(0), 2),
                       "Wrong number of cells on point 0");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(1), 2),
                       "Wrong number of cells on point 1");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(2), 4),
                       "Wrong number of cells on point 2");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(3), 3),
                       "Wrong number of cells on point 3");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(4), 2),
                       "Wrong number of cells on point 4");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(5), 2),
                       "Wrong number of cells on point 5");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(6), 1),
                       "Wrong number of cells on point 6");
  VISKORES_TEST_ASSERT(test_equal(numCellsPortal.Get(7), 2),
                       "Wrong number of cells on point 7");
}

////
//// BEGIN-EXAMPLE DataSetCopyOperator
////
viskores::cont::DataSet AddFieldsExample(const viskores::cont::DataSet& input)
{
  viskores::cont::DataSet output;
  output = input;

  // Add interesting fields...

  return output;
}
////
//// END-EXAMPLE DataSetCopyOperator
////

////
//// BEGIN-EXAMPLE DataSetCopyStructure
////
viskores::cont::DataSet RemoveFieldExample(const viskores::cont::DataSet& input,
                                           const std::string& fieldToRemove)
{
  viskores::cont::DataSet output;
  output.CopyStructure(input);

  for (viskores::IdComponent fieldId = 0; fieldId < input.GetNumberOfFields(); ++fieldId)
  {
    viskores::cont::Field field = input.GetField(fieldId);
    if (field.GetName() != fieldToRemove)
    {
      output.AddField(field);
    }
  }

  return output;
}
////
//// END-EXAMPLE DataSetCopyStructure
////

void AddFieldData()
{
  std::cout << "Add field data." << std::endl;

  ////
  //// BEGIN-EXAMPLE AddFieldData
  ////
  // Make a simple structured data set.
  const viskores::Id3 pointDimensions(20, 20, 10);
  const viskores::Id3 cellDimensions = pointDimensions - viskores::Id3(1, 1, 1);
  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  viskores::cont::DataSet dataSet = dataSetBuilder.Create(pointDimensions);

  // Create a field that identifies points on the boundary.
  std::vector<viskores::UInt8> boundaryPoints;
  for (viskores::Id zIndex = 0; zIndex < pointDimensions[2]; zIndex++)
  {
    for (viskores::Id yIndex = 0; yIndex < pointDimensions[1]; yIndex++)
    {
      for (viskores::Id xIndex = 0; xIndex < pointDimensions[0]; xIndex++)
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
  std::vector<viskores::UInt8> boundaryCells;
  for (viskores::Id zIndex = 0; zIndex < cellDimensions[2]; zIndex++)
  {
    for (viskores::Id yIndex = 0; yIndex < cellDimensions[1]; yIndex++)
    {
      for (viskores::Id xIndex = 0; xIndex < cellDimensions[0]; xIndex++)
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

  ////
  //// BEGIN-EXAMPLE IterateFields
  ////
  std::cout << "Fields in data:";
  for (viskores::IdComponent fieldId = 0; fieldId < dataSet.GetNumberOfFields();
       ++fieldId)
  {
    viskores::cont::Field field = dataSet.GetField(fieldId);
    std::cout << " " << field.GetName();
  }
  std::cout << std::endl;
  ////
  //// END-EXAMPLE IterateFields
  ////

  viskores::cont::DataSet copy1 = AddFieldsExample(dataSet);
  VISKORES_TEST_ASSERT(copy1.GetNumberOfFields() == dataSet.GetNumberOfFields());
  VISKORES_TEST_ASSERT(copy1.GetNumberOfPoints() == dataSet.GetNumberOfPoints());
  VISKORES_TEST_ASSERT(copy1.GetNumberOfCells() == dataSet.GetNumberOfCells());

  viskores::cont::DataSet copy2 = RemoveFieldExample(dataSet, "boundary_cells");
  VISKORES_TEST_ASSERT(copy2.GetNumberOfFields() == dataSet.GetNumberOfFields() - 1);
  VISKORES_TEST_ASSERT(copy2.GetNumberOfPoints() == dataSet.GetNumberOfPoints());
  VISKORES_TEST_ASSERT(copy2.GetNumberOfCells() == dataSet.GetNumberOfCells());
}

void CreateCellSetPermutation()
{
  std::cout << "Create a cell set permutation" << std::endl;

  ////
  //// BEGIN-EXAMPLE CreateCellSetPermutation
  ////
  // Create a simple data set.
  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  viskores::cont::DataSet originalDataSet =
    dataSetBuilder.Create(viskores::Id3(33, 33, 26));
  viskores::cont::CellSetStructured<3> originalCellSet;
  originalDataSet.GetCellSet().AsCellSet(originalCellSet);

  // Create a permutation array for the cells. Each value in the array refers
  // to a cell in the original cell set. This particular array selects every
  // 10th cell.
  viskores::cont::ArrayHandleCounting<viskores::Id> permutationArray(0, 10, 2560);

  // Create a permutation of that cell set containing only every 10th cell.
  viskores::cont::CellSetPermutation<viskores::cont::CellSetStructured<3>,
                                     viskores::cont::ArrayHandleCounting<viskores::Id>>
    permutedCellSet(permutationArray, originalCellSet);
  ////
  //// END-EXAMPLE CreateCellSetPermutation
  ////

  std::cout << "Num points: " << permutedCellSet.GetNumberOfPoints() << std::endl;
  VISKORES_TEST_ASSERT(permutedCellSet.GetNumberOfPoints() == 28314,
                       "Wrong number of points.");
  std::cout << "Num cells: " << permutedCellSet.GetNumberOfCells() << std::endl;
  VISKORES_TEST_ASSERT(permutedCellSet.GetNumberOfCells() == 2560,
                       "Wrong number of cells.");
}

void CreatePartitionedDataSet()
{
  std::cout << "Creating partitioned data." << std::endl;

  ////
  //// BEGIN-EXAMPLE CreatePartitionedDataSet
  ////
  // Create two uniform data sets
  viskores::cont::DataSetBuilderUniform dataSetBuilder;

  viskores::cont::DataSet dataSet1 = dataSetBuilder.Create(viskores::Id3(10, 10, 10));
  viskores::cont::DataSet dataSet2 = dataSetBuilder.Create(viskores::Id3(30, 30, 30));

  // Add the datasets to a multi block
  viskores::cont::PartitionedDataSet partitionedData;
  partitionedData.AppendPartitions({ dataSet1, dataSet2 });
  ////
  //// END-EXAMPLE CreatePartitionedDataSet
  ////

  VISKORES_TEST_ASSERT(partitionedData.GetNumberOfPartitions() == 2,
                       "Incorrect number of blocks");
}

void QueryPartitionedDataSet()
{
  std::cout << "Query on a partitioned data." << std::endl;

  viskores::cont::testing::MakeTestDataSet makeData;

  viskores::cont::PartitionedDataSet partitionedData;
  partitionedData.AppendPartitions(
    { makeData.Make2DExplicitDataSet0(), makeData.Make3DExplicitDataSet5() });

  ////
  //// BEGIN-EXAMPLE QueryPartitionedDataSet
  ////
  // Get the bounds of a multi-block data set
  viskores::Bounds bounds = viskores::cont::BoundsCompute(partitionedData);

  // Get the overall min/max of a field named "cellvar"
  viskores::cont::ArrayHandle<viskores::Range> cellvarRanges =
    viskores::cont::FieldRangeCompute(partitionedData, "cellvar");

  // Assuming the "cellvar" field has scalar values, then cellvarRanges has one entry
  viskores::Range cellvarRange = cellvarRanges.ReadPortal().Get(0);
  ////
  //// END-EXAMPLE QueryPartitionedDataSet
  ////

  std::cout << bounds << std::endl;
  VISKORES_TEST_ASSERT(
    test_equal(bounds, viskores::Bounds(0.0, 3.0, 0.0, 4.0, 0.0, 1.0)), "Bad bounds");

  std::cout << cellvarRange << std::endl;
  VISKORES_TEST_ASSERT(test_equal(cellvarRange, viskores::Range(0, 130.5)), "Bad range");
}

void FilterPartitionedDataSet()
{
  std::cout << "Filter on a partitioned data." << std::endl;

  viskores::cont::testing::MakeTestDataSet makeData;

  viskores::cont::PartitionedDataSet partitionedData;
  partitionedData.AppendPartitions(
    { makeData.Make3DUniformDataSet0(), makeData.Make3DUniformDataSet1() });

  ////
  //// BEGIN-EXAMPLE FilterPartitionedDataSet
  ////
  viskores::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetActiveField("pointvar", viskores::cont::Field::Association::Points);

  viskores::cont::PartitionedDataSet results = cellAverage.Execute(partitionedData);
  ////
  //// END-EXAMPLE FilterPartitionedDataSet
  ////

  VISKORES_TEST_ASSERT(results.GetNumberOfPartitions() == 2,
                       "Incorrect number of blocks.");
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
  return viskores::cont::testing::Testing::Run(
    DataSetCreationNamespace::Test, argc, argv);
}
