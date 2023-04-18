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
#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/internal/DataSetIntegrator.h>
#include <vtkm/filter/flow/internal/ParticleMessenger.h>

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename DSIType, template <typename> class ResultType, typename ParticleType>
class AdvectAlgorithm
{
public:
  AdvectAlgorithm(const vtkm::filter::flow::internal::BoundsMap& bm,
                  std::vector<DSIType>& blocks,
                  bool useAsyncComm)
    : Blocks(blocks)
    , BoundsMap(bm)
    , NumRanks(this->Comm.size())
    , Rank(this->Comm.rank())
    , UseAsynchronousCommunication(useAsyncComm)
  {
  }

  void Execute(vtkm::Id numSteps,
               vtkm::FloatDefault stepSize,
               const vtkm::cont::ArrayHandle<ParticleType>& seeds)
  {
    this->SetNumberOfSteps(numSteps);
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
      if (b.template GetOutput<ParticleType>(ds))
        output.AppendPartition(ds);
    }

    return output;
  }

  void SetStepSize(vtkm::FloatDefault stepSize) { this->StepSize = stepSize; }
  void SetNumberOfSteps(vtkm::Id numSteps) { this->NumberOfSteps = numSteps; }
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

    vtkm::Id nLocal = static_cast<vtkm::Id>(this->Active.size() + this->Inactive.size());
    this->ComputeTotalNumParticles(nLocal);

    while (this->TotalNumTerminatedParticles < this->TotalNumParticles)
    {
      std::vector<ParticleType> v;
      vtkm::Id numTerm = 0, blockId = -1;
      if (this->GetActiveParticles(v, blockId))
      {
        //make this a pointer to avoid the copy?
        auto& block = this->GetDataSet(blockId);
        DSIHelperInfoType bb =
          DSIHelperInfo<ParticleType>(v, this->BoundsMap, this->ParticleBlockIDsMap);
        block.Advect(bb, this->StepSize, this->NumberOfSteps);
        numTerm = this->UpdateResult(bb.Get<DSIHelperInfo<ParticleType>>());
      }

      vtkm::Id numTermMessages = 0;
      this->Communicate(messenger, numTerm, numTermMessages);

      this->TotalNumTerminatedParticles += (numTerm + numTermMessages);
      if (this->TotalNumTerminatedParticles > this->TotalNumParticles)
        throw vtkm::cont::ErrorFilterExecution("Particle count error");
    }
  }

  virtual void ClearParticles()
  {
    this->Active.clear();
    this->Inactive.clear();
    this->ParticleBlockIDsMap.clear();
  }

  void ComputeTotalNumParticles(const vtkm::Id& numLocal)
  {
    long long total = static_cast<long long>(numLocal);
#ifdef VTKM_ENABLE_MPI
    MPI_Comm mpiComm = vtkmdiy::mpi::mpi_cast(this->Comm.handle());
    MPI_Allreduce(MPI_IN_PLACE, &total, 1, MPI_LONG_LONG, MPI_SUM, mpiComm);
#endif
    this->TotalNumParticles = static_cast<vtkm::Id>(total);
  }

  DataSetIntegrator<DSIType>& GetDataSet(vtkm::Id id)
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
      this->ParticleBlockIDsMap[pit->GetID()] = *bit;
      pit++;
      bit++;
    }

    //Update Active
    this->Active.insert(this->Active.end(), particles.begin(), particles.end());
  }

  virtual bool GetActiveParticles(std::vector<ParticleType>& particles, vtkm::Id& blockId)
  {
    //DRP:  particles = std::move(this->Active2[blockId]);
    particles.clear();
    blockId = -1;
    if (this->Active.empty())
      return false;

    blockId = this->ParticleBlockIDsMap[this->Active.front().GetID()][0];
    auto it = this->Active.begin();
    while (it != this->Active.end())
    {
      auto p = *it;
      if (blockId == this->ParticleBlockIDsMap[p.GetID()][0])
      {
        particles.emplace_back(p);
        it = this->Active.erase(it);
      }
      else
        it++;
    }

    return !particles.empty();
  }

  void Communicate(vtkm::filter::flow::internal::ParticleMessenger<ParticleType>& messenger,
                   vtkm::Id numLocalTerminations,
                   vtkm::Id& numTermMessages)
  {
    std::vector<ParticleType> outgoing;
    std::vector<vtkm::Id> outgoingRanks;

    this->GetOutgoingParticles(outgoing, outgoingRanks);

    std::vector<ParticleType> incoming;
    std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> incomingBlockIDs;
    numTermMessages = 0;
    bool block = false;
#ifdef VTKM_ENABLE_MPI
    block = this->GetBlockAndWait(messenger.UsingSyncCommunication(), numLocalTerminations);
#endif

    messenger.Exchange(outgoing,
                       outgoingRanks,
                       this->ParticleBlockIDsMap,
                       numLocalTerminations,
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
    this->Update(this->Active, particles, idsMap);
  }

  virtual void UpdateInactive(const std::vector<ParticleType>& particles,
                              const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap)
  {
    this->Update(this->Inactive, particles, idsMap);
  }

  void Update(std::vector<ParticleType>& arr,
              const std::vector<ParticleType>& particles,
              const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& idsMap)
  {
    VTKM_ASSERT(particles.size() == idsMap.size());

    arr.insert(arr.end(), particles.begin(), particles.end());
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


  virtual bool GetBlockAndWait(const bool& syncComm, const vtkm::Id& numLocalTerm)
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

      if (haveNoWork &&
          (numLocalTerm + this->TotalNumTerminatedParticles < this->TotalNumParticles))
        return true;

      return false;
    }
  }

  //Member data
  std::vector<ParticleType> Active;
  std::vector<DSIType> Blocks;
  vtkm::filter::flow::internal::BoundsMap BoundsMap;
  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  std::vector<ParticleType> Inactive;
  vtkm::Id NumberOfSteps;
  vtkm::Id NumRanks;
  //{particleId : {block IDs}}
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;
  vtkm::Id Rank;
  vtkm::FloatDefault StepSize;
  vtkm::Id TotalNumParticles = 0;
  vtkm::Id TotalNumTerminatedParticles = 0;
  bool UseAsynchronousCommunication = true;
};

}
}
}
} //vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_AdvectAlgorithm_h
