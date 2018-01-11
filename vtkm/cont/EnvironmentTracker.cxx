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

#if defined(VTKM_ENABLE_MPI)

// clang-format off
#include <vtkm/thirdparty/diy/Configure.h>
#include VTKM_DIY(diy/mpi.hpp)
// clang-format on

#else
namespace diy
{
namespace mpi
{
class communicator
{
};
}
}
#endif

namespace vtkm
{
namespace cont
{
#if defined(VTKM_ENABLE_MPI)
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
  return vtkm::cont::internal::GlobalCommuncator;
}
#else
void EnvironmentTracker::SetCommunicator(const diy::mpi::communicator&)
{
}

const diy::mpi::communicator& EnvironmentTracker::GetCommunicator()
{
  static diy::mpi::communicator tmp;
  return tmp;
}
#endif
} // namespace vtkm::cont
} // namespace vtkm
