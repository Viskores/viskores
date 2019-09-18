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
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

struct TestWholeCellSetIn
{
  template <typename VisitTopology, typename IncidentTopology>
  struct WholeCellSetWorklet : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn indices,
                                  WholeCellSetIn<VisitTopology, IncidentTopology>,
                                  FieldOut numberOfElements,
                                  FieldOut shapes,
                                  FieldOut numberOfindices,
                                  FieldOut connectionSum);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
    using InputDomain = _1;

    template <typename ConnectivityType>
    VTKM_EXEC void operator()(vtkm::Id index,
                              const ConnectivityType& connectivity,
                              vtkm::Id& numberOfElements,
                              vtkm::UInt8& shape,
                              vtkm::IdComponent& numberOfIndices,
                              vtkm::Id& connectionSum) const
    {
      numberOfElements = connectivity.GetNumberOfElements();
      shape = connectivity.GetCellShape(index).Id;
      numberOfIndices = connectivity.GetNumberOfIndices(index);

      typename ConnectivityType::IndicesType indices = connectivity.GetIndices(index);
      if (numberOfIndices != indices.GetNumberOfComponents())
      {
        this->RaiseError("Got wrong number of connections.");
      }

      connectionSum = 0;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < indices.GetNumberOfComponents();
           componentIndex++)
      {
        connectionSum += indices[componentIndex];
      }
    }
  };

  template <typename CellSetType>
  VTKM_CONT static void RunCells(const CellSetType& cellSet,
                                 vtkm::cont::ArrayHandle<vtkm::Id> numberOfElements,
                                 vtkm::cont::ArrayHandle<vtkm::UInt8> shapeIds,
                                 vtkm::cont::ArrayHandle<vtkm::IdComponent> numberOfIndices,
                                 vtkm::cont::ArrayHandle<vtkm::Id> connectionSum)
  {
    using WorkletType =
      WholeCellSetWorklet<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>;
    vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(vtkm::cont::ArrayHandleIndex(cellSet.GetNumberOfCells()),
                      cellSet,
                      numberOfElements,
                      shapeIds,
                      numberOfIndices,
                      &connectionSum);
  }

  template <typename CellSetType>
  VTKM_CONT static void RunPoints(const CellSetType* cellSet,
                                  vtkm::cont::ArrayHandle<vtkm::Id> numberOfElements,
                                  vtkm::cont::ArrayHandle<vtkm::UInt8> shapeIds,
                                  vtkm::cont::ArrayHandle<vtkm::IdComponent> numberOfIndices,
                                  vtkm::cont::ArrayHandle<vtkm::Id> connectionSum)
  {
    using WorkletType =
      WholeCellSetWorklet<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>;
    vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(vtkm::cont::ArrayHandleIndex(cellSet->GetNumberOfPoints()),
                      cellSet,
                      numberOfElements,
                      &shapeIds,
                      numberOfIndices,
                      connectionSum);
  }
};

template <typename CellSetType,
          typename ShapeArrayType,
          typename NumIndicesArrayType,
          typename ConnectionSumArrayType>
VTKM_CONT void TryCellConnectivity(const CellSetType& cellSet,
                                   const ShapeArrayType& expectedShapeIds,
                                   const NumIndicesArrayType& expectedNumberOfIndices,
                                   const ConnectionSumArrayType& expectedSum)
{
  std::cout << "  trying point to cell connectivity" << std::endl;
  vtkm::cont::ArrayHandle<vtkm::Id> numberOfElements;
  vtkm::cont::ArrayHandle<vtkm::UInt8> shapeIds;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numberOfIndices;
  vtkm::cont::ArrayHandle<vtkm::Id> connectionSum;

  TestWholeCellSetIn::RunCells(cellSet, numberOfElements, shapeIds, numberOfIndices, connectionSum);

  std::cout << "    Number of elements: " << numberOfElements.GetPortalConstControl().Get(0)
            << std::endl;
  VTKM_TEST_ASSERT(test_equal_portals(numberOfElements.GetPortalConstControl(),
                                      vtkm::cont::make_ArrayHandleConstant(
                                        cellSet.GetNumberOfCells(), cellSet.GetNumberOfCells())
                                        .GetPortalConstControl()),
                   "Incorrect number of elements.");

  std::cout << "    Shape Ids: ";
  vtkm::cont::printSummary_ArrayHandle(shapeIds, std::cout, true);
  VTKM_TEST_ASSERT(
    test_equal_portals(shapeIds.GetPortalConstControl(), expectedShapeIds.GetPortalConstControl()),
    "Incorrect shape Ids.");

  std::cout << "    Number of indices: ";
  vtkm::cont::printSummary_ArrayHandle(numberOfIndices, std::cout, true);
  VTKM_TEST_ASSERT(test_equal_portals(numberOfIndices.GetPortalConstControl(),
                                      expectedNumberOfIndices.GetPortalConstControl()),
                   "Incorrect number of indices.");

  std::cout << "    Sum of indices: ";
  vtkm::cont::printSummary_ArrayHandle(connectionSum, std::cout, true);
  VTKM_TEST_ASSERT(
    test_equal_portals(connectionSum.GetPortalConstControl(), expectedSum.GetPortalConstControl()),
    "Incorrect sum of indices.");
}

