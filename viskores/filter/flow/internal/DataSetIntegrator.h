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

#ifndef viskores_filter_flow_internal_DataSetIntegrator_h
#define viskores_filter_flow_internal_DataSetIntegrator_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/ParticleArrayCopy.h>
#include <viskores/filter/flow/FlowTypes.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/filter/flow/worklet/EulerIntegrator.h>
#include <viskores/filter/flow/worklet/IntegratorStatus.h>
#include <viskores/filter/flow/worklet/ParticleAdvection.h>
#include <viskores/filter/flow/worklet/RK4Integrator.h>
#include <viskores/filter/flow/worklet/Stepper.h>

#include <viskores/cont/Variant.h>

#include <viskores/thirdparty/diy/diy.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename ParticleType>
class DSIHelperInfo
{
public:
  DSIHelperInfo(
    const std::vector<ParticleType>& v,
    const viskores::filter::flow::internal::BoundsMap& boundsMap)
    : BoundsMap(boundsMap)
    , Particles(v)
  {
  }

  void Clear()
  {
    this->OutParticles = {};
    this->OutParticleIDs = {};
    this->OutNextCounts = {};
    this->OutNextOffsets = {};
    this->OutFlatNextBlocks = {};
    this->TermIdx = {};
    this->TermID = {};
  }

  void Validate(viskores::Id num)
  {
    const viskores::Id outCount = this->OutParticles.GetNumberOfValues();
    const viskores::Id termCount = this->TermIdx.GetNumberOfValues();

    //Make sure we didn't miss anything. Every particle goes into a single bucket.
    if ((num != (outCount + termCount)) ||
        (this->TermIdx.GetNumberOfValues() != this->TermID.GetNumberOfValues()) ||
        (this->OutParticles.GetNumberOfValues() != this->OutParticleIDs.GetNumberOfValues()) ||
        (this->OutParticles.GetNumberOfValues() != this->OutNextCounts.GetNumberOfValues()) ||
        (this->OutParticles.GetNumberOfValues() != this->OutNextOffsets.GetNumberOfValues()))
    {
      throw viskores::cont::ErrorFilterExecution("Particle count mismatch after classification");
    }
  }

  viskores::filter::flow::internal::BoundsMap BoundsMap;

  std::vector<ParticleType> Particles;
  viskores::cont::ArrayHandle<ParticleType> OutParticles;
  viskores::cont::ArrayHandle<viskores::Id> OutParticleIDs;
  viskores::cont::ArrayHandle<viskores::Id> OutNextCounts;
  viskores::cont::ArrayHandle<viskores::Id> OutNextOffsets;
  viskores::cont::ArrayHandle<viskores::Id> OutFlatNextBlocks;
  viskores::cont::ArrayHandle<viskores::Id> TermID;
  viskores::cont::ArrayHandle<viskores::Id> TermIdx;
};

namespace detail
{

struct IsNonZero
{
  template <typename T>
  VISKORES_EXEC_CONT bool operator()(const T& x) const
  {
    return x != T(0);
  }
};

template <typename ParticleType>
class ResetParticleStatus : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut particle, FieldOut terminated);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  VISKORES_EXEC void operator()(ParticleType& particle, viskores::UInt8& terminated) const
  {
    auto status = particle.GetStatus();
    if (status.CheckTerminate())
    {
      terminated = 1;
    }
    else
    {
      terminated = 0;
      particle.SetStatus(viskores::ParticleStatus{});
    }
  }
};

template <typename ParticleType>
class CountNextBlocks : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT CountNextBlocks(viskores::Id currentBlockId, const viskores::Bounds& globalBounds)
    : CurrentBlockId(currentBlockId)
    , GlobalBounds(globalBounds)
  {
  }

  using ControlSignature = void(FieldIn particle,
                                FieldIn terminated,
                                WholeArrayIn blockBounds,
                                FieldOut nextCount);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  template <typename BoundsPortalType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const viskores::UInt8& terminated,
                                const BoundsPortalType& blockBounds,
                                viskores::Id& nextCount) const
  {
    nextCount = 0;
    if (terminated != 0)
      return;

    const auto& position = particle.GetPosition();
    if (!this->GlobalBounds.Contains(position))
      return;

    const viskores::Id numBlocks = blockBounds.GetNumberOfValues();
    for (viskores::Id blockId = 0; blockId < numBlocks; ++blockId)
    {
      if (blockId != this->CurrentBlockId && blockBounds.Get(blockId).Contains(position))
        ++nextCount;
    }
  }

private:
  viskores::Id CurrentBlockId;
  viskores::Bounds GlobalBounds;
};

