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


#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/filter/flow/internal/DataSetIntegrator.h>
#include <viskores/worklet/WorkletMapField.h>

#include <utility>
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

namespace detail
{

template <typename ParticleType>
class CountSeedBlocks : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particle, ExecObject locator, FieldOut count);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename LocatorType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const LocatorType& locator,
                                viskores::Id& count) const
  {
    count = locator.CountAllCells(particle.GetPosition());
  }
};

template <typename ParticleType>
class FindSeedBlocks : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particle, ExecObject locator, FieldOut cellIds, FieldOut pCoords);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  template <typename LocatorType, typename CellIdsVecType, typename PCoordsVecType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const LocatorType& locator,
                                CellIdsVecType& cellIds,
                                PCoordsVecType& pCoords) const
  {
    locator.FindAllCells(particle.GetPosition(), cellIds, pCoords);
  }
};

class SelectFirstSeedBlock : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn cellIds, FieldOut blockId);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  template <typename CellIdsVecType>
  VISKORES_EXEC void operator()(const CellIdsVecType& cellIds, viskores::Id& blockId) const
  {
    blockId = -1;
    const viskores::IdComponent numIds = cellIds.GetNumberOfComponents();
    for (viskores::IdComponent i = 0; i < numIds; ++i)
    {
      const viskores::Id cid = cellIds[i];
      if (cid >= 0 && (blockId < 0 || cid < blockId))
        blockId = cid;
    }
  }
};

class KeepSeedOnRank : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT KeepSeedOnRank(viskores::Id rank)
    : Rank(rank)
  {
  }

  using ControlSignature = void(FieldIn blockId, WholeArrayIn blockPrimaryRanks, FieldOut keepMask);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename PrimaryRankPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& blockId,
                                const PrimaryRankPortalType& blockPrimaryRanks,
                                viskores::UInt8& keepMask) const
  {
    keepMask = viskores::UInt8{ 0 };
    if (blockId < 0)
      return;

    keepMask = static_cast<viskores::UInt8>(blockPrimaryRanks.Get(blockId) == this->Rank);
  }

private:
  viskores::Id Rank;
};

struct IsSelected
{
  template <typename T>
  VISKORES_EXEC_CONT bool operator()(const T& x) const
  {
    return x != T(0);
  }
};

} // namespace detail

template <typename DSIType>
class AdvectAlgorithm
{
public:
  using ParticleType = typename DSIType::PType;
  struct PendingParticle
  {
    ParticleType Particle;
    viskores::Id BlockID;
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

    const viskores::Id numSeeds = seeds.GetNumberOfValues();
    if (numSeeds == 0)
      return;

    const viskores::Id numBlocks = this->BoundsMap.GetTotalNumBlocks();
    if (numBlocks <= 0)
      return;

    viskores::cont::Invoker invoke;

    viskores::cont::ArrayHandle<viskores::Id> hitCountsAH;
    invoke(detail::CountSeedBlocks<ParticleType>{}, seeds, this->BoundsMap.GetLocator(), hitCountsAH);

    const viskores::Id totalHits = viskores::cont::Algorithm::Reduce(hitCountsAH, viskores::Id(0));
    auto hitOffsetsAH = viskores::cont::ConvertNumComponentsToOffsets(hitCountsAH);

    viskores::cont::ArrayHandle<viskores::Id> allHitCellIdsAH;
    viskores::cont::ArrayHandle<viskores::Vec3f> allHitPCoordsAH;
    allHitCellIdsAH.AllocateAndFill(totalHits, viskores::Id(-1));
    allHitPCoordsAH.Allocate(totalHits);

    auto hitCellIdsVec = viskores::cont::make_ArrayHandleGroupVecVariable(allHitCellIdsAH, hitOffsetsAH);
    auto hitPCoordsVec = viskores::cont::make_ArrayHandleGroupVecVariable(allHitPCoordsAH, hitOffsetsAH);
    invoke(detail::FindSeedBlocks<ParticleType>{},
           seeds,
           this->BoundsMap.GetLocator(),
           hitCellIdsVec,
           hitPCoordsVec);

    viskores::cont::ArrayHandle<viskores::Id> seedBlockIDsAH;
    invoke(detail::SelectFirstSeedBlock{}, hitCellIdsVec, seedBlockIDsAH);

    std::vector<viskores::Id> blockPrimaryRanks(static_cast<std::size_t>(numBlocks), -1);
    for (viskores::Id blockId = 0; blockId < numBlocks; ++blockId)
    {
      const auto& ranks = this->BoundsMap.FindRankRef(blockId);
      if (!ranks.empty())
        blockPrimaryRanks[static_cast<std::size_t>(blockId)] = static_cast<viskores::Id>(ranks[0]);
    }

    auto blockPrimaryRanksAH =
      viskores::cont::make_ArrayHandle(blockPrimaryRanks, viskores::CopyFlag::On);

    viskores::cont::ArrayHandle<viskores::UInt8> keepMaskAH;
    invoke(detail::KeepSeedOnRank{ this->Rank },
           seedBlockIDsAH,
           blockPrimaryRanksAH,
           keepMaskAH);

