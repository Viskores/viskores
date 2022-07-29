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
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  try
  {
    vtkm::io::VTKDataSetReader reader("data/kitchen.vtk");

    // PROBLEM! ... we aren't reading from a file, so we have an empty vtkm::cont::DataSet.
    //vtkm::cont::DataSet ds_from_file = reader.ReadDataSet();
    vtkm::cont::DataSet ds_from_file;

    vtkm::filter::contour::Contour contour;
    contour.SetActiveField("c1");
    contour.SetNumberOfIsoValues(3);
    contour.SetIsoValue(0, 0.05);
    contour.SetIsoValue(1, 0.10);
    contour.SetIsoValue(2, 0.15);

    vtkm::cont::DataSet ds_from_contour = contour.Execute(ds_from_file);
    vtkm::io::VTKDataSetWriter writer("out_mc.vtk");
    writer.WriteDataSet(ds_from_contour);
  }
  catch (const vtkm::cont::Error& error)
  {
    std::cerr << "VTK-m error occurred!: " << error.GetMessage() << std::endl;
    return 1;
  }

  return 0;
}
