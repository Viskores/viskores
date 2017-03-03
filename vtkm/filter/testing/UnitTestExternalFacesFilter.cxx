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

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/ExternalFaces.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace {

void TestExternalFacesExplicitGrid()
{
  vtkm::cont::DataSet ds = MakeTestDataSet().Make3DExplicitDataSet5();

  //Run the External Faces filter
  vtkm::filter::ExternalFaces externalFaces;
  vtkm::filter::ResultDataSet result = externalFaces.Execute(ds);

  VTKM_TEST_ASSERT(result.IsValid(), "Results should be valid");

  // map fields
  for (vtkm::IdComponent i = 0; i < ds.GetNumberOfFields(); ++i)
  {
    externalFaces.MapFieldOntoOutput(result, ds.GetField(i));
  }

  vtkm::cont::DataSet resultds = result.GetDataSet();

  // verify cellset
  vtkm::cont::CellSetExplicit<> &new_cellSet =
    resultds.GetCellSet(0).Cast<vtkm::cont::CellSetExplicit<> >();
  const vtkm::Id numExtFaces_out = new_cellSet.GetNumberOfCells();
  const vtkm::Id numExtFaces_actual = 12;
  VTKM_TEST_ASSERT(numExtFaces_out == numExtFaces_actual,
                   "Number of External Faces mismatch");

  // verify fields
  VTKM_TEST_ASSERT(resultds.HasField("pointvar"),
                   "Point field not mapped succesfully");
}

void TestExternalFacesFilter()
{
  TestExternalFacesExplicitGrid();
}

} // anonymous namespace


int UnitTestExternalFacesFilter(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestExternalFacesFilter);
}
