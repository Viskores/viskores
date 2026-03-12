//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_filter_flow_internal_AdvectAlgorithm_h
#define viskores_filter_flow_internal_AdvectAlgorithm_h


#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/filter/flow/internal/DataSetIntegrator.h>
#ifdef VISKORES_ENABLE_MPI
#include <viskores/filter/flow/internal/AdvectAlgorithmTerminator.h>
#include <viskores/filter/flow/internal/ParticleExchanger.h>
#include <viskores/thirdparty/diy/diy.h>
#include <viskores/thirdparty/diy/mpi-cast.h>
#endif

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename DSIType>
class AdvectAlgorithm
{
public:
  using ParticleType = typename DSIType::PType;
  struct PendingParticle
  {
    ParticleType Particle;
    std::vector<viskores::Id> BlockIDs;
  };

  AdvectAlgorithm(const viskores::filter::flow::internal::BoundsMap& bm,
                  std::vector<DSIType>& blocks)
    : Blocks(blocks)
    , BoundsMap(bm)
#ifdef VISKORES_ENABLE_MPI
    , Exchanger(this->Comm)
    , Terminator(this->Comm)
#endif
    , NumRanks(this->Comm.size())
    , Rank(this->Comm.rank())
  {
  }

  void Execute(const viskores::cont::ArrayHandle<ParticleType>& seeds,
               viskores::FloatDefault stepSize)
  {
    this->SetStepSize(stepSize);
    this->SetSeeds(seeds);

    this->Go();
  }

  viskores::cont::PartitionedDataSet GetOutput() const
  {
    viskores::cont::PartitionedDataSet output;
    for (const auto& b : this->Blocks)
    {
      viskores::cont::DataSet ds;
      if (b.GetOutput(ds))
        output.AppendPartition(ds);
    }
    return output;
  }

  void SetStepSize(viskores::FloatDefault stepSize) { this->StepSize = stepSize; }

