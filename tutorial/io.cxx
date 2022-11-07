//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Example 1: very simple VTK-m program.
// Read data set, write it out.
//
#include <vtkm/cont/Initialize.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

int main(int argc, char** argv)
{
  vtkm::cont::Initialize(argc, argv);

  const char* input = "data/kitchen.vtk";
  vtkm::io::VTKDataSetReader reader(input);
  vtkm::cont::DataSet ds = reader.ReadDataSet();
  vtkm::io::VTKDataSetWriter writer("out_io.vtk");
  writer.WriteDataSet(ds);

  return 0;
}
