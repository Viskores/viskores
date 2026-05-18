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
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
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

#include <algorithm>
#include <utility>
#include <vector>

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{

using BlockIdArrayHandle = viskores::cont::ArrayHandle<viskores::Id>;
using CandidateBlockIdArrayHandle =
  viskores::cont::ArrayHandleGroupVecVariable<BlockIdArrayHandle, BlockIdArrayHandle>;

template <typename ParticleType>
class DSIHelperInfo
{
public:
  DSIHelperInfo(const viskores::cont::ArrayHandle<ParticleType>& particles,
                const CandidateBlockIdArrayHandle& candidateBlockIds,
                const viskores::filter::flow::internal::BoundsMap& boundsMap)
    : BoundsMapPtr(&boundsMap)
    , Particles(particles)
    , CandidateBlockIDs(candidateBlockIds)
  {
  }

  DSIHelperInfo(viskores::cont::ArrayHandle<ParticleType>&& particles,
                CandidateBlockIdArrayHandle&& candidateBlockIds,
                const viskores::filter::flow::internal::BoundsMap& boundsMap)
    : BoundsMapPtr(&boundsMap)
    , Particles(std::move(particles))
    , CandidateBlockIDs(std::move(candidateBlockIds))
  {
  }

  const viskores::filter::flow::internal::BoundsMap& GetBoundsMap() const
  {
    VISKORES_ASSERT(this->BoundsMapPtr != nullptr);
    return *this->BoundsMapPtr;
  }

  void Clear()
  {
    this->OutParticles = {};
    this->OutNextBlockIDs = {};
    this->OutCandidateBlockIDs = {};
    this->TermIdx = {};
  }

  void Validate(viskores::Id num)
  {
    const viskores::Id outCount = this->OutParticles.GetNumberOfValues();
    const viskores::Id termCount = this->TermIdx.GetNumberOfValues();

    //Make sure we didn't miss anything. Every particle goes into a single bucket.
    if ((num != (outCount + termCount)) ||
        (this->Particles.GetNumberOfValues() != this->CandidateBlockIDs.GetNumberOfValues()) ||
        (this->OutParticles.GetNumberOfValues() != this->OutNextBlockIDs.GetNumberOfValues()) ||
        (this->OutParticles.GetNumberOfValues() != this->OutCandidateBlockIDs.GetNumberOfValues()))
    {
      throw viskores::cont::ErrorFilterExecution("Particle count mismatch after classification");
    }
  }

  const viskores::filter::flow::internal::BoundsMap* BoundsMapPtr = nullptr;

  viskores::cont::ArrayHandle<ParticleType> Particles;
  // Mirrors Particles. Each entry is the remaining coarse block candidates for
  // that particle, including the current block while it is being advected.
  CandidateBlockIdArrayHandle CandidateBlockIDs;
  viskores::cont::ArrayHandle<ParticleType> OutParticles;
  viskores::cont::ArrayHandle<viskores::Id> OutNextBlockIDs;
  // Mirrors OutParticles and preserves retry state across the scheduler or MPI.
  CandidateBlockIdArrayHandle OutCandidateBlockIDs;
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
  using ControlSignature = void(FieldInOut particle, FieldOut terminated, FieldOut retryCandidates);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  VISKORES_EXEC void operator()(ParticleType& particle,
                                viskores::UInt8& terminated,
                                viskores::UInt8& retryCandidates) const
  {
    auto status = particle.GetStatus();
    if (status.CheckTerminate())
    {
      terminated = 1;
      retryCandidates = 0;
    }
    else
    {
      terminated = 0;
      // A particle that failed spatial evaluation before taking any steps
      // should try the next existing candidate block rather than recomputing
      // from the same position and returning to a block that already failed.
      retryCandidates =
        static_cast<viskores::UInt8>(status.CheckSpatialBounds() && !status.CheckTookAnySteps());
      particle.SetStatus(viskores::ParticleStatus{});
    }
  }
};

class CountCandidateComponents : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn candidateBlockIds, FieldOut count);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  template <typename CandidateBlockIdsType>
  VISKORES_EXEC void operator()(const CandidateBlockIdsType& candidateBlockIds,
                                viskores::Id& count) const
  {
    count = candidateBlockIds.GetNumberOfComponents();
  }
};

