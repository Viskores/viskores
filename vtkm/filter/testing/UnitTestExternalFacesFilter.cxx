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

#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/ExternalFaces.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace {

// convert a 5x5x5 uniform grid to unstructured grid
vtkm::cont::DataSet MakeDataTestSet1()
{
  vtkm::cont::DataSet ds = MakeTestDataSet().Make3DUniformDataSet1();

  vtkm::filter::CleanGrid clean;
  vtkm::filter::ResultDataSet result = clean.Execute(ds);
  for (vtkm::IdComponent i = 0; i < ds.GetNumberOfFields(); ++i)
  {
    clean.MapFieldOntoOutput(result, ds.GetField(i));
  }

  return result.GetDataSet();
}

vtkm::cont::DataSet MakeDataTestSet2()
{
  return MakeTestDataSet().Make3DExplicitDataSet5();
}

void TestExternalFacesExplicitGrid(const vtkm::cont::DataSet &ds,
                                   bool compactPoints,
                                   vtkm::Id numExpectedExtFaces,
                                   vtkm::Id numExpectedPoints = 0)
{
  //Run the External Faces filter
  vtkm::filter::ExternalFaces externalFaces;
  externalFaces.SetCompactPoints(compactPoints);
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
  const vtkm::Id numOutputExtFaces = new_cellSet.GetNumberOfCells();
  VTKM_TEST_ASSERT(numOutputExtFaces == numExpectedExtFaces,
                   "Number of External Faces mismatch");

  // verify fields
  VTKM_TEST_ASSERT(resultds.HasField("pointvar"),
                   "Point field not mapped succesfully");

  // verify CompactPoints
  if (compactPoints)
  {
    vtkm::Id numOutputPoints =
      resultds.GetCoordinateSystem(0).GetData().GetNumberOfValues();
    VTKM_TEST_ASSERT(numOutputPoints == numExpectedPoints,
                     "Incorrect number of points after compacting");
  }
}

void TestWithHexahedraMesh()
{
  std::cout << "Testing with Hexahedra mesh\n";
  vtkm::cont::DataSet ds = MakeDataTestSet1();
  std::cout << "Compact Points Off\n";
  TestExternalFacesExplicitGrid(ds, false, 96); // 4x4 * 6 = 96
  std::cout << "Compact Points On\n";
  TestExternalFacesExplicitGrid(ds, true, 96, 98); // 5x5x5 - 3x3x3 = 98
}

void TestWithHeterogeneousMesh()
{
  std::cout << "Testing with Heterogeneous mesh\n";
  vtkm::cont::DataSet ds = MakeDataTestSet2();
  std::cout << "Compact Points Off\n";
  TestExternalFacesExplicitGrid(ds, false, 12);
  std::cout << "Compact Points On\n";
  TestExternalFacesExplicitGrid(ds, true, 12, 11);
}

void TestExternalFacesFilter()
{
  TestWithHeterogeneousMesh();
  TestWithHexahedraMesh();
}

} // anonymous namespace


int UnitTestExternalFacesFilter(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestExternalFacesFilter);
}
