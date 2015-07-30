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

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/CellType.h>
#include <vtkm/RegularConnectivity.h>

static void TwoDimRegularTest();
static void ThreeDimRegularTest();

void TestDataSet_Regular()
{
    std::cout << std::endl;
    std::cout << "--TestDataSet_Regular--" << std::endl << std::endl;

    TwoDimRegularTest();
    ThreeDimRegularTest();
}

static void
TwoDimRegularTest()
{
    std::cout<<"2D Regular data set"<<std::endl;
    vtkm::cont::testing::MakeTestDataSet testDataSet;

    vtkm::cont::DataSet dataSet = testDataSet.Make2DRegularDataSet0();

    typedef vtkm::cont::CellSetStructured<2> CellSetType;
    CellSetType cellSet = dataSet.GetCellSet(0).CastTo<CellSetType>();

    VTKM_TEST_ASSERT(dataSet.GetNumberOfCellSets() == 1,
                     "Incorrect number of cell sets");
    VTKM_TEST_ASSERT(dataSet.GetNumberOfFields() == 4,
                     "Incorrect number of fields");
    VTKM_TEST_ASSERT(cellSet.Structure.GetNumberOfNodes() == 6,
                     "Incorrect number of nodes");
    VTKM_TEST_ASSERT(cellSet.Structure.GetNumberOfCells() == 2,
                     "Incorrect number of cells");

    vtkm::Id numCells = cellSet.Structure.GetNumberOfCells();
    for (vtkm::Id cellIndex = 0; cellIndex < numCells; cellIndex++)
    {
      VTKM_TEST_ASSERT(cellSet.Structure.GetNumberOfIndices() == 4,
                       "Incorrect number of cell indices");
      vtkm::CellType shape = cellSet.Structure.GetElementShapeType();
      VTKM_TEST_ASSERT(shape == vtkm::VTKM_PIXEL, "Incorrect element type.");
    }

    vtkm::RegularConnectivity<vtkm::cont::NODE, vtkm::cont::CELL,2> nodeToCell =
        cellSet.GetNodeToCellConnectivity();
    vtkm::RegularConnectivity<vtkm::cont::CELL, vtkm::cont::NODE,2> cellToNode =
        cellSet.GetCellToNodeConnectivity();

    vtkm::Id cells[2][4] = {{0,1,3,4}, {1,2,4,5}};
    vtkm::Vec<vtkm::Id,4> nodeIds;
    for (vtkm::Id cellIndex = 0; cellIndex < 2; cellIndex++)
    {
      nodeToCell.GetIndices(cellIndex, nodeIds);
      for (vtkm::IdComponent nodeIndex = 0; nodeIndex < 4; nodeIndex++)
      {
        VTKM_TEST_ASSERT(nodeIds[nodeIndex] == cells[cellIndex][nodeIndex],
                         "Incorrect node ID for cell");
      }
    }

    vtkm::Id expectedCellIds[6][4] = {{0,-1,-1,-1},
                                      {0,1,-1,-1},
                                      {0,-1,-1,-1},
                                      {0,1,-1,-1},
                                      {2,-1,-1,-1},
                                      {2,3,-1,-1}};

    for (vtkm::Id pointIndex = 0; pointIndex < 6; pointIndex++)
    {
      vtkm::Vec<vtkm::Id,4> retrievedCellIds;
      cellToNode.GetIndices(pointIndex, retrievedCellIds);
      for (vtkm::IdComponent cellIndex = 0; cellIndex < 4; cellIndex++)
        VTKM_TEST_ASSERT(
              retrievedCellIds[cellIndex] == expectedCellIds[pointIndex][cellIndex],
              "Incorrect cell ID for node");
    }
}

static void
ThreeDimRegularTest()
{
    std::cout<<"3D Regular data set"<<std::endl;
    vtkm::cont::testing::MakeTestDataSet testDataSet;

    vtkm::cont::DataSet dataSet = testDataSet.Make3DRegularDataSet0();

    typedef vtkm::cont::CellSetStructured<3> CellSetType;
    CellSetType cellSet = dataSet.GetCellSet(0).CastTo<CellSetType>();

    VTKM_TEST_ASSERT(dataSet.GetNumberOfCellSets() == 1,
                     "Incorrect number of cell sets");

    VTKM_TEST_ASSERT(dataSet.GetNumberOfFields() == 5,
                     "Incorrect number of fields");

    VTKM_TEST_ASSERT(cellSet.Structure.GetNumberOfNodes() == 18,
                     "Incorrect number of nodes");

    VTKM_TEST_ASSERT(cellSet.Structure.GetNumberOfCells() == 4,
                     "Incorrect number of cells");

    vtkm::Id numCells = cellSet.Structure.GetNumberOfCells();
    for (vtkm::Id cellIndex = 0; cellIndex < numCells; cellIndex++)
    {
      VTKM_TEST_ASSERT(cellSet.Structure.GetNumberOfIndices() == 8,
                       "Incorrect number of cell indices");
      vtkm::CellType shape = cellSet.Structure.GetElementShapeType();
      VTKM_TEST_ASSERT(shape == vtkm::VTKM_VOXEL, "Incorrect element type.");
    }

    //Test regular connectivity.
    vtkm::RegularConnectivity<vtkm::cont::NODE, vtkm::cont::CELL,3> nodeToCell =
        cellSet.GetNodeToCellConnectivity();
    vtkm::Id expectedPointIds[8] = {0,1,3,4,6,7,9,10};
    vtkm::Vec<vtkm::Id,8> retrievedPointIds;
    nodeToCell.GetIndices(0, retrievedPointIds);
    for (vtkm::IdComponent nodeIndex = 0; nodeIndex < 8; nodeIndex++)
    {
      VTKM_TEST_ASSERT(
            retrievedPointIds[nodeIndex] == expectedPointIds[nodeIndex],
            "Incorrect node ID for cell");
    }

    vtkm::RegularConnectivity<vtkm::cont::CELL, vtkm::cont::NODE,3> cellToNode =
        cellSet.GetCellToNodeConnectivity();
    vtkm::Vec<vtkm::Id,8> expectedCellIds;
    vtkm::Id retrievedCellIds[8] = {0,-1,-1,-1,-1,-1,-1,-1};
    cellToNode.GetIndices(0, expectedCellIds);
    for (vtkm::IdComponent nodeIndex = 0; nodeIndex < 8; nodeIndex++)
    {
      VTKM_TEST_ASSERT(
            expectedCellIds[nodeIndex] == retrievedCellIds[nodeIndex],
            "Incorrect cell ID for node");
    }
}

int UnitTestDataSetRegular(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestDataSet_Regular);
}
