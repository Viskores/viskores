//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_BoundsMap_h
#define vtk_m_filter_flow_internal_BoundsMap_h

#include <vtkm/Bounds.h>
#include <vtkm/cont/AssignerPartitionedDataSet.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/PartitionedDataSet.h>

#include <vtkm/thirdparty/diy/diy.h>

#ifdef VTKM_ENABLE_MPI
#include <mpi.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>
#endif

#include <algorithm>
#include <iostream>
#include <set>
#include <vector>

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

class VTKM_ALWAYS_EXPORT BoundsMap
{
public:
  BoundsMap() {}

  BoundsMap(const vtkm::cont::DataSet& dataSet) { this->Init({ dataSet }); }

  BoundsMap(const vtkm::cont::DataSet& dataSet, const vtkm::Id& blockId)
  {
    this->Init({ dataSet }, { blockId });
  }

  BoundsMap(const std::vector<vtkm::cont::DataSet>& dataSets) { this->Init(dataSets); }

  BoundsMap(const vtkm::cont::PartitionedDataSet& pds) { this->Init(pds.GetPartitions()); }

  BoundsMap(const vtkm::cont::PartitionedDataSet& pds, const std::vector<vtkm::Id>& blockIds)
  {
    this->Init(pds.GetPartitions(), blockIds);
  }

  vtkm::Bounds GetGlobalBounds() const { return this->GlobalBounds; }

  vtkm::Bounds GetBlockBounds(vtkm::Id idx) const
  {
    VTKM_ASSERT(idx >= 0 && static_cast<std::size_t>(idx) < this->BlockBounds.size());

    return this->BlockBounds[static_cast<std::size_t>(idx)];
  }

  vtkm::Id GetLocalBlockId(vtkm::Id idx) const
  {
    VTKM_ASSERT(idx >= 0 && idx < this->LocalNumBlocks);
    return this->LocalIDs[static_cast<std::size_t>(idx)];
  }

  std::vector<int> FindRank(vtkm::Id blockId) const
  {
    auto it = this->BlockToRankMap.find(blockId);
    if (it == this->BlockToRankMap.end())
      return {};

    return it->second;
  }

  std::vector<vtkm::Id> FindBlocks(const vtkm::Vec3f& p) const { return this->FindBlocks(p, -1); }

  std::vector<vtkm::Id> FindBlocks(const vtkm::Vec3f& p,
                                   const std::vector<vtkm::Id>& ignoreBlocks) const
  {
    vtkm::Id ignoreID = (ignoreBlocks.empty() ? -1 : ignoreBlocks[0]);
    return FindBlocks(p, ignoreID);
  }

  std::vector<vtkm::Id> FindBlocks(const vtkm::Vec3f& p, vtkm::Id ignoreBlock) const
  {
    std::vector<vtkm::Id> blockIDs;
    if (this->GlobalBounds.Contains(p))
    {
      vtkm::Id blockId = 0;
      for (auto& it : this->BlockBounds)
      {
        if (blockId != ignoreBlock && it.Contains(p))
          blockIDs.emplace_back(blockId);
        blockId++;
      }
    }

    return blockIDs;
  }

  vtkm::Id GetTotalNumBlocks() const { return this->TotalNumBlocks; }
  vtkm::Id GetLocalNumBlocks() const { return this->LocalNumBlocks; }

private:
  void Init(const std::vector<vtkm::cont::DataSet>& dataSets, const std::vector<vtkm::Id>& blockIds)
  {
    if (dataSets.size() != blockIds.size())
      throw vtkm::cont::ErrorFilterExecution("Number of datasets and block ids must match");

    this->LocalIDs = blockIds;
    this->LocalNumBlocks = dataSets.size();

    vtkmdiy::mpi::communicator comm = vtkm::cont::EnvironmentTracker::GetCommunicator();

    //1. Get the min/max blockId
    vtkm::Id locMinId = 0, locMaxId = 0;
    if (!this->LocalIDs.empty())
    {
      locMinId = *std::min_element(this->LocalIDs.begin(), this->LocalIDs.end());
      locMaxId = *std::max_element(this->LocalIDs.begin(), this->LocalIDs.end());
    }

    vtkm::Id globalMinId = 0, globalMaxId = 0;

    vtkmdiy::mpi::all_reduce(comm, locMinId, globalMinId, vtkmdiy::mpi::minimum<vtkm::Id>{});
    vtkmdiy::mpi::all_reduce(comm, locMaxId, globalMaxId, vtkmdiy::mpi::maximum<vtkm::Id>{});
    if (globalMinId != 0 || (globalMaxId - globalMinId) < 1)
      throw vtkm::cont::ErrorFilterExecution("Invalid block ids");

    //2. Find out how many blocks everyone has.
    std::vector<vtkm::Id> locBlockCounts(comm.size(), 0), globalBlockCounts(comm.size(), 0);
    locBlockCounts[comm.rank()] = this->LocalIDs.size();
    vtkmdiy::mpi::all_reduce(comm, locBlockCounts, globalBlockCounts, std::plus<vtkm::Id>{});

    //note: there might be duplicates...
    vtkm::Id globalNumBlocks =
      std::accumulate(globalBlockCounts.begin(), globalBlockCounts.end(), vtkm::Id{ 0 });

    //3. given the counts per rank, calc offset for this rank.
    vtkm::Id offset = 0;
    for (int i = 0; i < comm.rank(); i++)
      offset += globalBlockCounts[i];

    //4. calc the blocks on each rank.
    std::vector<vtkm::Id> localBlockIds(globalNumBlocks, 0);
    vtkm::Id idx = 0;
    for (const auto& bid : this->LocalIDs)
      localBlockIds[offset + idx++] = bid;

    //use an MPI_Alltoallv instead.
    std::vector<vtkm::Id> globalBlockIds(globalNumBlocks, 0);
    vtkmdiy::mpi::all_reduce(comm, localBlockIds, globalBlockIds, std::plus<vtkm::Id>{});


    //5. create a rank -> blockId map.
    //  rankToBlockIds[rank] = {this->LocalIDs on rank}.
    std::vector<std::vector<vtkm::Id>> rankToBlockIds(comm.size());

    offset = 0;
    for (int rank = 0; rank < comm.size(); rank++)
    {
      vtkm::Id numBIds = globalBlockCounts[rank];
      rankToBlockIds[rank].resize(numBIds);
      for (vtkm::Id i = 0; i < numBIds; i++)
        rankToBlockIds[rank][i] = globalBlockIds[offset + i];

      offset += numBIds;
    }

    //6. there might be duplicates, so count number of UNIQUE blocks.
    std::set<vtkm::Id> globalUniqueBlockIds;
    globalUniqueBlockIds.insert(globalBlockIds.begin(), globalBlockIds.end());
    this->TotalNumBlocks = globalUniqueBlockIds.size();

    //Build a vector of :  blockIdsToRank[blockId] = {ranks that have this blockId}
    std::vector<std::vector<vtkm::Id>> blockIdsToRank(this->TotalNumBlocks);
    for (int rank = 0; rank < comm.size(); rank++)
    {
      for (const auto& bid : rankToBlockIds[rank])
      {
        blockIdsToRank[bid].push_back(rank);
        this->BlockToRankMap[bid].push_back(rank);
      }
    }

    this->Build(dataSets);
  }

