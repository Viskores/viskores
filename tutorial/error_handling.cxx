//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/Initialize.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

int main(int argc, char** argv)
{
  auto opts = viskores::cont::InitializeOptions::DefaultAnyDevice;
  viskores::cont::InitializeResult config = viskores::cont::Initialize(argc, argv, opts);

  try
  {
    viskores::io::VTKDataSetReader reader("data/kitchen.vtk");

    // PROBLEM! ... we aren't reading from a file, so we have an empty viskores::cont::DataSet.
    //viskores::cont::DataSet ds_from_file = reader.ReadDataSet();
    viskores::cont::DataSet ds_from_file;

    viskores::filter::contour::Contour contour;
    contour.SetActiveField("c1");
    contour.SetNumberOfIsoValues(3);
    contour.SetIsoValue(0, 0.05);
    contour.SetIsoValue(1, 0.10);
    contour.SetIsoValue(2, 0.15);

    viskores::cont::DataSet ds_from_contour = contour.Execute(ds_from_file);
    viskores::io::VTKDataSetWriter writer("out_mc.vtk");
    writer.WriteDataSet(ds_from_contour);
  }
  catch (const viskores::cont::Error& error)
  {
    std::cerr << "Viskores error occurred!: " << error.GetMessage() << std::endl;
    return 1;
  }

  return 0;
}
