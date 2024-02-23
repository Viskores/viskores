//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/geometry_refinement/Triangulate.h>
#include <vtkm/filter/multi_block/MergeDataSets.h>
#include <vtkm/worklet/WorkletMapField.h>
namespace
{
struct SetPointValuesV4Worklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);
  template <typename CoordinatesType, typename V4Type>
  VTKM_EXEC void operator()(const CoordinatesType& coordinates, V4Type& vec4) const
  {
    vec4 = {
      coordinates[0] * 0.1, coordinates[1] * 0.1, coordinates[2] * 0.1, coordinates[0] * 0.1
    };
    return;
  }
};
struct SetPointValuesV1Worklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);
  template <typename CoordinatesType, typename ScalarType>
  VTKM_EXEC void operator()(const CoordinatesType& coordinates, ScalarType& value) const
  {
    value = (coordinates[0] + coordinates[1] + coordinates[2]) * 0.1;
    return;
  }
};
vtkm::cont::DataSet CreateSingleCellSetData(vtkm::Vec3f coordinates[4])
{
  const int connectivitySize = 6;
  vtkm::Id pointId[connectivitySize] = { 0, 1, 2, 1, 2, 3 };
  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  connectivity.Allocate(connectivitySize);
  for (vtkm::Id i = 0; i < connectivitySize; ++i)
  {
    connectivity.WritePortal().Set(i, pointId[i]);
  }
  vtkm::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(4, vtkm::CELL_SHAPE_TRIANGLE, 3, connectivity);
  vtkm::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coords", coordinates, 4, vtkm::CopyFlag::On));
  dataSet.SetCellSet(cellSet);

  std::vector<vtkm::Float32> pointvar(4);
  std::iota(pointvar.begin(), pointvar.end(), 15.f);
  std::vector<vtkm::Float32> cellvar(connectivitySize / 3);
  std::iota(cellvar.begin(), cellvar.end(), 132.f);
  dataSet.AddPointField("pointVar", pointvar);
  dataSet.AddCellField("cellVar", cellvar);
  return dataSet;
}

vtkm::cont::DataSet CreateUniformData(vtkm::Vec2f origin)
{
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dimensions(3, 2);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions, origin, vtkm::Vec2f(1, 1));
  constexpr vtkm::Id nVerts = 6;
  constexpr vtkm::Float32 var[nVerts] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  dataSet.AddPointField("pointVar", var, nVerts);
  constexpr vtkm::Float32 cellvar[2] = { 100.1f, 200.1f };
  dataSet.AddCellField("cellVar", cellvar, 2);
  return dataSet;
}

