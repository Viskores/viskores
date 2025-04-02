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
//=======================================================================================
//
//  Parallel Peak Pruning v. 2.0
//
//  Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// BranchDecompositionTreeMaker.h
//
//=======================================================================================
//
// COMMENTS:
//
//      This class computes the branch decomposition tree of top-volume branches
//
//=======================================================================================


#ifndef vtk_m_filter_scalar_topology_worklet_BranchDecompositionTreeMaker_h
#define vtk_m_filter_scalar_topology_worklet_BranchDecompositionTreeMaker_h


#ifdef DEBUG_PRINT
#define DEBUG_BRANCH_DECOMPOSITION_TREE_MAKER
#endif

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/NotNoSuchElementPredicate.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/AssignValueWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/BinarySearchWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/BranchParentComparator.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/ClarifyBranchEndSupernodeTypeWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/GetBranchHierarchyWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/GetBranchVolumeWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/Predicates.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/TopVolumeBranchData.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/UpdateInfoByBranchDirectionWorklet.h>

namespace vtkm
{
namespace filter
{
namespace scalar_topology
{

/// Facture class for augmenting the hierarchical contour tree to enable computations of measures, e.g., volumne
class BranchDecompositionTreeMaker
{ // class BranchDecompositionTreeMaker
public:
  void ComputeTopVolumeBranchHierarchy(const vtkm::cont::DataSet& bdDataSet,
                                       TopVolumeBranchData& topVolumeData);
}; // class BranchDecompositionTreeMaker


/// <summary>
///   Pipeline to compute the hierarchy of top branches by volume
/// </summary>
inline void BranchDecompositionTreeMaker::ComputeTopVolumeBranchHierarchy(
  const vtkm::cont::DataSet& bdDataSet,
  TopVolumeBranchData& topVolumeData)
{
  using vtkm::worklet::contourtree_augmented::IdArrayType;

  // Used internally to Invoke worklets
  vtkm::cont::Invoker invoke;

  // NOTE: Any variables without "LocalEnd" refer to branch global ends
  // we need upper/lower local ends and global ends for hierarchy of branches
  auto upperLocalEndIds = bdDataSet.GetField("UpperEndLocalIds")
                            .GetData()
                            .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto lowerLocalEndIds = bdDataSet.GetField("LowerEndLocalIds")
                            .GetData()
                            .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto globalRegularIds = bdDataSet.GetField("RegularNodeGlobalIds")
                            .GetData()
                            .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  IdArrayType upperEndGRIds =
    bdDataSet.GetField("UpperEndGlobalRegularIds").GetData().AsArrayHandle<IdArrayType>();
  IdArrayType lowerEndGRIds =
    bdDataSet.GetField("LowerEndGlobalRegularIds").GetData().AsArrayHandle<IdArrayType>();

  // let's check which top volume branches are known by the block
  // We check the branchGRId of top volume branches to see whether there are matches within the block
  const vtkm::Id nTopVolBranches = topVolumeData.TopVolumeBranchLowerEndGRId.GetNumberOfValues();
  // sortedBranchOrder: the branch order (in the ascending order of branch root)
  // the high-level idea is to sort the branch root global regular ids
  // and for each top-volume branch, we use binary search to get the original branch index
  // if the top-volume branch does not exist in the block, it will be dropped out
  IdArrayType sortedBranchGRId;
  IdArrayType sortedBranchOrder;
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleIndex(topVolumeData.BranchRootGRId.GetNumberOfValues()),
    sortedBranchOrder);
  vtkm::cont::Algorithm::Copy(topVolumeData.BranchRootGRId, sortedBranchGRId);
  vtkm::cont::Algorithm::SortByKey(sortedBranchGRId, sortedBranchOrder);

  topVolumeData.TopVolBranchKnownByBlockStencil.Allocate(nTopVolBranches);
  topVolumeData.TopVolBranchGROrder.Allocate(nTopVolBranches);

