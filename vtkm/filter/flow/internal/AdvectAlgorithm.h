//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_AdvectAlgorithm_h
#define vtk_m_filter_flow_internal_AdvectAlgorithm_h

#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/filter/flow/internal/AdvectAlgorithmTerminator.h>
#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/internal/DataSetIntegrator.h>
#include <vtkm/filter/flow/internal/ParticleExchanger.h>
#include <vtkm/filter/flow/internal/ParticleMessenger.h>
#ifdef VTKM_ENABLE_MPI
#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>
#endif

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

/*
ParticleMessenger::Exchange()
 - SendParticles(outData--> map[dstRank]=vector of pairs);
 -- SendParticles(map...)
 --- for each m : map  SendParticles(m);
 ---- SendParticles(dst, container)
 ----- serialize, SendData(dst, buff);
 ------ SendDataAsync(dst,buff)
 -------  header??, req=mpi_isend(), store req.

 - RecvAny(data, block);
 -- RecvData(tags, buffers, block)
 --- RecvDataAsyncProbe(tag, buffers, block)
 ---- while (true)
 ----- if block:  MPI_Probe() msgReceived=true
 ----- else : MPI_Iprobe msgReceived = check
 ----- if msgRecvd: MPI_Get_count(), MPI_Recv(), buffers, blockAndWait=false


*/

template <typename DSIType>
class AdvectAlgorithm
{
public:
  using ParticleType = typename DSIType::PType;

  AdvectAlgorithm(const vtkm::filter::flow::internal::BoundsMap& bm,
                  std::vector<DSIType>& blocks,
                  bool useAsyncComm)
    : Blocks(blocks)
    , BoundsMap(bm)
    , NumRanks(this->Comm.size())
    , Rank(this->Comm.rank())
    , UseAsynchronousCommunication(useAsyncComm)
    , Terminator(this->Comm)
    , Exchanger(this->Comm)
  {
  }

  void Execute(const vtkm::cont::ArrayHandle<ParticleType>& seeds, vtkm::FloatDefault stepSize)
  {
    this->SetStepSize(stepSize);
    this->SetSeeds(seeds);
    this->Go();
  }

  vtkm::cont::PartitionedDataSet GetOutput() const
  {
    vtkm::cont::PartitionedDataSet output;
    for (const auto& b : this->Blocks)
    {
      vtkm::cont::DataSet ds;
      if (b.GetOutput(ds))
        output.AppendPartition(ds);
    }
    return output;
  }

  void SetStepSize(vtkm::FloatDefault stepSize) { this->StepSize = stepSize; }

  void SetSeeds(const vtkm::cont::ArrayHandle<ParticleType>& seeds)
  {
    this->ClearParticles();

    vtkm::Id n = seeds.GetNumberOfValues();
    auto portal = seeds.ReadPortal();

    std::vector<std::vector<vtkm::Id>> blockIDs;
    std::vector<ParticleType> particles;
    for (vtkm::Id i = 0; i < n; i++)
    {
      const ParticleType p = portal.Get(i);
      std::vector<vtkm::Id> ids = this->BoundsMap.FindBlocks(p.GetPosition());

      //Note: For duplicate blocks, this will give the seeds to the rank that are first in the list.
      if (!ids.empty())
      {
        auto ranks = this->BoundsMap.FindRank(ids[0]);
        if (!ranks.empty() && this->Rank == ranks[0])
        {
          particles.emplace_back(p);
          blockIDs.emplace_back(ids);
        }
      }
    }
    this->SetSeedArray(particles, blockIDs);
  }

  //Advect all the particles.
  virtual void Go()
  {
    vtkm::filter::flow::internal::ParticleMessenger<ParticleType> messenger(
      this->Comm, this->UseAsynchronousCommunication, this->BoundsMap, 1, 128);

    while (!this->Terminator.Done())
    {
      std::vector<ParticleType> v;
      vtkm::Id blockId = -1;
      if (this->GetActiveParticles(v, blockId))
      {
        //make this a pointer to avoid the copy?
        auto& block = this->GetDataSet(blockId);
        DSIHelperInfo<ParticleType> bb(v, this->BoundsMap, this->ParticleBlockIDsMap);
        block.Advect(bb, this->StepSize);
        this->UpdateResult(bb);
      }

      this->Communicate(messenger);
      this->Terminator.Control(!this->Active.empty());
    }
  }

