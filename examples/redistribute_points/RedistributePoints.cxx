//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/Initialize.h>

#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

#include <vtkm/thirdparty/diy/diy.h>

#include "RedistributePoints.h"

#include <sstream>
using std::cout;
using std::endl;

int main(int argc, char* argv[])
{
  // Process vtk-m general args
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  auto config = vtkm::cont::Initialize(argc, argv, opts);

  vtkmdiy::mpi::environment env(argc, argv);
  auto comm = vtkmdiy::mpi::communicator(MPI_COMM_WORLD);
  vtkm::cont::EnvironmentTracker::SetCommunicator(comm);

  if (argc != 3)
  {
    cout << "Usage: " << endl
         << "$ " << argv[0] << " [options] <input-vtk-file> <output-file-prefix>" << endl;
    cout << config.Usage << endl;
    return EXIT_FAILURE;
  }

  vtkm::cont::DataSet input;
  if (comm.rank() == 0)
  {
    vtkm::io::reader::VTKDataSetReader reader(argv[1]);
    input = reader.ReadDataSet();
  }

  example::RedistributePoints redistributor;
  auto output = redistributor.Execute(input);

  std::ostringstream str;
  str << argv[2] << "-" << comm.rank() << ".vtk";

  vtkm::io::writer::VTKDataSetWriter writer(str.str());
  writer.WriteDataSet(output);
  return EXIT_SUCCESS;
}
