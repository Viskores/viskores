//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/DataSet.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/Initialize.h>

#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/thirdparty/diy/diy.h>

#include "RedistributePoints.h"

#include <sstream>
using std::cout;
using std::endl;

int main(int argc, char* argv[])
{
  // Process viskores general args
  auto opts = viskores::cont::InitializeOptions::DefaultAnyDevice;
  auto config = viskores::cont::Initialize(argc, argv, opts);

  viskoresdiy::mpi::environment env(argc, argv);
  viskoresdiy::mpi::communicator comm;
  viskores::cont::EnvironmentTracker::SetCommunicator(comm);

  if (argc != 3)
  {
    cout << "Usage: " << endl
         << "$ " << argv[0] << " [options] <input-vtk-file> <output-file-prefix>" << endl;
    cout << config.Usage << endl;
    return EXIT_FAILURE;
  }

  viskores::cont::DataSet input;
  if (comm.rank() == 0)
  {
    viskores::io::VTKDataSetReader reader(argv[1]);
    input = reader.ReadDataSet();
  }

  example::RedistributePoints redistributor;
  auto output = redistributor.Execute(input);

  std::ostringstream str;
  str << argv[2] << "-" << comm.rank() << ".vtk";

  viskores::io::VTKDataSetWriter writer(str.str());
  writer.WriteDataSet(output);
  return EXIT_SUCCESS;
}
