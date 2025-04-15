//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Initialize.h>

#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/filter/LagrangianStructures.h>

int main(int argc, char** argv)
{
  viskores::cont::Initialize(argc, argv);

  if (argc < 3)
  {
    std::cout << "Usage : flte <input dataset> <vector field name>" << std::endl;
  }
  std::string datasetName(argv[1]);
  std::string variableName(argv[2]);

  std::cout << "Reading input dataset" << std::endl;
  viskores::cont::DataSet input;
  viskores::io::VTKDataSetReader reader(datasetName);
  input = reader.ReadDataSet();
  std::cout << "Read input dataset" << std::endl;

  viskores::filter::LagrangianStructures lcsFilter;
  lcsFilter.SetStepSize(0.025f);
  lcsFilter.SetNumberOfSteps(500);
  lcsFilter.SetAdvectionTime(0.025f * 500);
  lcsFilter.SetOutputFieldName("gradient");
  lcsFilter.SetActiveField(variableName);

  viskores::cont::DataSet output = lcsFilter.Execute(input);
  viskores::io::VTKDataSetWriter writer("out.vtk");
  writer.WriteDataSet(output);
  std::cout << "Written output dataset" << std::endl;

  return 0;
}
