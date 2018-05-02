//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/cont/AssignerMultiBlock.h>

#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/MultiBlock.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include VTKM_DIY(diy/mpi.hpp)
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

#include <algorithm> // std::lower_bound
#include <numeric>   // std::iota

namespace vtkm
{
namespace cont
{

VTKM_CONT
AssignerMultiBlock::AssignerMultiBlock(const vtkm::cont::MultiBlock& mb)
  : AssignerMultiBlock(mb.GetNumberOfBlocks())
{
}

VTKM_CONT
AssignerMultiBlock::AssignerMultiBlock(vtkm::Id num_blocks)
  : diy::StaticAssigner(vtkm::cont::EnvironmentTracker::GetCommunicator().size(), 1)
  , IScanBlockCounts()
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  if (comm.size() > 1)
  {
    vtkm::Id iscan;
    diy::mpi::scan(comm, num_blocks, iscan, std::plus<vtkm::Id>());
    diy::mpi::all_gather(comm, iscan, this->IScanBlockCounts);
  }
  else
  {
    this->IScanBlockCounts.push_back(num_blocks);
  }

  this->set_nblocks(static_cast<int>(this->IScanBlockCounts.back()));
}

VTKM_CONT
void AssignerMultiBlock::local_gids(int my_rank, std::vector<int>& gids) const
{
  const size_t s_rank = static_cast<size_t>(my_rank);
  if (my_rank == 0)
  {
    assert(this->IScanBlockCounts.size() > 0);
    gids.resize(static_cast<size_t>(this->IScanBlockCounts[s_rank]));
    std::iota(gids.begin(), gids.end(), 0);
  }
  else if (my_rank > 0 && s_rank < this->IScanBlockCounts.size())
  {
    gids.resize(
      static_cast<size_t>(this->IScanBlockCounts[s_rank] - this->IScanBlockCounts[s_rank - 1]));
    std::iota(gids.begin(), gids.end(), static_cast<int>(this->IScanBlockCounts[s_rank - 1]));
  }
}

VTKM_CONT
int AssignerMultiBlock::rank(int gid) const
{
  return static_cast<int>(
    std::lower_bound(this->IScanBlockCounts.begin(), this->IScanBlockCounts.end(), gid + 1) -
    this->IScanBlockCounts.begin());
}


} // vtkm::cont
} // vtkm