class CopySelectedCandidateBlockIds : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn inputIndex,
                                WholeArrayIn candidateBlockIds,
                                FieldOut selectedCandidateBlockIds);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename CandidateBlockIdsPortalType, typename SelectedCandidateBlockIdsType>
  VISKORES_EXEC void operator()(const viskores::Id& inputIndex,
                                const CandidateBlockIdsPortalType& candidateBlockIds,
                                SelectedCandidateBlockIdsType& selectedCandidateBlockIds) const
  {
    auto source = candidateBlockIds.Get(inputIndex);
    const viskores::IdComponent n = selectedCandidateBlockIds.GetNumberOfComponents();
    for (viskores::IdComponent i = 0; i < n; ++i)
      selectedCandidateBlockIds[i] = source[i];
  }
};

template <typename CandidateBlockIdsArrayType, typename StencilArrayType, typename PredicateType>
VISKORES_CONT CandidateBlockIdArrayHandle
CopyCandidateBlockIDsIf(const CandidateBlockIdsArrayType& candidateBlockIds,
                        const BlockIdArrayHandle& candidateCounts,
                        const StencilArrayType& stencil,
                        PredicateType predicate)
{
  viskores::cont::ArrayHandle<viskores::Id> selectedInputIndices;
  viskores::cont::ArrayHandle<viskores::Id> selectedCounts;
  viskores::cont::Algorithm::CopyIf(viskores::cont::ArrayHandleIndex(stencil.GetNumberOfValues()),
                                    stencil,
                                    selectedInputIndices,
                                    predicate);
  viskores::cont::Algorithm::CopyIf(candidateCounts, stencil, selectedCounts, predicate);

  const viskores::Id totalSelectedComponents =
    viskores::cont::Algorithm::Reduce(selectedCounts, viskores::Id(0));
  auto selectedOffsets = viskores::cont::ConvertNumComponentsToOffsets(selectedCounts);

  viskores::cont::ArrayHandle<viskores::Id> selectedComponents;
  selectedComponents.AllocateAndFill(totalSelectedComponents, viskores::Id(-1));
  auto selectedCandidateBlockIds =
    viskores::cont::make_ArrayHandleGroupVecVariable(selectedComponents, selectedOffsets);

  viskores::cont::Invoker invoke;
  invoke(CopySelectedCandidateBlockIds{},
         selectedInputIndices,
         candidateBlockIds,
         selectedCandidateBlockIds);

  return selectedCandidateBlockIds;
}

template <typename ParticleType>
class CountNextCandidateBlocks : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particle,
                                FieldIn terminated,
                                FieldIn retryCandidates,
                                FieldIn candidateBlockIds,
                                ExecObject locator,
                                FieldOut count);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  VISKORES_CONT CountNextCandidateBlocks(viskores::Id currentBlockId)
    : CurrentBlockId(currentBlockId)
  {
  }

  template <typename CandidateBlockIdsType, typename LocatorType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const viskores::UInt8& terminated,
                                const viskores::UInt8& retryCandidates,
                                const CandidateBlockIdsType& candidateBlockIds,
                                const LocatorType& locator,
                                viskores::Id& count) const
  {
    if (terminated != 0)
    {
      count = 0;
      return;
    }

    if (retryCandidates != 0)
    {
      count = 0;
      const viskores::IdComponent n = candidateBlockIds.GetNumberOfComponents();
      for (viskores::IdComponent i = 0; i < n; ++i)
      {
        const viskores::Id candidate = candidateBlockIds[i];
        if (candidate >= 0 && candidate != this->CurrentBlockId)
          ++count;
      }
      return;
    }

    count = locator.CountAllCells(particle.GetPosition());
  }

private:
  viskores::Id CurrentBlockId;
};

template <typename ParticleType>
class FindNextCandidateBlocks : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particle,
                                FieldIn terminated,
                                FieldIn retryCandidates,
                                FieldIn candidateBlockIds,
                                ExecObject locator,
                                FieldOut nextCandidateBlockIds);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  VISKORES_CONT FindNextCandidateBlocks(viskores::Id currentBlockId)
    : CurrentBlockId(currentBlockId)
  {
  }

  template <typename CandidateBlockIdsType,
            typename LocatorType,
            typename NextCandidateBlockIdsType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const viskores::UInt8& terminated,
                                const viskores::UInt8& retryCandidates,
                                const CandidateBlockIdsType& candidateBlockIds,
                                const LocatorType& locator,
                                NextCandidateBlockIdsType& nextCandidateBlockIds) const
  {
    if (terminated != 0)
      return;

    const viskores::IdComponent n = nextCandidateBlockIds.GetNumberOfComponents();
    viskores::IdComponent outIndex = 0;
    if (retryCandidates != 0)
    {
      const viskores::IdComponent numCandidates = candidateBlockIds.GetNumberOfComponents();
      for (viskores::IdComponent i = 0; i < numCandidates && outIndex < n; ++i)
      {
        const viskores::Id candidate = candidateBlockIds[i];
        if (candidate >= 0 && candidate != this->CurrentBlockId)
          nextCandidateBlockIds[outIndex++] = candidate;
      }
    }
    else
    {
      locator.FindAllCellIds(particle.GetPosition(), nextCandidateBlockIds);

      // Fresh locator queries can still include the block that just released
      // the particle on exact shared boundaries. Compact it out so a particle
      // does not immediately return to the same block.
      for (viskores::IdComponent i = 0; i < n; ++i)
      {
        const viskores::Id candidate = nextCandidateBlockIds[i];
        if (candidate >= 0 && candidate != this->CurrentBlockId)
          nextCandidateBlockIds[outIndex++] = candidate;
      }
    }

    for (viskores::IdComponent i = outIndex; i < n; ++i)
      nextCandidateBlockIds[i] = -1;
  }

