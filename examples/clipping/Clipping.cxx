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

#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/filter/contour/ClipWithField.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[])
{
  viskores::cont::Initialize(argc, argv);

  if ((argc < 4) || (argc > 5))
  {
    std::cerr << "Usage: " << argv[0] << " in_data.vtk field_name clip_value [out_data.vtk]\n\n";
    std::cerr << "For example, you could use the example.vtk that comes with the Viskores source:\n\n";
    std::cerr << "  " << argv[0]
              << " <path-to-viskores-source>/data/data/third_party/visit/example.vtk temp 3.5\n";
    return 1;
  }
  std::string infilename = argv[1];
  std::string infield = argv[2];
  double fieldValue = std::atof(argv[3]);
  std::string outfilename = "out_data.vtk";
  if (argc == 5)
  {
    outfilename = argv[4];
  }

  viskores::io::VTKDataSetReader reader(infilename);
  viskores::cont::DataSet input = reader.ReadDataSet();

  viskores::filter::contour::ClipWithField clipFilter;
  clipFilter.SetActiveField(infield);
  clipFilter.SetClipValue(fieldValue);
  viskores::cont::DataSet output = clipFilter.Execute(input);

  viskores::io::VTKDataSetWriter writer(outfilename);
  writer.WriteDataSet(output);

  return 0;
}
