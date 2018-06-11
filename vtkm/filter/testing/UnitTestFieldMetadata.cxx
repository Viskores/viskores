//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/CellAverage.h>

namespace
{

vtkm::cont::Field makeCellField()
{
  return vtkm::cont::Field("foo",
                           vtkm::cont::Field::Association::CELL_SET,
                           std::string(),
                           vtkm::cont::ArrayHandle<vtkm::Float32>());
}
vtkm::cont::Field makePointField()
{
  return vtkm::cont::Field(
    "foo", vtkm::cont::Field::Association::POINTS, vtkm::cont::ArrayHandle<vtkm::Float32>());
}

void TestFieldTypesUnknown()
{
  vtkm::filter::FieldMetadata defaultMD;
  VTKM_TEST_ASSERT(defaultMD.IsPointField() == false, "default is not point or cell");
  VTKM_TEST_ASSERT(defaultMD.IsCellField() == false, "default is not point or cell");

  //verify the field helper works properly
  vtkm::cont::Field field1;
  vtkm::filter::FieldMetadata makeMDFromField(field1);
  VTKM_TEST_ASSERT(makeMDFromField.IsPointField() == false, "makeMDFromField is not point or cell");
  VTKM_TEST_ASSERT(makeMDFromField.IsCellField() == false, "makeMDFromField is not point or cell");
}

void TestFieldTypesPoint()
{
  vtkm::filter::FieldMetadata helperMD(makePointField());
  VTKM_TEST_ASSERT(helperMD.IsPointField() == true, "point should be a point field");
  VTKM_TEST_ASSERT(helperMD.IsCellField() == false, "point can't be a cell field");

  //verify the field helper works properly
  vtkm::Float32 vars[6] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  auto field = vtkm::cont::make_Field("pointvar", vtkm::cont::Field::Association::POINTS, vars, 6);
  vtkm::filter::FieldMetadata makeMDFromField(field);
  VTKM_TEST_ASSERT(makeMDFromField.IsPointField() == true, "point should be a point field");
  VTKM_TEST_ASSERT(makeMDFromField.IsCellField() == false, "point can't be a cell field");
}

void TestFieldTypesCell()
{
  vtkm::filter::FieldMetadata defaultMD;
  vtkm::filter::FieldMetadata helperMD(makeCellField());
  VTKM_TEST_ASSERT(helperMD.IsPointField() == false, "cell can't be a point field");
  VTKM_TEST_ASSERT(helperMD.IsCellField() == true, "cell should be a cell field");

  //verify the field helper works properly
  vtkm::Float32 vars[6] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };
  auto field = vtkm::cont::make_Field(
    "pointvar", vtkm::cont::Field::Association::CELL_SET, std::string(), vars, 6);
  vtkm::filter::FieldMetadata makeMDFromField(field);
  VTKM_TEST_ASSERT(makeMDFromField.IsPointField() == false, "cell can't be a point field");
  VTKM_TEST_ASSERT(makeMDFromField.IsCellField() == true, "cell should be a cell field");
}

void TestFieldMetadata()
{
  TestFieldTypesUnknown();
  TestFieldTypesPoint();
  TestFieldTypesCell();
}
}

int UnitTestFieldMetadata(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestFieldMetadata);
}
