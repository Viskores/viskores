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


int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;

  // SetLogLevelName must be called before Initialize
  vtkm::cont::SetLogLevelName(vtkm::cont::LogLevel::UserFirst, "tut_log");
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  const std::string input = "data/kitchen.vtk";
  vtkm::io::VTKDataSetReader reader(input);
  VTKM_LOG_F(vtkm::cont::LogLevel::Info, "Reading from file %s", input.c_str());
  vtkm::cont::DataSet ds_from_file = reader.ReadDataSet();
  VTKM_LOG_F(vtkm::cont::LogLevel::Info, "Done reading from file %s", input.c_str());

  const std::string output = "out_logging.vtk";
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Writing to file" << output);
  vtkm::io::VTKDataSetWriter writer(output);
  writer.WriteDataSet(ds_from_file);
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Done writing to file" << output);

  return 0;
}
