//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#include <viskores/filter/scalar_topology/internal/SelectTopVolumeContoursFunctor.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/GetOuterEndWorklet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_contours/BranchVolumeComparator.h>

#include <viskores/Types.h>

#ifdef DEBUG_PRINT
#define DEBUG_PRINT_COMBINED_HIGH_VOLUME_BRANCH
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#endif

namespace viskores
{
namespace filter
{
namespace scalar_topology
{
namespace internal
{

void SelectTopVolumeContoursFunctor::operator()(
  SelectTopVolumeContoursBlock* b,
  const viskoresdiy::ReduceProxy& rp // communication proxy
) const
{
  if (this->nSavedBranches < 1)
    return;
  // Get our rank and DIY id
  //const viskores::Id rank = viskores::cont::EnvironmentTracker::GetCommunicator().rank();
  const auto selfid = rp.gid();

  // Aliases to reduce verbosity
  using IdArrayType = viskores::worklet::contourtree_augmented::IdArrayType;

  viskores::cont::Invoker invoke;

  if (rp.in_link().size() == 0)
  {
    for (int cc = 0; cc < rp.out_link().size(); ++cc)
    {
      auto target = rp.out_link().target(cc);
      if (target.gid != selfid)
      {
#ifdef DEBUG_PRINT_COMBINED_HIGH_VOLUME_BRANCH
        rp.enqueue(target, b->GlobalBlockId);
        VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "Block " << b->GlobalBlockId << " enqueue to Block " << target.gid);
#endif
        auto topVolBranchRootGRIdPortal = b->TopVolumeBranchRootGRId.ReadPortal();
        auto topVolBranchVolumePortal = b->TopVolumeBranchVolume.ReadPortal();
        auto topVolBranchSaddleEpsilonPortal = b->TopVolumeBranchSaddleEpsilon.ReadPortal();
        auto topVolBranchUpperEndPortal = b->TopVolumeBranchUpperEndGRId.ReadPortal();
        auto topVolBranchLowerEndPortal = b->TopVolumeBranchLowerEndGRId.ReadPortal();

        viskores::Id nBranches = topVolBranchRootGRIdPortal.GetNumberOfValues();

        rp.enqueue(target, nBranches);
        for (viskores::Id branch = 0; branch < nBranches; ++branch)
          rp.enqueue(target, topVolBranchRootGRIdPortal.Get(branch));
        for (viskores::Id branch = 0; branch < nBranches; ++branch)
          rp.enqueue(target, topVolBranchVolumePortal.Get(branch));
        for (viskores::Id branch = 0; branch < nBranches; ++branch)
          rp.enqueue(target, topVolBranchSaddleEpsilonPortal.Get(branch));
        for (viskores::Id branch = 0; branch < nBranches; ++branch)
          rp.enqueue(target, topVolBranchUpperEndPortal.Get(branch));
        for (viskores::Id branch = 0; branch < nBranches; ++branch)
          rp.enqueue(target, topVolBranchLowerEndPortal.Get(branch));

        auto resolveArray = [&](const auto& inArray) {
          using InArrayHandleType = std::decay_t<decltype(inArray)>;
          using ValueType = typename InArrayHandleType::ValueType;
          auto topVolBranchSaddleIsoValuePortal = inArray.ReadPortal();
          for (viskores::Id branch = 0; branch < nBranches; ++branch)
            rp.enqueue<ValueType>(target, topVolBranchSaddleIsoValuePortal.Get(branch));
        };
        b->TopVolumeBranchSaddleIsoValue
          .CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(resolveArray);

        // rp.enqueue(target, b->TopVolumeBranchRootGRId);
        // rp.enqueue(target, b->TopVolumeBranchVolume);
      }
    }
  }
  else
  {
    for (int i = 0; i < rp.in_link().size(); ++i)
    {
      int ingid = rp.in_link().target(i).gid;
      if (ingid == selfid)
        continue;

        // copy incoming to the block
#ifdef DEBUG_PRINT_COMBINED_HIGH_VOLUME_BRANCH
      int incomingGlobalBlockId;
      rp.dequeue(ingid, incomingGlobalBlockId);
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 "Combining local block " << b->GlobalBlockId << " with incoming block "
                                          << incomingGlobalBlockId);
#endif

      // dequeue the data from other blocks.
      // nIncomingBranches
      // array of incoming branch global regular ID
      // array of incoming branch volume
      // array of branch epsilon direction
      // array of branch saddle end value
      // TODO/FIXME: This is a workaround for a bug in DIY/viskores.
      // Replace with dequeuing ArrayHandles once bug is fixed.
      viskores::Id nIncoming;
      rp.dequeue(ingid, nIncoming);

      IdArrayType incomingTopVolBranchGRId;
      incomingTopVolBranchGRId.Allocate(nIncoming);
      auto incomingTopVolBranchGRIdPortal = incomingTopVolBranchGRId.WritePortal();
      for (viskores::Id branch = 0; branch < nIncoming; ++branch)
      {
        viskores::Id incomingTmpBranchGRId;
        rp.dequeue(ingid, incomingTmpBranchGRId);
        incomingTopVolBranchGRIdPortal.Set(branch, incomingTmpBranchGRId);
      }

      //  TODO/FIXME: This is a workaround for a bug in DIY/viskores.
      //  Replace with dequeuing ArrayHandles once bug is fixed.
      // rp.dequeue(ingid, incomingTopVolBranchGRId);

      IdArrayType incomingTopVolBranchVolume;
      incomingTopVolBranchVolume.Allocate(nIncoming);
      auto incomingTopVolBranchVolumePortal = incomingTopVolBranchVolume.WritePortal();
      for (viskores::Id branch = 0; branch < nIncoming; ++branch)
      {
        viskores::Id incomingTmpBranchVolume;
        rp.dequeue(ingid, incomingTmpBranchVolume);
        incomingTopVolBranchVolumePortal.Set(branch, incomingTmpBranchVolume);
      }

      // TODO/FIXME: This is a workaround for a bug in DIY/viskores.
      // Replace with dequeuing ArrayHandles once bug is fixed.
      // rp.dequeue(ingid, incomingTopVolBranchVolume);

      IdArrayType incomingTopVolBranchSaddleEpsilon;
      incomingTopVolBranchSaddleEpsilon.Allocate(nIncoming);
      auto incomingTopVolBranchSaddleEpsilonPortal =
        incomingTopVolBranchSaddleEpsilon.WritePortal();
      for (viskores::Id branch = 0; branch < nIncoming; ++branch)
      {
        viskores::Id incomingTmpBranchSaddleEpsilon;
        rp.dequeue(ingid, incomingTmpBranchSaddleEpsilon);
        incomingTopVolBranchSaddleEpsilonPortal.Set(branch, incomingTmpBranchSaddleEpsilon);
      }

      // TODO/FIXME: This is a workaround for a bug in DIY/viskores.
      // Replace with dequeuing ArrayHandles once bug is fixed.
      // rp.dequeue(ingid, incomingTopVolBranchSaddleEpsilon);

      IdArrayType incomingTopVolBranchUpperEnd;
      incomingTopVolBranchUpperEnd.Allocate(nIncoming);
      auto incomingTopVolBranchUpperEndPortal = incomingTopVolBranchUpperEnd.WritePortal();
      for (viskores::Id branch = 0; branch < nIncoming; ++branch)
      {
        viskores::Id incomingTmpBranchUpperEnd;
        rp.dequeue(ingid, incomingTmpBranchUpperEnd);
        incomingTopVolBranchUpperEndPortal.Set(branch, incomingTmpBranchUpperEnd);
      }

      // TODO/FIXME: This is a workaround for a bug in DIY/viskores.
      // Replace with dequeuing ArrayHandles once bug is fixed.
      // rp.dequeue(ingid, incomingTopVolBranchUpperEnd);

      IdArrayType incomingTopVolBranchLowerEnd;
      incomingTopVolBranchLowerEnd.Allocate(nIncoming);
      auto incomingTopVolBranchLowerEndPortal = incomingTopVolBranchLowerEnd.WritePortal();
      for (viskores::Id branch = 0; branch < nIncoming; ++branch)
      {
        viskores::Id incomingTmpBranchLowerEnd;
        rp.dequeue(ingid, incomingTmpBranchLowerEnd);
        incomingTopVolBranchLowerEndPortal.Set(branch, incomingTmpBranchLowerEnd);
      }

      // TODO/FIXME: This is a workaround for a bug in DIY/viskores.
      // Replace with dequeuing ArrayHandles once bug is fixed.
      // rp.dequeue(ingid, incomingTopVolBranchLowerEnd);

      auto resolveArray = [&](auto& inArray) {
        using InArrayHandleType = std::decay_t<decltype(inArray)>;
        using ValueType = typename InArrayHandleType::ValueType;
        InArrayHandleType incomingTopVolBranchSaddleIsoValue;
        incomingTopVolBranchSaddleIsoValue.Allocate(nIncoming);
        auto incomingTopVolBranchSaddleIsoValuePortal =
          incomingTopVolBranchSaddleIsoValue.WritePortal();
        for (viskores::Id branch = 0; branch < nIncoming; ++branch)
        {
          ValueType incomingSaddleValue;
          rp.dequeue<ValueType>(ingid, incomingSaddleValue);
          incomingTopVolBranchSaddleIsoValuePortal.Set(branch, incomingSaddleValue);
        }

        // TODO/FIXME: This is a workaround for a bug in DIY/viskores.
        // Replace with dequeuing ArrayHandles once bug is fixed.
        // rp.dequeue<InArrayHandleType>(ingid, incomingTopVolBranchSaddleIsoValue);

        viskores::Id nSelf = b->TopVolumeBranchRootGRId.GetNumberOfValues();

#ifdef DEBUG_PRINT_COMBINED_HIGH_VOLUME_BRANCH
        VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "nIncoming = " << nIncoming << ", nSelf = " << nSelf);
        {
          std::stringstream rs;
          viskores::worklet::contourtree_augmented::PrintHeader(nIncoming, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "incomingTopBranchId", incomingTopVolBranchGRId, -1, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "incomingTopBranchVol", incomingTopVolBranchVolume, -1, rs);
          viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
            "incomingSaddleVal", incomingTopVolBranchSaddleIsoValue, -1, rs);

          viskores::worklet::contourtree_augmented::PrintHeader(nSelf, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "selfTopBranchId", b->TopVolumeBranchRootGRId, -1, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "selfTopBranchVol", b->TopVolumeBranchVolume, -1, rs);
          viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
            "selfTopSaddleVal", inArray, -1, rs);
          VISKORES_LOG_S(viskores::cont::LogLevel::Info, rs.str());
        }
#endif
        // merge incoming branches with self branches
        IdArrayType mergedTopVolBranchGRId;
        IdArrayType mergedTopVolBranchVolume;
        IdArrayType mergedTopVolBranchSaddleEpsilon;
        IdArrayType mergedTopVolBranchUpperEnd;
        IdArrayType mergedTopVolBranchLowerEnd;
        InArrayHandleType mergedTopVolBranchSaddleIsoValue;
        mergedTopVolBranchGRId.Allocate(nIncoming + nSelf);
        mergedTopVolBranchVolume.Allocate(nIncoming + nSelf);
        mergedTopVolBranchSaddleEpsilon.Allocate(nIncoming + nSelf);
        mergedTopVolBranchUpperEnd.Allocate(nIncoming + nSelf);
        mergedTopVolBranchLowerEnd.Allocate(nIncoming + nSelf);
        mergedTopVolBranchSaddleIsoValue.Allocate(nIncoming + nSelf);