  // We use a custom BinarySearchWorklet.
  // This worklet searches for given values in a sorted array and returns the stencil & index if the value exists in the array.
  // topVolumeData.TopVolBranchGROrder: the order of the topVolBranch (by global regular ids)
  //                            among all known branches.
  auto idxIfBranchWithinBlockWorklet =
    vtkm::worklet::scalar_topology::select_top_volume_branches::BinarySearchWorklet();
  invoke(idxIfBranchWithinBlockWorklet,
         topVolumeData.TopVolumeBranchRootGRId,
         sortedBranchGRId,
         topVolumeData.TopVolBranchKnownByBlockStencil,
         topVolumeData.TopVolBranchGROrder);

  // Dropping out top-volume branches that are not known by the block.

  // the index of top-volume branches known by the block among all top-volume branches
  IdArrayType topVolBranchKnownByBlockIndex;
  vtkm::cont::ArrayHandleIndex topVolBranchesIndex(nTopVolBranches);
  vtkm::cont::Algorithm::CopyIf(topVolBranchesIndex,
                                topVolumeData.TopVolBranchKnownByBlockStencil,
                                topVolBranchKnownByBlockIndex);

  const vtkm::Id nTopVolBranchKnownByBlock = topVolBranchKnownByBlockIndex.GetNumberOfValues();

  // filtered topVolumeData.TopVolBranchGROrder, by removing NO_SUCH_ELEMENT
  IdArrayType topVolBranchFilteredGROrder;

  // topVolumeData.TopVolBranchInfoActualIndex: the information index of the top-volume branch
  vtkm::cont::Algorithm::CopyIf(topVolumeData.TopVolBranchGROrder,
                                topVolumeData.TopVolBranchKnownByBlockStencil,
                                topVolBranchFilteredGROrder);
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    sortedBranchOrder, topVolBranchFilteredGROrder, topVolumeData.TopVolBranchInfoActualIndex);

  // filtered branch saddle epsilons, global lower/upper end GR ids,
  IdArrayType topVolFilteredBranchSaddleEpsilon;
  IdArrayType topVolFilteredLowerEndGRId;
  IdArrayType topVolFilteredUpperEndGRId;
  vtkm::cont::Algorithm::CopyIf(topVolumeData.TopVolumeBranchSaddleEpsilon,
                                topVolumeData.TopVolBranchKnownByBlockStencil,
                                topVolFilteredBranchSaddleEpsilon);
  vtkm::cont::Algorithm::CopyIf(topVolumeData.TopVolumeBranchUpperEndGRId,
                                topVolumeData.TopVolBranchKnownByBlockStencil,
                                topVolFilteredUpperEndGRId);
  vtkm::cont::Algorithm::CopyIf(topVolumeData.TopVolumeBranchLowerEndGRId,
                                topVolumeData.TopVolBranchKnownByBlockStencil,
                                topVolFilteredLowerEndGRId);

