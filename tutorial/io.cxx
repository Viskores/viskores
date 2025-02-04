//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Example 1: very simple Viskores program.
// Read data set, write it out.
//
#include <viskores/cont/Initialize.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

int main(int argc, char** argv)
{
  viskores::cont::Initialize(argc, argv);

  const char* input = "data/kitchen.vtk";
  viskores::io::VTKDataSetReader reader(input);
  viskores::cont::DataSet ds = reader.ReadDataSet();
  viskores::io::VTKDataSetWriter writer("out_io.vtk");
  writer.WriteDataSet(ds);

  return 0;
}