void TestUniformSameFieldsSameDataTypeSingleCellSet()
{
  std::cout << "TestUniformSameFieldsSameDataTypeSingleCellSet" << std::endl;
  const int nVerts = 4;
  vtkm::Vec3f coordinates1[nVerts] = { vtkm::Vec3f(0.0, 0.0, 0.0),
                                       vtkm::Vec3f(1.0, 0.0, 0.0),
                                       vtkm::Vec3f(0.0, 1.0, 0.0),
                                       vtkm::Vec3f(1.0, 1.0, 0.0) };
  vtkm::cont::DataSet dataSet1 = CreateSingleCellSetData(coordinates1);
  vtkm::Vec3f coordinates2[nVerts] = { vtkm::Vec3f(1.0, 0.0, 0.0),
                                       vtkm::Vec3f(2.0, 0.0, 0.0),
                                       vtkm::Vec3f(1.0, 1.0, 0.0),
                                       vtkm::Vec3f(2.0, 1.0, 0.0) };
  vtkm::cont::DataSet dataSet2 = CreateSingleCellSetData(coordinates2);
  vtkm::cont::PartitionedDataSet inputDataSets;
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  vtkm::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  //Validating result cell sets
  auto cellSet = result.GetPartition(0).GetCellSet();
  vtkm::cont::CellSetSingleType<> singleType = cellSet.AsCellSet<vtkm::cont::CellSetSingleType<>>();
  VTKM_TEST_ASSERT(singleType.GetCellShapeAsId() == 5, "Wrong cellShape Id");
  VTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == 4, "Wrong numberOfCells");
  VTKM_TEST_ASSERT(cellSet.GetNumberOfPoints() == 8, "Wrong numberOfPoints");
  const vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray = singleType.GetConnectivityArray(
    vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  vtkm::cont::ArrayHandle<vtkm::Id> validateConnArray =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 2, 1, 2, 3, 4, 5, 6, 5, 6, 7 });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(connectivityArray, validateConnArray));
  //Validating result fields
  vtkm::cont::ArrayHandle<vtkm::Float32> validatePointVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>({ 15.0, 16.0, 17.0, 18.0, 15.0, 16.0, 17.0, 18.0 });
  vtkm::cont::ArrayHandle<vtkm::Float32> validateCellVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>({ 132, 133, 132, 133 });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(result.GetPartition(0).GetField("pointVar").GetData(),
                                           validatePointVar),
                   "wrong pointVar values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
  //Validating result coordinates
  vtkm::cont::CoordinateSystem coords = result.GetPartition(0).GetCoordinateSystem();
  vtkm::cont::ArrayHandle<vtkm::Vec3f> resultCoords =
    coords.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>();
  vtkm::cont::ArrayHandle<vtkm::Vec3f> validateCoords =
    vtkm::cont::make_ArrayHandle<vtkm::Vec3f>({ { 0, 0, 0 },
                                                { 1, 0, 0 },
                                                { 0, 1, 0 },
                                                { 1, 1, 0 },
                                                { 1, 0, 0 },
                                                { 2, 0, 0 },
                                                { 1, 1, 0 },
                                                { 2, 1, 0 } });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(resultCoords, validateCoords),
                   "wrong validateCoords values");
}

