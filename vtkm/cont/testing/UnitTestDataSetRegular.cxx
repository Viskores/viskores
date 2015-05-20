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
//  Copyright 2014. Los Alamos National Security
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
    vtkm::cont::testing::MakeTestDataSet tds;

    vtkm::cont::DataSet *ds = tds.Make2DRegularDataSet0();
    ds->PrintSummary(std::cout);

    vtkm::cont::CellSetStructured<2> *cs;
    cs = dynamic_cast<vtkm::cont::CellSetStructured<2> *>(ds->GetCellSet(0));
    VTKM_TEST_ASSERT(cs, "Invalid Cell Set");

    VTKM_TEST_ASSERT(ds->GetNumberOfCellSets() == 1,
                     "Incorrect number of cell sets");
    VTKM_TEST_ASSERT(ds->GetNumberOfFields() == 4,
                     "Incorrect number of fields");
    VTKM_TEST_ASSERT(cs->structure.GetNumberOfNodes() == 6,
                     "Incorrect number of nodes");
    VTKM_TEST_ASSERT(cs->structure.GetNumberOfCells() == 2,
                     "Incorrect number of cells");
    
    vtkm::Id numCells = cs->structure.GetNumberOfCells();
    for (int i = 0; i < numCells; i++)
    {
        VTKM_TEST_ASSERT(cs->structure.GetNumberOfIndices() == 4,
                         "Incorrect number of cell indices");
        vtkm::CellType shape = cs->structure.GetElementShapeType();
        VTKM_TEST_ASSERT(shape == vtkm::VTKM_PIXEL, "Incorrect element type.");
    }

    vtkm::RegularConnectivity<vtkm::cont::NODE,
                              vtkm::cont::CELL,2> nodeToCell = cs->GetNodeToCellConnectivity();
    vtkm::RegularConnectivity<vtkm::cont::CELL,
                              vtkm::cont::NODE,2> cellToNode = cs->GetCellToNodeConnectivity();
    
    vtkm::Id cells[2][4] = {{0,1,3,4}, {1,2,4,5}};
    vtkm::Vec<vtkm::Id,4> nodeIds;
    for (int i = 0; i < 2; i++)
    {
        nodeToCell.GetIndices(i, nodeIds);
        for (int j = 0; j < 4; j++)
        {
            VTKM_TEST_ASSERT(nodeIds[j] == cells[i][j],
                             "Incorrect node ID for cell");
        }
    }

    vtkm::Id nodes[6][4] = {{0,-1,-1,-1},
                            {0,1,-1,-1},
                            {0,-1,-1,-1},
                            {0,1,-1,-1},
                            {2,-1,-1,-1},
                            {2,3,-1,-1}};
                            
    for (int i = 0; i < 6; i++)
    {
        vtkm::Vec<vtkm::Id,4> cellIds;
        cellToNode.GetIndices(i, cellIds);
        for (int j = 0; j < 4; j++)
            VTKM_TEST_ASSERT(cellIds[j] == nodes[i][j],
                             "Incorrect cell ID for node");
    }
    
    delete ds;
}

static void
ThreeDimRegularTest()
{
    std::cout<<"3D Regular data set"<<std::endl;
    vtkm::cont::testing::MakeTestDataSet tds;

    vtkm::cont::DataSet *ds = tds.Make3DRegularDataSet0();
    ds->PrintSummary(std::cout);

    vtkm::cont::CellSetStructured<3> *cs;
    cs = dynamic_cast<vtkm::cont::CellSetStructured<3> *>(ds->GetCellSet(0));
    VTKM_TEST_ASSERT(cs, "Invalid Cell Set");

    VTKM_TEST_ASSERT(ds->GetNumberOfCellSets() == 1,
                     "Incorrect number of cell sets");

    VTKM_TEST_ASSERT(ds->GetNumberOfFields() == 5,
                     "Incorrect number of fields");

    VTKM_TEST_ASSERT(cs->structure.GetNumberOfNodes() == 18,
                     "Incorrect number of nodes");

    VTKM_TEST_ASSERT(cs->structure.GetNumberOfCells() == 4,
                     "Incorrect number of cells");
    
    vtkm::Id numCells = cs->structure.GetNumberOfCells();
    for (int i = 0; i < numCells; i++)
    {
        VTKM_TEST_ASSERT(cs->structure.GetNumberOfIndices() == 8,
                         "Incorrect number of cell indices");
        vtkm::CellType shape = cs->structure.GetElementShapeType();
        VTKM_TEST_ASSERT(shape == vtkm::VTKM_VOXEL, "Incorrect element type.");
    }

    //Test regular connectivity.
    vtkm::RegularConnectivity<vtkm::cont::NODE,
                              vtkm::cont::CELL,3> nodeToCell = cs->GetNodeToCellConnectivity();
    vtkm::Id nodes[8] = {0,1,3,4,6,7,9,10};
    vtkm::Vec<vtkm::Id,8> nodeIds;
    nodeToCell.GetIndices(0, nodeIds);
    for (int i = 0; i < 8; i++)
        VTKM_TEST_ASSERT(nodeIds[i] == nodes[i],
                             "Incorrect node ID for cell");

    vtkm::RegularConnectivity<vtkm::cont::CELL,
                              vtkm::cont::NODE,3> cellToNode = cs->GetCellToNodeConnectivity();
    vtkm::Vec<vtkm::Id,8> cellIds;
    vtkm::Id cells[8] = {0,-1,-1,-1,-1,-1,-1,-1};
    cellToNode.GetIndices(0, cellIds);
    for (int i = 0; i < 8; i++)
        VTKM_TEST_ASSERT(cellIds[i] == cells[i],
                         "Incorrect cell ID for node");
    
    delete cs;
}

int UnitTestDataSetRegular(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestDataSet_Regular);
}