template <typename CellSetType,
          typename ShapeArrayType,
          typename NumIndicesArrayType,
          typename ConnectionSumArrayType>
VTKM_CONT void TryPointConnectivity(const CellSetType& cellSet,
                                    const ShapeArrayType& expectedShapeIds,
                                    const NumIndicesArrayType& expectedNumberOfIndices,
                                    const ConnectionSumArrayType& expectedSum)
{
  std::cout << "  trying cell to point connectivity" << std::endl;
  vtkm::cont::ArrayHandle<vtkm::Id> numberOfElements;
  vtkm::cont::ArrayHandle<vtkm::UInt8> shapeIds;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numberOfIndices;
  vtkm::cont::ArrayHandle<vtkm::Id> connectionSum;

  TestWholeCellSetIn::RunPoints(
    &cellSet, numberOfElements, shapeIds, numberOfIndices, connectionSum);

  std::cout << "    Number of elements: " << numberOfElements.GetPortalConstControl().Get(0)
            << std::endl;
  VTKM_TEST_ASSERT(test_equal_portals(numberOfElements.GetPortalConstControl(),
                                      vtkm::cont::make_ArrayHandleConstant(
                                        cellSet.GetNumberOfPoints(), cellSet.GetNumberOfPoints())
                                        .GetPortalConstControl()),
                   "Incorrect number of elements.");

  std::cout << "    Shape Ids: ";
  vtkm::cont::printSummary_ArrayHandle(shapeIds, std::cout, true);
  VTKM_TEST_ASSERT(
    test_equal_portals(shapeIds.GetPortalConstControl(), expectedShapeIds.GetPortalConstControl()),
    "Incorrect shape Ids.");

  std::cout << "    Number of indices: ";
  vtkm::cont::printSummary_ArrayHandle(numberOfIndices, std::cout, true);
  VTKM_TEST_ASSERT(test_equal_portals(numberOfIndices.GetPortalConstControl(),
                                      expectedNumberOfIndices.GetPortalConstControl()),
                   "Incorrect number of indices.");

  std::cout << "    Sum of indices: ";
  vtkm::cont::printSummary_ArrayHandle(connectionSum, std::cout, true);
  VTKM_TEST_ASSERT(
    test_equal_portals(connectionSum.GetPortalConstControl(), expectedSum.GetPortalConstControl()),
    "Incorrect sum of indices.");
}

VTKM_CONT
void TryExplicitGrid()
{
  std::cout << "Testing explicit grid." << std::endl;
  vtkm::cont::DataSet dataSet = vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();
  vtkm::cont::CellSetExplicit<> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  vtkm::UInt8 expectedCellShapes[] = { vtkm::CELL_SHAPE_HEXAHEDRON,
                                       vtkm::CELL_SHAPE_PYRAMID,
                                       vtkm::CELL_SHAPE_TETRA,
                                       vtkm::CELL_SHAPE_WEDGE };

  vtkm::IdComponent expectedCellNumIndices[] = { 8, 5, 4, 6 };

  vtkm::Id expectedCellIndexSum[] = { 28, 22, 29, 41 };

  vtkm::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(cellSet,
                      vtkm::cont::make_ArrayHandle(expectedCellShapes, numCells),
                      vtkm::cont::make_ArrayHandle(expectedCellNumIndices, numCells),
                      vtkm::cont::make_ArrayHandle(expectedCellIndexSum, numCells));

  vtkm::IdComponent expectedPointNumIndices[] = { 1, 2, 2, 1, 2, 4, 4, 2, 2, 1, 2 };

  vtkm::Id expectedPointIndexSum[] = { 0, 1, 1, 0, 3, 6, 6, 3, 3, 3, 5 };

  vtkm::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    vtkm::cont::make_ArrayHandleConstant(vtkm::CellShapeTagVertex::Id, numPoints),
    vtkm::cont::make_ArrayHandle(expectedPointNumIndices, numPoints),
    vtkm::cont::make_ArrayHandle(expectedPointIndexSum, numPoints));
}