private:
  viskores::Id CurrentBlockId;
};

class SelectNextBlock : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn terminated,
                                FieldIn candidateCellIds,
                                WholeArrayIn blockOwnedByRank,
                                FieldOut nextBlockId);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  template <typename CandidateCellIdsType, typename OwnedPortalType>
  VISKORES_EXEC void operator()(const viskores::UInt8& terminated,
                                const CandidateCellIdsType& candidateCellIds,
                                const OwnedPortalType& blockOwnedByRank,
                                viskores::Id& nextBlockId) const
  {
    nextBlockId = -1;
    if (terminated != 0)
      return;

    viskores::Id firstAny = -1;
    viskores::Id firstLocal = -1;
    const viskores::IdComponent n = candidateCellIds.GetNumberOfComponents();
    for (viskores::IdComponent i = 0; i < n; ++i)
    {
      const viskores::Id cid = candidateCellIds[i];
      if (cid < 0)
        continue;

      if (firstAny < 0 || cid < firstAny)
        firstAny = cid;

      if (blockOwnedByRank.Get(cid) != viskores::UInt8{ 0 } && (firstLocal < 0 || cid < firstLocal))
        firstLocal = cid;
    }

    nextBlockId = (firstLocal >= 0 ? firstLocal : firstAny);
  }
};

class ClassificationMasks : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn terminated,
                                FieldIn nextBlockId,
                                FieldOut termMask,
                                FieldOut outMask);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  VISKORES_EXEC void operator()(const viskores::UInt8& terminated,
                                const viskores::Id& nextBlockId,
                                viskores::UInt8& termMask,
                                viskores::UInt8& outMask) const
  {
    const bool isTerminated = (terminated != 0) || (nextBlockId < 0);
    termMask = static_cast<viskores::UInt8>(isTerminated);
    outMask = static_cast<viskores::UInt8>((terminated == 0) && (nextBlockId >= 0));
  }
};

template <typename ParticleType>
class SetTerminateStatus : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn terminated, FieldIn nextBlockId, FieldInOut particle);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  VISKORES_EXEC void operator()(const viskores::UInt8& terminated,
                                const viskores::Id& nextBlockId,
                                ParticleType& particle) const
  {
    if (terminated != 0 || nextBlockId >= 0)
      return;

    auto status = particle.GetStatus();
    status.SetTerminate();
    particle.SetStatus(status);
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
  VISKORES_CONT inline const viskores::cont::ArrayHandle<viskores::UInt8>& GetBlockOwnedByRankArray(
    const viskores::filter::flow::internal::BoundsMap& boundsMap) const;

  VISKORES_CONT inline void ClassifyParticles(viskores::cont::ArrayHandle<ParticleType>& particles,
                                              DSIHelperInfo<ParticleType>& dsiInfo) const;

  //Data members.
  viskores::Id Id;
  viskores::filter::flow::IntegrationSolverType SolverType;
  viskoresdiy::mpi::communicator Comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  viskores::Id Rank;
  bool CopySeedArray = false;
  mutable const viskores::filter::flow::internal::BoundsMap* CachedOwnedByRankBoundsMap = nullptr;
  mutable viskores::cont::ArrayHandle<viskores::UInt8> BlockOwnedByRankAH;
};

