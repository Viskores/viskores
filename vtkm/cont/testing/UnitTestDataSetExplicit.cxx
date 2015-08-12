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

void TestDataSet_Explicit()
{
  vtkm::cont::testing::MakeTestDataSet tds;
  vtkm::cont::DataSet ds = tds.Make3DExplicitDataSet1();

  VTKM_TEST_ASSERT(ds.GetNumberOfCellSets() == 1,
                       "Incorrect number of cell sets");

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 5,
                       "Incorrect number of fields");

  // test various field-getting methods and associations
  const vtkm::cont::Field &f1 = ds.GetField("pointvar");
  VTKM_TEST_ASSERT(f1.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS,
                       "Association of 'pointvar' was not ASSOC_POINTS");

  try
  {
    const vtkm::cont::Field &f2 = ds.GetField("cellvar",
                                            vtkm::cont::Field::ASSOC_CELL_SET);
  }
  catch (...)
  {
    VTKM_TEST_FAIL("Failed to get field 'cellvar' with ASSOC_CELL_SET.");
  }


  try
  {
    const vtkm::cont::Field &f2 = ds.GetField("cellvar",
                                              vtkm::cont::Field::ASSOC_POINTS);
    VTKM_TEST_FAIL("Failed to get expected error for association mismatch.");
  }
  catch (vtkm::cont::ErrorControlBadValue error)
  {
    std::cout << "Caught expected error for association mismatch: "
              << std::endl << "    " << error.GetMessage() << std::endl;
  }
}

}


int UnitTestDataSetExplicit(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestDataSet_Explicit);
}
