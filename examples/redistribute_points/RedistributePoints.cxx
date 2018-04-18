//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include VTKM_DIY(diy/mpi.hpp)
VTKM_THIRDPARTY_POST_INCLUDE

#include "RedistributePoints.h"

#include <sstream>
using std::cout;
using std::endl;

int main(int argc, char* argv[])
{
  diy::mpi::environment env(argc, argv);
  auto comm = diy::mpi::communicator(MPI_COMM_WORLD);
  vtkm::cont::EnvironmentTracker::SetCommunicator(comm);

  if (argc != 3)
  {
    cout << "Usage: " << endl
         << "$ " << argv[0] << " <input-vtk-file> <output-file-prefix>" << endl;
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
