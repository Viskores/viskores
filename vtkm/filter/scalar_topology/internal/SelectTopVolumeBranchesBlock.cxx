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

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/filter/scalar_topology/internal/SelectTopVolumeBranchesBlock.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/DataSetMesh.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/FindSuperArcForUnknownNode.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/AboveThresholdWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/BranchParentComparator.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/ClarifyBranchEndSupernodeTypeWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/GetBranchHierarchyWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/GetBranchVolumeWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/Predicates.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/UpdateInfoByBranchDirectionWorklet.h>

#ifdef DEBUG_PRINT
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#endif

namespace vtkm
{
namespace filter
{
namespace scalar_topology
{
namespace internal
{

SelectTopVolumeBranchesBlock::SelectTopVolumeBranchesBlock(vtkm::Id localBlockNo, int globalBlockId)
  : LocalBlockNo(localBlockNo)
  , GlobalBlockId(globalBlockId)
{
}

void SelectTopVolumeBranchesBlock::SortBranchByVolume(const vtkm::cont::DataSet& bdDataSet,
                                                      const vtkm::Id totalVolume)
{
  /// Pipeline to compute the branch volume
  /// 1. check both ends of the branch. If both leaves, then main branch, volume = totalVolume
  /// 2. for other branches, check the direction of the inner superarc
  ///    branch volume = (inner superarc points to the senior-most node) ?
  ///                     dependentVolume[innerSuperarc] :
  ///                     reverseVolume[innerSuperarc]
  /// NOTE: reverseVolume = totalVolume - dependentVolume + intrinsicVolume

  // Generally, if ending superarc has intrinsicVol == dependentVol, then it is a leaf node
  vtkm::cont::ArrayHandle<bool> isLowerLeaf;
  vtkm::cont::ArrayHandle<bool> isUpperLeaf;

  auto upperEndIntrinsicVolume = bdDataSet.GetField("UpperEndIntrinsicVolume")
                                   .GetData()
                                   .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto upperEndDependentVolume = bdDataSet.GetField("UpperEndDependentVolume")
                                   .GetData()
                                   .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto lowerEndIntrinsicVolume = bdDataSet.GetField("LowerEndIntrinsicVolume")
                                   .GetData()
                                   .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto lowerEndDependentVolume = bdDataSet.GetField("LowerEndDependentVolume")
                                   .GetData()
                                   .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  auto lowerEndSuperarcId = bdDataSet.GetField("LowerEndSuperarcId")
                              .GetData()
                              .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto upperEndSuperarcId = bdDataSet.GetField("UpperEndSuperarcId")
                              .GetData()
                              .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto branchRoot = bdDataSet.GetField("BranchRootByBranch")
                      .GetData()
                      .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  vtkm::cont::Algorithm::Transform(
    upperEndIntrinsicVolume, upperEndDependentVolume, isUpperLeaf, vtkm::Equal());
  vtkm::cont::Algorithm::Transform(
    lowerEndIntrinsicVolume, lowerEndDependentVolume, isLowerLeaf, vtkm::Equal());

  // NOTE: special cases (one-superarc branches) exist
  // if the upper end superarc == lower end superarc == branch root superarc
  // then it's probably not a leaf-leaf branch (Both equalities have to be satisfied!)
  // exception: the entire domain has only one superarc (intrinsic == dependent == total - 1)
  // then it is a leaf-leaf branch
  vtkm::cont::Invoker invoke;

  vtkm::worklet::scalar_topology::select_top_volume_branches::ClarifyBranchEndSupernodeTypeWorklet
    clarifyNodeTypeWorklet(totalVolume);

  invoke(clarifyNodeTypeWorklet,
         lowerEndSuperarcId,
         lowerEndIntrinsicVolume,
         upperEndSuperarcId,
         upperEndIntrinsicVolume,
         branchRoot,
         isLowerLeaf,
         isUpperLeaf);

  vtkm::cont::UnknownArrayHandle upperEndValue = bdDataSet.GetField("UpperEndValue").GetData();

  // Based on the direction info of the branch, store epsilon direction and isovalue of the saddle
  auto resolveArray = [&](const auto& inArray) {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    using ValueType = typename InArrayHandleType::ValueType;

    vtkm::cont::ArrayHandle<ValueType> branchSaddleIsoValue;
    branchSaddleIsoValue.Allocate(isLowerLeaf.GetNumberOfValues());
    this->TopVolumeData.BranchSaddleEpsilon.Allocate(isLowerLeaf.GetNumberOfValues());

    vtkm::worklet::scalar_topology::select_top_volume_branches::UpdateInfoByBranchDirectionWorklet<
      ValueType>
      updateInfoWorklet;
    auto lowerEndValue = bdDataSet.GetField("LowerEndValue")
                           .GetData()
                           .AsArrayHandle<vtkm::cont::ArrayHandle<ValueType>>();

    invoke(updateInfoWorklet,
           isLowerLeaf,
           isUpperLeaf,
           inArray,
           lowerEndValue,
           this->TopVolumeData.BranchSaddleEpsilon,
           branchSaddleIsoValue);
    this->TopVolumeData.BranchSaddleIsoValue = branchSaddleIsoValue;
  };

  upperEndValue.CastAndCallForTypes<vtkm::TypeListScalarAll, vtkm::cont::StorageListBasic>(
    resolveArray);

  // Compute the branch volume based on the upper/lower end superarc volumes
  vtkm::worklet::contourtree_augmented::IdArrayType branchVolume;
  vtkm::worklet::scalar_topology::select_top_volume_branches::GetBranchVolumeWorklet
    getBranchVolumeWorklet(totalVolume);

  invoke(getBranchVolumeWorklet,  // worklet
         lowerEndSuperarcId,      // input
         lowerEndIntrinsicVolume, // input
         lowerEndDependentVolume, // input
         upperEndSuperarcId,      // input
         upperEndIntrinsicVolume, // input
         upperEndDependentVolume, // input
         isLowerLeaf,
         isUpperLeaf,
         branchVolume); // output

#ifdef DEBUG_PRINT
  std::stringstream resultStream;
  resultStream << "Branch Volume In The Block" << std::endl;
  const vtkm::Id nVolume = branchVolume.GetNumberOfValues();
  vtkm::worklet::contourtree_augmented::PrintHeader(nVolume, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "BranchVolume", branchVolume, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices("isLowerLeaf", isLowerLeaf, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices("isUpperLeaf", isUpperLeaf, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "LowerEndIntrinsicVol", lowerEndIntrinsicVolume, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "LowerEndDependentVol", lowerEndDependentVolume, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "UpperEndIntrinsicVol", upperEndIntrinsicVolume, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "UpperEndDependentVol", upperEndDependentVolume, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "LowerEndSuperarc", lowerEndSuperarcId, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "UpperEndSuperarc", upperEndSuperarcId, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices("BranchRoot", branchRoot, -1, resultStream);
  resultStream << std::endl;
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, resultStream.str());
#endif

  vtkm::cont::Algorithm::Copy(branchVolume, this->TopVolumeData.BranchVolume);

  const vtkm::Id nBranches = lowerEndSuperarcId.GetNumberOfValues();
  vtkm::cont::ArrayHandleIndex branchesIdx(nBranches);
  vtkm::worklet::contourtree_augmented::IdArrayType sortedBranches;
  vtkm::cont::Algorithm::Copy(branchesIdx, sortedBranches);

  // sort the branch volume
  vtkm::cont::Algorithm::SortByKey(branchVolume, sortedBranches, vtkm::SortGreater());
  vtkm::cont::Algorithm::Copy(sortedBranches, this->TopVolumeData.SortedBranchByVolume);
}

// Select the local top K branches by volume
void SelectTopVolumeBranchesBlock::SelectLocalTopVolumeBranches(
  const vtkm::cont::DataSet& bdDataset,
  const vtkm::Id nSavedBranches)
{
  using vtkm::worklet::contourtree_augmented::IdArrayType;
  // copy the top volume branches into a smaller array
  // we skip index 0 because it must be the main branch (which has the highest volume)
  vtkm::Id nActualSavedBranches =
    std::min(nSavedBranches, this->TopVolumeData.SortedBranchByVolume.GetNumberOfValues() - 1);

  vtkm::worklet::contourtree_augmented::IdArrayType topVolumeBranch;
  vtkm::cont::Algorithm::CopySubRange(
    this->TopVolumeData.SortedBranchByVolume, 1, nActualSavedBranches, topVolumeBranch);

  auto branchRootByBranch = bdDataset.GetField("BranchRootByBranch")
                              .GetData()
                              .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  const vtkm::Id nBranches = branchRootByBranch.GetNumberOfValues();

  auto branchRootGRId = bdDataset.GetField("BranchRootGRId")
                          .GetData()
                          .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  auto upperEndGRId = bdDataset.GetField("UpperEndGlobalRegularIds")
                        .GetData()
                        .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  auto lowerEndGRId = bdDataset.GetField("LowerEndGlobalRegularIds")
                        .GetData()
                        .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  vtkm::cont::Algorithm::Copy(branchRootByBranch, this->TopVolumeData.BranchRootByBranch);
  vtkm::cont::Algorithm::Copy(branchRootGRId, this->TopVolumeData.BranchRootGRId);

  // This seems weird, but we temporarily put the initialization of computing the branch decomposition tree here
  this->TopVolumeData.IsParentBranch.AllocateAndFill(nBranches, false);

  // we permute all branch information to align with the order by volume
  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    branchRootGRId, topVolumeBranch, this->TopVolumeData.TopVolumeBranchRootGRId);

  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    upperEndGRId, topVolumeBranch, this->TopVolumeData.TopVolumeBranchUpperEndGRId);

  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    lowerEndGRId, topVolumeBranch, this->TopVolumeData.TopVolumeBranchLowerEndGRId);

  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    this->TopVolumeData.BranchVolume, topVolumeBranch, this->TopVolumeData.TopVolumeBranchVolume);