void TestUniformSameFieldsSameDataType()
{
  std::cout << "TestUniformSameFieldsSameDataType" << std::endl;
  vtkm::cont::PartitionedDataSet inputDataSets;
  vtkm::cont::DataSet dataSet0 = CreateUniformData(vtkm::Vec2f(0.0, 0.0));
  vtkm::cont::DataSet dataSet1 = CreateUniformData(vtkm::Vec2f(3.0, 0.0));
  inputDataSets.AppendPartition(dataSet0);
  inputDataSets.AppendPartition(dataSet1);
  vtkm::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  //validating cellsets
  auto cellSet = result.GetPartition(0).GetCellSet();
  vtkm::cont::CellSetExplicit<> explicitType = cellSet.AsCellSet<vtkm::cont::CellSetExplicit<>>();
  const vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray = explicitType.GetConnectivityArray(
    vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  const vtkm::cont::ArrayHandle<vtkm::UInt8> shapesArray =
    explicitType.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  const vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray =
    explicitType.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  vtkm::cont::ArrayHandle<vtkm::Id> validateConnectivity =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 4, 3, 1, 2, 5, 4, 6, 7, 10, 9, 7, 8, 11, 10 });
  vtkm::cont::ArrayHandle<vtkm::UInt8> validateShapes =
    vtkm::cont::make_ArrayHandle<vtkm::UInt8>({ 9, 9, 9, 9 });
  vtkm::cont::ArrayHandle<vtkm::Id> validateOffsets =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 4, 8, 12, 16 });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(connectivityArray, validateConnectivity),
                   "wrong connectivity array");
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(shapesArray, validateShapes),
                   "wrong connectivity array");
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(offsetsArray, validateOffsets),
                   "wrong connectivity array");
  // validating fields
  vtkm::cont::ArrayHandle<vtkm::Float32> validatePointVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(
      { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f, 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f });
  vtkm::cont::ArrayHandle<vtkm::Float32> validateCellVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>({ 100.1f, 200.1f, 100.1f, 200.1f });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(result.GetPartition(0).GetField("pointVar").GetData(),
                                           validatePointVar),
                   "wrong pointVar values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
  //validating coordinates
  vtkm::cont::CoordinateSystem coords = result.GetPartition(0).GetCoordinateSystem();
  vtkm::cont::ArrayHandle<vtkm::Vec3f> resultCoords =
    coords.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>();
  vtkm::cont::ArrayHandle<vtkm::Vec3f> validateCoords =
    vtkm::cont::make_ArrayHandle<vtkm::Vec3f>({ { 0, 0, 0 },
                                                { 1, 0, 0 },
                                                { 2, 0, 0 },
                                                { 0, 1, 0 },
                                                { 1, 1, 0 },
                                                { 2, 1, 0 },
                                                { 3, 0, 0 },
                                                { 4, 0, 0 },
                                                { 5, 0, 0 },
                                                { 3, 1, 0 },
                                                { 4, 1, 0 },
                                                { 5, 1, 0 } });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(resultCoords, validateCoords),
                   "wrong validateCoords values");
}
void TestTriangleSameFieldsSameDataType()
{
  std::cout << "TestTriangleSameFieldsSameDataType" << std::endl;
  vtkm::cont::PartitionedDataSet input;
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id3 dimensions(3, 2, 1);
  vtkm::cont::DataSet dataSet0 = dsb.Create(dimensions,
                                            vtkm::make_Vec<vtkm::FloatDefault>(0, 0, 0),
                                            vtkm::make_Vec<vtkm::FloatDefault>(1, 1, 0));
  constexpr vtkm::Id nVerts = 6;
  constexpr vtkm::Float32 var[nVerts] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  dataSet0.AddPointField("pointVar", var, nVerts);
  constexpr vtkm::Float32 cellvar[2] = { 100.1f, 200.1f };
  dataSet0.AddCellField("cellVar", cellvar, 2);
  vtkm::filter::geometry_refinement::Triangulate triangulate;
  auto tranDataSet0 = triangulate.Execute(dataSet0);
  vtkm::cont::DataSet dataSet1 = dsb.Create(dimensions,
                                            vtkm::make_Vec<vtkm::FloatDefault>(3, 0, 0),
                                            vtkm::make_Vec<vtkm::FloatDefault>(1, 1, 0));
  constexpr vtkm::Float32 var1[nVerts] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  dataSet1.AddPointField("pointVar", var1, nVerts);
  constexpr vtkm::Float32 cellvar1[2] = { 100.1f, 200.1f };
  dataSet1.AddCellField("cellVar", cellvar1, 2);
  auto tranDataSet1 = triangulate.Execute(dataSet1);
  input.AppendPartition(tranDataSet0);
  input.AppendPartition(tranDataSet1);
  vtkm::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(input);
  //validating results
  auto cellSet = result.GetPartition(0).GetCellSet();
  vtkm::cont::CellSetSingleType<> singleType = cellSet.AsCellSet<vtkm::cont::CellSetSingleType<>>();
  VTKM_TEST_ASSERT(singleType.GetCellShapeAsId() == 5, "Wrong cellShape Id");
  VTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == 8, "Wrong numberOfCells");
  VTKM_TEST_ASSERT(cellSet.GetNumberOfPoints() == 12, "Wrong numberOfPoints");
  const vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray = singleType.GetConnectivityArray(
    vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  vtkm::cont::ArrayHandle<vtkm::Id> validateConnArray = vtkm::cont::make_ArrayHandle<vtkm::Id>(
    { 0, 1, 4, 0, 4, 3, 1, 2, 5, 1, 5, 4, 6, 7, 10, 6, 10, 9, 7, 8, 11, 7, 11, 10 });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(connectivityArray, validateConnArray));
  //Validating result fields
  vtkm::cont::ArrayHandle<vtkm::Float32> validatePointVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(
      { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f, 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f });
  vtkm::cont::ArrayHandle<vtkm::Float32> validateCellVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(
      { 100.1f, 100.1f, 200.1f, 200.1f, 100.1f, 100.1f, 200.1f, 200.1f });

  VTKM_TEST_ASSERT(test_equal_ArrayHandles(result.GetPartition(0).GetField("pointVar").GetData(),
                                           validatePointVar),
                   "wrong pointVar values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
  //Validating result coordinates
  vtkm::cont::CoordinateSystem coords = result.GetPartition(0).GetCoordinateSystem();
  vtkm::cont::ArrayHandle<vtkm::Vec3f> resultCoords =
    coords.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>();
  vtkm::cont::ArrayHandle<vtkm::Vec3f> validateCoords =
    vtkm::cont::make_ArrayHandle<vtkm::Vec3f>({ { 0, 0, 0 },
                                                { 1, 0, 0 },
                                                { 2, 0, 0 },
                                                { 0, 1, 0 },
                                                { 1, 1, 0 },
                                                { 2, 1, 0 },
                                                { 3, 0, 0 },
                                                { 4, 0, 0 },
                                                { 5, 0, 0 },
                                                { 3, 1, 0 },
                                                { 4, 1, 0 },
                                                { 5, 1, 0 } });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(resultCoords, validateCoords),
                   "wrong validateCoords values");
}

