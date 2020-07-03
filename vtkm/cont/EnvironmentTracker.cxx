//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/EnvironmentTracker.h>

#include <vtkm/thirdparty/diy/diy.h>

#include <memory>

namespace vtkm
{
namespace cont
{
namespace internal
{
static std::unique_ptr<vtkmdiy::mpi::communicator> GlobalCommuncator;
}

void EnvironmentTracker::SetCommunicator(const vtkmdiy::mpi::communicator& comm)
{
  if (!internal::GlobalCommuncator)
  {
    internal::GlobalCommuncator.reset(new vtkmdiy::mpi::communicator(comm));
  }
  else
  {
    *internal::GlobalCommuncator = comm;
  }
}

const vtkmdiy::mpi::communicator& EnvironmentTracker::GetCommunicator()
{
  if (!internal::GlobalCommuncator)
  {
    internal::GlobalCommuncator.reset(new vtkmdiy::mpi::communicator());
  }
  return *internal::GlobalCommuncator;
}
} // namespace vtkm::cont
} // namespace vtkm