template <typename ParticleType>
class FillNextBlocks : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT FillNextBlocks(viskores::Id currentBlockId, const viskores::Bounds& globalBounds)
    : CurrentBlockId(currentBlockId)
    , GlobalBounds(globalBounds)
  {
  }

  using ControlSignature = void(FieldIn particle,
                                FieldIn terminated,
                                FieldIn nextCount,
                                FieldIn nextOffset,
                                WholeArrayIn blockBounds,
                                WholeArrayIn blockOwnedByRank,
                                WholeArrayOut flatNextBlocks);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  template <typename BoundsPortalType, typename OwnedPortalType, typename FlatPortalType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const viskores::UInt8& terminated,
                                const viskores::Id& nextCount,
                                const viskores::Id& nextOffset,
                                const BoundsPortalType& blockBounds,
                                const OwnedPortalType& blockOwnedByRank,
                                FlatPortalType& flatNextBlocks) const
  {
    if (terminated != 0 || nextCount == 0)
      return;

    const auto& position = particle.GetPosition();
    if (!this->GlobalBounds.Contains(position))
      return;

    const viskores::Id numBlocks = blockBounds.GetNumberOfValues();
    viskores::Id firstLocalBlock = -1;
    if (nextCount > 1)
    {
      for (viskores::Id blockId = 0; blockId < numBlocks; ++blockId)
      {
        if (blockId != this->CurrentBlockId && blockBounds.Get(blockId).Contains(position) &&
            blockOwnedByRank.Get(blockId) != viskores::UInt8{ 0 })
        {
          firstLocalBlock = blockId;
          break;
        }
      }
    }

    viskores::Id writeIdx = nextOffset;
    if (firstLocalBlock >= 0)
      flatNextBlocks.Set(writeIdx++, firstLocalBlock);

    for (viskores::Id blockId = 0; blockId < numBlocks; ++blockId)
    {
      if (blockId == this->CurrentBlockId || blockId == firstLocalBlock)
        continue;

      if (blockBounds.Get(blockId).Contains(position))
        flatNextBlocks.Set(writeIdx++, blockId);
    }

    VISKORES_ASSERT(writeIdx == nextOffset + nextCount);
  }

private:
  viskores::Id CurrentBlockId;
  viskores::Bounds GlobalBounds;
};

class ClassificationMasks : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn terminated, FieldIn nextCount, FieldOut termMask, FieldOut outMask);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  VISKORES_EXEC void operator()(const viskores::UInt8& terminated,
                                const viskores::Id& nextCount,
                                viskores::UInt8& termMask,
                                viskores::UInt8& outMask) const
  {
    const bool isTerminated = (terminated != 0) || (nextCount == 0);
    termMask = static_cast<viskores::UInt8>(isTerminated);
    outMask = static_cast<viskores::UInt8>((terminated == 0) && (nextCount > 0));
  }
};

template <typename ParticleType>
class SetTerminateStatus : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn terminated, FieldIn nextCount, FieldInOut particle);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  VISKORES_EXEC void operator()(const viskores::UInt8& terminated,
                                const viskores::Id& nextCount,
                                ParticleType& particle) const
  {
    if (terminated != 0 || nextCount > 0)
      return;

    auto status = particle.GetStatus();
    status.SetTerminate();
    particle.SetStatus(status);
  }
};

template <typename ParticleType>
class ExtractParticleId : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particle, FieldOut particleId);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  VISKORES_EXEC void operator()(const ParticleType& particle, viskores::Id& particleId) const
  {
    particleId = particle.GetID();
  }
};

} // namespace detail

template <typename Derived, typename ParticleType>
class DataSetIntegrator
{
public:
  DataSetIntegrator(viskores::Id id, viskores::filter::flow::IntegrationSolverType solverType)
    : Id(id)
    , SolverType(solverType)
    , Rank(this->Comm.rank())
  {
    //check that things are valid.
  }

  VISKORES_CONT viskores::Id GetID() const { return this->Id; }
  VISKORES_CONT void SetCopySeedFlag(bool val) { this->CopySeedArray = val; }

  VISKORES_CONT
  void Advect(DSIHelperInfo<ParticleType>& b,
              viskores::FloatDefault stepSize) //move these to member data(?)
  {
    Derived* inst = static_cast<Derived*>(this);
    inst->DoAdvect(b, stepSize);
  }

  VISKORES_CONT bool GetOutput(viskores::cont::DataSet& dataset) const
  {
    Derived* inst = static_cast<Derived*>(this);
    return inst->GetOutput(dataset);
  }

protected:
  VISKORES_CONT inline void ClassifyParticles(
    viskores::cont::ArrayHandle<ParticleType>& particles,
    DSIHelperInfo<ParticleType>& dsiInfo) const;

  //Data members.
  viskores::Id Id;
  viskores::filter::flow::IntegrationSolverType SolverType;
  viskoresdiy::mpi::communicator Comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  viskores::Id Rank;
  bool CopySeedArray = false;
};