void TestDiffCellsSameFieldsSameDataType()
{
  std::cout << "TestDiffCellsSameFieldsSameDataType" << std::endl;
  vtkm::Vec3f coordinates1[4] = { vtkm::Vec3f(0.0, 0.0, 0.0),
                                  vtkm::Vec3f(1.0, 0.0, 0.0),
                                  vtkm::Vec3f(0.0, 1.0, 0.0),
                                  vtkm::Vec3f(1.0, 1.0, 0.0) };
  vtkm::cont::DataSet dataSet0 = CreateSingleCellSetData(coordinates1);
  vtkm::cont::DataSet dataSet1 = CreateUniformData(vtkm::Vec2f(3.0, 0.0));
  vtkm::cont::PartitionedDataSet input;
  input.AppendPartition(dataSet0);
  input.AppendPartition(dataSet1);
  vtkm::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(input);
  //validating cellsets
  auto cellSet = result.GetPartition(0).GetCellSet();
  vtkm::cont::CellSetExplicit<> explicitType = cellSet.AsCellSet<vtkm::cont::CellSetExplicit<>>();
  const vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray = explicitType.GetConnectivityArray(
    vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  const vtkm::cont::ArrayHandle<vtkm::UInt8> shapesArray =
    explicitType.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  const vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray =
    explicitType.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
  vtkm::cont::ArrayHandle<vtkm::Id> validateConnectivity =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 1, 2, 1, 2, 3, 4, 5, 8, 7, 5, 6, 9, 8 });
  vtkm::cont::ArrayHandle<vtkm::UInt8> validateShapes =
    vtkm::cont::make_ArrayHandle<vtkm::UInt8>({ 5, 5, 9, 9 });
  vtkm::cont::ArrayHandle<vtkm::Id> validateOffsets =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 3, 6, 10, 14 });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(connectivityArray, validateConnectivity),
                   "wrong connectivity array");
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(shapesArray, validateShapes),
                   "wrong connectivity array");
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(offsetsArray, validateOffsets),
                   "wrong connectivity array");
  // Validating fields
  vtkm::cont::ArrayHandle<vtkm::Float32> validatePointVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(
      { 15.f, 16.f, 17.f, 18.f, 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f });
  vtkm::cont::ArrayHandle<vtkm::Float32> validateCellVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>({ 132.0f, 133.0f, 100.1f, 200.1f });

  VTKM_TEST_ASSERT(test_equal_ArrayHandles(result.GetPartition(0).GetField("pointVar").GetData(),
                                           validatePointVar),
                   "wrong pointVar values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
  //Validating coordinates
  vtkm::cont::CoordinateSystem coords = result.GetPartition(0).GetCoordinateSystem();
  vtkm::cont::ArrayHandle<vtkm::Vec3f> resultCoords =
    coords.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>();
  vtkm::cont::ArrayHandle<vtkm::Vec3f> validateCoords =
    vtkm::cont::make_ArrayHandle<vtkm::Vec3f>({ { 0, 0, 0 },
                                                { 1, 0, 0 },
                                                { 0, 1, 0 },
                                                { 1, 1, 0 },
                                                { 3, 0, 0 },
                                                { 4, 0, 0 },
                                                { 5, 0, 0 },
                                                { 3, 1, 0 },
                                                { 4, 1, 0 },
                                                { 5, 1, 0 } });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(resultCoords, validateCoords), "Wrong coords values");
}