        viskores::cont::Algorithm::CopySubRange(
          incomingTopVolBranchGRId, 0, nIncoming, mergedTopVolBranchGRId, 0);
        viskores::cont::Algorithm::CopySubRange(
          incomingTopVolBranchVolume, 0, nIncoming, mergedTopVolBranchVolume, 0);
        viskores::cont::Algorithm::CopySubRange(
          incomingTopVolBranchSaddleEpsilon, 0, nIncoming, mergedTopVolBranchSaddleEpsilon, 0);
        viskores::cont::Algorithm::CopySubRange(
          incomingTopVolBranchUpperEnd, 0, nIncoming, mergedTopVolBranchUpperEnd, 0);
        viskores::cont::Algorithm::CopySubRange(
          incomingTopVolBranchLowerEnd, 0, nIncoming, mergedTopVolBranchLowerEnd, 0);
        viskores::cont::Algorithm::CopySubRange<ValueType, ValueType>(
          incomingTopVolBranchSaddleIsoValue, 0, nIncoming, mergedTopVolBranchSaddleIsoValue, 0);
        viskores::cont::Algorithm::CopySubRange(
          b->TopVolumeBranchRootGRId, 0, nSelf, mergedTopVolBranchGRId, nIncoming);
        viskores::cont::Algorithm::CopySubRange(
          b->TopVolumeBranchVolume, 0, nSelf, mergedTopVolBranchVolume, nIncoming);
        viskores::cont::Algorithm::CopySubRange(
          b->TopVolumeBranchSaddleEpsilon, 0, nSelf, mergedTopVolBranchSaddleEpsilon, nIncoming);
        viskores::cont::Algorithm::CopySubRange(
          b->TopVolumeBranchUpperEndGRId, 0, nSelf, mergedTopVolBranchUpperEnd, nIncoming);
        viskores::cont::Algorithm::CopySubRange(
          b->TopVolumeBranchLowerEndGRId, 0, nSelf, mergedTopVolBranchLowerEnd, nIncoming);
        viskores::cont::Algorithm::CopySubRange<ValueType, ValueType>(
          inArray, 0, nSelf, mergedTopVolBranchSaddleIsoValue, nIncoming);

