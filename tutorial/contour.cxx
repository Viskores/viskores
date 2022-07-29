//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Example 2: do a contour!, write it out.
//
#include <vtkm/cont/Initialize.h>
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  vtkm::io::VTKDataSetReader reader("data/kitchen.vtk");
  vtkm::cont::DataSet ds_from_file = reader.ReadDataSet();

  vtkm::filter::contour::Contour contour;
  contour.SetActiveField("c1");
  contour.SetNumberOfIsoValues(3);
  contour.SetIsoValue(0, 0.05);
  contour.SetIsoValue(1, 0.10);
  contour.SetIsoValue(2, 0.15);

  vtkm::cont::DataSet ds_from_contour = contour.Execute(ds_from_file);
  vtkm::io::VTKDataSetWriter writer("out_mc.vtk");
  writer.WriteDataSet(ds_from_contour);

  return 0;
}