template <typename Derived, typename ParticleType>
VISKORES_CONT inline const viskores::cont::ArrayHandle<viskores::UInt8>&
DataSetIntegrator<Derived, ParticleType>::GetBlockOwnedByRankArray(
  const viskores::filter::flow::internal::BoundsMap& boundsMap) const
{
  const viskores::Id numBlocks = boundsMap.GetTotalNumBlocks();
  if (this->CachedOwnedByRankBoundsMap == &boundsMap &&
      this->BlockOwnedByRankAH.GetNumberOfValues() == numBlocks)
  {
    return this->BlockOwnedByRankAH;
  }

  std::vector<viskores::UInt8> blockOwnedByRank(static_cast<std::size_t>(numBlocks),
                                                viskores::UInt8{ 0 });
  for (viskores::Id blockId = 0; blockId < numBlocks; ++blockId)
  {
    const auto& ranks = boundsMap.FindRankRef(blockId);
    if (std::find(ranks.begin(), ranks.end(), static_cast<int>(this->Rank)) != ranks.end())
      blockOwnedByRank[static_cast<std::size_t>(blockId)] = viskores::UInt8{ 1 };
  }

  this->BlockOwnedByRankAH =
    viskores::cont::make_ArrayHandle(blockOwnedByRank, viskores::CopyFlag::On);
  this->CachedOwnedByRankBoundsMap = &boundsMap;
  return this->BlockOwnedByRankAH;
}

template <typename Derived, typename ParticleType>
VISKORES_CONT inline void DataSetIntegrator<Derived, ParticleType>::ClassifyParticles(
  viskores::cont::ArrayHandle<ParticleType>& particles,
  DSIHelperInfo<ParticleType>& dsiInfo) const
{
  dsiInfo.Clear();
  const viskores::Id numParticles = particles.GetNumberOfValues();
  const auto& boundsMap = dsiInfo.GetBoundsMap();
  const auto& blockOwnedByRankAH = this->GetBlockOwnedByRankArray(boundsMap);

  viskores::cont::Invoker invoke;

  viskores::cont::ArrayHandle<viskores::UInt8> termInitial;
  viskores::cont::ArrayHandle<viskores::UInt8> retryCandidates;
  invoke(detail::ResetParticleStatus<ParticleType>{}, particles, termInitial, retryCandidates);

  viskores::cont::ArrayHandle<viskores::Id> candidateCountsAH;
  invoke(detail::CountNextCandidateBlocks<ParticleType>{ this->Id },
         particles,
         termInitial,
         retryCandidates,
         dsiInfo.CandidateBlockIDs,
         boundsMap.GetLocator(),
         candidateCountsAH);

  const viskores::Id totalCandidates =
    viskores::cont::Algorithm::Reduce(candidateCountsAH, viskores::Id(0));
  auto candidateOffsetsAH = viskores::cont::ConvertNumComponentsToOffsets(candidateCountsAH);

  viskores::cont::ArrayHandle<viskores::Id> allCandidateCellIDsAH;
  allCandidateCellIDsAH.AllocateAndFill(totalCandidates, viskores::Id(-1));

  auto candidateCellIDsVec =
    viskores::cont::make_ArrayHandleGroupVecVariable(allCandidateCellIDsAH, candidateOffsetsAH);
  invoke(detail::FindNextCandidateBlocks<ParticleType>{ this->Id },
         particles,
         termInitial,
         retryCandidates,
         dsiInfo.CandidateBlockIDs,
         boundsMap.GetLocator(),
         candidateCellIDsVec);

  viskores::cont::ArrayHandle<viskores::Id> nextBlockIDsAH;
  invoke(detail::SelectNextBlock{},
         termInitial,
         candidateCellIDsVec,
         blockOwnedByRankAH,
         nextBlockIDsAH);

  viskores::cont::ArrayHandle<viskores::UInt8> termMask, outMask;
  invoke(detail::ClassificationMasks{}, termInitial, nextBlockIDsAH, termMask, outMask);
  invoke(detail::SetTerminateStatus<ParticleType>{}, termInitial, nextBlockIDsAH, particles);

  viskores::cont::ArrayHandleIndex allIndices(numParticles);
  viskores::cont::ArrayHandle<viskores::Id> termIdxAH;
  viskores::cont::Algorithm::CopyIf(allIndices, termMask, termIdxAH, detail::IsNonZero{});
  dsiInfo.TermIdx = termIdxAH;

  viskores::cont::ArrayHandle<ParticleType> outParticlesAH;
  viskores::cont::ArrayHandle<viskores::Id> outNextBlockIDsAH;
  viskores::cont::Algorithm::CopyIf(particles, outMask, outParticlesAH, detail::IsNonZero{});
  viskores::cont::Algorithm::CopyIf(
    nextBlockIDsAH, outMask, outNextBlockIDsAH, detail::IsNonZero{});
  dsiInfo.OutParticles = outParticlesAH;
  dsiInfo.OutNextBlockIDs = outNextBlockIDsAH;
  dsiInfo.OutCandidateBlockIDs = detail::CopyCandidateBlockIDsIf(
    candidateCellIDsVec, candidateCountsAH, outMask, detail::IsNonZero{});

  dsiInfo.Validate(numParticles);
}

}
}
}
} //viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_DataSetIntegrator_h
