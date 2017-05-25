//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

#include <vtkm/worklet/Clip.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

typedef vtkm::Vec<vtkm::Float32, 3> FloatVec3;

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    std::cout << "Usage: " << std::endl
              << "$ " << argv[0] << " <input_vtk_file> [fieldName] <isoval> <output_vtk_file>"
              << std::endl;
    return 1;
  }

  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;
  std::cout << "Device Adapter Name: " << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
            << std::endl;

  vtkm::io::reader::VTKDataSetReader reader(argv[1]);
  vtkm::cont::DataSet input = reader.ReadDataSet();

  vtkm::cont::Field scalarField = (argc == 5) ? input.GetField(argv[2]) : input.GetField(0);

  vtkm::Float32 clipValue = std::stof(argv[argc - 2]);
  vtkm::worklet::Clip clip;

  vtkm::cont::Timer<DeviceAdapter> total;
  vtkm::cont::Timer<DeviceAdapter> timer;
  vtkm::cont::CellSetExplicit<> outputCellSet =
    clip.Run(input.GetCellSet(0), scalarField.GetData().ResetTypeList(vtkm::TypeListTagScalarAll()),
             clipValue, DeviceAdapter());
  vtkm::Float64 clipTime = timer.GetElapsedTime();

  vtkm::cont::DataSet output;
  output.AddCellSet(outputCellSet);

  timer.Reset();
  vtkm::cont::DynamicArrayHandle coords =
    clip.ProcessField(input.GetCoordinateSystem(0), DeviceAdapter());
  vtkm::Float64 processCoordinatesTime = timer.GetElapsedTime();
  output.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coords));

  timer.Reset();
  for (vtkm::Id i = 0; i < input.GetNumberOfFields(); ++i)
  {
    vtkm::cont::Field inField = input.GetField(i);
    if (inField.GetAssociation() != vtkm::cont::Field::ASSOC_POINTS)
    {
      continue; // clip only supports point fields for now.
    }
    vtkm::cont::DynamicArrayHandle data =
      clip.ProcessField(inField.GetData().ResetTypeList(vtkm::TypeListTagAll()), DeviceAdapter());
    output.AddField(vtkm::cont::Field(inField.GetName(), vtkm::cont::Field::ASSOC_POINTS, data));
  }
  vtkm::Float64 processScalarsTime = timer.GetElapsedTime();

  vtkm::Float64 totalTime = total.GetElapsedTime();

  std::cout << "Timings: " << std::endl
            << "clip: " << clipTime << std::endl
            << "process coordinates: " << processCoordinatesTime << std::endl
            << "process scalars: " << processScalarsTime << std::endl
            << "Total: " << totalTime << std::endl;

  vtkm::io::writer::VTKDataSetWriter writer(argv[argc - 1]);
  writer.WriteDataSet(output);

  return 0;
}
