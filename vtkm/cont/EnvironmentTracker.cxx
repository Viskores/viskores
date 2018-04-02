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
#include <vtkm/cont/EnvironmentTracker.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include VTKM_DIY(diy/mpi.hpp)
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace vtkm
{
namespace cont
{
namespace internal
{
static diy::mpi::communicator GlobalCommuncator(MPI_COMM_NULL);
}

void EnvironmentTracker::SetCommunicator(const diy::mpi::communicator& comm)
{
  vtkm::cont::internal::GlobalCommuncator = comm;
}

const diy::mpi::communicator& EnvironmentTracker::GetCommunicator()
{
#ifndef DIY_NO_MPI
  int flag;
  MPI_Initialized(&flag);
  if (!flag)
  {
    int argc = 0;
    char** argv = nullptr;
    MPI_Init(&argc, &argv);
    internal::GlobalCommuncator = diy::mpi::communicator(MPI_COMM_WORLD);
  }
#endif
  return vtkm::cont::internal::GlobalCommuncator;
}
} // namespace vtkm::cont
} // namespace vtkm
