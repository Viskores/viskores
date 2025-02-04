//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Example 4: do a contour and a clip-with-field, and write it out.
//
#include <viskores/cont/Initialize.h>
#include <viskores/filter/contour/ClipWithField.h>
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

  viskores::filter::contour::ClipWithField clip;
  clip.SetActiveField("ke");
  clip.SetClipValue(1e-7);
  //clip.SetInvertClip(true); // <1e-7 instead of >1e-7

  viskores::cont::DataSet ds_from_clip = clip.Execute(ds_from_contour);

  viskores::io::VTKDataSetWriter writer("out_2filters.vtk");
  writer.WriteDataSet(ds_from_clip);

  return 0;
}
