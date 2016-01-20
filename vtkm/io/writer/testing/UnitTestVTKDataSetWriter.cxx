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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/io/writer/VTKDataSetWriter.h>

namespace {

void TestVTKExplicitWrite()
{
  vtkm::cont::testing::MakeTestDataSet tds;

  std::ofstream out1("fileA1.vtk");
  vtkm::io::writer::VTKDataSetWriter::Write(out1,
    tds.Make3DExplicitDataSet0());
  out1.close();

  // force it to output an explicit grid as points
  std::ofstream out2("fileA2.vtk");
  vtkm::io::writer::VTKDataSetWriter::Write(out2,
    tds.Make3DExplicitDataSet0(), -1);
  out2.close();

  std::ofstream out3("fileA3.vtk");
  vtkm::io::writer::VTKDataSetWriter::Write(out3,
    tds.Make3DExplicitDataSet0());
  out3.close();

  std::ofstream out4("fileA4.vtk");
  vtkm::io::writer::VTKDataSetWriter::Write(out4,
    tds.Make3DExplicitDataSetCowNose());
  out4.close();
}

void TestVTKUniformWrite()
{
  vtkm::cont::testing::MakeTestDataSet tds;

  std::ofstream out1("fileB1.vtk");
  vtkm::io::writer::VTKDataSetWriter::Write(out1,
    tds.Make2DUniformDataSet0());
  out1.close();

  std::ofstream out2("fileB2.vtk");
  vtkm::io::writer::VTKDataSetWriter::Write(out2,
    tds.Make3DUniformDataSet0());
  out2.close();

  // force it to output an explicit grid as points
  std::ofstream out3("fileB3.vtk");
  vtkm::io::writer::VTKDataSetWriter::Write(out3,
    tds.Make3DUniformDataSet0(), -1);
  out3.close();
}

void TestVTKWrite()
{
  TestVTKExplicitWrite();
  TestVTKUniformWrite();
}

} //Anonymous namespace

int UnitTestVTKDataSetWriter(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestVTKWrite);
}
