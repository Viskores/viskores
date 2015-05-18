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

#if 0

    ds.x_idx = 0;
    ds.y_idx = 1;
    ds.z_idx = 2;

    const int nVerts = 18;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1};
    vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
    vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.1, 40.1, 50.2, 60.2, 70.2, 80.2, 90.3, 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5};

    ds.AddFieldViaCopy(xVals, nVerts);
    ds.AddFieldViaCopy(yVals, nVerts);
    ds.AddFieldViaCopy(zVals, nVerts);

    //Set node scalar
    ds.AddFieldViaCopy(vars, nVerts);

    //Set cell scalar
    vtkm::Float32 cellvar[4] = {100.1, 100.2, 100.3, 100.4};
    ds.AddFieldViaCopy(cellvar, 4);
    
    static const vtkm::IdComponent dim = 3;
    vtkm::cont::CellSetStructured<dim> *cs = new vtkm::cont::CellSetStructured<dim>("cells");
    ds.AddCellSet(cs);

    //Set regular structure
    cs->structure.SetNodeDimension(3,2,3);

    //Run a worklet to populate a cell centered field.
    vtkm::Float32 cellVals[4] = {-1.1, -1.2, -1.3, -1.4};
    ds.AddFieldViaCopy(cellVals, 4);

    VTKM_TEST_ASSERT(ds.GetNumberOfCellSets() == 1,
                     "Incorrect number of cell sets");

    VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 6,
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
                              vtkm::cont::CELL,dim> nodeToCell = cs->GetNodeToCellConnectivity();
    vtkm::Vec<vtkm::Id,8> nodeIds;
    nodeToCell.GetIndices(5, nodeIds);
    for (int i = 0; i < 8; i++)
        std::cout<<i<<": nodeIds= "<<nodeIds[i]<<std::endl;
    std::cout<<std::endl;

    vtkm::RegularConnectivity<vtkm::cont::CELL,
                              vtkm::cont::NODE,dim> cellToNode = cs->GetCellToNodeConnectivity();
    for (int i = 0; i < 4; i++)
    {
        vtkm::Vec<vtkm::Id,8> cellIds;
        cellToNode.GetIndices(i, cellIds);
        for (int i = 0; i < 8; i++)
            std::cout<<i<<": cellIds= "<<cellIds[i]<<std::endl;
        std::cout<<std::endl;
    }

    //cleanup memory now
    delete cs;
#endif
}

static void
TwoDimRegularTest()
{
    std::cout<<"2D Regular data set"<<std::endl;
    vtkm::cont::DataSet ds_2d;

    //2D case.
    ds_2d.x_idx = 0;
    ds_2d.y_idx = 1;
    ds_2d.z_idx = -1;

    const int nVerts = 6;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1};
    vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.1, 40.1, 50.1, 60.1};
    ds_2d.AddFieldViaCopy(xVals, nVerts);
    ds_2d.AddFieldViaCopy(yVals, nVerts);
    
    //set node scalar.
    ds_2d.AddFieldViaCopy(vars, nVerts);

    vtkm::Float32 cellvar[1] = {100.1};
    ds_2d.AddFieldViaCopy(cellvar, 1);

    vtkm::cont::CellSetStructured<2> *cs = new vtkm::cont::CellSetStructured<2>("cells");
    //Set regular structure
    cs->structure.SetNodeDimension(3,2);
    ds_2d.AddCellSet(cs);

    VTKM_TEST_ASSERT(ds_2d.GetNumberOfCellSets() == 1,
                     "Incorrect number of cell sets");
    //std::cout<<"Num nodes= "<<cs->structure.GetNumberOfNodes()<<std::endl;
    //std::cout<<"Num cells= "<<cs->structure.GetNumberOfCells()<<std::endl;
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
    
    delete cs;
}

static void
ThreeDimRegularTest()
{
    std::cout<<"3D Regular data set"<<std::endl;
    vtkm::cont::DataSet ds;

    ds.x_idx = 0;
    ds.y_idx = 1;
    ds.z_idx = 2;

    const int nVerts = 18;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1};
    vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
    vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.1, 40.1, 50.2, 60.2, 70.2, 80.2, 90.3, 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5};

    ds.AddFieldViaCopy(xVals, nVerts);
    ds.AddFieldViaCopy(yVals, nVerts);
    ds.AddFieldViaCopy(zVals, nVerts);

    //Set node scalar
    ds.AddFieldViaCopy(vars, nVerts);

    //Set cell scalar
    vtkm::Float32 cellvar[4] = {100.1, 100.2, 100.3, 100.4};
    ds.AddFieldViaCopy(cellvar, 4);
    
    static const vtkm::IdComponent dim = 3;
    vtkm::cont::CellSetStructured<dim> *cs = new vtkm::cont::CellSetStructured<dim>("cells");
    ds.AddCellSet(cs);

    //Set regular structure
    cs->structure.SetNodeDimension(3,2,3);

    //Run a worklet to populate a cell centered field.
    vtkm::Float32 cellVals[4] = {-1.1, -1.2, -1.3, -1.4};
    ds.AddFieldViaCopy(cellVals, 4);

    VTKM_TEST_ASSERT(ds.GetNumberOfCellSets() == 1,
                     "Incorrect number of cell sets");

    VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 6,
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
                              vtkm::cont::CELL,dim> nodeToCell = cs->GetNodeToCellConnectivity();
    vtkm::Id nodes[8] = {0,1,3,4,6,7,9,10};
    vtkm::Vec<vtkm::Id,8> nodeIds;
    nodeToCell.GetIndices(0, nodeIds);
    for (int i = 0; i < 8; i++)
        VTKM_TEST_ASSERT(nodeIds[i] == nodes[i],
                             "Incorrect node ID for cell");

    vtkm::RegularConnectivity<vtkm::cont::CELL,
                              vtkm::cont::NODE,dim> cellToNode = cs->GetCellToNodeConnectivity();
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