  void SetSeeds(const viskores::cont::ArrayHandle<ParticleType>& seeds)
  {
    this->ClearParticles();

    viskores::Id n = seeds.GetNumberOfValues();
    auto portal = seeds.ReadPortal();

    std::vector<std::vector<viskores::Id>> blockIDs;
    std::vector<ParticleType> particles;
    for (viskores::Id i = 0; i < n; i++)
    {
      const ParticleType p = portal.Get(i);
      std::vector<viskores::Id> ids = this->BoundsMap.FindBlocks(p.GetPosition());

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

  virtual bool HaveWork()
  {
    const bool haveParticles = !this->Active.empty() || !this->Inactive.empty();
#ifndef VISKORES_ENABLE_MPI
    return haveParticles;
#else
    return haveParticles || this->Exchanger.HaveWork();
#endif
  }

  virtual bool GetDone()
  {
#ifndef VISKORES_ENABLE_MPI
    return !this->HaveWork();
#else
    return this->Terminator.Done();
#endif
  }

  //Advect all the particles.
  virtual void Go()
  {
    while (!this->GetDone())
    {
      std::vector<ParticleType> v;
      viskores::Id blockId = -1;

      if (this->GetActiveParticles(v, blockId))
      {
        //make this a pointer to avoid the copy?
        auto& block = this->GetDataSet(blockId);
        DSIHelperInfo<ParticleType> bb(v, this->BoundsMap);
        block.Advect(bb, this->StepSize);
        this->UpdateResult(bb);
      }

      this->ExchangeParticles();
    }
  }

  virtual void ClearParticles()
  {
    this->Active.clear();
    this->Inactive.clear();
  }

  DataSetIntegrator<DSIType, ParticleType>& GetDataSet(viskores::Id id)
  {
    for (auto& it : this->Blocks)
      if (it.GetID() == id)
        return it;

    throw viskores::cont::ErrorFilterExecution("Bad block");
  }

  virtual void SetSeedArray(const std::vector<ParticleType>& particles,
                            const std::vector<std::vector<viskores::Id>>& blockIds)
  {
    VISKORES_ASSERT(particles.size() == blockIds.size());

    auto pit = particles.begin();
    auto bit = blockIds.begin();
    while (pit != particles.end() && bit != blockIds.end())
    {
      viskores::Id blockId0 = (*bit)[0];
      if (this->Active.find(blockId0) == this->Active.end())
        this->Active[blockId0] = { *pit };
      else
        this->Active[blockId0].emplace_back(*pit);
      pit++;
      bit++;
    }
  }

  virtual bool GetActiveParticles(std::vector<ParticleType>& particles, viskores::Id& blockId)
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
#ifndef VISKORES_ENABLE_MPI
    this->SerialExchange();
#else
    // MPI with only 1 rank.
    if (this->NumRanks == 1)
      this->SerialExchange();
    else
    {
      std::vector<ParticleType> outgoing;
      std::vector<viskores::Id> outgoingRanks;
      std::vector<std::vector<viskores::Id>> outgoingBlockIDs;

      this->GetOutgoingParticles(outgoing, outgoingRanks, outgoingBlockIDs);

      std::vector<ParticleType> incoming;
      std::vector<std::vector<viskores::Id>> incomingBlockIDs;

      this->Exchanger.Exchange(outgoing, outgoingRanks, outgoingBlockIDs, incoming, incomingBlockIDs);

      this->UpdateActive(incoming, incomingBlockIDs);
    }

    this->Terminator.Control(this->HaveWork());
#endif
  }

  void SerialExchange()
  {
    for (auto& pending : this->Inactive)
    {
      VISKORES_ASSERT(!pending.BlockIDs.empty());
      this->Active[pending.BlockIDs[0]].emplace_back(std::move(pending.Particle));
    }
    this->Inactive.clear();
  }

#ifdef VISKORES_ENABLE_MPI
  void GetOutgoingParticles(std::vector<ParticleType>& outgoing,
                            std::vector<viskores::Id>& outgoingRanks,
                            std::vector<std::vector<viskores::Id>>& outgoingBlockIDs)
  {
    outgoing.clear();
    outgoingRanks.clear();
    outgoingBlockIDs.clear();

    outgoing.reserve(this->Inactive.size());
    outgoingRanks.reserve(this->Inactive.size());
    outgoingBlockIDs.reserve(this->Inactive.size());

    std::vector<ParticleType> particlesStaying;
    std::vector<std::vector<viskores::Id>> particlesStayingBlockIDs;
    //Send out Everything.
    for (const auto& pending : this->Inactive)
    {
      const auto& bid = pending.BlockIDs;
      VISKORES_ASSERT(!bid.empty());

      auto ranks = this->BoundsMap.FindRank(bid[0]);
      VISKORES_ASSERT(!ranks.empty());

      //Only 1 rank has the block.
      if (ranks.size() == 1)
      {
        if (ranks[0] == this->Rank)
        {
          particlesStaying.emplace_back(pending.Particle);
          particlesStayingBlockIDs.emplace_back(bid);
        }
        else
        {
          outgoing.emplace_back(pending.Particle);
          outgoingRanks.emplace_back(ranks[0]);
          outgoingBlockIDs.emplace_back(bid);
        }
      }
      else
      {
        //Multiple ranks have the block, decide where it should go...

        //Random selection:
        viskores::Id outRank = ranks[static_cast<std::size_t>(std::rand() % ranks.size())];
        if (outRank == this->Rank)
        {
          particlesStayingBlockIDs.emplace_back(bid);
          particlesStaying.emplace_back(pending.Particle);
        }
        else
        {
          outgoing.emplace_back(pending.Particle);
          outgoingRanks.emplace_back(outRank);
          outgoingBlockIDs.emplace_back(bid);
        }
      }
    }

    this->Inactive.clear();
    VISKORES_ASSERT(outgoing.size() == outgoingRanks.size() &&
                    outgoing.size() == outgoingBlockIDs.size());

    VISKORES_ASSERT(particlesStaying.size() == particlesStayingBlockIDs.size());
    if (!particlesStaying.empty())
      this->UpdateActive(particlesStaying, particlesStayingBlockIDs);
  }
#endif

  virtual void UpdateActive(
    const std::vector<ParticleType>& particles,
    const std::vector<std::vector<viskores::Id>>& blockIds)
  {
    VISKORES_ASSERT(particles.size() == blockIds.size());

    if (!particles.empty())
    {
      for (std::size_t i = 0; i < particles.size(); ++i)
      {
        const auto& bids = blockIds[i];
        VISKORES_ASSERT(!bids.empty());
        this->Active[bids[0]].emplace_back(particles[i]);
      }
    }
  }

  virtual void UpdateInactive(const DSIHelperInfo<ParticleType>& stuff)
  {
    const viskores::Id numOutgoing = stuff.OutParticles.GetNumberOfValues();
    if (numOutgoing == 0)
      return;

    VISKORES_ASSERT(numOutgoing == stuff.OutNextCounts.GetNumberOfValues() &&
                    numOutgoing == stuff.OutNextOffsets.GetNumberOfValues());

    this->Inactive.reserve(this->Inactive.size() + static_cast<std::size_t>(numOutgoing));

    auto outParticlesPortal = stuff.OutParticles.ReadPortal();
    auto outCountsPortal = stuff.OutNextCounts.ReadPortal();
    auto outOffsetsPortal = stuff.OutNextOffsets.ReadPortal();
    auto flatNextPortal = stuff.OutFlatNextBlocks.ReadPortal();

    for (viskores::Id i = 0; i < numOutgoing; ++i)
    {
      PendingParticle pending;
      pending.Particle = outParticlesPortal.Get(i);
      const viskores::Id count = outCountsPortal.Get(i);
      const viskores::Id offset = outOffsetsPortal.Get(i);

      VISKORES_ASSERT(count > 0);
      pending.BlockIDs.reserve(static_cast<std::size_t>(count));
      for (viskores::Id j = 0; j < count; ++j)
        pending.BlockIDs.emplace_back(flatNextPortal.Get(offset + j));
      this->Inactive.emplace_back(std::move(pending));
    }
  }

  viskores::Id UpdateResult(const DSIHelperInfo<ParticleType>& stuff)
  {
    this->UpdateInactive(stuff);
    return stuff.TermIdx.GetNumberOfValues();
  }

  //Member data
  // {blockId, std::vector of particles}
  std::unordered_map<viskores::Id, std::vector<ParticleType>> Active;
  std::vector<DSIType> Blocks;
  viskores::filter::flow::internal::BoundsMap BoundsMap;
  viskoresdiy::mpi::communicator Comm = viskores::cont::EnvironmentTracker::GetCommunicator();
#ifdef VISKORES_ENABLE_MPI
  ParticleExchanger<ParticleType> Exchanger;
  AdvectAlgorithmTerminator Terminator;
#endif
  std::vector<PendingParticle> Inactive;
  viskores::Id MaxNumberOfSteps = 0;
  viskores::Id NumRanks;
  viskores::Id Rank;
  viskores::FloatDefault StepSize;
};

}
}
}
} //viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_AdvectAlgorithm_h