  virtual void ClearParticles()
  {
    this->Active.clear();
    this->Inactive.clear();
    this->ParticleBlockIDsMap.clear();
  }

  DataSetIntegrator<DSIType, ParticleType>& GetDataSet(vtkm::Id id)
  {
    for (auto& it : this->Blocks)
      if (it.GetID() == id)
        return it;

    throw vtkm::cont::ErrorFilterExecution("Bad block");
  }

  virtual void SetSeedArray(const std::vector<ParticleType>& particles,
                            const std::vector<std::vector<vtkm::Id>>& blockIds)
  {
    VTKM_ASSERT(particles.size() == blockIds.size());

    auto pit = particles.begin();
    auto bit = blockIds.begin();
    while (pit != particles.end() && bit != blockIds.end())
    {
      vtkm::Id blockId0 = (*bit)[0];
      this->ParticleBlockIDsMap[pit->GetID()] = *bit;
      if (this->Active.find(blockId0) == this->Active.end())
        this->Active[blockId0] = { *pit };
      else
        this->Active[blockId0].emplace_back(*pit);
      pit++;
      bit++;
    }
  }

  virtual bool GetActiveParticles(std::vector<ParticleType>& particles, vtkm::Id& blockId)
  {
    particles.clear();
    blockId = -1;
    if (this->Active.empty())
      return false;

    //If only one, return it.
    if (this->Active.size() == 1)
    {
      blockId = this->Active.begin()->first;
      particles = std::move(this->Active.begin()->second);
      this->Active.clear();
    }
    else
    {
      //Find the blockId with the most particles.
      std::size_t maxNum = 0;
      auto maxIt = this->Active.end();
      for (auto it = this->Active.begin(); it != this->Active.end(); it++)
      {
        auto sz = it->second.size();
        if (sz > maxNum)
        {
          maxNum = sz;
          maxIt = it;
        }
      }

      if (maxNum == 0)
      {
        this->Active.clear();
        return false;
      }

      blockId = maxIt->first;
      particles = std::move(maxIt->second);
      this->Active.erase(maxIt);
    }

    return !particles.empty();
  }

  void ExchangeParticles()
  {
    //    this->Exchanger.Exchange(outgoing, outgoingRanks, this->ParticleBlockIDsMap, incoming, incomingBlockIDs, block);
  }

  void Communicate(vtkm::filter::flow::internal::ParticleMessenger<ParticleType>& messenger)
  {
    std::vector<ParticleType> outgoing;
    std::vector<vtkm::Id> outgoingRanks;

    this->GetOutgoingParticles(outgoing, outgoingRanks);

    std::vector<ParticleType> incoming;
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> incomingBlockIDs;

    bool block = false;
#ifdef VTKM_ENABLE_MPI
    block = this->GetBlockAndWait(messenger.UsingSyncCommunication());
#endif

    //    this->Exchanger.Exchange(outgoing, outgoingRanks, this->ParticleBlockIDsMap, incoming, incomingBlockIDs, block);

    vtkm::Id numTermMessages;
    messenger.Exchange(outgoing,
                       outgoingRanks,
                       this->ParticleBlockIDsMap,
                       0,
                       incoming,
                       incomingBlockIDs,
                       numTermMessages,
                       block);

    //Cleanup what was sent.
    for (const auto& p : outgoing)
      this->ParticleBlockIDsMap.erase(p.GetID());

    this->UpdateActive(incoming, incomingBlockIDs);
  }

