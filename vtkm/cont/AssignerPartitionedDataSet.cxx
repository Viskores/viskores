//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/AssignerPartitionedDataSet.h>

#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/PartitionedDataSet.h>


#include <vtkm/thirdparty/diy/diy.h>

#include <algorithm> // std::lower_bound
#include <numeric>   // std::iota

namespace vtkm
{
namespace cont
{

VTKM_CONT
AssignerPartitionedDataSet::AssignerPartitionedDataSet(const vtkm::cont::PartitionedDataSet& pds)
  : AssignerPartitionedDataSet(pds.GetNumberOfPartitions())
{
}

VTKM_CONT
AssignerPartitionedDataSet::AssignerPartitionedDataSet(vtkm::Id num_partitions)
  : vtkmdiy::StaticAssigner(vtkm::cont::EnvironmentTracker::GetCommunicator().size(), 1)
  , IScanPartitionCounts()
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  if (comm.size() > 1)
  {
    vtkm::Id iscan;
    vtkmdiy::mpi::scan(comm, num_partitions, iscan, std::plus<vtkm::Id>());
    vtkmdiy::mpi::all_gather(comm, iscan, this->IScanPartitionCounts);
  }
  else
  {
    this->IScanPartitionCounts.push_back(num_partitions);
  }

  this->set_nblocks(static_cast<int>(this->IScanPartitionCounts.back()));
}

VTKM_CONT
AssignerPartitionedDataSet::~AssignerPartitionedDataSet()
{
}

VTKM_CONT
void AssignerPartitionedDataSet::local_gids(int my_rank, std::vector<int>& gids) const
{
  const size_t s_rank = static_cast<size_t>(my_rank);
  if (my_rank == 0)
  {
    assert(this->IScanPartitionCounts.size() > 0);
    gids.resize(static_cast<size_t>(this->IScanPartitionCounts[s_rank]));
    std::iota(gids.begin(), gids.end(), 0);
  }
  else if (my_rank > 0 && s_rank < this->IScanPartitionCounts.size())
  {
    gids.resize(static_cast<size_t>(this->IScanPartitionCounts[s_rank] -
                                    this->IScanPartitionCounts[s_rank - 1]));
    std::iota(gids.begin(), gids.end(), static_cast<int>(this->IScanPartitionCounts[s_rank - 1]));
  }
}

VTKM_CONT
int AssignerPartitionedDataSet::rank(int gid) const
{
  return static_cast<int>(std::lower_bound(this->IScanPartitionCounts.begin(),
                                           this->IScanPartitionCounts.end(),
                                           gid + 1) -
                          this->IScanPartitionCounts.begin());
}


} // vtkm::cont
} // vtkm
