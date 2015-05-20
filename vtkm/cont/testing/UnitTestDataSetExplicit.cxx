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
#include <vtkm/cont/testing/MakeTestDataSet.h>

namespace {

void TestDataSet_Explicit()
{
  vtkm::cont::testing::MakeTestDataSet tds;
  vtkm::cont::DataSet *ds = tds.Make3DExplicitDataSet1();

  VTKM_TEST_ASSERT(ds->GetNumberOfCellSets() == 1,
                       "Incorrect number of cell sets");

  VTKM_TEST_ASSERT(ds->GetNumberOfFields() == 5,
                       "Incorrect number of fields");

  //cleanup memory
  delete ds;
}

}


int UnitTestDataSetExplicit(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestDataSet_Explicit);
}
