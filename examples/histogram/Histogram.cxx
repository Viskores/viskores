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

/*
 * This example demonstrates how one can write a filter that uses MPI
 * for hybrid-parallelism. The `vtkm::filter::Histogram` is another approach for
 * implementing the same that uses DIY. This example doesn't use DIY, instead
 * uses MPI calls directly.
 */

#include "HistogramMPI.h"

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/EnvironmentTracker.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include VTKM_DIY(diy/mpi.hpp)
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

#include <mpi.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace
{
template <typename T>
VTKM_CONT vtkm::cont::ArrayHandle<T> CreateArray(T min, T max, vtkm::Id numVals)
{
  std::mt19937 gen;
  std::uniform_real_distribution<double> dis(static_cast<double>(min), static_cast<double>(max));

  vtkm::cont::ArrayHandle<T> handle;
  handle.Allocate(numVals);

  std::generate(vtkm::cont::ArrayPortalToIteratorBegin(handle.GetPortalControl()),
                vtkm::cont::ArrayPortalToIteratorEnd(handle.GetPortalControl()),
                [&]() { return static_cast<T>(dis(gen)); });
  return handle;
}
}

int main(int argc, char* argv[])
{
  // setup MPI environment.
  MPI_Init(&argc, &argv);

  // tell VTK-m the communicator to use.
  vtkm::cont::EnvironmentTracker::SetCommunicator(diy::mpi::communicator(MPI_COMM_WORLD));

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc != 2)
  {
    if (rank == 0)
    {
      std::cout << "Usage: " << std::endl << "$ " << argv[0] << " <num-bins>" << std::endl;
    }
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  const vtkm::Id num_bins = static_cast<vtkm::Id>(std::atoi(argv[1]));
  const vtkm::Id numVals = 1024;

  vtkm::cont::MultiBlock mb;
  vtkm::cont::DataSet ds;
  vtkm::cont::DataSetFieldAdd::AddPointField(ds, "pointvar", CreateArray(-1024, 1024, numVals));
  mb.AddBlock(ds);

  example::HistogramMPI histogram;
  histogram.SetActiveField("pointvar");
  histogram.SetNumberOfBins(std::max<vtkm::Id>(1, num_bins));
  vtkm::cont::MultiBlock result = histogram.Execute(mb);

  vtkm::cont::ArrayHandle<vtkm::Id> bins;
  result.GetBlock(0).GetField("histogram").GetData().CopyTo(bins);
  auto binPortal = bins.GetPortalConstControl();
  if (rank == 0)
  {
    // print histogram.
    std::cout << "Histogram (" << num_bins << ")" << std::endl;
    vtkm::Id count = 0;
    for (vtkm::Id cc = 0; cc < num_bins; ++cc)
    {
      std::cout << "  bin[" << cc << "] = " << binPortal.Get(cc) << std::endl;
      count += binPortal.Get(cc);
    }
    if (count != numVals * size)
    {
      std::cout << "ERROR: bins mismatched!" << std::endl;
      MPI_Finalize();
      return EXIT_FAILURE;
    }
  }

  MPI_Finalize();
  return EXIT_SUCCESS;
}
