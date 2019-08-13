//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#include <cmath>
#include <string>
#include <vector>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/Initialize.h>

#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

#include <vtkm/filter/LagrangianStructures.h>

int main(int argc, char** argv)
{
  vtkm::cont::Initialize(argc, argv);

  if (argc < 3)
  {
    std::cout << "Usage : flte <input dataset> <vector field name>" << std::endl;
  }
  std::string datasetName(argv[1]);
  std::string variableName(argv[2]);

  std::cout << "Reading input dataset" << std::endl;
  vtkm::cont::DataSet input;
  vtkm::io::reader::VTKDataSetReader reader(datasetName);
  input = reader.ReadDataSet();
  std::cout << "Read input dataset" << std::endl;

  vtkm::filter::LagrangianStructures lcsFilter;
  lcsFilter.SetStepSize(0.025f);
  lcsFilter.SetNumberOfSteps(500);
  lcsFilter.SetAdvectionTime(0.025f * 500);
  lcsFilter.SetOutputFieldName("gradient");
  lcsFilter.SetActiveField(variableName);

  vtkm::cont::DataSet output = lcsFilter.Execute(input);
  vtkm::io::writer::VTKDataSetWriter writer("out.vtk");
  writer.WriteDataSet(output);
  std::cout << "Written output dataset" << std::endl;

  return 0;
}
