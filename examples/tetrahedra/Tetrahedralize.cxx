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

#include <viskores/filter/geometry_refinement/Tetrahedralize.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[])
{
  viskores::cont::Initialize(argc, argv);

  if ((argc < 2) || (argc > 3))
  {
    std::cerr << "Usage: " << argv[0] << " in_data.vtk [out_data.vtk]\n\n";
    std::cerr
      << "For example, you could use the example.vtk that comes with the Viskores source:\n\n";
    std::cerr << "  " << argv[0]
              << " <path-to-viskores-source>/data/data/third_party/visit/example.vtk\n";
    return 1;
  }
  std::string infilename = argv[1];
  std::string outfilename = "out_tets.vtk";
  if (argc == 3)
  {
    outfilename = argv[2];
  }

  viskores::io::VTKDataSetReader reader(infilename);
  viskores::cont::DataSet input = reader.ReadDataSet();

  viskores::filter::geometry_refinement::Tetrahedralize tetrahedralizeFilter;
  viskores::cont::DataSet output = tetrahedralizeFilter.Execute(input);

  viskores::io::VTKDataSetWriter writer(outfilename);
  writer.WriteDataSet(output);

  return 0;
}
