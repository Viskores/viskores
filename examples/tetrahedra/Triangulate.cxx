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

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/filter/geometry_refinement/Triangulate.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[])
{
  vtkm::cont::Initialize(argc, argv);

  if ((argc < 2) || (argc > 3))
  {
    std::cerr << "Usage: " << argv[0] << " in_data.vtk [out_data.vtk]\n\n";
    std::cerr << "For example, you could use the vanc.vtk that comes with the VTK-m source:\n\n";
    std::cerr << "  " << argv[0] << " <path-to-vtkm-source>/data/data/rectilinear/vanc.vtk\n";
    return 1;
  }
  std::string infilename = argv[1];
  std::string outfilename = "out_tris.vtk";
  if (argc == 3)
  {
    outfilename = argv[2];
  }

  vtkm::io::VTKDataSetReader reader(infilename);
  vtkm::cont::DataSet input = reader.ReadDataSet();

  vtkm::filter::geometry_refinement::Triangulate triangulateFilter;
  vtkm::cont::DataSet output = triangulateFilter.Execute(input);

  vtkm::io::VTKDataSetWriter writer(outfilename);
  writer.WriteDataSet(output);

  return 0;
}