        // Sort all branches (incoming + self) based on volume
        // sorting key: (volume, branch global regular ID)
        // the highest volume comes first, the lowest branch GR ID comes first
        viskores::cont::ArrayHandleIndex mergedBranchId(nIncoming + nSelf);
        IdArrayType sortedBranchId;
        viskores::cont::Algorithm::Copy(mergedBranchId, sortedBranchId);
        viskores::worklet::scalar_topology::select_top_volume_contours::BranchVolumeComparator
          branchVolumeComparator(mergedTopVolBranchGRId, mergedTopVolBranchVolume);
        viskores::cont::Algorithm::Sort(sortedBranchId, branchVolumeComparator);

        // permute the branch information based on sorting
        IdArrayType permutedTopVolBranchGRId;
        viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id, IdArrayType>(
          mergedTopVolBranchGRId, sortedBranchId, permutedTopVolBranchGRId);
        IdArrayType permutedTopVolBranchVolume;
        viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id, IdArrayType>(
          mergedTopVolBranchVolume, sortedBranchId, permutedTopVolBranchVolume);
        IdArrayType permutedTopVolBranchSaddleEpsilon;
        viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id, IdArrayType>(
          mergedTopVolBranchSaddleEpsilon, sortedBranchId, permutedTopVolBranchSaddleEpsilon);
        IdArrayType permutedTopVolBranchUpperEnd;
        viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id, IdArrayType>(
          mergedTopVolBranchUpperEnd, sortedBranchId, permutedTopVolBranchUpperEnd);
        IdArrayType permutedTopVolBranchLowerEnd;
        viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id, IdArrayType>(
          mergedTopVolBranchLowerEnd, sortedBranchId, permutedTopVolBranchLowerEnd);
        InArrayHandleType permutedTopVolBranchSaddleIsoValue;
        viskores::worklet::contourtree_augmented::PermuteArrayWithRawIndex<InArrayHandleType>(
          mergedTopVolBranchSaddleIsoValue, sortedBranchId, permutedTopVolBranchSaddleIsoValue);

#ifdef DEBUG_PRINT_COMBINED_HIGH_VOLUME_BRANCH
        {
          std::stringstream rs;
          viskores::worklet::contourtree_augmented::PrintHeader(nIncoming + nSelf, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "permutedTopBranchId", permutedTopVolBranchGRId, -1, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "permutedTopBranchVol", permutedTopVolBranchVolume, -1, rs);
          viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
            "permutedTopSaddleVal", permutedTopVolBranchSaddleIsoValue, -1, rs);
        }
#endif

