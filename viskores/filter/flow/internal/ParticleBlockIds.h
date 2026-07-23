//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_flow_internal_ParticleBlockIds_h
#define viskores_filter_flow_internal_ParticleBlockIds_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/Invoker.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/worklet/WorkletMapField.h>

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

// Variable-length candidate block IDs for seeds. Each value is the group of block IDs
// associated with one seed, backed by flat block ID storage and an offsets array.
using CandidateBlockIdArrayHandle =
  viskores::cont::ArrayHandleGroupVecVariable<BlockIdArrayHandle, BlockIdArrayHandle>;

// A view of the candidate groups selected for this rank. The permutation keeps the
// selected groups in seed order without copying their variable-length block ID storage.
using SelectedCandidateBlockIdArrayHandle =
  viskores::cont::ArrayHandlePermutation<BlockIdArrayHandle, CandidateBlockIdArrayHandle>;

// Routing data for the seeds retained by this rank. The arrays are index-aligned,
// and each particle's nonempty candidate group starts with its primary block ID.
template <typename ParticleType>
struct SeedBlockRoutingResult
{
  viskores::cont::ArrayHandle<ParticleType> Particles;
  SelectedCandidateBlockIdArrayHandle CandidateBlockIds;
};

namespace detail
{

class CountParticleBlockIdsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particles, ExecObject locator, FieldOut count);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename ParticleType, typename LocatorType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const LocatorType& locator,
                                viskores::IdComponent& count) const
  {
    count = locator.CountAllCells(particle.GetPosition());
  }
};

class FindParticleBlockIdsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particles, ExecObject locator, FieldOut blockIds);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename ParticleType, typename LocatorType, typename BlockIdVecType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const LocatorType& locator,
                                BlockIdVecType& blockIds) const
  {
    locator.FindAllCellIds(particle.GetPosition(), blockIds);
  }
};

// Counts the blocks whose bounding boxes contain each particle. Because block
// bounding boxes can overlap, a particle can be a candidate for multiple blocks.
class CountExactParticleBlockIdsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particle,
                                FieldIn candidateBlockIds,
                                WholeArrayIn blockBounds,
                                FieldOut count);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  template <typename ParticleType, typename CandidateBlockIdsType, typename BlockBoundsPortalType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const CandidateBlockIdsType& candidateBlockIds,
                                const BlockBoundsPortalType& blockBounds,
                                viskores::IdComponent& count) const
  {
    count = 0;
    const auto position = particle.GetPosition();
    const viskores::IdComponent numCandidates = candidateBlockIds.GetNumberOfComponents();
    for (viskores::IdComponent i = 0; i < numCandidates; ++i)
    {
      const viskores::Id blockId = candidateBlockIds[i];
      if (blockBounds.Get(blockId).Contains(position))
        ++count;
    }
  }
};

// Writes the candidate block IDs that pass the original block-bounds check.
class FindExactParticleBlockIdsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn particle,
                                FieldIn candidateBlockIds,
                                WholeArrayIn blockBounds,
                                FieldInOut blockIds);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  template <typename ParticleType,
            typename CandidateBlockIdsType,
            typename BlockBoundsPortalType,
            typename BlockIdsType>
  VISKORES_EXEC void operator()(const ParticleType& particle,
                                const CandidateBlockIdsType& candidateBlockIds,
                                const BlockBoundsPortalType& blockBounds,
                                BlockIdsType& blockIds) const
  {
    const auto position = particle.GetPosition();
    const viskores::IdComponent numCandidates = candidateBlockIds.GetNumberOfComponents();
    viskores::IdComponent outputIndex = 0;
    for (viskores::IdComponent i = 0; i < numCandidates; ++i)
    {
      const viskores::Id blockId = candidateBlockIds[i];
      if (blockBounds.Get(blockId).Contains(position))
        blockIds[outputIndex++] = blockId;
    }
    VISKORES_ASSERT(outputIndex == blockIds.GetNumberOfComponents());
  }
};