void TestDifferentCoords()
{
  std::cout << "TestDifferentCoords" << std::endl;
  vtkm::cont::PartitionedDataSet inputDataSets;
  vtkm::cont::DataSet dataSet0 = CreateUniformData(vtkm::Vec2f(0.0, 0.0));
  vtkm::Vec3f coordinates[6];
  dataSet0.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordsExtra", coordinates, 6, vtkm::CopyFlag::On));
  vtkm::cont::DataSet dataSet1 = CreateUniformData(vtkm::Vec2f(3.0, 0.0));
  inputDataSets.AppendPartition(dataSet0);
  inputDataSets.AppendPartition(dataSet1);
  vtkm::filter::multi_block::MergeDataSets mergeDataSets;
  try
  {
    mergeDataSets.Execute(inputDataSets);
  }
  catch (vtkm::cont::ErrorExecution& e)
  {
    VTKM_TEST_ASSERT(e.GetMessage().find("Data sets have different number of coordinate systems") !=
                     std::string::npos);
  }
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dimensions(3, 2);
  vtkm::cont::DataSet dataSet2 = dsb.Create(dimensions, vtkm::Vec2f(0.0, 0.0), vtkm::Vec2f(1, 1));
  constexpr vtkm::Float32 var2[6] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  dataSet2.AddPointField("pointVarExtra", var2, 6);
  constexpr vtkm::Float32 cellvar2[2] = { 100.1f, 200.1f };
  dataSet2.AddCellField("cellVarExtra", cellvar2, 2);
  vtkm::cont::PartitionedDataSet inputDataSets2;
  inputDataSets2.AppendPartition(dataSet1);
  inputDataSets2.AppendPartition(dataSet2);
  try
  {
    mergeDataSets.Execute(inputDataSets2);
  }
  catch (vtkm::cont::ErrorExecution& e)
  {
    VTKM_TEST_ASSERT(e.GetMessage().find("Coordinates system name:") != std::string::npos);
  }
}

