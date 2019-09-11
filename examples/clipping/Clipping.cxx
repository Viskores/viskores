//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Initialize.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/io/writer/VTKDataSetWriter.h>

#include <vtkm/filter/ClipWithField.h>

int main(int argc, char* argv[])
{
  vtkm::cont::Initialize(argc, argv, vtkm::cont::InitializeOptions::Strict);

  vtkm::cont::DataSet input = vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSetCowNose();

  vtkm::filter::ClipWithField clipFilter;
  clipFilter.SetActiveField("pointvar");
  clipFilter.SetClipValue(20.0);
  vtkm::cont::DataSet output = clipFilter.Execute(input);

  vtkm::io::writer::VTKDataSetWriter writer("out_data.vtk");
  writer.WriteDataSet(output);

  return 0;
}