    viskores::cont::ArrayHandle<ParticleType> particlesAH;
    viskores::cont::ArrayHandle<viskores::Id> blockIDsAH;
    viskores::cont::Algorithm::CopyIf(seeds, keepMaskAH, particlesAH, detail::IsSelected{});
    viskores::cont::Algorithm::CopyIf(
      seedBlockIDsAH, keepMaskAH, blockIDsAH, detail::IsSelected{});

    const viskores::Id numOwnedSeeds = particlesAH.GetNumberOfValues();
    std::vector<viskores::Id> blockIDs;
    std::vector<ParticleType> particles;
    particles.reserve(static_cast<std::size_t>(numOwnedSeeds));
    blockIDs.reserve(static_cast<std::size_t>(numOwnedSeeds));

    auto particlesPortal = particlesAH.ReadPortal();
    auto blockIDsPortal = blockIDsAH.ReadPortal();
    for (viskores::Id i = 0; i < numOwnedSeeds; ++i)
    {
      particles.emplace_back(particlesPortal.Get(i));
      blockIDs.emplace_back(blockIDsPortal.Get(i));
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
        DSIHelperInfo<ParticleType> bb(std::move(v), this->BoundsMap);
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
                            const std::vector<viskores::Id>& blockIds)
  {
    VISKORES_ASSERT(particles.size() == blockIds.size());

    for (std::size_t i = 0; i < particles.size(); ++i)
    {
      viskores::Id blockId0 = blockIds[i];
      if (this->Active.find(blockId0) == this->Active.end())
        this->Active[blockId0] = { particles[i] };
      else
        this->Active[blockId0].emplace_back(particles[i]);
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
      std::vector<viskores::Id> outgoingBlockIDs;

      this->GetOutgoingParticles(outgoing, outgoingRanks, outgoingBlockIDs);

      std::vector<ParticleType> incoming;
      std::vector<viskores::Id> incomingBlockIDs;

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
      this->Active[pending.BlockID].emplace_back(std::move(pending.Particle));
    }
    this->Inactive.clear();
  }

#ifdef VISKORES_ENABLE_MPI
  void GetOutgoingParticles(std::vector<ParticleType>& outgoing,
                            std::vector<viskores::Id>& outgoingRanks,
                            std::vector<viskores::Id>& outgoingBlockIDs)
  {
    outgoing.clear();
    outgoingRanks.clear();
    outgoingBlockIDs.clear();

    outgoing.reserve(this->Inactive.size());
    outgoingRanks.reserve(this->Inactive.size());
    outgoingBlockIDs.reserve(this->Inactive.size());

    std::vector<ParticleType> particlesStaying;
    std::vector<viskores::Id> particlesStayingBlockIDs;
    //Send out Everything.
    for (auto& pending : this->Inactive)
    {
      viskores::Id bid = pending.BlockID;
      const auto& ranks = this->BoundsMap.FindRankRef(bid);
      VISKORES_ASSERT(!ranks.empty());

      //Only 1 rank has the block.
      if (ranks.size() == 1)
      {
        viskores::Id outRank = static_cast<viskores::Id>(ranks[0]);
        if (outRank == this->Rank)
        {
          particlesStaying.emplace_back(std::move(pending.Particle));
          particlesStayingBlockIDs.emplace_back(bid);
        }
        else
        {
          outgoing.emplace_back(std::move(pending.Particle));
          outgoingRanks.emplace_back(outRank);
          outgoingBlockIDs.emplace_back(bid);
        }
      }
      else
      {
        //Multiple ranks have the block, decide where it should go...

        //Random selection:
        viskores::Id outRank =
          static_cast<viskores::Id>(ranks[static_cast<std::size_t>(std::rand() % ranks.size())]);
        if (outRank == this->Rank)
        {
          particlesStayingBlockIDs.emplace_back(bid);
          particlesStaying.emplace_back(std::move(pending.Particle));
        }
        else
        {
          outgoing.emplace_back(std::move(pending.Particle));
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
    const std::vector<viskores::Id>& blockIds)
  {
    VISKORES_ASSERT(particles.size() == blockIds.size());

    if (!particles.empty())
    {
      for (std::size_t i = 0; i < particles.size(); ++i)
        this->Active[blockIds[i]].emplace_back(particles[i]);
    }
  }

  virtual void UpdateInactive(const DSIHelperInfo<ParticleType>& stuff)
  {
    const viskores::Id numOutgoing = stuff.OutParticles.GetNumberOfValues();
    if (numOutgoing == 0)
      return;

    VISKORES_ASSERT(numOutgoing == stuff.OutNextBlockIDs.GetNumberOfValues());

    this->Inactive.reserve(this->Inactive.size() + static_cast<std::size_t>(numOutgoing));

    auto outParticlesPortal = stuff.OutParticles.ReadPortal();
    auto outNextBlockIDsPortal = stuff.OutNextBlockIDs.ReadPortal();

    for (viskores::Id i = 0; i < numOutgoing; ++i)
    {
      PendingParticle pending;
      pending.Particle = outParticlesPortal.Get(i);
      pending.BlockID = outNextBlockIDsPortal.Get(i);
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
  std::vector<DSIType>& Blocks;
  const viskores::filter::flow::internal::BoundsMap& BoundsMap;
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