void TestSameFieldsDifferentDataType()
{
  std::cout << "TestSameFieldsDifferentDataType" << std::endl;
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dimensions(3, 2);
  vtkm::cont::DataSet dataSet1 = dsb.Create(dimensions, vtkm::Vec2f(0.0, 0.0), vtkm::Vec2f(1, 1));
  constexpr vtkm::Float32 var[6] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  dataSet1.AddPointField("pointVar", var, 6);
  constexpr vtkm::Float32 cellvar[2] = { 100.1f, 200.1f };
  dataSet1.AddCellField("cellVar", cellvar, 2);
  vtkm::cont::DataSet dataSet2 = dsb.Create(dimensions, vtkm::Vec2f(0.0, 0.0), vtkm::Vec2f(1, 1));
  constexpr vtkm::Id var2[6] = { 10, 20, 30, 40, 50, 60 };
  dataSet2.AddPointField("pointVar", var2, 6);
  constexpr vtkm::Id cellvar2[2] = { 100, 200 };
  dataSet2.AddCellField("cellVar", cellvar2, 2);
  vtkm::cont::PartitionedDataSet inputDataSets;
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  vtkm::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  //Validating fields in results, they will use the first partition's field type
  vtkm::cont::ArrayHandle<vtkm::Float32> validatePointVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(
      { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f });
  vtkm::cont::ArrayHandle<vtkm::Float32> validateCellVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>({ 100.1f, 200.1f, 100.0f, 200.0f });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(result.GetPartition(0).GetField("pointVar").GetData(),
                                           validatePointVar),
                   "wrong pointVar values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
}
void TestMissingFieldsAndSameFieldName()
{
  std::cout << "TestMissingFieldsAndSameFieldName" << std::endl;
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dimensions(3, 2);
  vtkm::cont::DataSet dataSet1 = dsb.Create(dimensions, vtkm::Vec2f(0.0, 0.0), vtkm::Vec2f(1, 1));
  constexpr vtkm::Float32 pointVar[6] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  vtkm::cont::DataSet dataSet2 = dsb.Create(dimensions, vtkm::Vec2f(0.0, 0.0), vtkm::Vec2f(1, 1));
  constexpr vtkm::Id cellvar[2] = { 100, 200 };
  vtkm::cont::PartitionedDataSet inputDataSets;
  dataSet1.AddPointField("pointVar", pointVar, 6);
  dataSet2.AddCellField("cellVar", cellvar, 2);
  //For testing the case where one field is associated with point in one partition
  //and one field (with a same name) is associated with cell in another partition
  dataSet1.AddPointField("fieldSameName", pointVar, 6);
  dataSet2.AddCellField("fieldSameName", cellvar, 2);
  //For testing the case where one partition have point field and a cell field with the same name.
  dataSet1.AddPointField("fieldSameName2", pointVar, 6);
  dataSet2.AddPointField("fieldSameName2", pointVar, 6);
  dataSet2.AddCellField("fieldSameName2", cellvar, 2);
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  vtkm::filter::multi_block::MergeDataSets mergeDataSets;
  mergeDataSets.SetInvalidValue(vtkm::Float64(0));
  auto result = mergeDataSets.Execute(inputDataSets);
  //Validating fields in results, they will use InvalidValues for missing fields
  vtkm::cont::ArrayHandle<vtkm::Float32> validatePointVar1 =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(
      { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
  vtkm::cont::ArrayHandle<vtkm::Float32> validatePointVar2 =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>(
      { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f, 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f });
  vtkm::cont::ArrayHandle<vtkm::Id> validateCellVar =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 0, 0, 100, 200 });
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(
      result.GetPartition(0).GetField("pointVar", vtkm::cont::Field::Association::Points).GetData(),
      validatePointVar1),
    "wrong pointVar values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(
      result.GetPartition(0).GetField("cellVar", vtkm::cont::Field::Association::Cells).GetData(),
      validateCellVar),
    "wrong cellVar values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0)
                              .GetField("fieldSameName", vtkm::cont::Field::Association::Points)
                              .GetData(),
                            validatePointVar1),
    "wrong fieldSameName values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0)
                              .GetField("fieldSameName", vtkm::cont::Field::Association::Cells)
                              .GetData(),
                            validateCellVar),
    "wrong fieldSameName values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0)
                              .GetField("fieldSameName2", vtkm::cont::Field::Association::Points)
                              .GetData(),
                            validatePointVar2),
    "wrong fieldSameName2 values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0)
                              .GetField("fieldSameName2", vtkm::cont::Field::Association::Cells)
                              .GetData(),
                            validateCellVar),
    "wrong fieldSameName2 values");
}