VTKM_CONT
void TryCellSetPermutation()
{
  std::cout << "Testing permutation grid." << std::endl;
  vtkm::cont::DataSet dataSet = vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();
  vtkm::cont::CellSetExplicit<> originalCellSet;
  dataSet.GetCellSet().CopyTo(originalCellSet);

  vtkm::Id permutationArray[] = { 2, 0, 1 };

  vtkm::cont::CellSetPermutation<vtkm::cont::CellSetExplicit<>, vtkm::cont::ArrayHandle<vtkm::Id>>
    cellSet(vtkm::cont::make_ArrayHandle(permutationArray, 3), originalCellSet);

  vtkm::UInt8 expectedCellShapes[] = { vtkm::CELL_SHAPE_TETRA,
                                       vtkm::CELL_SHAPE_HEXAHEDRON,
                                       vtkm::CELL_SHAPE_PYRAMID };

  vtkm::IdComponent expectedCellNumIndices[] = { 4, 8, 5 };

  vtkm::Id expectedCellIndexSum[] = { 29, 28, 22 };

  vtkm::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(cellSet,
                      vtkm::cont::make_ArrayHandle(expectedCellShapes, numCells),
                      vtkm::cont::make_ArrayHandle(expectedCellNumIndices, numCells),
                      vtkm::cont::make_ArrayHandle(expectedCellIndexSum, numCells));

  // Permutation cell set does not support cell to point connectivity.
}

VTKM_CONT
void TryStructuredGrid3D()
{
  std::cout << "Testing 3D structured grid." << std::endl;
  vtkm::cont::DataSet dataSet = vtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet0();
  vtkm::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  vtkm::Id expectedCellIndexSum[4] = { 40, 48, 88, 96 };

  vtkm::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(
    cellSet,
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(vtkm::CELL_SHAPE_HEXAHEDRON, numCells),
    vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(8, numCells),
    vtkm::cont::make_ArrayHandle(expectedCellIndexSum, numCells));

  vtkm::IdComponent expectedPointNumIndices[18] = { 1, 2, 1, 1, 2, 1, 2, 4, 2,
                                                    2, 4, 2, 1, 2, 1, 1, 2, 1 };

  vtkm::Id expectedPointIndexSum[18] = { 0, 1, 1, 0, 1, 1, 2, 6, 4, 2, 6, 4, 2, 5, 3, 2, 5, 3 };

  vtkm::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(vtkm::CELL_SHAPE_VERTEX, numPoints),
    vtkm::cont::make_ArrayHandle(expectedPointNumIndices, numPoints),
    vtkm::cont::make_ArrayHandle(expectedPointIndexSum, numPoints));
}

VTKM_CONT
void TryStructuredGrid2D()
{
  std::cout << "Testing 2D structured grid." << std::endl;
  vtkm::cont::DataSet dataSet = vtkm::cont::testing::MakeTestDataSet().Make2DUniformDataSet0();
  vtkm::cont::CellSetStructured<2> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  vtkm::Id expectedCellIndexSum[2] = { 8, 12 };

  vtkm::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(cellSet,
                      vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(vtkm::CELL_SHAPE_QUAD, numCells),
                      vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(4, numCells),
                      vtkm::cont::make_ArrayHandle(expectedCellIndexSum, numCells));

  vtkm::IdComponent expectedPointNumIndices[6] = { 1, 2, 1, 1, 2, 1 };

  vtkm::Id expectedPointIndexSum[6] = { 0, 1, 1, 0, 1, 1 };

  vtkm::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(vtkm::CELL_SHAPE_VERTEX, numPoints),
    vtkm::cont::make_ArrayHandle(expectedPointNumIndices, numPoints),
    vtkm::cont::make_ArrayHandle(expectedPointIndexSum, numPoints));
}

VTKM_CONT
void TryStructuredGrid1D()
{
  std::cout << "Testing 1D structured grid." << std::endl;
  vtkm::cont::DataSet dataSet = vtkm::cont::testing::MakeTestDataSet().Make1DUniformDataSet0();
  vtkm::cont::CellSetStructured<1> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  vtkm::Id expectedCellIndexSum[5] = { 1, 3, 5, 7, 9 };

  vtkm::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(cellSet,
                      vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(vtkm::CELL_SHAPE_LINE, numCells),
                      vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(2, numCells),
                      vtkm::cont::make_ArrayHandle(expectedCellIndexSum, numCells));

  vtkm::IdComponent expectedPointNumIndices[6] = { 1, 2, 2, 2, 2, 1 };

  vtkm::Id expectedPointIndexSum[6] = { 0, 1, 3, 5, 7, 4 };

  vtkm::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(vtkm::CELL_SHAPE_VERTEX, numPoints),
    vtkm::cont::make_ArrayHandle(expectedPointNumIndices, numPoints),
    vtkm::cont::make_ArrayHandle(expectedPointIndexSum, numPoints));
}

VTKM_CONT
void RunWholeCellSetInTests()
{
  TryExplicitGrid();
  TryCellSetPermutation();
  TryStructuredGrid3D();
  TryStructuredGrid2D();
  TryStructuredGrid1D();
}

int UnitTestWholeCellSetIn(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RunWholeCellSetInTests, argc, argv);
}