        // there may be duplicate branches. We remove duplicate branches based on global regular IDs
        // We can reuse the filter from removing duplicate branches in the process of collecting branches
        IdArrayType oneIfUniqueBranch;
        oneIfUniqueBranch.Allocate(nIncoming + nSelf);
        viskores::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
          OneIfBranchEndWorklet oneIfUniqueWorklet;
        invoke(oneIfUniqueWorklet, mergedBranchId, permutedTopVolBranchGRId, oneIfUniqueBranch);

        // Remove duplicate
        IdArrayType mergedUniqueBranchGRId;
        IdArrayType mergedUniqueBranchVolume;
        IdArrayType mergedUniqueBranchSaddleEpsilon;
        IdArrayType mergedUniqueBranchUpperEnd;
        IdArrayType mergedUniqueBranchLowerEnd;
        InArrayHandleType mergedUniqueBranchSaddleIsoValue;

        viskores::cont::Algorithm::CopyIf(
          permutedTopVolBranchGRId, oneIfUniqueBranch, mergedUniqueBranchGRId);
        viskores::cont::Algorithm::CopyIf(
          permutedTopVolBranchVolume, oneIfUniqueBranch, mergedUniqueBranchVolume);
        viskores::cont::Algorithm::CopyIf(
          permutedTopVolBranchSaddleEpsilon, oneIfUniqueBranch, mergedUniqueBranchSaddleEpsilon);
        viskores::cont::Algorithm::CopyIf(
          permutedTopVolBranchUpperEnd, oneIfUniqueBranch, mergedUniqueBranchUpperEnd);
        viskores::cont::Algorithm::CopyIf(
          permutedTopVolBranchLowerEnd, oneIfUniqueBranch, mergedUniqueBranchLowerEnd);
        viskores::cont::Algorithm::CopyIf(
          permutedTopVolBranchSaddleIsoValue, oneIfUniqueBranch, mergedUniqueBranchSaddleIsoValue);