template <typename Derived, typename ParticleType>
VISKORES_CONT inline void DataSetIntegrator<Derived, ParticleType>::ClassifyParticles(
  viskores::cont::ArrayHandle<ParticleType>& particles,
  DSIHelperInfo<ParticleType>& dsiInfo) const
{
  dsiInfo.Clear();
  const viskores::Id numParticles = particles.GetNumberOfValues();

  std::vector<viskores::Bounds> blockBounds;
  std::vector<viskores::UInt8> blockOwnedByRank;
  const viskores::Id numBlocks = dsiInfo.BoundsMap.GetTotalNumBlocks();
  blockBounds.resize(static_cast<std::size_t>(numBlocks));
  blockOwnedByRank.resize(static_cast<std::size_t>(numBlocks), viskores::UInt8{ 0 });
  for (viskores::Id blockId = 0; blockId < numBlocks; ++blockId)
  {
    blockBounds[static_cast<std::size_t>(blockId)] = dsiInfo.BoundsMap.GetBlockBounds(blockId);
    auto ranks = dsiInfo.BoundsMap.FindRank(blockId);
    if (std::find(ranks.begin(), ranks.end(), static_cast<int>(this->Rank)) != ranks.end())
      blockOwnedByRank[static_cast<std::size_t>(blockId)] = viskores::UInt8{ 1 };
  }

  auto blockBoundsAH = viskores::cont::make_ArrayHandle(blockBounds, viskores::CopyFlag::On);
  auto blockOwnedByRankAH =
    viskores::cont::make_ArrayHandle(blockOwnedByRank, viskores::CopyFlag::On);

  viskores::cont::Invoker invoke;

  viskores::cont::ArrayHandle<viskores::UInt8> termInitial;
  invoke(detail::ResetParticleStatus<ParticleType>{}, particles, termInitial);

  viskores::cont::ArrayHandle<viskores::Id> nextCounts;
  invoke(detail::CountNextBlocks<ParticleType>{ this->Id, dsiInfo.BoundsMap.GetGlobalBounds() },
         particles,
         termInitial,
         blockBoundsAH,
         nextCounts);

  viskores::cont::ArrayHandle<viskores::Id> nextOffsets;
  viskores::Id numFlatNextBlocks = viskores::cont::Algorithm::ScanExclusive(nextCounts, nextOffsets);

  viskores::cont::ArrayHandle<viskores::Id> flatNextBlocks;
  flatNextBlocks.Allocate(numFlatNextBlocks);
  invoke(detail::FillNextBlocks<ParticleType>{ this->Id, dsiInfo.BoundsMap.GetGlobalBounds() },
         particles,
         termInitial,
         nextCounts,
         nextOffsets,
         blockBoundsAH,
         blockOwnedByRankAH,
         flatNextBlocks);

  viskores::cont::ArrayHandle<viskores::UInt8> termMask, outMask;
  invoke(detail::ClassificationMasks{}, termInitial, nextCounts, termMask, outMask);
  invoke(detail::SetTerminateStatus<ParticleType>{}, termInitial, nextCounts, particles);

  viskores::cont::ArrayHandle<viskores::Id> particleIds;
  invoke(detail::ExtractParticleId<ParticleType>{}, particles, particleIds);

  viskores::cont::ArrayHandleIndex allIndices(numParticles);
  viskores::cont::ArrayHandle<viskores::Id> termIdxAH, termIdAH;
  viskores::cont::Algorithm::CopyIf(allIndices, termMask, termIdxAH, detail::IsNonZero{});
  viskores::cont::Algorithm::CopyIf(particleIds, termMask, termIdAH, detail::IsNonZero{});
  dsiInfo.TermIdx = termIdxAH;
  dsiInfo.TermID = termIdAH;

  viskores::cont::ArrayHandle<ParticleType> outParticlesAH;
  viskores::cont::ArrayHandle<viskores::Id> outParticleIdsAH, outCountsAH, outOffsetsAH;
  viskores::cont::Algorithm::CopyIf(particles, outMask, outParticlesAH, detail::IsNonZero{});
  viskores::cont::Algorithm::CopyIf(particleIds, outMask, outParticleIdsAH, detail::IsNonZero{});
  viskores::cont::Algorithm::CopyIf(nextCounts, outMask, outCountsAH, detail::IsNonZero{});
  viskores::cont::Algorithm::CopyIf(nextOffsets, outMask, outOffsetsAH, detail::IsNonZero{});
  dsiInfo.OutParticles = outParticlesAH;
  dsiInfo.OutParticleIDs = outParticleIdsAH;
  dsiInfo.OutNextCounts = outCountsAH;
  dsiInfo.OutNextOffsets = outOffsetsAH;
  dsiInfo.OutFlatNextBlocks = flatNextBlocks;

  dsiInfo.Validate(numParticles);
}

}
}
}
} //viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_DataSetIntegrator_h