  void Init(const std::vector<vtkm::cont::DataSet>& dataSets)
  {
    this->LocalNumBlocks = dataSets.size();

    vtkm::cont::AssignerPartitionedDataSet assigner(this->LocalNumBlocks);
    this->TotalNumBlocks = assigner.nblocks();
    std::vector<int> ids;

    vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
    assigner.local_gids(Comm.rank(), ids);
    for (const auto& i : ids)
      this->LocalIDs.emplace_back(static_cast<vtkm::Id>(i));

    for (vtkm::Id id = 0; id < this->TotalNumBlocks; id++)
      this->BlockToRankMap[id] = { assigner.rank(static_cast<int>(id)) };
    this->Build(dataSets);
  }

  void Build(const std::vector<vtkm::cont::DataSet>& dataSets)
  {
    std::vector<vtkm::Float64> vals(static_cast<std::size_t>(this->TotalNumBlocks * 6), 0);
    std::vector<vtkm::Float64> vals2(vals.size());

    std::vector<vtkm::Float64> localMins((this->TotalNumBlocks * 3),
                                         std::numeric_limits<vtkm::Float64>::max());
    std::vector<vtkm::Float64> localMaxs((this->TotalNumBlocks * 3),
                                         -std::numeric_limits<vtkm::Float64>::max());

    for (std::size_t i = 0; i < this->LocalIDs.size(); i++)
    {
      const vtkm::cont::DataSet& ds = dataSets[static_cast<std::size_t>(i)];
      vtkm::Bounds bounds = ds.GetCoordinateSystem().GetBounds();

      vtkm::Id localID = this->LocalIDs[i];
      localMins[localID * 3 + 0] = bounds.X.Min;
      localMins[localID * 3 + 1] = bounds.Y.Min;
      localMins[localID * 3 + 2] = bounds.Z.Min;
      localMaxs[localID * 3 + 0] = bounds.X.Max;
      localMaxs[localID * 3 + 1] = bounds.Y.Max;
      localMaxs[localID * 3 + 2] = bounds.Z.Max;
    }

    std::vector<vtkm::Float64> globalMins, globalMaxs;

#ifdef VTKM_ENABLE_MPI
    globalMins.resize(this->TotalNumBlocks * 3);
    globalMaxs.resize(this->TotalNumBlocks * 3);

    vtkmdiy::mpi::communicator comm = vtkm::cont::EnvironmentTracker::GetCommunicator();

    vtkmdiy::mpi::all_reduce(comm, localMins, globalMins, vtkmdiy::mpi::minimum<vtkm::Float64>{});
    vtkmdiy::mpi::all_reduce(comm, localMaxs, globalMaxs, vtkmdiy::mpi::maximum<vtkm::Float64>{});
#else
    globalMins = localMins;
    globalMaxs = localMaxs;
#endif

    this->BlockBounds.resize(static_cast<std::size_t>(this->TotalNumBlocks));
    this->GlobalBounds = vtkm::Bounds();

    std::size_t idx = 0;
    for (auto& block : this->BlockBounds)
    {
      block = vtkm::Bounds(globalMins[idx + 0],
                           globalMaxs[idx + 0],
                           globalMins[idx + 1],
                           globalMaxs[idx + 1],
                           globalMins[idx + 2],
                           globalMaxs[idx + 2]);
      this->GlobalBounds.Include(block);
      idx += 3;
    }
  }

  vtkm::Id LocalNumBlocks = 0;
  std::vector<vtkm::Id> LocalIDs;
  std::map<vtkm::Id, std::vector<vtkm::Int32>> BlockToRankMap;
  vtkm::Id TotalNumBlocks = 0;
  std::vector<vtkm::Bounds> BlockBounds;
  vtkm::Bounds GlobalBounds;
};

}
}
}
} // namespace vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_BoundsMap_h
