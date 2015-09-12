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
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

namespace {

template<typename T, typename Storage>
bool TestArrayHandle(const vtkm::cont::ArrayHandle<T, Storage> &ah, const T *expected,
                     vtkm::Id size)
{
  if (size != ah.GetNumberOfValues())
  {
    return false;
  }

  for (vtkm::Id i = 0; i < size; ++i)
  {
    if (ah.GetPortalConstControl().Get(i) != expected[i])
    {
      return false;
    }
  }

  return true;
}


void TestDataSet_Explicit()
{
  vtkm::cont::testing::MakeTestDataSet tds;
  vtkm::cont::DataSet ds = tds.Make3DExplicitDataSet1();

  ds.PrintSummary(std::cout);

  VTKM_TEST_ASSERT(ds.GetNumberOfCellSets() == 1,
                       "Incorrect number of cell sets");

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2,
                       "Incorrect number of fields");

  // test various field-getting methods and associations
  const vtkm::cont::Field &f1 = ds.GetField("pointvar");
  VTKM_TEST_ASSERT(f1.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS,
                       "Association of 'pointvar' was not ASSOC_POINTS");

  try
  {
    //const vtkm::cont::Field &f2 =
    ds.GetField("cellvar", vtkm::cont::Field::ASSOC_CELL_SET);
  }
  catch (...)
  {
    VTKM_TEST_FAIL("Failed to get field 'cellvar' with ASSOC_CELL_SET.");
  }

  try
  {
    //const vtkm::cont::Field &f3 =
    ds.GetField("cellvar", vtkm::cont::Field::ASSOC_POINTS);
    VTKM_TEST_FAIL("Failed to get expected error for association mismatch.");
  }
  catch (vtkm::cont::ErrorControlBadValue error)
  {
    std::cout << "Caught expected error for association mismatch: "
              << std::endl << "    " << error.GetMessage() << std::endl;
  }

  VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                   "Incorrect number of coordinate systems");

  // test cell-to-point connectivity
  vtkm::cont::CellSetExplicit<> &cellset =
      ds.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();

  cellset.BuildConnectivity(vtkm::TopologyElementTagCell(),
                            vtkm::TopologyElementTagPoint());

  vtkm::Id connectivitySize = 7;
  vtkm::Id numPoints = 5;

  vtkm::Id correctShapes[] = {1, 1, 1, 1, 1};
  vtkm::Id correctNumIndices[] = {1, 2, 2, 1, 1};
  vtkm::Id correctConnectivity[] = {0, 0, 1, 0, 1, 1, 1};

  vtkm::cont::ArrayHandle<vtkm::Id> shapes = cellset.GetShapesArray(
    vtkm::TopologyElementTagCell(),vtkm::TopologyElementTagPoint());
  vtkm::cont::ArrayHandle<vtkm::Id> numIndices = cellset.GetNumIndicesArray(
    vtkm::TopologyElementTagCell(),vtkm::TopologyElementTagPoint());
  vtkm::cont::ArrayHandle<vtkm::Id> conn = cellset.GetConnectivityArray(
    vtkm::TopologyElementTagCell(),vtkm::TopologyElementTagPoint());

  VTKM_TEST_ASSERT(TestArrayHandle(shapes,
        correctShapes,
        numPoints),
      "Got incorrect shapes");
  VTKM_TEST_ASSERT(TestArrayHandle(numIndices,
        correctNumIndices,
        numPoints),
      "Got incorrect shapes");
  VTKM_TEST_ASSERT(TestArrayHandle(conn,
        correctConnectivity,
        connectivitySize),
      "Got incorrect conectivity");
}

}


int UnitTestDataSetExplicit(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestDataSet_Explicit);
}