  // for each top-vol branch known by the block
  // we get their upper end and lower end local ids
  IdArrayType topVolBranchUpperLocalEnd;
  IdArrayType topVolBranchLowerLocalEnd;
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    upperLocalEndIds, topVolumeData.TopVolBranchInfoActualIndex, topVolBranchUpperLocalEnd);
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    lowerLocalEndIds, topVolumeData.TopVolBranchInfoActualIndex, topVolBranchLowerLocalEnd);

  IdArrayType topVolLowerLocalEndGRId;
  IdArrayType topVolUpperLocalEndGRId;
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    globalRegularIds, topVolBranchLowerLocalEnd, topVolLowerLocalEndGRId);
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    globalRegularIds, topVolBranchUpperLocalEnd, topVolUpperLocalEndGRId);

  // Below is the code to compute the branch hierarchy of top-volume branches
  // We need this information because we not only want to visualize the contour
  // on top-volume branches, but also on their parent branches.
  // Because we use volume as the metric, the parent branch of a top-volume branch
  // is either a top-volume branch or the root branch (where both ends are leaf nodes)
  vtkm::worklet::scalar_topology::select_top_volume_branches::BranchSaddleIsKnownWorklet
    branchSaddleIsKnownWorklet;
  // the branch saddle local ID if the saddle end is known by the block
  IdArrayType branchSaddleIsKnown;
  branchSaddleIsKnown.Allocate(nTopVolBranchKnownByBlock);

  invoke(branchSaddleIsKnownWorklet,        // worklet
         topVolFilteredLowerEndGRId,        // input
         topVolBranchLowerLocalEnd,         // input
         topVolLowerLocalEndGRId,           // input
         topVolFilteredUpperEndGRId,        // input
         topVolBranchUpperLocalEnd,         // input
         topVolUpperLocalEndGRId,           // input
         topVolFilteredBranchSaddleEpsilon, // input
         branchSaddleIsKnown);              // output
  // the order of top volume branches with parents known by the block
  IdArrayType topVolChildBranch;
  IdArrayType topVolChildBranchSaddle;

  vtkm::cont::Algorithm::CopyIf(topVolBranchKnownByBlockIndex,
                                branchSaddleIsKnown,
                                topVolChildBranch,
                                vtkm::worklet::contourtree_augmented::NotNoSuchElementPredicate());
  vtkm::cont::Algorithm::CopyIf(branchSaddleIsKnown,
                                branchSaddleIsKnown,
                                topVolChildBranchSaddle,
                                vtkm::worklet::contourtree_augmented::NotNoSuchElementPredicate());

  const vtkm::Id nChildBranch = topVolChildBranch.GetNumberOfValues();
  // to compute the parent branch, we need to
  // 1. for the branch saddle end, collect all superarcs involving it
  // 2. get the branch information for selected superarcs
  // 3. eliminate branch information for branches sharing the same saddle end
  auto superarcs =
    bdDataSet.GetField("Superarcs").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto branchRoots =
    bdDataSet.GetField("BranchRoots").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  VTKM_ASSERT(superarcs.GetNumberOfValues() == branchRoots.GetNumberOfValues());

  // we sort all superarcs by target to allow binary search
  IdArrayType superarcsByTarget;
  vtkm::worklet::scalar_topology::select_top_volume_branches::SuperarcTargetComparator
    superarcComparator(superarcs);
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleIndex(superarcs.GetNumberOfValues()),
                              superarcsByTarget);
  vtkm::cont::Algorithm::Sort(superarcsByTarget, superarcComparator);

  IdArrayType permutedSuperarcs;
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    superarcs, superarcsByTarget, permutedSuperarcs);

  IdArrayType permutedBranchRoots;
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    branchRoots, superarcsByTarget, permutedBranchRoots);

  // the branch root of the superarc of the branch saddle supernode
  IdArrayType topVolChildBranchSaddleBranchRoot;
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    branchRoots, topVolChildBranchSaddle, topVolChildBranchSaddleBranchRoot);

  // the GR Ids of the superarc of the branch saddle supernode
  IdArrayType topVolChildBranchSaddleGRIds;
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    globalRegularIds, topVolChildBranchSaddle, topVolChildBranchSaddleGRIds);
  // there is a debate to find all superarcs connect to a supernode
  // strategy 1. iterate through saddles and parallelize over superarcs for search
  // time complexity: O(nTopVolBranches)
  //   (nTopVolBranches usually <= 100, based on input parameter setting)
  //
  // strategy 2. parallelize over all saddles and use binary search to find superarcs
  // time complexity: O(log_2(nSuperarcs)) (nSuperarcs can be considerably large)
  //
  // here, we choose strategy 2 for better scalability to high nTopVolBranches
  // but when nTopVolBranches <= 10, strategy 1 is theoretically faster

  // note: after getting the branch root superarc, we use binary search to get the branch order
  // because BranchRootByBranch is sorted by branch root (superarc) id

