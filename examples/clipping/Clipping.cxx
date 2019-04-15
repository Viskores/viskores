//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Initialize.h>
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

struct FieldMapper
{
  vtkm::cont::VariantArrayHandle& Output;
  vtkm::worklet::Clip& Worklet;
  bool IsCellField;

  FieldMapper(vtkm::cont::VariantArrayHandle& output,
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
      this->Output = this->Worklet.ProcessCellField(input);
    }
    else
    {
      this->Output = this->Worklet.ProcessPointField(input);
    }
  }
};

} // end anon namespace

int main(int argc, char* argv[])
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);
  if (argc < 4)
  {
    std::cout << "Usage: " << std::endl
              << "$ " << argv[0]
              << " [-d device] <input_vtk_file> [fieldName] <isoval> <output_vtk_file>"
              << std::endl;
    return 1;
  }

  vtkm::io::reader::VTKDataSetReader reader(argv[1]);
  vtkm::cont::DataSet input = reader.ReadDataSet();

  vtkm::cont::Field scalarField = (argc == 5) ? input.GetField(argv[2]) : input.GetField(0);

  vtkm::Float32 clipValue = std::stof(argv[argc - 2]);
  vtkm::worklet::Clip clip;

  vtkm::cont::Timer total;
  total.Start();
  vtkm::cont::Timer timer;
  timer.Start();
  bool invertClip = false;
  vtkm::cont::CellSetExplicit<> outputCellSet =
    clip.Run(input.GetCellSet(0),
             scalarField.GetData().ResetTypes(vtkm::TypeListTagScalarAll()),
             clipValue,
             invertClip);
  vtkm::Float64 clipTime = timer.GetElapsedTime();

  vtkm::cont::DataSet output;
  output.AddCellSet(outputCellSet);


  auto inCoords = input.GetCoordinateSystem(0).GetData();
  timer.Start();
  auto outCoords = clip.ProcessCellField(inCoords);
  vtkm::Float64 processCoordinatesTime = timer.GetElapsedTime();
  output.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", outCoords));

  timer.Start();
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

    vtkm::cont::VariantArrayHandle outField;
    FieldMapper fieldMapper(outField, clip, isCellField);
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