  vtkm::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<vtkm::Id, IdArrayType>(
    this->TopVolumeData.BranchSaddleEpsilon,
    topVolumeBranch,
    this->TopVolumeData.TopVolumeBranchSaddleEpsilon);

  auto resolveArray = [&](const auto& inArray) {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    InArrayHandleType topVolBranchSaddleIsoValue;
    vtkm::worklet::contourtree_augmented::PermuteArrayWithRawIndex<InArrayHandleType>(
      inArray, topVolumeBranch, topVolBranchSaddleIsoValue);
    this->TopVolumeData.TopVolumeBranchSaddleIsoValue = topVolBranchSaddleIsoValue;
  };

  this->TopVolumeData.BranchSaddleIsoValue
    .CastAndCallForTypes<vtkm::TypeListScalarAll, vtkm::cont::StorageListBasic>(resolveArray);
}

void SelectTopVolumeBranchesBlock::ComputeTopVolumeBranchHierarchy(
  const vtkm::cont::DataSet& bdDataSet)
{
  this->BDTMaker.ComputeTopVolumeBranchHierarchy(bdDataSet, this->TopVolumeData);
}

vtkm::Id SelectTopVolumeBranchesBlock::ExcludeTopVolumeBranchByThreshold(
  const vtkm::Id presimplifyThreshold)
{
  using vtkm::worklet::contourtree_augmented::IdArrayType;

  // the stencil for top-volume branches of whether passing the threshold
  vtkm::cont::ArrayHandle<bool> topVolumeAboveThreshold;
  topVolumeAboveThreshold.AllocateAndFill(
    this->TopVolumeData.TopVolumeBranchVolume.GetNumberOfValues(), true);

  vtkm::cont::Invoker invoke;
  vtkm::worklet::scalar_topology::select_top_volume_branches::AboveThresholdWorklet
    aboveThresholdWorklet(presimplifyThreshold);
  invoke(aboveThresholdWorklet, this->TopVolumeData.TopVolumeBranchVolume, topVolumeAboveThreshold);

  // using the stencil to filter the top-volume branch information
  IdArrayType filteredTopVolumeBranchRootGRId;
  vtkm::cont::Algorithm::CopyIf(this->TopVolumeData.TopVolumeBranchRootGRId,
                                topVolumeAboveThreshold,
                                filteredTopVolumeBranchRootGRId);
  vtkm::cont::Algorithm::Copy(filteredTopVolumeBranchRootGRId,
                              this->TopVolumeData.TopVolumeBranchRootGRId);

  IdArrayType filteredTopVolumeBranchVolume;
  vtkm::cont::Algorithm::CopyIf(this->TopVolumeData.TopVolumeBranchVolume,
                                topVolumeAboveThreshold,
                                filteredTopVolumeBranchVolume);
  vtkm::cont::Algorithm::Copy(filteredTopVolumeBranchVolume,
                              this->TopVolumeData.TopVolumeBranchVolume);

  auto resolveArray = [&](auto& inArray) {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    InArrayHandleType filteredTopVolumeBranchSaddleIsoValue;
    vtkm::cont::Algorithm::CopyIf(
      inArray, topVolumeAboveThreshold, filteredTopVolumeBranchSaddleIsoValue);

    inArray.Allocate(filteredTopVolumeBranchVolume.GetNumberOfValues());
    vtkm::cont::Algorithm::Copy(filteredTopVolumeBranchSaddleIsoValue, inArray);
  };
  this->TopVolumeData.TopVolumeBranchSaddleIsoValue
    .CastAndCallForTypes<vtkm::TypeListScalarAll, vtkm::cont::StorageListBasic>(resolveArray);

  IdArrayType filteredTopVolumeBranchSaddleEpsilon;
  vtkm::cont::Algorithm::CopyIf(this->TopVolumeData.TopVolumeBranchSaddleEpsilon,
                                topVolumeAboveThreshold,
                                filteredTopVolumeBranchSaddleEpsilon);
  vtkm::cont::Algorithm::Copy(filteredTopVolumeBranchSaddleEpsilon,
                              this->TopVolumeData.TopVolumeBranchSaddleEpsilon);

  IdArrayType filteredTopVolumeBranchUpperEndGRId;
  vtkm::cont::Algorithm::CopyIf(this->TopVolumeData.TopVolumeBranchUpperEndGRId,
                                topVolumeAboveThreshold,
                                filteredTopVolumeBranchUpperEndGRId);
  vtkm::cont::Algorithm::Copy(filteredTopVolumeBranchUpperEndGRId,
                              this->TopVolumeData.TopVolumeBranchUpperEndGRId);

  IdArrayType filteredTopVolumeBranchLowerEndGRId;
  vtkm::cont::Algorithm::CopyIf(this->TopVolumeData.TopVolumeBranchLowerEndGRId,
                                topVolumeAboveThreshold,
                                filteredTopVolumeBranchLowerEndGRId);
  vtkm::cont::Algorithm::Copy(filteredTopVolumeBranchLowerEndGRId,
                              this->TopVolumeData.TopVolumeBranchLowerEndGRId);

  return filteredTopVolumeBranchVolume.GetNumberOfValues();
}

} // namespace internal
} // namespace scalar_topology
} // namespace filter
} // namespace vtkm