#ifdef DEBUG_PRINT
  std::stringstream parentBranchStream;
  vtkm::worklet::contourtree_augmented::PrintHeader(nChildBranch, parentBranchStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Child Branch Saddle", topVolChildBranchSaddle, -1, parentBranchStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Child Saddle Root", topVolChildBranchSaddleBranchRoot, -1, parentBranchStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Child Saddle GR Id", topVolChildBranchSaddleGRIds, -1, parentBranchStream);
  // the volume of the child branch
  IdArrayType topVolChildBranchVolume;
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    topVolumeData.TopVolumeBranchVolume, topVolChildBranch, topVolChildBranchVolume);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Child Branch Volume", topVolChildBranchVolume, -1, parentBranchStream);

  vtkm::worklet::contourtree_augmented::PrintHeader(superarcs.GetNumberOfValues(),
                                                    parentBranchStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Permuted Superarcs", permutedSuperarcs, -1, parentBranchStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Permuted Branch roots", permutedBranchRoots, -1, parentBranchStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "BranchRootByBranch", topVolumeData.BranchRootByBranch, -1, parentBranchStream);

  VTKM_LOG_S(vtkm::cont::LogLevel::Info, parentBranchStream.str());
#endif // DEBUG_PRINT

  // the corresponding parent branch of child branches
  IdArrayType topVolChildBranchParent;
  topVolChildBranchParent.AllocateAndFill(nChildBranch,
                                          vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT);
  vtkm::worklet::scalar_topology::select_top_volume_branches::GetParentBranchWorklet
    getParentBranchWorklet;
  invoke(getParentBranchWorklet,
         topVolChildBranchSaddle,
         topVolChildBranchSaddleBranchRoot,
         topVolChildBranchSaddleGRIds,
         permutedSuperarcs,
         permutedBranchRoots,
         topVolumeData.BranchRootByBranch,
         upperEndGRIds,
         lowerEndGRIds,
         topVolChildBranchParent);

  topVolumeData.TopVolumeBranchParent.AllocateAndFill(
    nTopVolBranches, vtkm::Id(vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT));

  vtkm::worklet::scalar_topology::select_top_volume_branches::AssignValueByIndex assignParentBranch;
  // for each top volume branch, assign the parent branch info id in the block
  invoke(assignParentBranch,
         topVolChildBranch,
         topVolChildBranchParent,
         topVolumeData.TopVolumeBranchParent);
  // for each branch, assign true if it is a parent branch
  invoke(assignParentBranch,
         topVolChildBranchParent,
         vtkm::cont::ArrayHandleConstant<bool>(true, nChildBranch),
         topVolumeData.IsParentBranch);
  // sort all top-volume branches based on
  // 1. parent branch info id: topVolumeData.TopVolumeBranchParent
  // 2. saddle-end value: topVolumeData.TopVolumeBranchSaddleIsovalue
  // 3. branch root global regular id (anything that can break tie)
  IdArrayType topVolSortForOuterSaddleIdx;
  vtkm::cont::Algorithm::Copy(topVolBranchesIndex, topVolSortForOuterSaddleIdx);

  auto resolveBranchParentSorter = [&](const auto& inArray) {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    using ValueType = typename InArrayHandleType::ValueType;

    vtkm::worklet::scalar_topology::select_top_volume_branches::BranchParentComparator<ValueType>
      parentComparator(
        topVolumeData.TopVolumeBranchParent, inArray, topVolumeData.TopVolumeBranchRootGRId);

    // sort index for all top volume branches
    vtkm::cont::Algorithm::Sort(topVolSortForOuterSaddleIdx, parentComparator);
  };
  topVolumeData.TopVolumeBranchSaddleIsoValue
    .CastAndCallForTypes<vtkm::TypeListScalarAll, vtkm::cont::StorageListBasic>(
      resolveBranchParentSorter);

  IdArrayType parentPermutation;
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    topVolumeData.TopVolumeBranchParent, topVolSortForOuterSaddleIdx, parentPermutation);

  // When parent is NO_SUCH_ELEMENT, parentSaddleEps obtains 0
  // However, the corresponding element will be discarded in collecting outer saddles
  IdArrayType parentSaddleEpsPermutation;
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    topVolumeData.BranchSaddleEpsilon, parentPermutation, parentSaddleEpsPermutation);

  // Some branches have parent=NO_SUCH_ELEMENT (no parent)
  // we collect the isovalue of the first and/or the last branches for each parent branch
  // we collect the first if branchSaddleEpsilon(parent) < 0
  //         or the last if branchSaddleEpsilon(parent) > 0
  //         or both if branchSaddleEpsilon(parent) == 0
  IdArrayType IsOuterSaddle;
  IsOuterSaddle.Allocate(nTopVolBranches);
  vtkm::worklet::scalar_topology::select_top_volume_branches::CollectOuterSaddle
    collectOuterSaddleWorklet;
  invoke(collectOuterSaddleWorklet, parentSaddleEpsPermutation, parentPermutation, IsOuterSaddle);

  // after sorting by index back
  // each top volume branch know whether it is the outer saddle of its parent
  vtkm::cont::Algorithm::SortByKey(topVolSortForOuterSaddleIdx, IsOuterSaddle);

  // collect branches that need contours on extra minima/maxima
  // we store the information of the parent branches (on both directions)
  IdArrayType extraMaximaParentBranch;
  IdArrayType extraMinimaParentBranch;
  IdArrayType extraMaximaParentBranchRootGRId;
  IdArrayType extraMinimaParentBranchRootGRId;

  IdArrayType allBranchGRIdByVolume;
  IdArrayType branchGRIdByVolumeIdx;

  // we need global branch order including the root branch
  // this information should be consistent globally
  allBranchGRIdByVolume.Allocate(nTopVolBranches + 1);
  vtkm::cont::Algorithm::CopySubRange(
    topVolumeData.TopVolumeBranchRootGRId, 0, nTopVolBranches, allBranchGRIdByVolume, 1);

  // we manually insert the main branch into allBranchGRIdByVolume
  auto topBranchGRIdWritePortal = allBranchGRIdByVolume.WritePortal();
  auto sortedBranchByVolPortal = topVolumeData.SortedBranchByVolume.ReadPortal();
  auto branchGRIdReadPortal = topVolumeData.BranchRootGRId.ReadPortal();
  topBranchGRIdWritePortal.Set(0, branchGRIdReadPortal.Get(sortedBranchByVolPortal.Get(0)));

  // sort branches by branch root global regular ids
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleIndex(allBranchGRIdByVolume.GetNumberOfValues()), branchGRIdByVolumeIdx);
  vtkm::cont::Algorithm::SortByKey(allBranchGRIdByVolume, branchGRIdByVolumeIdx);

  // find out which branches are parents for the saddle-maxima branches
  vtkm::cont::Algorithm::CopyIf(
    topVolumeData.TopVolumeBranchParent,
    IsOuterSaddle,
    extraMaximaParentBranch,
    vtkm::worklet::scalar_topology::select_top_volume_branches::IsExtraMaximum());

  // find out which branches are parents for the saddle-minima branches
  vtkm::cont::Algorithm::CopyIf(
    topVolumeData.TopVolumeBranchParent,
    IsOuterSaddle,
    extraMinimaParentBranch,
    vtkm::worklet::scalar_topology::select_top_volume_branches::IsExtraMinimum());

  // Update 01/09/2025
  // We record the saddle end global regular IDs for each parent branch.
  // This array will be used for extra branches on both sides.
  IdArrayType topVolumeBranchSaddleEndGRId;
  vtkm::cont::Algorithm::Copy(topVolumeData.TopVolumeBranchUpperEndGRId,
                              topVolumeBranchSaddleEndGRId);
  invoke(vtkm::worklet::scalar_topology::select_top_volume_branches::AssignValueByPositivity{},
         topVolumeData.TopVolumeBranchSaddleEpsilon,
         topVolumeData.TopVolumeBranchLowerEndGRId,
         topVolumeBranchSaddleEndGRId);

  // if we have parent branches to extract contours above the saddle ends of the child branch
  if (extraMaximaParentBranch.GetNumberOfValues())
  {
    vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
      upperLocalEndIds, extraMaximaParentBranch, topVolumeData.ExtraMaximaBranchUpperEnd);
    // WARNING: the lower end of these extra branches should be the separating saddle
    // i.e., the saddle that splits the child branch and the other upper side of the parent branch.
    vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
      lowerLocalEndIds, extraMaximaParentBranch, topVolumeData.ExtraMaximaBranchLowerEnd);
    vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
      topVolumeData.BranchRootGRId, extraMaximaParentBranch, extraMaximaParentBranchRootGRId);

    // it is safe to use lower bounds here because the branch should be findable
    IdArrayType permutedExtraMaximaBranchOrder;
    vtkm::cont::Algorithm::LowerBounds(
      allBranchGRIdByVolume, extraMaximaParentBranchRootGRId, permutedExtraMaximaBranchOrder);

    vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
      branchGRIdByVolumeIdx, permutedExtraMaximaBranchOrder, topVolumeData.ExtraMaximaBranchOrder);

    // Update 01/09/2025
    // We record the saddle end global regular IDs for each parent branch.
    vtkm::cont::Algorithm::CopyIf(
      topVolumeBranchSaddleEndGRId,
      IsOuterSaddle,
      topVolumeData.ExtraMaximaBranchSaddleGRId,
      vtkm::worklet::scalar_topology::select_top_volume_branches::IsExtraMaximum());
  }

  // if we have parent branches to extract contours below the saddle ends of the child branch
  if (extraMinimaParentBranch.GetNumberOfValues())
  {
    vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
      upperLocalEndIds, extraMinimaParentBranch, topVolumeData.ExtraMinimaBranchUpperEnd);
    vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
      lowerLocalEndIds, extraMinimaParentBranch, topVolumeData.ExtraMinimaBranchLowerEnd);
    vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
      topVolumeData.BranchRootGRId, extraMinimaParentBranch, extraMinimaParentBranchRootGRId);

    // it is safe to use lower bounds here because the branch should be findable
    IdArrayType permutedExtraMinimaBranchOrder;
    vtkm::cont::Algorithm::LowerBounds(
      allBranchGRIdByVolume, extraMinimaParentBranchRootGRId, permutedExtraMinimaBranchOrder);

    vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
      branchGRIdByVolumeIdx, permutedExtraMinimaBranchOrder, topVolumeData.ExtraMinimaBranchOrder);

    // Update 01/09/2025
    // We record the saddle end global regular IDs for each parent branch.
    vtkm::cont::Algorithm::CopyIf(
      topVolumeBranchSaddleEndGRId,
      IsOuterSaddle,
      topVolumeData.ExtraMinimaBranchSaddleGRId,
      vtkm::worklet::scalar_topology::select_top_volume_branches::IsExtraMinimum());
  }

  // Update saddle isovalues for extra contours
  auto resolveExtraContourSaddleValue = [&](const auto& inArray) {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;

    if (extraMaximaParentBranch.GetNumberOfValues())
    {
      InArrayHandleType extraMaximaBranchIsoValue;
      vtkm::cont::Algorithm::CopyIf(
        inArray,
        IsOuterSaddle,
        extraMaximaBranchIsoValue,
        vtkm::worklet::scalar_topology::select_top_volume_branches::IsExtraMaximum());
      topVolumeData.ExtraMaximaBranchIsoValue = extraMaximaBranchIsoValue;
    }

    if (extraMinimaParentBranch.GetNumberOfValues())
    {
      InArrayHandleType extraMinimaBranchIsoValue;
      vtkm::cont::Algorithm::CopyIf(
        inArray,
        IsOuterSaddle,
        extraMinimaBranchIsoValue,
        vtkm::worklet::scalar_topology::select_top_volume_branches::IsExtraMinimum());
      topVolumeData.ExtraMinimaBranchIsoValue = extraMinimaBranchIsoValue;
    }
  };
  topVolumeData.TopVolumeBranchSaddleIsoValue
    .CastAndCallForTypes<vtkm::TypeListScalarAll, vtkm::cont::StorageListBasic>(
      resolveExtraContourSaddleValue);
}


} // namespace scalar_topology
} // namespace filter
} // namespace vtkm

#endif
