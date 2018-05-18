//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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

using FloatVec3 = vtkm::Vec<vtkm::Float32, 3>;

namespace
{

template <typename DeviceTag>
struct FieldMapper
{
  vtkm::cont::DynamicArrayHandle& Output;
  vtkm::worklet::Clip& Worklet;
  bool IsCellField;

  FieldMapper(vtkm::cont::DynamicArrayHandle& output,
              vtkm::worklet::Clip& worklet,
              bool isCellField)
    : Output(output)
    , Worklet(worklet)
    , IsCellField(isCellField)
  {
  }

  template <typename ArrayType>
  void operator()(const ArrayType& input) const
  {
    if (this->IsCellField)
    {
      this->Output = this->Worklet.ProcessCellField(input, DeviceTag());
    }
    else
    {
      this->Output = this->Worklet.ProcessPointField(input, DeviceTag());
    }
  }
};

} // end anon namespace

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    std::cout << "Usage: " << std::endl
              << "$ " << argv[0] << " <input_vtk_file> [fieldName] <isoval> <output_vtk_file>"
              << std::endl;
    return 1;
  }

  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  std::cout << "Device Adapter Name: " << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
            << std::endl;

  vtkm::io::reader::VTKDataSetReader reader(argv[1]);
  vtkm::cont::DataSet input = reader.ReadDataSet();

  vtkm::cont::Field scalarField = (argc == 5) ? input.GetField(argv[2]) : input.GetField(0);

  vtkm::Float32 clipValue = std::stof(argv[argc - 2]);
  vtkm::worklet::Clip clip;

  vtkm::cont::Timer<DeviceAdapter> total;
  vtkm::cont::Timer<DeviceAdapter> timer;
  bool invertClip = false;
  vtkm::cont::CellSetExplicit<> outputCellSet =
    clip.Run(input.GetCellSet(0),
             scalarField.GetData().ResetTypeList(vtkm::TypeListTagScalarAll()),
             clipValue,
             invertClip,
             DeviceAdapter());
  vtkm::Float64 clipTime = timer.GetElapsedTime();

  vtkm::cont::DataSet output;
  output.AddCellSet(outputCellSet);


  auto inCoords = input.GetCoordinateSystem(0).GetData();
  timer.Reset();
  auto outCoords = clip.ProcessCellField(inCoords, DeviceAdapter());
  vtkm::Float64 processCoordinatesTime = timer.GetElapsedTime();
  output.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", outCoords));

  timer.Reset();
  for (vtkm::Id i = 0; i < input.GetNumberOfFields(); ++i)
  {
    vtkm::cont::Field inField = input.GetField(i);
    bool isCellField;
    switch (inField.GetAssociation())
    {
      case vtkm::cont::Field::Association::POINTS:
        isCellField = false;
        break;

      case vtkm::cont::Field::Association::CELL_SET:
        isCellField = true;
        break;

      default:
        continue;
    }

    vtkm::cont::DynamicArrayHandle outField;
    FieldMapper<DeviceAdapter> fieldMapper(outField, clip, isCellField);
    inField.GetData().CastAndCall(fieldMapper);
    output.AddField(vtkm::cont::Field(inField.GetName(), inField.GetAssociation(), outField));
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