        viskores::Id nMergedUnique = mergedUniqueBranchGRId.GetNumberOfValues();

#ifdef DEBUG_PRINT_COMBINED_HIGH_VOLUME_BRANCH
        {
          std::stringstream rs;
          viskores::worklet::contourtree_augmented::PrintHeader(nMergedUnique, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "mergedUniqueBranchId", mergedUniqueBranchGRId, -1, rs);
          viskores::worklet::contourtree_augmented::PrintIndices(
            "mergedUniqueBranchVol", mergedUniqueBranchVolume, -1, rs);
          viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
            "mergedUniqueSaddleVal", mergedUniqueBranchSaddleIsoValue, -1, rs);
        }
#endif

        // After removing duplicate, if there are more branches than we need
        // We only save the top nSavedBranches branches
        if (nMergedUnique > this->nSavedBranches)
        {
          viskores::cont::Algorithm::CopySubRange(
            mergedUniqueBranchGRId, 0, this->nSavedBranches, b->TopVolumeBranchRootGRId);
          viskores::cont::Algorithm::CopySubRange(
            mergedUniqueBranchVolume, 0, this->nSavedBranches, b->TopVolumeBranchVolume);
          viskores::cont::Algorithm::CopySubRange(mergedUniqueBranchSaddleEpsilon,
                                              0,
                                              this->nSavedBranches,
                                              b->TopVolumeBranchSaddleEpsilon);
          viskores::cont::Algorithm::CopySubRange(
            mergedUniqueBranchUpperEnd, 0, this->nSavedBranches, b->TopVolumeBranchUpperEndGRId);
          viskores::cont::Algorithm::CopySubRange(
            mergedUniqueBranchLowerEnd, 0, this->nSavedBranches, b->TopVolumeBranchLowerEndGRId);
          // InArrayHandleType subRangeUniqueBranchSaddleIsoValue;
          inArray.Allocate(this->nSavedBranches);
          viskores::cont::Algorithm::CopySubRange(
            mergedUniqueBranchSaddleIsoValue, 0, this->nSavedBranches, inArray);
          // inArray = subRangeUniqueBranchSaddleIsoValue;
        }
        else
        {
          viskores::cont::Algorithm::Copy(mergedUniqueBranchGRId, b->TopVolumeBranchRootGRId);
          viskores::cont::Algorithm::Copy(mergedUniqueBranchVolume, b->TopVolumeBranchVolume);
          viskores::cont::Algorithm::Copy(mergedUniqueBranchSaddleEpsilon,
                                      b->TopVolumeBranchSaddleEpsilon);
          viskores::cont::Algorithm::Copy(mergedUniqueBranchUpperEnd, b->TopVolumeBranchUpperEndGRId);
          viskores::cont::Algorithm::Copy(mergedUniqueBranchLowerEnd, b->TopVolumeBranchLowerEndGRId);
          inArray.Allocate(nMergedUnique);
          viskores::cont::Algorithm::Copy(mergedUniqueBranchSaddleIsoValue, inArray);
        }
      };
      b->TopVolumeBranchSaddleIsoValue
        .CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(resolveArray);
    }
  }
}

} // namespace internal
} // namespace scalar_topology
} // namespace filter
} // namespace viskores
