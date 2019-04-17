//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/writer/VTKDataSetWriter.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestVTKExplicitWrite()
{
  vtkm::cont::testing::MakeTestDataSet tds;

  vtkm::io::writer::VTKDataSetWriter writer1("fileA1.vtk");
  writer1.WriteDataSet(tds.Make3DExplicitDataSet0());

  // force it to output an explicit grid as points
  vtkm::io::writer::VTKDataSetWriter writer2("fileA2.vtk");
  writer2.WriteDataSet(tds.Make3DExplicitDataSet0(), -1);

  vtkm::io::writer::VTKDataSetWriter writer3("fileA3.vtk");
  writer3.WriteDataSet(tds.Make3DExplicitDataSet0());

  vtkm::io::writer::VTKDataSetWriter writer4("fileA4.vtk");
  writer4.WriteDataSet(tds.Make3DExplicitDataSetCowNose());
}

void TestVTKUniformWrite()
{
  vtkm::cont::testing::MakeTestDataSet tds;

  vtkm::io::writer::VTKDataSetWriter writer1("fileB1.vtk");
  writer1.WriteDataSet(tds.Make2DUniformDataSet0());

  vtkm::io::writer::VTKDataSetWriter writer2("fileB2.vtk");
  writer2.WriteDataSet(tds.Make3DUniformDataSet0());

  // force it to output an explicit grid as points
  vtkm::io::writer::VTKDataSetWriter writer3("fileB3.vtk");
  writer3.WriteDataSet(tds.Make3DUniformDataSet0(), -1);
}

void TestVTKWrite()
{
  TestVTKExplicitWrite();
  TestVTKUniformWrite();
}

} //Anonymous namespace

int UnitTestVTKDataSetWriter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestVTKWrite, argc, argv);
}