// Sorts each particle's candidate block IDs into ascending order.
class SortParticleBlockIdsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut blockIds);
  using ExecutionSignature = void(_1);
  using InputDomain = _1;

  template <typename BlockIdVecType>
  VISKORES_EXEC void operator()(BlockIdVecType& blockIds) const
  {
    const viskores::IdComponent numIds = blockIds.GetNumberOfComponents();
    // Sort each particle's candidate block IDs in place with an insertion sort.
    // Candidate lists are expected to be short. If large lists become common,
    // consider a segmented sort, such as sorting composite (particle, block) keys.
    for (viskores::IdComponent i = 1; i < numIds; ++i)
    {
      const viskores::Id value = blockIds[i];
      viskores::IdComponent j = i;
      while (j > 0 && blockIds[j - 1] > value)
      {
        blockIds[j] = blockIds[j - 1];
        --j;
      }
      blockIds[j] = value;
    }
  }
};

// Marks seeds whose first candidate block is owned by the current rank.
class KeepSeedOnRankWorklet : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT KeepSeedOnRankWorklet(viskores::Id rank)
    : Rank(rank)
  {
  }

  using ControlSignature = void(FieldIn candidateBlockIds,
                                WholeArrayIn blockPrimaryRanks,
                                FieldOut keep);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename CandidateBlockIdsType, typename PrimaryRankPortalType>
  VISKORES_EXEC void operator()(const CandidateBlockIdsType& candidateBlockIds,
                                const PrimaryRankPortalType& blockPrimaryRanks,
                                bool& keep) const
  {
    keep = false;
    if (candidateBlockIds.GetNumberOfComponents() == 0)
      return;

    const viskores::Id blockId = candidateBlockIds[0];
    keep = blockPrimaryRanks.Get(blockId) == this->Rank;
  }

private:
  viskores::Id Rank;
};

} // namespace detail

template <typename ParticleType, typename StorageType>
VISKORES_CONT auto FindParticleBlockIds(
  const viskores::cont::ArrayHandle<ParticleType, StorageType>& particles,
  const viskores::filter::flow::internal::BoundsMap& boundsMap)
{
  // A particle can be inside multiple overlapping blocks. Count first so the
  // variable-length block id groups can be allocated exactly, then fill them.
  viskores::cont::Invoker invoker;
  viskores::cont::ArrayHandle<viskores::IdComponent> blockCounts;
  if (boundsMap.GetTotalNumBlocks() > 0)
    invoker(detail::CountParticleBlockIdsWorklet{}, particles, boundsMap.GetLocator(), blockCounts);
  else
    blockCounts.AllocateAndFill(particles.GetNumberOfValues(), viskores::IdComponent{ 0 });

  viskores::Id totalCandidateIds = 0;
  auto candidateOffsets =
    viskores::cont::ConvertNumComponentsToOffsets(blockCounts, totalCandidateIds);

  BlockIdArrayHandle candidateIds;
  candidateIds.Allocate(totalCandidateIds);
  auto groupedCandidateIds =
    viskores::cont::make_ArrayHandleGroupVecVariable(candidateIds, candidateOffsets);

  if (totalCandidateIds > 0)
    invoker(detail::FindParticleBlockIdsWorklet{},
            particles,
            boundsMap.GetLocator(),
            groupedCandidateIds);

  if (totalCandidateIds == 0)
    return groupedCandidateIds;

  // CellLocatorUniformBins uses FloatDefault coordinates during execution.
  // Treat it as a broad phase, then check its candidates against the original
  // Float64 BoundsMap values so narrowing cannot change seed ownership.
  const viskores::Id numBlocks = boundsMap.GetTotalNumBlocks();
  std::vector<viskores::Bounds> blockBounds;
  blockBounds.reserve(static_cast<std::size_t>(numBlocks));
  for (viskores::Id blockId = 0; blockId < numBlocks; ++blockId)
  {
    blockBounds.emplace_back(boundsMap.GetBlockBounds(blockId));
  }
  auto blockBoundsAH = viskores::cont::make_ArrayHandleMove(std::move(blockBounds));

  viskores::cont::ArrayHandle<viskores::IdComponent> exactBlockCounts;
  invoker(detail::CountExactParticleBlockIdsWorklet{},
          particles,
          groupedCandidateIds,
          blockBoundsAH,
          exactBlockCounts);

  viskores::Id totalBlockIds = 0;
  auto exactOffsets =
    viskores::cont::ConvertNumComponentsToOffsets(exactBlockCounts, totalBlockIds);
  BlockIdArrayHandle blockIds;
  blockIds.Allocate(totalBlockIds);
  auto groupedBlockIds = viskores::cont::make_ArrayHandleGroupVecVariable(blockIds, exactOffsets);

  if (totalBlockIds > 0)
  {
    invoker(detail::FindExactParticleBlockIdsWorklet{},
            particles,
            groupedCandidateIds,
            blockBoundsAH,
            groupedBlockIds);
    invoker(detail::SortParticleBlockIdsWorklet{}, groupedBlockIds);
  }
  return groupedBlockIds;
}

