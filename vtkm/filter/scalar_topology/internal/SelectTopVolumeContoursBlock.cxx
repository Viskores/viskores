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
#include <vtkm/filter/scalar_topology/internal/SelectTopVolumeContoursBlock.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_contours/ClarifyBranchEndSupernodeTypeWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_contours/GetBranchVolumeWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_contours/UpdateInfoByBranchDirectionWorklet.h>

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

SelectTopVolumeContoursBlock::SelectTopVolumeContoursBlock(vtkm::Id localBlockNo, int globalBlockId)
  : LocalBlockNo(localBlockNo)
  , GlobalBlockId(globalBlockId)
{
}

void SelectTopVolumeContoursBlock::SortBranchByVolume(
  const vtkm::cont::DataSet& hierarchicalTreeDataSet,
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

  auto upperEndIntrinsicVolume = hierarchicalTreeDataSet.GetField("UpperEndIntrinsicVolume")
                                   .GetData()
                                   .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto upperEndDependentVolume = hierarchicalTreeDataSet.GetField("UpperEndDependentVolume")
                                   .GetData()
                                   .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto lowerEndIntrinsicVolume = hierarchicalTreeDataSet.GetField("LowerEndIntrinsicVolume")
                                   .GetData()
                                   .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto lowerEndDependentVolume = hierarchicalTreeDataSet.GetField("LowerEndDependentVolume")
                                   .GetData()
                                   .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto lowerEndSuperarcId = hierarchicalTreeDataSet.GetField("LowerEndSuperarcId")
                              .GetData()
                              .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto upperEndSuperarcId = hierarchicalTreeDataSet.GetField("UpperEndSuperarcId")
                              .GetData()
                              .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto branchRoot = hierarchicalTreeDataSet.GetField("BranchRoot")
                      .GetData()
                      .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  vtkm::cont::Algorithm::Transform(
    upperEndIntrinsicVolume, upperEndDependentVolume, isUpperLeaf, vtkm::Equal());
  vtkm::cont::Algorithm::Transform(
    lowerEndIntrinsicVolume, lowerEndDependentVolume, isLowerLeaf, vtkm::Equal());


  // NOTE: special cases (one-superarc branches) exist
  // if the upper end superarc == lower end superarc == branch root superarc
  // then it's probably not a leaf-leaf branch (Both equality has to be satisfied!)
  // exception: the entire domain has only one superarc (intrinsic == dependent == total - 1)
  // then it is a leaf-leaf branch
  vtkm::cont::Invoker invoke;

  vtkm::worklet::scalar_topology::select_top_volume_contours::ClarifyBranchEndSupernodeTypeWorklet
    clarifyNodeTypeWorklet(totalVolume);

  invoke(clarifyNodeTypeWorklet,
         lowerEndSuperarcId,
         lowerEndIntrinsicVolume,
         upperEndSuperarcId,
         upperEndIntrinsicVolume,
         branchRoot,
         isLowerLeaf,
         isUpperLeaf);

  vtkm::cont::UnknownArrayHandle upperEndValue =
    hierarchicalTreeDataSet.GetField("UpperEndValue").GetData();

  // Based on the direction info of the branch, store epsilon direction and isovalue of the saddle
  auto resolveArray = [&](const auto& inArray) {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    using ValueType = typename InArrayHandleType::ValueType;

    vtkm::cont::ArrayHandle<ValueType> branchSaddleIsoValue;
    branchSaddleIsoValue.Allocate(isLowerLeaf.GetNumberOfValues());
    this->BranchSaddleEpsilon.Allocate(isLowerLeaf.GetNumberOfValues());

    vtkm::worklet::scalar_topology::select_top_volume_contours::UpdateInfoByBranchDirectionWorklet<
      ValueType>
      updateInfoWorklet;
    auto lowerEndValue = hierarchicalTreeDataSet.GetField("LowerEndValue")
                           .GetData()
                           .AsArrayHandle<vtkm::cont::ArrayHandle<ValueType>>();

    invoke(updateInfoWorklet,
           isLowerLeaf,
           isUpperLeaf,
           inArray,
           lowerEndValue,
           this->BranchSaddleEpsilon,
           branchSaddleIsoValue);
    this->BranchSaddleIsoValue = branchSaddleIsoValue;
  };

  upperEndValue.CastAndCallForTypes<vtkm::TypeListScalarAll, vtkm::cont::StorageListBasic>(
    resolveArray);

  vtkm::worklet::contourtree_augmented::IdArrayType branchVolume;
  vtkm::worklet::scalar_topology::select_top_volume_contours::GetBranchVolumeWorklet
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

  vtkm::cont::Algorithm::Copy(branchVolume, this->BranchVolume);

  const vtkm::Id nBranches = lowerEndSuperarcId.GetNumberOfValues();
  vtkm::cont::ArrayHandleIndex branchesIdx(nBranches);
  vtkm::worklet::contourtree_augmented::IdArrayType sortedBranches;
  vtkm::cont::Algorithm::Copy(branchesIdx, sortedBranches);

  // sort the branch volume
  vtkm::cont::Algorithm::SortByKey(branchVolume, sortedBranches, vtkm::SortGreater());
  vtkm::cont::Algorithm::Copy(sortedBranches, this->SortedBranchByVolume);
}

} // namespace internal
} // namespace scalar_topology
} // namespace filter
} // namespace vtkm
