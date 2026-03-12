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
    this->OutNextBlockIDs = {};
    this->TermIdx = {};
  }

  void Validate(viskores::Id num)
  {
    const viskores::Id outCount = this->OutParticles.GetNumberOfValues();
    const viskores::Id termCount = this->TermIdx.GetNumberOfValues();

    //Make sure we didn't miss anything. Every particle goes into a single bucket.
    if ((num != (outCount + termCount)) ||
        (this->OutParticles.GetNumberOfValues() != this->OutNextBlockIDs.GetNumberOfValues()))
    {
      throw viskores::cont::ErrorFilterExecution("Particle count mismatch after classification");
    }
  }

  viskores::filter::flow::internal::BoundsMap BoundsMap;

  std::vector<ParticleType> Particles;
  viskores::cont::ArrayHandle<ParticleType> OutParticles;
  viskores::cont::ArrayHandle<viskores::Id> OutNextBlockIDs;
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
class CountCandidateBlocks : public viskores::worklet::WorkletMapField
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
class FindCandidateBlocks : public viskores::worklet::WorkletMapField
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

class SelectNextBlock : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT SelectNextBlock(viskores::Id currentBlockId)
    : CurrentBlockId(currentBlockId)
  {
  }

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
      if (cid < 0 || cid == this->CurrentBlockId)
        continue;

      if (firstAny < 0 || cid < firstAny)
        firstAny = cid;

      if (blockOwnedByRank.Get(cid) != viskores::UInt8{ 0 } && (firstLocal < 0 || cid < firstLocal))
        firstLocal = cid;
    }

    nextBlockId = (firstLocal >= 0 ? firstLocal : firstAny);
  }

private:
  viskores::Id CurrentBlockId;
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

  std::vector<viskores::UInt8> blockOwnedByRank;
  const viskores::Id numBlocks = dsiInfo.BoundsMap.GetTotalNumBlocks();
  blockOwnedByRank.resize(static_cast<std::size_t>(numBlocks), viskores::UInt8{ 0 });
  for (viskores::Id blockId = 0; blockId < numBlocks; ++blockId)
  {
    const auto& ranks = dsiInfo.BoundsMap.FindRankRef(blockId);
    if (std::find(ranks.begin(), ranks.end(), static_cast<int>(this->Rank)) != ranks.end())
      blockOwnedByRank[static_cast<std::size_t>(blockId)] = viskores::UInt8{ 1 };
  }

  auto blockOwnedByRankAH =
    viskores::cont::make_ArrayHandle(blockOwnedByRank, viskores::CopyFlag::On);

  viskores::cont::Invoker invoke;

  viskores::cont::ArrayHandle<viskores::UInt8> termInitial;
  invoke(detail::ResetParticleStatus<ParticleType>{}, particles, termInitial);

  viskores::cont::ArrayHandle<viskores::Id> candidateCountsAH;
  invoke(detail::CountCandidateBlocks<ParticleType>{},
         particles,
         dsiInfo.BoundsMap.GetLocator(),
         candidateCountsAH);

  const viskores::Id totalCandidates =
    viskores::cont::Algorithm::Reduce(candidateCountsAH, viskores::Id(0));
  auto candidateOffsetsAH = viskores::cont::ConvertNumComponentsToOffsets(candidateCountsAH);

  viskores::cont::ArrayHandle<viskores::Id> allCandidateCellIDsAH;
  viskores::cont::ArrayHandle<viskores::Vec3f> allCandidatePCoordsAH;
  allCandidateCellIDsAH.AllocateAndFill(totalCandidates, viskores::Id(-1));
  allCandidatePCoordsAH.Allocate(totalCandidates);

  auto candidateCellIDsVec =
    viskores::cont::make_ArrayHandleGroupVecVariable(allCandidateCellIDsAH, candidateOffsetsAH);
  auto candidatePCoordsVec =
    viskores::cont::make_ArrayHandleGroupVecVariable(allCandidatePCoordsAH, candidateOffsetsAH);
  invoke(detail::FindCandidateBlocks<ParticleType>{},
         particles,
         dsiInfo.BoundsMap.GetLocator(),
         candidateCellIDsVec,
         candidatePCoordsVec);

  viskores::cont::ArrayHandle<viskores::Id> nextBlockIDsAH;
  invoke(detail::SelectNextBlock{ this->Id },
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

  dsiInfo.Validate(numParticles);
}

}
}
}
} //viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_DataSetIntegrator_h
