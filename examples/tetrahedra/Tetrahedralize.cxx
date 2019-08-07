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

#include <vtkm/filter/Tetrahedralize.h>

int main(int argc, char* argv[])
{
  vtkm::cont::Initialize(argc, argv, vtkm::cont::InitializeOptions::Strict);

  vtkm::cont::DataSet input =
    vtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet3(vtkm::Id3(25, 25, 25));

  vtkm::filter::Tetrahedralize tetrahedralizeFilter;
  vtkm::cont::DataSet output = tetrahedralizeFilter.Execute(input);

  vtkm::io::writer::VTKDataSetWriter writer("out_tets.vtk");
  writer.WriteDataSet(output);

  return 0;
}
