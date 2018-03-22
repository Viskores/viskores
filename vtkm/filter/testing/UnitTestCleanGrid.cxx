//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/filter/CleanGrid.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestUniformGrid(vtkm::filter::CleanGrid clean)
{
  std::cout << "Testing 'clean' uniform grid." << std::endl;

  vtkm::cont::testing::MakeTestDataSet makeData;

  vtkm::cont::DataSet inData = makeData.Make2DUniformDataSet0();

  clean.SetFieldsToPass({ "pointvar", "cellvar" });
  vtkm::cont::DataSet outData = clean.Execute(inData);
  VTKM_TEST_ASSERT(outData.HasField("pointvar"), "Failed to map point field");
  VTKM_TEST_ASSERT(outData.HasField("cellvar"), "Failed to map point field");

  vtkm::cont::CellSetExplicit<> outCellSet;
  outData.GetCellSet().CopyTo(outCellSet);
  VTKM_TEST_ASSERT(outCellSet.GetNumberOfPoints() == 6, "Wrong number of points");
  VTKM_TEST_ASSERT(outCellSet.GetNumberOfCells() == 2, "Wrong number of cells");
  vtkm::Vec<vtkm::Id, 4> cellIds;
  outCellSet.GetIndices(0, cellIds);
  VTKM_TEST_ASSERT((cellIds == vtkm::Vec<vtkm::Id, 4>(0, 1, 4, 3)), "Bad cell ids");
  outCellSet.GetIndices(1, cellIds);
  VTKM_TEST_ASSERT((cellIds == vtkm::Vec<vtkm::Id, 4>(1, 2, 5, 4)), "Bad cell ids");

  vtkm::cont::ArrayHandle<vtkm::Float32> outPointField;
  outData.GetField("pointvar").GetData().CopyTo(outPointField);
  VTKM_TEST_ASSERT(outPointField.GetNumberOfValues() == 6, "Wrong point field size.");
  VTKM_TEST_ASSERT(test_equal(outPointField.GetPortalConstControl().Get(1), 20.1),
                   "Bad point field value");
  VTKM_TEST_ASSERT(test_equal(outPointField.GetPortalConstControl().Get(4), 50.1),
                   "Bad point field value");

  vtkm::cont::ArrayHandle<vtkm::Float32> outCellField;
  outData.GetField("cellvar").GetData().CopyTo(outCellField);
  VTKM_TEST_ASSERT(outCellField.GetNumberOfValues() == 2, "Wrong cell field size.");
  VTKM_TEST_ASSERT(test_equal(outCellField.GetPortalConstControl().Get(0), 100.1),
                   "Bad cell field value");
  VTKM_TEST_ASSERT(test_equal(outCellField.GetPortalConstControl().Get(1), 200.1),
                   "Bad cell field value");
}

void RunTest()
{
  vtkm::filter::CleanGrid clean;

  std::cout << "*** Test wqith compact point fields on" << std::endl;
  clean.SetCompactPointFields(true);
  TestUniformGrid(clean);

  std::cout << "*** Test wqith compact point fields off" << std::endl;
  clean.SetCompactPointFields(false);
  TestUniformGrid(clean);
}

} // anonymous namespace

int UnitTestCleanGrid(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(RunTest);
}
