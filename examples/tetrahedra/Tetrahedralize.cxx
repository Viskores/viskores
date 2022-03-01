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

#include <vtkm/filter/geometry_refinement/Tetrahedralize.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[])
{
  vtkm::cont::Initialize(argc, argv);

  if ((argc < 2) || (argc > 3))
  {
    std::cerr << "Usage: " << argv[0] << " in_data.vtk [out_data.vtk]\n\n";
    std::cerr << "For example, you could use the ucd3d.vtk that comes with the VTK-m source:\n\n";
    std::cerr << "  " << argv[0] << " <path-to-vtkm-source>/data/data/unstructured/ucd3d.vtk\n";
    return 1;
  }
  std::string infilename = argv[1];
  std::string outfilename = "out_tets.vtk";
  if (argc == 3)
  {
    outfilename = argv[2];
  }

  vtkm::io::VTKDataSetReader reader(infilename);
  vtkm::cont::DataSet input = reader.ReadDataSet();

  vtkm::filter::geometry_refinement::Tetrahedralize tetrahedralizeFilter;
  vtkm::cont::DataSet output = tetrahedralizeFilter.Execute(input);

  vtkm::io::VTKDataSetWriter writer(outfilename);
  writer.WriteDataSet(output);

  return 0;
}