  void GetOutgoingParticles(std::vector<ParticleType>& outgoing,
                            std::vector<vtkm::Id>& outgoingRanks)
  {
    outgoing.clear();
    outgoingRanks.clear();

    outgoing.reserve(this->Inactive.size());
    outgoingRanks.reserve(this->Inactive.size());

    std::vector<ParticleType> particlesStaying;
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> particlesStayingBlockIDs;
    //Send out Everything.
    for (const auto& p : this->Inactive)
    {
      const auto& bid = this->ParticleBlockIDsMap[p.GetID()];
      VTKM_ASSERT(!bid.empty());

      auto ranks = this->BoundsMap.FindRank(bid[0]);
      VTKM_ASSERT(!ranks.empty());

      if (ranks.size() == 1)
      {
        if (ranks[0] == this->Rank)
        {
          particlesStaying.emplace_back(p);
          particlesStayingBlockIDs[p.GetID()] = this->ParticleBlockIDsMap[p.GetID()];
        }
        else
        {
          outgoing.emplace_back(p);
          outgoingRanks.emplace_back(ranks[0]);
        }
      }
      else
      {
        //Decide where it should go...

        //Random selection:
        vtkm::Id outRank = std::rand() % ranks.size();
        if (outRank == this->Rank)
        {
          particlesStayingBlockIDs[p.GetID()] = this->ParticleBlockIDsMap[p.GetID()];
          particlesStaying.emplace_back(p);
        }
        else
        {
          outgoing.emplace_back(p);
          outgoingRanks.emplace_back(outRank);
        }
      }
    }

    this->Inactive.clear();
    VTKM_ASSERT(outgoing.size() == outgoingRanks.size());

    VTKM_ASSERT(particlesStaying.size() == particlesStayingBlockIDs.size());
    if (!particlesStaying.empty())
      this->UpdateActive(particlesStaying, particlesStayingBlockIDs);
  }

  virtual void UpdateActive(const std::vector<ParticleType>& particles,
                            const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap)
  {
    VTKM_ASSERT(particles.size() == idsMap.size());

    if (!particles.empty())
    {
      this->Terminator.AddWork();

      for (auto pit = particles.begin(); pit != particles.end(); pit++)
      {
        vtkm::Id particleID = pit->GetID();
        const auto& it = idsMap.find(particleID);
        VTKM_ASSERT(it != idsMap.end() && !it->second.empty());
        vtkm::Id blockId = it->second[0];
        this->Active[blockId].emplace_back(*pit);
      }

      for (const auto& it : idsMap)
        this->ParticleBlockIDsMap[it.first] = it.second;
    }
  }

  virtual void UpdateInactive(const std::vector<ParticleType>& particles,
                              const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap)
  {
    VTKM_ASSERT(particles.size() == idsMap.size());

    this->Inactive.insert(this->Inactive.end(), particles.begin(), particles.end());
    for (const auto& it : idsMap)
      this->ParticleBlockIDsMap[it.first] = it.second;
  }

  vtkm::Id UpdateResult(const DSIHelperInfo<ParticleType>& stuff)
  {
    this->UpdateActive(stuff.InBounds.Particles, stuff.InBounds.BlockIDs);
    this->UpdateInactive(stuff.OutOfBounds.Particles, stuff.OutOfBounds.BlockIDs);

    vtkm::Id numTerm = static_cast<vtkm::Id>(stuff.TermID.size());
    //Update terminated particles.
    if (numTerm > 0)
    {
      for (const auto& id : stuff.TermID)
        this->ParticleBlockIDsMap.erase(id);
    }

    return numTerm;
  }


  virtual bool GetBlockAndWait(const bool& syncComm)
  {
    bool haveNoWork = this->Active.empty() && this->Inactive.empty();

    //Using syncronous communication we should only block and wait if we have no particles
    if (syncComm)
    {
      return haveNoWork;
    }
    else
    {
      //Otherwise, for asyncronous communication, there are only two cases where blocking would deadlock.
      //1. There are active particles.
      //2. numLocalTerm + this->TotalNumberOfTerminatedParticles == this->TotalNumberOfParticles
      //So, if neither are true, we can safely block and wait for communication to come in.

      //      if (this->Terminator.State == AdvectAlgorithmTerminator::AdvectAlgorithmTerminatorState::STATE_2)
      //        return true;

      //      if (haveNoWork && (numLocalTerm + this->TotalNumTerminatedParticles < this->TotalNumParticles))
      //        return true;

      return false;
    }
  }

  //Member data
  // {blockId, std::vector of particles}
  std::unordered_map<vtkm::Id, std::vector<ParticleType>> Active;
  std::vector<DSIType> Blocks;
  vtkm::filter::flow::internal::BoundsMap BoundsMap;
  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  std::vector<ParticleType> Inactive;
  vtkm::Id MaxNumberOfSteps = 0;
  vtkm::Id NumRanks;
  //{particleId : {block IDs}}
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;
  vtkm::Id Rank;
  vtkm::FloatDefault StepSize;
  bool UseAsynchronousCommunication = true;
  AdvectAlgorithmTerminator Terminator;

  ParticleExchanger<ParticleType> Exchanger;
};

}
}
}
} //vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_AdvectAlgorithm_h
