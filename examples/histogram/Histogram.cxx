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

/*
 * This example demonstrates how one can write a filter that uses MPI
 * for hybrid-parallelism. The `viskores::filter::Histogram` is another approach for
 * implementing the same that uses DIY. This example doesn't use DIY, instead
 * uses MPI calls directly.
 */

#include "HistogramMPI.h"

#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/Initialize.h>

#include <viskores/thirdparty/diy/diy.h>

#include <mpi.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace
{
template <typename T>
VISKORES_CONT viskores::cont::ArrayHandle<T> CreateArray(T min, T max, viskores::Id numVals)
{
  std::mt19937 gen;
  std::uniform_real_distribution<double> dis(static_cast<double>(min), static_cast<double>(max));

  viskores::cont::ArrayHandle<T> handle;
  handle.Allocate(numVals);

  std::generate(viskores::cont::ArrayPortalToIteratorBegin(handle.WritePortal()),
                viskores::cont::ArrayPortalToIteratorEnd(handle.WritePortal()),
                [&]() { return static_cast<T>(dis(gen)); });
  return handle;
}
}

int main(int argc, char* argv[])
{
  //parse out all viskores related command line options
  auto opts =
    viskores::cont::InitializeOptions::DefaultAnyDevice | viskores::cont::InitializeOptions::Strict;
  viskores::cont::Initialize(argc, argv, opts);

  // setup MPI environment.
  viskoresdiy::mpi::environment env(argc, argv); // will finalize on destruction

  viskoresdiy::mpi::communicator world; // the default is MPI_COMM_WORLD

  // tell Viskores the communicator to use.
  viskores::cont::EnvironmentTracker::SetCommunicator(world);

  int rank = world.rank();
  int size = world.size();

  if (argc != 2)
  {
    if (rank == 0)
    {
      std::cout << "Usage: " << std::endl << "$ " << argv[0] << " <num-bins>" << std::endl;
    }
    return EXIT_FAILURE;
  }

  const viskores::Id num_bins = static_cast<viskores::Id>(std::atoi(argv[1]));
  const viskores::Id numVals = 1024;

  viskores::cont::PartitionedDataSet pds;
  viskores::cont::DataSet ds;
  ds.AddPointField("pointvar", CreateArray(-1024, 1024, numVals));
  pds.AppendPartition(ds);

  example::HistogramMPI histogram;
  histogram.SetActiveField("pointvar");
  histogram.SetNumberOfBins(std::max<viskores::Id>(1, num_bins));
  viskores::cont::PartitionedDataSet result = histogram.Execute(pds);

  viskores::cont::ArrayHandle<viskores::Id> bins;
  result.GetPartition(0).GetField("histogram").GetData().AsArrayHandle(bins);
  auto binPortal = bins.ReadPortal();
  if (rank == 0)
  {
    // print histogram.
    std::cout << "Histogram (" << num_bins << ")" << std::endl;
    viskores::Id count = 0;
    for (viskores::Id cc = 0; cc < num_bins; ++cc)
    {
      std::cout << "  bin[" << cc << "] = " << binPortal.Get(cc) << std::endl;
      count += binPortal.Get(cc);
    }
    if (count != numVals * size)
    {
      std::cout << "ERROR: bins mismatched!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
