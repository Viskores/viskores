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

#define WRITE_FILE(MakeTestDataMethod)                                                             \
  TestVTKWriteTestData(#MakeTestDataMethod, tds.MakeTestDataMethod())

void TestVTKWriteTestData(const std::string& methodName, const vtkm::cont::DataSet& data)
{
  std::cout << "Writing " << methodName << std::endl;
  vtkm::io::writer::VTKDataSetWriter writer(methodName + ".vtk");
  writer.WriteDataSet(data);
}

void TestVTKExplicitWrite()
{
  vtkm::cont::testing::MakeTestDataSet tds;

  WRITE_FILE(Make1DExplicitDataSet0);

  WRITE_FILE(Make2DExplicitDataSet0);

  WRITE_FILE(Make3DExplicitDataSet0);
  WRITE_FILE(Make3DExplicitDataSet1);
  WRITE_FILE(Make3DExplicitDataSet2);
  WRITE_FILE(Make3DExplicitDataSet3);
  WRITE_FILE(Make3DExplicitDataSet4);
  WRITE_FILE(Make3DExplicitDataSet5);
  WRITE_FILE(Make3DExplicitDataSet6);
  WRITE_FILE(Make3DExplicitDataSet7);
  WRITE_FILE(Make3DExplicitDataSet8);
  WRITE_FILE(Make3DExplicitDataSetZoo);
  WRITE_FILE(Make3DExplicitDataSetPolygonal);
  WRITE_FILE(Make3DExplicitDataSetCowNose);

  std::cout << "Force writer to output an explicit grid as points" << std::endl;
  vtkm::io::writer::VTKDataSetWriter writer("Make3DExplicitDataSet0-no-grid.vtk");
  writer.WriteDataSet(tds.Make3DExplicitDataSet0(), true);
}

void TestVTKUniformWrite()
{
  vtkm::cont::testing::MakeTestDataSet tds;

  WRITE_FILE(Make1DUniformDataSet0);
  WRITE_FILE(Make1DUniformDataSet1);
  WRITE_FILE(Make1DUniformDataSet2);

  WRITE_FILE(Make2DUniformDataSet0);
  WRITE_FILE(Make2DUniformDataSet1);
  WRITE_FILE(Make2DUniformDataSet2);

  WRITE_FILE(Make3DUniformDataSet0);
  WRITE_FILE(Make3DUniformDataSet1);
  // WRITE_FILE(Make3DUniformDataSet2); Skip this one. It's really big.
  WRITE_FILE(Make3DUniformDataSet3);

  WRITE_FILE(Make3DRegularDataSet0);
  WRITE_FILE(Make3DRegularDataSet1);

  std::cout << "Force writer to output a uniform grid as points" << std::endl;
  vtkm::io::writer::VTKDataSetWriter writer("Make3DUniformDataSet0-no-grid.vtk");
  writer.WriteDataSet(tds.Make3DUniformDataSet0(), true);
}

void TestVTKRectilinearWrite()
{
  vtkm::cont::testing::MakeTestDataSet tds;

  WRITE_FILE(Make2DRectilinearDataSet0);

  WRITE_FILE(Make3DRectilinearDataSet0);

  std::cout << "Force writer to output a rectilinear grid as points" << std::endl;
  vtkm::io::writer::VTKDataSetWriter writer("Make3DRectilinearDataSet0-no-grid.vtk");
  writer.WriteDataSet(tds.Make3DRectilinearDataSet0(), true);
}

void TestVTKWrite()
{
  TestVTKExplicitWrite();
  TestVTKUniformWrite();
  TestVTKRectilinearWrite();
}

} //Anonymous namespace

int UnitTestVTKDataSetWriter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestVTKWrite, argc, argv);
}