template <typename ParticleType, typename StorageType>
VISKORES_CONT SeedBlockRoutingResult<ParticleType> RouteSeedsToBlocks(
  const viskores::cont::ArrayHandle<ParticleType, StorageType>& particles,
  const viskores::filter::flow::internal::BoundsMap& boundsMap,
  viskores::Id rank)
{
  // Find every block whose bounds contain each seed. The candidate groups
  // are ordered so that their first block can be used as a deterministic owner.
  auto candidateBlockIds = FindParticleBlockIds(particles, boundsMap);

  // BoundsMap stores block ownership on the host. Build a dense block-to-primary-rank
  // lookup and place it in an ArrayHandle for the ownership worklet.
  const viskores::Id numBlocks = boundsMap.GetTotalNumBlocks();
  std::vector<viskores::Id> blockPrimaryRanks(static_cast<std::size_t>(numBlocks),
                                              viskores::Id{ -1 });
  for (viskores::Id blockId = 0; blockId < numBlocks; ++blockId)
  {
    const auto& ranks = boundsMap.FindRank(blockId);
    if (!ranks.empty())
      blockPrimaryRanks[static_cast<std::size_t>(blockId)] = ranks[0];
  }
  auto blockPrimaryRanksAH = viskores::cont::make_ArrayHandleMove(std::move(blockPrimaryRanks));

  // Keep a seed only on the primary rank of its first candidate block. This assigns
  // each seed to one rank even when its position is inside overlapping block bounds.
  // Seeds outside all blocks have empty candidate groups and are discarded.
  viskores::cont::Invoker invoker;
  viskores::cont::ArrayHandle<bool> keepMask;
  invoker(detail::KeepSeedOnRankWorklet{ rank }, candidateBlockIds, blockPrimaryRanksAH, keepMask);

  // Collect the original indices of the seeds retained by this rank. CopyIf preserves
  // their order, so these indices align with the compacted particle array.
  viskores::cont::ArrayHandle<viskores::Id> selectedSeedIndices;
  viskores::cont::Algorithm::CopyIf(
    viskores::cont::ArrayHandleIndex(keepMask.GetNumberOfValues()), keepMask, selectedSeedIndices);

  // Compact the particles. CopyIf preserves their input order.
  SeedBlockRoutingResult<ParticleType> result;
  viskores::cont::Algorithm::CopyIf(particles, keepMask, result.Particles);

  // ArrayHandleGroupVecVariable cannot be resized by CopyIf. Use a permutation view to
  // select complete candidate groups without rebuilding their offsets or copying block IDs.
  // The view retains the underlying grouped array and presents one group per retained seed.
  result.CandidateBlockIds =
    viskores::cont::make_ArrayHandlePermutation(selectedSeedIndices, candidateBlockIds);
  return result;
}
}
}
}
} // namespace viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_ParticleBlockIds_h