void TestCustomizedVecField()
{
  std::cout << "TestCustomizedVecField" << std::endl;
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dimensions(3, 2);
  vtkm::cont::DataSet dataSet1 = dsb.Create(dimensions, vtkm::Vec2f(0.0, 0.0), vtkm::Vec2f(1, 1));
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 4>> pointVar1Vec4;
  pointVar1Vec4.Allocate(6);
  vtkm::cont::Invoker invoker;
  invoker(SetPointValuesV4Worklet{}, dataSet1.GetCoordinateSystem().GetData(), pointVar1Vec4);
  dataSet1.AddPointField("pointVarV4", pointVar1Vec4);
  vtkm::cont::DataSet dataSet2 = dsb.Create(dimensions, vtkm::Vec2f(3.0, 0.0), vtkm::Vec2f(1, 1));
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 4>> pointVar2Vec4;
  pointVar2Vec4.Allocate(6);
  invoker(SetPointValuesV4Worklet{}, dataSet2.GetCoordinateSystem().GetData(), pointVar2Vec4);
  dataSet2.AddPointField("pointVarV4", pointVar2Vec4);
  vtkm::cont::PartitionedDataSet inputDataSets;
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  vtkm::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 4>> validatePointVar;
  //Set point validatePointVar array based on coordinates.
  invoker(SetPointValuesV4Worklet{},
          result.GetPartition(0).GetCoordinateSystem().GetData(),
          validatePointVar);
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(result.GetPartition(0).GetField("pointVarV4").GetData(),
                                           validatePointVar),
                   "wrong pointVar values");
}

void TestMoreThanTwoPartitions()
{
  std::cout << "TestMoreThanTwoPartitions" << std::endl;
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dimensions(3, 2);
  vtkm::cont::Invoker invoker;
  vtkm::cont::PartitionedDataSet inputDataSets;
  for (vtkm::Id i = 0; i < 5; i++)
  {
    for (vtkm::Id j = 0; j < 5; j++)
    {
      vtkm::cont::DataSet dataSet = dsb.Create(
        dimensions,
        vtkm::Vec2f(static_cast<vtkm::FloatDefault>(i), static_cast<vtkm::FloatDefault>(j)),
        vtkm::Vec2f(1, 1));
      vtkm::cont::ArrayHandle<vtkm::Float64> pointVarArray;
      invoker(SetPointValuesV1Worklet{}, dataSet.GetCoordinateSystem().GetData(), pointVarArray);
      dataSet.AddPointField("pointVar", pointVarArray);
      inputDataSets.AppendPartition(dataSet);
    }
  }
  vtkm::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  vtkm::cont::ArrayHandle<vtkm::Float64> validatePointVar;
  invoker(SetPointValuesV1Worklet{},
          result.GetPartition(0).GetCoordinateSystem().GetData(),
          validatePointVar);
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(result.GetPartition(0).GetField("pointVar").GetData(),
                                           validatePointVar),
                   "wrong pointVar values");
}

void TestEmptyPartitions()
{
  std::cout << "TestEmptyPartitions" << std::endl;
  vtkm::cont::PartitionedDataSet inputDataSets;
  vtkm::cont::DataSet dataSet1 = CreateUniformData(vtkm::Vec2f(0.0, 0.0));
  vtkm::cont::DataSet dataSet2;
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  vtkm::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);
  //Validating data sets
  VTKM_TEST_ASSERT(result.GetNumberOfPartitions() == 1, "Wrong number of partitions");
  auto cellSet = result.GetPartition(0).GetCellSet();
  VTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == 2, "Wrong numberOfCells");
  VTKM_TEST_ASSERT(cellSet.GetNumberOfPoints() == 6, "Wrong numberOfPoints");
  vtkm::cont::ArrayHandle<vtkm::Float32> validatePointVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>({ 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f });
  vtkm::cont::ArrayHandle<vtkm::Float32> validateCellVar =
    vtkm::cont::make_ArrayHandle<vtkm::Float32>({ 100.1f, 200.1f });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(result.GetPartition(0).GetField("pointVar").GetData(),
                                           validatePointVar),
                   "wrong pointVar values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
  vtkm::cont::PartitionedDataSet inputDataSets2;
  inputDataSets2.AppendPartition(dataSet2);
  inputDataSets2.AppendPartition(dataSet1);
  auto result2 = mergeDataSets.Execute(inputDataSets2);
  VTKM_TEST_ASSERT(result2.GetNumberOfPartitions() == 1, "Wrong number of partitions");
  cellSet = result2.GetPartition(0).GetCellSet();
  VTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == 2, "Wrong numberOfCells");
  VTKM_TEST_ASSERT(cellSet.GetNumberOfPoints() == 6, "Wrong numberOfPoints");
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(result2.GetPartition(0).GetField("pointVar").GetData(),
                                           validatePointVar),
                   "wrong pointVar values");
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(result2.GetPartition(0).GetField("cellVar").GetData(), validateCellVar),
    "wrong cellVar values");
}

