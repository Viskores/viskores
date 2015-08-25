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

#include <vtkm/CellType.h>

#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterSerial.h>

#include <vtkm/exec/ConnectivityStructured.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

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
  VTKM_TEST_ASSERT(cellSet.GetNumberOfPoints() == 6,
                   "Incorrect number of points");
  VTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == 2,
                   "Incorrect number of cells");

  // test various field-getting methods and associations
  const vtkm::cont::Field &f1 = dataSet.GetField("pointvar");
  VTKM_TEST_ASSERT(f1.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS,
                       "Association of 'pointvar' was not ASSOC_POINTS");

  try
  {
    //const vtkm::cont::Field &f2 = 
    dataSet.GetField("cellvar", vtkm::cont::Field::ASSOC_CELL_SET);
  }
  catch (...)
  {
    VTKM_TEST_FAIL("Failed to get field 'cellvar' with ASSOC_CELL_SET.");
  }

  try
  {
    //const vtkm::cont::Field &f3 = 
    dataSet.GetField("cellvar", vtkm::cont::Field::ASSOC_POINTS);
    VTKM_TEST_FAIL("Failed to get expected error for association mismatch.");
  }
  catch (vtkm::cont::ErrorControlBadValue error)
  {
    std::cout << "Caught expected error for association mismatch: "
              << std::endl << "    " << error.GetMessage() << std::endl;
  }

  vtkm::Id numCells = cellSet.GetNumberOfCells();
  for (vtkm::Id cellIndex = 0; cellIndex < numCells; cellIndex++)
  {
    VTKM_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(cellIndex) == 4,
                     "Incorrect number of cell indices");
    vtkm::CellType shape = cellSet.GetCellShape();
    VTKM_TEST_ASSERT(shape == vtkm::VTKM_PIXEL, "Incorrect element type.");
  }

  vtkm::exec::ConnectivityStructured<
      vtkm::TopologyElementTagPoint,
      vtkm::TopologyElementTagCell,
      2> pointToCell =
      cellSet.PrepareForInput(
        vtkm::cont::DeviceAdapterTagSerial(),
        vtkm::TopologyElementTagPoint(),
        vtkm::TopologyElementTagCell());
  vtkm::exec::ConnectivityStructured<
      vtkm::TopologyElementTagCell,
      vtkm::TopologyElementTagPoint,
      2> cellToPoint =
      cellSet.PrepareForInput(
        vtkm::cont::DeviceAdapterTagSerial(),
        vtkm::TopologyElementTagCell(),
        vtkm::TopologyElementTagPoint());


  vtkm::Id cells[2][4] = {{0,1,3,4}, {1,2,4,5}};
  for (vtkm::Id cellIndex = 0; cellIndex < 2; cellIndex++)
  {
    vtkm::Vec<vtkm::Id,4> pointIds = pointToCell.GetIndices(cellIndex);
    for (vtkm::IdComponent localPointIndex = 0;
         localPointIndex < 4;
         localPointIndex++)
    {
      VTKM_TEST_ASSERT(
            pointIds[localPointIndex] == cells[cellIndex][localPointIndex],
            "Incorrect point ID for cell");
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
    vtkm::Vec<vtkm::Id,4> retrievedCellIds =
      cellToPoint.GetIndices(pointIndex);
    for (vtkm::IdComponent cellIndex = 0; cellIndex < 4; cellIndex++)
      VTKM_TEST_ASSERT(
            retrievedCellIds[cellIndex] == expectedCellIds[pointIndex][cellIndex],
            "Incorrect cell ID for point");
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

  VTKM_TEST_ASSERT(cellSet.GetNumberOfPoints() == 18,
                   "Incorrect number of points");

  VTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == 4,
                   "Incorrect number of cells");

  // test various field-getting methods and associations
  const vtkm::cont::Field &f1 = dataSet.GetField("pointvar");
  VTKM_TEST_ASSERT(f1.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS,
                       "Association of 'pointvar' was not ASSOC_POINTS");

  try
  {
    //const vtkm::cont::Field &f2 = 
    dataSet.GetField("cellvar", vtkm::cont::Field::ASSOC_CELL_SET);
  }
  catch (...)
  {
    VTKM_TEST_FAIL("Failed to get field 'cellvar' with ASSOC_CELL_SET.");
  }

  try
  {
    //const vtkm::cont::Field &f3 = 
    dataSet.GetField("cellvar", vtkm::cont::Field::ASSOC_POINTS);
    VTKM_TEST_FAIL("Failed to get expected error for association mismatch.");
  }
  catch (vtkm::cont::ErrorControlBadValue error)
  {
    std::cout << "Caught expected error for association mismatch: "
              << std::endl << "    " << error.GetMessage() << std::endl;
  }

  vtkm::Id numCells = cellSet.GetNumberOfCells();
  for (vtkm::Id cellIndex = 0; cellIndex < numCells; cellIndex++)
  {
    VTKM_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(cellIndex) == 8,
                     "Incorrect number of cell indices");
    vtkm::CellType shape = cellSet.GetCellShape();
    VTKM_TEST_ASSERT(shape == vtkm::VTKM_VOXEL, "Incorrect element type.");
  }

  //Test regular connectivity.
  vtkm::exec::ConnectivityStructured<
      vtkm::TopologyElementTagPoint,
      vtkm::TopologyElementTagCell,
      3> pointToCell =
      cellSet.PrepareForInput(
        vtkm::cont::DeviceAdapterTagSerial(),
        vtkm::TopologyElementTagPoint(),
        vtkm::TopologyElementTagCell());
  vtkm::Id expectedPointIds[8] = {0,1,3,4,6,7,9,10};
  vtkm::Vec<vtkm::Id,8> retrievedPointIds = pointToCell.GetIndices(0);
  for (vtkm::IdComponent localPointIndex = 0;
       localPointIndex < 8;
       localPointIndex++)
  {
    VTKM_TEST_ASSERT(
          retrievedPointIds[localPointIndex] == expectedPointIds[localPointIndex],
          "Incorrect point ID for cell");
  }

  vtkm::exec::ConnectivityStructured<
      vtkm::TopologyElementTagCell,
      vtkm::TopologyElementTagPoint,
      3> cellToPoint =
      cellSet.PrepareForInput(
        vtkm::cont::DeviceAdapterTagSerial(),
        vtkm::TopologyElementTagCell(),
        vtkm::TopologyElementTagPoint());
  vtkm::Id retrievedCellIds[6] = {0,-1,-1,-1,-1,-1};
  vtkm::Vec<vtkm::Id,6> expectedCellIds = cellToPoint.GetIndices(0);
  for (vtkm::IdComponent localPointIndex = 0;
       localPointIndex < 6;
       localPointIndex++)
  {
    VTKM_TEST_ASSERT(
          expectedCellIds[localPointIndex] == retrievedCellIds[localPointIndex],
          "Incorrect cell ID for point");
  }
}

int UnitTestDataSetRegular(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestDataSet_Regular);
}
