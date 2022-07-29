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
#include <vtkm/cont/Field.h>
#include <vtkm/cont/PartitionedDataSet.h>

#include <vtkm/thirdparty/diy/diy.h>

#ifdef VTKM_ENABLE_MPI
#include <mpi.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>
#endif

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
  BoundsMap()
    : LocalNumBlocks(0)
    , TotalNumBlocks(0)
  {
  }

  BoundsMap(const vtkm::cont::DataSet& dataSet)
    : LocalNumBlocks(1)
    , TotalNumBlocks(0)
  {
    this->Init({ dataSet });
  }

  BoundsMap(const std::vector<vtkm::cont::DataSet>& dataSets)
    : LocalNumBlocks(static_cast<vtkm::Id>(dataSets.size()))
    , TotalNumBlocks(0)
  {
    this->Init(dataSets);
  }

  BoundsMap(const vtkm::cont::PartitionedDataSet& pds)
    : LocalNumBlocks(pds.GetNumberOfPartitions())
    , TotalNumBlocks(0)
  {
    this->Init(pds.GetPartitions());
  }

  vtkm::Id GetLocalBlockId(vtkm::Id idx) const
  {
    VTKM_ASSERT(idx >= 0 && idx < this->LocalNumBlocks);
    return this->LocalIDs[static_cast<std::size_t>(idx)];
  }

  int FindRank(vtkm::Id blockId) const
  {
    auto it = this->BlockToRankMap.find(blockId);
    if (it == this->BlockToRankMap.end())
      return -1;
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
  void Init(const std::vector<vtkm::cont::DataSet>& dataSets)
  {
    vtkm::cont::AssignerPartitionedDataSet assigner(this->LocalNumBlocks);
    this->TotalNumBlocks = assigner.nblocks();
    std::vector<int> ids;

    vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
    assigner.local_gids(Comm.rank(), ids);
    for (const auto& i : ids)
      this->LocalIDs.emplace_back(static_cast<vtkm::Id>(i));

    for (vtkm::Id id = 0; id < this->TotalNumBlocks; id++)
      this->BlockToRankMap[id] = assigner.rank(static_cast<int>(id));
    this->Build(dataSets);
  }

  void Build(const std::vector<vtkm::cont::DataSet>& dataSets)
  {
    std::vector<vtkm::Float64> vals(static_cast<std::size_t>(this->TotalNumBlocks * 6), 0);
    std::vector<vtkm::Float64> vals2(vals.size());

    for (std::size_t i = 0; i < this->LocalIDs.size(); i++)
    {
      const vtkm::cont::DataSet& ds = dataSets[static_cast<std::size_t>(i)];
      vtkm::Bounds bounds = ds.GetCoordinateSystem().GetBounds();

      std::size_t idx = static_cast<std::size_t>(this->LocalIDs[i] * 6);
      vals[idx++] = bounds.X.Min;
      vals[idx++] = bounds.X.Max;
      vals[idx++] = bounds.Y.Min;
      vals[idx++] = bounds.Y.Max;
      vals[idx++] = bounds.Z.Min;
      vals[idx++] = bounds.Z.Max;
    }

#ifdef VTKM_ENABLE_MPI
    vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
    MPI_Comm mpiComm = vtkmdiy::mpi::mpi_cast(Comm.handle());
    MPI_Allreduce(vals.data(), vals2.data(), vals.size(), MPI_DOUBLE, MPI_SUM, mpiComm);
#else
    vals2 = vals;
#endif

    this->BlockBounds.resize(static_cast<std::size_t>(this->TotalNumBlocks));
    this->GlobalBounds = vtkm::Bounds();
    std::size_t idx = 0;
    for (auto& block : this->BlockBounds)
    {
      block = vtkm::Bounds(vals2[idx + 0],
                           vals2[idx + 1],
                           vals2[idx + 2],
                           vals2[idx + 3],
                           vals2[idx + 4],
                           vals2[idx + 5]);
      this->GlobalBounds.Include(block);
      idx += 6;
    }
  }

  vtkm::Id LocalNumBlocks;
  std::vector<vtkm::Id> LocalIDs;
  std::map<vtkm::Id, vtkm::Int32> BlockToRankMap;
  vtkm::Id TotalNumBlocks;
  std::vector<vtkm::Bounds> BlockBounds;
  vtkm::Bounds GlobalBounds;
};

}
}
}
} // namespace vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_BoundsMap_h