void TestMissingVectorFields()
{
  std::cout << "TestMissingVectorFields" << std::endl;
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dimensions(3, 2);
  vtkm::cont::DataSet dataSet1 = dsb.Create(dimensions, vtkm::Vec2f(0.0, 0.0), vtkm::Vec2f(1, 1));
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 4>> pointVarVec4;
  pointVarVec4.Allocate(6);
  vtkm::cont::Invoker invoker;
  invoker(SetPointValuesV4Worklet{}, dataSet1.GetCoordinateSystem().GetData(), pointVarVec4);
  dataSet1.AddPointField("pointVarV4", pointVarVec4);
  vtkm::cont::DataSet dataSet2 = dsb.Create(dimensions, vtkm::Vec2f(0.0, 0.0), vtkm::Vec2f(1, 1));
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> cellVarVec3 =
    vtkm::cont::make_ArrayHandle<vtkm::Vec3f_64>({ { 1.0, 2.0, 3.0 }, { 4.0, 5.0, 6.0 } });
  dataSet2.AddCellField("cellVarV3", cellVarVec3);
  vtkm::cont::PartitionedDataSet inputDataSets;
  inputDataSets.AppendPartition(dataSet1);
  inputDataSets.AppendPartition(dataSet2);
  vtkm::filter::multi_block::MergeDataSets mergeDataSets;
  auto result = mergeDataSets.Execute(inputDataSets);

  //checking results
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 4>> validatePointVar =
    vtkm::cont::make_ArrayHandle<vtkm::Vec<vtkm::Float64, 4>>(
      { { 0, 0, 0, 0 },
        { 0.1, 0, 0, 0.1 },
        { 0.2, 0, 0, 0.2 },
        { 0, 0.1, 0, 0 },
        { 0.1, 0.1, 0, 0.1 },
        { 0.2, 0.1, 0, 0.2 },
        { vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64() },
        { vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64() },
        { vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64() },
        { vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64() },
        { vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64() },
        { vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64() } });
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> validateCellVar =
    vtkm::cont::make_ArrayHandle<vtkm::Vec3f_64>({ { vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64() },
                                                   { vtkm::Nan64(), vtkm::Nan64(), vtkm::Nan64() },
                                                   { 1.0, 2.0, 3.0 },
                                                   { 4.0, 5.0, 6.0 } });
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(result.GetPartition(0).GetField("pointVarV4").GetData(),
                                           validatePointVar),
                   "wrong point values for TestMissingVectorFields");
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(result.GetPartition(0).GetField("cellVarV3").GetData(),
                                           validateCellVar),
                   "wrong cell values for TestMissingVectorFields");
}

void TestMergeDataSetsFilter()
{
  //same cell type (triangle), same field name, same data type, cellset is single type
  TestUniformSameFieldsSameDataTypeSingleCellSet();
  //same cell type (square), same field name, same data type
  TestUniformSameFieldsSameDataType();
  //same cell type (triangle), same field name, same data type
  TestTriangleSameFieldsSameDataType();
  //same cell type (square), same field name, different data type
  TestSameFieldsDifferentDataType();
  //different coordinates name
  TestDifferentCoords();
  //different cell types, same field name, same type
  TestDiffCellsSameFieldsSameDataType();
  //test multiple partitions
  TestMoreThanTwoPartitions();
  //some partitions have missing scalar fields
  TestMissingFieldsAndSameFieldName();
  //test empty partitions
  TestEmptyPartitions();
  //test customized types
  TestCustomizedVecField();
  //some partitions have missing vector fields
  TestMissingVectorFields();
}
} // anonymous namespace
int UnitTestMergeDataSetsFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMergeDataSetsFilter, argc, argv);
}
