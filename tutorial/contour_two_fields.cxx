//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Example 3: do a contour (but only evaluate two fields), write it out.
//
#include <viskores/cont/Initialize.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

int main(int argc, char** argv)
{
  viskores::cont::Initialize(argc, argv);

  viskores::io::VTKDataSetReader reader("data/kitchen.vtk");
  viskores::cont::DataSet ds_from_file = reader.ReadDataSet();

  viskores::filter::contour::Contour contour;
  contour.SetActiveField("c1");
  contour.SetFieldsToPass({ "c1", "ke" });
  contour.SetNumberOfIsoValues(3);
  contour.SetIsoValue(0, 0.05);
  contour.SetIsoValue(1, 0.10);
  contour.SetIsoValue(2, 0.15);

  viskores::cont::DataSet ds_from_contour = contour.Execute(ds_from_file);
  viskores::io::VTKDataSetWriter writer("out_mc_2fields.vtk");
  writer.WriteDataSet(ds_from_contour);

  return 0;
}
