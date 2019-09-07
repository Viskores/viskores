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

#ifndef vtkm_worklet_contourtree_augmented_contourtreemaker_h
#define vtkm_worklet_contourtree_augmented_contourtreemaker_h

#include <iomanip>

// local includes
#include <vtkm/worklet/contourtree_augmented/ArrayTransforms.h>
#include <vtkm/worklet/contourtree_augmented/ContourTree.h>
#include <vtkm/worklet/contourtree_augmented/MergeTree.h>
#include <vtkm/worklet/contourtree_augmented/MeshExtrema.h>
#include <vtkm/worklet/contourtree_augmented/Mesh_DEM_Triangulation.h>
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

// contourtree_maker_inc includes
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/AugmentMergeTrees_InitNewJoinSplitIDAndSuperparents.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/AugmentMergeTrees_SetAugmentedMergeArcs.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/CompressTrees_Step.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/ComputeHyperAndSuperStructure_HypernodesSetFirstSuperchild.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/ComputeHyperAndSuperStructure_PermuteArcs.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/ComputeHyperAndSuperStructure_ResetHyperparentsId.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/ComputeHyperAndSuperStructure_SetNewHypernodesAndArcs.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/ComputeRegularStructure_LocateSuperarcs.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/ComputeRegularStructure_SetArcs.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/ContourTreeNodeComparator.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/ContourTreeSuperNodeComparator.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/FindDegrees_FindRHE.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/FindDegrees_ResetUpAndDowndegree.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/FindDegrees_SubtractLHE.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/TransferLeafChains_CollapsePastRegular.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/TransferLeafChains_InitInAndOutbound.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/TransferLeafChains_TransferToContourTree.h>
#include <vtkm/worklet/contourtree_augmented/contourtreemaker/WasNotTransferred.h>

#include <vtkm/worklet/contourtree_augmented/activegraph/SuperArcNodeComparator.h>


//VTKM includes
#include <vtkm/Types.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/Invoker.h>



namespace contourtree_maker_inc_ns = vtkm::worklet::contourtree_augmented::contourtree_maker_inc;
namespace active_graph_inc_ns = vtkm::worklet::contourtree_augmented::active_graph_inc;

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{


class ContourTreeMaker
{ // class MergeTree
public:
  vtkm::cont::Invoker Invoke;

  // the contour tree, join tree & split tree to use
  ContourTree& contourTree;
  MergeTree &joinTree, &splitTree;

  // vectors of up and down degree kept during the computation
  IdArrayType updegree, downdegree;

  // vectors for tracking merge superarcs
  IdArrayType augmentedJoinSuperarcs, augmentedSplitSuperarcs;

  // vector for the active set of supernodes
  IdArrayType activeSupernodes;

  // counter for the number of iterations it took
  vtkm::Id nIterations;

  // constructor does the real work does the real work but mostly, it just calls the following two routines
  ContourTreeMaker(ContourTree& theContourTree, MergeTree& theJoinTree, MergeTree& theSplitTree);

  // computes the hyperarcs in the contour tree
  void ComputeHyperAndSuperStructure();

  // computes the regular arcs in the contour tree. Augment the contour tree with all regular vertices.
  void ComputeRegularStructure(MeshExtrema& meshExtrema);

  // compute the parital regular arcs by augmenting the contour tree with the relevant vertices on the boundary
  template <class Mesh, class MeshBoundaryExecObj>
  void ComputeBoundaryRegularStructure(MeshExtrema& meshExtrema,
                                       const Mesh& mesh,
                                       const MeshBoundaryExecObj& meshBoundary);

  // routine that augments the join & split tree with each other's supernodes
  //              the augmented trees will be stored in the joinSuperarcs / mergeSuperarcs arrays
  //              the sort IDs will be stored in the ContourTree's arrays, &c.
  void AugmentMergeTrees();

  // routine to transfer leaf chains to contour tree
  void TransferLeafChains(bool isJoin);

  // routine to collapse regular vertices
  void CompressTrees();

  // compresses active set of supernodes
  void CompressActiveSupernodes();

  // finds the degree of each supernode from the merge trees
  void FindDegrees();

  // debug routine
  void DebugPrint(const char* message, const char* fileName, long lineNum);

}; // class ContourTreeMaker


// TODO we should add an Init function to move the heavy-weight computions out of the constructor
// constructor
ContourTreeMaker::ContourTreeMaker(ContourTree& theContourTree,
                                   MergeTree& theJoinTree,
                                   MergeTree& theSplitTree)
  : contourTree(theContourTree)
  , joinTree(theJoinTree)
  , splitTree(theSplitTree)
  , updegree()
  , downdegree()
  , augmentedJoinSuperarcs()
  , augmentedSplitSuperarcs()
  , activeSupernodes()
  , nIterations(0)
{ // constructor
} //MakeContourTree()


void ContourTreeMaker::ComputeHyperAndSuperStructure()
{ // ComputeHyperAndSuperStructure()

  // augment the merge trees & establish the list of supernodes
  AugmentMergeTrees();

  // track how many iterations it takes
  nIterations = 0;

  // loop until no arcs remaining to be found
  // tree can end with either 0 or 1 vertices unprocessed
  // 0 means the last edge was pruned from both ends
  // 1 means that there were two final edges meeting at a vertex
  while (activeSupernodes.GetNumberOfValues() > 1)
  { // loop until no active vertices remaining
    // recompute the vertex degrees
    FindDegrees();

    // alternate iterations between upper & lower
    if (nIterations % 2 == 0)
      TransferLeafChains(true);
    else
      TransferLeafChains(false);

    // compress join & split trees
    CompressTrees();
    // compress the active list of supernodes
    CompressActiveSupernodes();
    nIterations++;
  } // loop until no active vertices remaining

  // test for final edges meeting
  if (activeSupernodes.GetNumberOfValues() == 1)
  { // meet at a vertex
    vtkm::Id superID = activeSupernodes.GetPortalControl().Get(0);
    contourTree.superarcs.GetPortalControl().Set(superID, (vtkm::Id)NO_SUCH_ELEMENT);
    contourTree.hyperarcs.GetPortalControl().Set(superID, (vtkm::Id)NO_SUCH_ELEMENT);
    contourTree.hyperparents.GetPortalControl().Set(superID, superID);
    contourTree.whenTransferred.GetPortalControl().Set(superID, nIterations | IS_HYPERNODE);
  } // meet at a vertex
  DebugPrint("Contour Tree Constructed. Now Swizzling", __FILE__, __LINE__);


  // next, we have to set up the hyper and super structure arrays one at a time
  // at present, all superarcs / hyperarcs are expressed in terms of supernode IDs
  // but we will want to move supernodes around.
  // the first step is therefore to find the new order of supernodes by sorting
  // we will use the hypernodes array for this, as we will want a copy to end up there
  vtkm::cont::ArrayHandleIndex initContourTreeHypernodes(
    contourTree.supernodes
      .GetNumberOfValues()); // create linear sequence of numbers 0, 1, .. nSupernodes
  vtkm::cont::Algorithm::Copy(initContourTreeHypernodes, contourTree.hypernodes);

  // now we sort hypernodes array with a comparator
  vtkm::cont::Algorithm::Sort(
    contourTree.hypernodes,
    contourtree_maker_inc_ns::ContourTreeSuperNodeComparator(
      contourTree.hyperparents, contourTree.supernodes, contourTree.whenTransferred));

  // we have to permute a bunch of arrays, so let's have some temporaries to store them
  IdArrayType permutedHyperparents;
  permuteArray<vtkm::Id>(contourTree.hyperparents, contourTree.hypernodes, permutedHyperparents);
  IdArrayType permutedSupernodes;
  permuteArray<vtkm::Id>(contourTree.supernodes, contourTree.hypernodes, permutedSupernodes);
  IdArrayType permutedSuperarcs;
  permuteArray<vtkm::Id>(contourTree.superarcs, contourTree.hypernodes, permutedSuperarcs);

  // now we establish the reverse index array
  IdArrayType superSortIndex;
  superSortIndex.Allocate(contourTree.supernodes.GetNumberOfValues());
  // The following copy is equivalent to
  // for (vtkm::Id supernode = 0; supernode < contourTree.supernodes.size(); supernode++)
  //   superSortIndex[contourTree.hypernodes[supernode]] = supernode;

  //typedef vtkm::cont::ArrayHandlePermutation<IdArrayType, vtkm::cont::ArrayHandleIndex> PermuteArrayHandleIndex;
  vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> permutedSuperSortIndex(
    contourTree.hypernodes, // index array
    superSortIndex);        // value array
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleIndex(contourTree.supernodes.GetNumberOfValues()), // source value array
    permutedSuperSortIndex);                                                  // target array

  // we then copy the supernodes & hyperparents back to the main array
  vtkm::cont::Algorithm::Copy(permutedSupernodes, contourTree.supernodes);
  vtkm::cont::Algorithm::Copy(permutedHyperparents, contourTree.hyperparents);


  // we need an extra permutation to get the superarcs correct
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_PermuteArcs permuteSuperarcsWorklet;
  this->Invoke(permuteSuperarcsWorklet,
               permutedSuperarcs,      // (input)
               superSortIndex,         // (input)
               contourTree.superarcs); // (output)

  // printIndices("Sorted", contourTree.hypernodes);
  // printIndices("Hyperparents", contourTree.hyperparents);
  // printIndices("Supernodes", contourTree.supernodes);
  // printIndices("Superarcs", contourTree.superarcs);
  // printIndices("Perm Superarcs", permutedSuperarcs);
  // printIndices("SuperSortIndex", superSortIndex);

  // we will permute the hyperarcs & copy them back with the new supernode target IDs
  IdArrayType permutedHyperarcs;
  permuteArray<vtkm::Id>(contourTree.hyperarcs, contourTree.hypernodes, permutedHyperarcs);
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_PermuteArcs permuteHyperarcsWorklet;
  this->Invoke(permuteHyperarcsWorklet, permutedHyperarcs, superSortIndex, contourTree.hyperarcs);

  // now swizzle the whenTransferred value
  IdArrayType permutedWhenTransferred;
  permuteArray<vtkm::Id>(
    contourTree.whenTransferred, contourTree.hypernodes, permutedWhenTransferred);
  vtkm::cont::Algorithm::Copy(permutedWhenTransferred, contourTree.whenTransferred);

  // now we compress both the hypernodes & hyperarcs
  IdArrayType newHypernodePosition;
  onefIfHypernode oneIfHypernodeFunctor;
  auto oneIfHypernodeArrayHandle = vtkm::cont::ArrayHandleTransform<IdArrayType, onefIfHypernode>(
    contourTree.whenTransferred, oneIfHypernodeFunctor);
  vtkm::cont::Algorithm::ScanExclusive(oneIfHypernodeArrayHandle, newHypernodePosition);

  vtkm::Id nHypernodes = 0;
  {
    vtkm::cont::ArrayHandle<vtkm::Id> temp;
    temp.Allocate(2);
    vtkm::cont::Algorithm::CopySubRange(
      newHypernodePosition, newHypernodePosition.GetNumberOfValues() - 1, 1, temp);
    vtkm::cont::Algorithm::CopySubRange(
      contourTree.whenTransferred, contourTree.whenTransferred.GetNumberOfValues() - 1, 1, temp, 1);
    auto portal = temp.GetPortalControl();
    nHypernodes = portal.Get(0) + oneIfHypernodeFunctor(portal.Get(1));
  }

  IdArrayType newHypernodes;
  newHypernodes.Allocate(nHypernodes);
  IdArrayType newHyperarcs;
  newHyperarcs.Allocate(nHypernodes);

  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_SetNewHypernodesAndArcs
    setNewHypernodesAndArcsWorklet;
  this->Invoke(setNewHypernodesAndArcsWorklet,
               contourTree.supernodes,
               contourTree.whenTransferred,
               contourTree.hypernodes,
               contourTree.hyperarcs,
               newHypernodePosition,
               newHypernodes,
               newHyperarcs);
  // swap in the new computed arrays.
  // vtkm ArrayHandles are smart so we can just swap the new data in here rather than copy
  //vtkm::cont::Algorithm::Copy(newHypernodes, contourTree.hypernodes);
  //vtkm::cont::Algorithm::Copy(newHyperarcs, contourTree.hyperarcs);
  contourTree.hypernodes.ReleaseResources();
  contourTree.hypernodes = newHypernodes;
  contourTree.hyperarcs.ReleaseResources();
  contourTree.hyperarcs = newHyperarcs;


  // now reuse the superSortIndex array for hypernode IDs
  // The following copy is equivalent to
  // for (vtkm::Id hypernode = 0; hypernode < contourTree.hypernodes.size(); hypernode++)
  //            superSortIndex[contourTree.hypernodes[hypernode]] = hypernode;
  // source data array is a simple linear index from 0 to #hypernodes
  vtkm::cont::ArrayHandleIndex tempHypernodeIndexArray(contourTree.hypernodes.GetNumberOfValues());
  // target data array for the copy operation is superSortIndex permuted by contourTree.hypernodes
  permutedSuperSortIndex = vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>(
    contourTree.hypernodes, superSortIndex);
  vtkm::cont::Algorithm::Copy(tempHypernodeIndexArray, permutedSuperSortIndex);

  // loop through the hyperparents array, setting the first one for each
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_HypernodesSetFirstSuperchild
    hypernodesSetFirstSuperchildWorklet;
  this->Invoke(hypernodesSetFirstSuperchildWorklet,
               contourTree.hyperparents,
               superSortIndex,
               contourTree.hypernodes);

  // do a separate loop to reset the hyperparent's ID
  // This does the following
  // for (vtkm::Id supernode = 0; supernode < contourTree.supernodes.size(); supernode++)
  //    contourTree.hyperparents[supernode] = superSortIndex[maskedIndex(contourTree.hyperparents[supernode])];
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_ResetHyperparentsId
    resetHyperparentsIdWorklet;
  this->Invoke(resetHyperparentsIdWorklet, superSortIndex, contourTree.hyperparents);

  DebugPrint("Contour Tree Super Structure Constructed", __FILE__, __LINE__);
} // ComputeHyperAndSuperStructure()


// computes the regular arcs in the contour tree
void ContourTreeMaker::ComputeRegularStructure(MeshExtrema& meshExtrema)
{ // ComputeRegularStructure()
  // First step - use the superstructure to set the superparent for all supernodes
  auto supernodesIndex = vtkm::cont::ArrayHandleIndex(
    contourTree.supernodes.GetNumberOfValues()); // Counting array of length #supernodes to
  auto permutedSuperparents = vtkm::cont::make_ArrayHandlePermutation(
    contourTree.supernodes,
    contourTree.superparents); // superparents array permmuted by the supernodes array
  vtkm::cont::Algorithm::Copy(supernodesIndex, permutedSuperparents);
  // The above copy is equivlant to
  // for (indexType supernode = 0; supernode < contourTree.supernodes.size(); supernode++)
  //    contourTree.superparents[contourTree.supernodes[supernode]] = supernode;

  // Second step - for all remaining (regular) nodes, locate the superarc to which they belong
  contourtree_maker_inc_ns::ComputeRegularStructure_LocateSuperarcs locateSuperarcsWorklet(
    contourTree.hypernodes.GetNumberOfValues(), contourTree.supernodes.GetNumberOfValues());
  this->Invoke(locateSuperarcsWorklet,
               contourTree.superparents,    // (input/output)
               contourTree.whenTransferred, // (input)
               contourTree.hyperparents,    // (input)
               contourTree.hyperarcs,       // (input)
               contourTree.hypernodes,      // (input)
               contourTree.supernodes,      // (input)
               meshExtrema.peaks,           // (input)
               meshExtrema.pits);           // (input)

  // We have now set the superparent correctly for each node, and need to sort them to get the correct regular arcs
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleIndex(contourTree.arcs.GetNumberOfValues()),
                              contourTree.nodes);

  vtkm::cont::Algorithm::Sort(contourTree.nodes,
                              contourtree_maker_inc_ns::ContourTreeNodeComparator(
                                contourTree.superparents, contourTree.superarcs));

  // now set the arcs based on the array
  contourtree_maker_inc_ns::ComputeRegularStructure_SetArcs setArcsWorklet(
    contourTree.arcs.GetNumberOfValues());
  this->Invoke(setArcsWorklet,
               contourTree.nodes,        // (input) arcSorter array
               contourTree.superparents, // (input)
               contourTree.superarcs,    // (input)
               contourTree.supernodes,   // (input)
               contourTree.arcs);        // (output)

  DebugPrint("Regular Structure Computed", __FILE__, __LINE__);
} // ComputeRegularStructure()

struct ContourTreeNoSuchElementSuperParents
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& x) const
  {
    return (!noSuchElement(x));
  }
};

void InitIdArrayTypeNoSuchElement(IdArrayType& idArray, vtkm::Id size)
{
  idArray.Allocate(size);

  vtkm::cont::ArrayHandleConstant<vtkm::Id> noSuchElementArray((vtkm::Id)NO_SUCH_ELEMENT, size);
  vtkm::cont::Algorithm::Copy(noSuchElementArray, idArray);
}

template <class Mesh, class MeshBoundaryExecObj>
void ContourTreeMaker::ComputeBoundaryRegularStructure(
  MeshExtrema& meshExtrema,
  const Mesh& mesh,
  const MeshBoundaryExecObj& meshBoundaryExecObj)
{ // ComputeRegularStructure()
  // First step - use the superstructure to set the superparent for all supernodes
  auto supernodesIndex = vtkm::cont::ArrayHandleIndex(contourTree.supernodes.GetNumberOfValues());
  IdArrayType superparents;
  InitIdArrayTypeNoSuchElement(superparents, mesh.GetNumberOfVertices());
  // superparents array permmuted by the supernodes array
  auto permutedSuperparents =
    vtkm::cont::make_ArrayHandlePermutation(contourTree.supernodes, superparents);
  vtkm::cont::Algorithm::Copy(supernodesIndex, permutedSuperparents);
  // The above copy is equivlant to
  // for (indexType supernode = 0; supernode < contourTree.supernodes.size(); supernode++)
  //    superparents[contourTree.supernodes[supernode]] = supernode;

  // Second step - for all remaining (regular) nodes, locate the superarc to which they belong
  contourtree_maker_inc_ns::ComputeRegularStructure_LocateSuperarcsOnBoundary
    locateSuperarcsOnBoundaryWorklet(contourTree.hypernodes.GetNumberOfValues(),
                                     contourTree.supernodes.GetNumberOfValues());
  this->Invoke(locateSuperarcsOnBoundaryWorklet,
               superparents,                // (input/output)
               contourTree.whenTransferred, // (input)
               contourTree.hyperparents,    // (input)
               contourTree.hyperarcs,       // (input)
               contourTree.hypernodes,      // (input)
               contourTree.supernodes,      // (input)
               meshExtrema.peaks,           // (input)
               meshExtrema.pits,            // (input)
               meshBoundaryExecObj);        // (input)

  // We have now set the superparent correctly for each node, and need to sort them to get the correct regular arcs
  // DAVID "ContourTreeMaker.h" line 338
  IdArrayType node;
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleIndex(superparents.GetNumberOfValues()),
                              contourTree.augmentnodes);
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleIndex(superparents.GetNumberOfValues()), node);
  vtkm::cont::Algorithm::CopyIf(
    node, superparents, contourTree.augmentnodes, ContourTreeNoSuchElementSuperParents());

  IdArrayType toCompressed;
  InitIdArrayTypeNoSuchElement(toCompressed, superparents.GetNumberOfValues());
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleIndex(contourTree.augmentnodes.GetNumberOfValues()), node);
  auto permutedToCompressed =
    vtkm::cont::make_ArrayHandlePermutation(contourTree.augmentnodes, // index array
                                            toCompressed);            // value array
  vtkm::cont::Algorithm::Copy(node,                                   // source value array
                              permutedToCompressed);                  // target array

  // Make superparents correspond to nodes
  IdArrayType tmpsuperparents;
  vtkm::cont::Algorithm::CopyIf(
    superparents, superparents, tmpsuperparents, ContourTreeNoSuchElementSuperParents());
  vtkm::cont::Algorithm::Copy(tmpsuperparents, superparents);

  // Create array for sorting
  IdArrayType augmentnodes_sorted;
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleIndex(contourTree.augmentnodes.GetNumberOfValues()),
    augmentnodes_sorted);

  // use a comparator to do the sort
  vtkm::cont::Algorithm::Sort(
    augmentnodes_sorted,
    contourtree_maker_inc_ns::ContourTreeNodeComparator(superparents, contourTree.superarcs));
  // now set the arcs based on the array
  InitIdArrayTypeNoSuchElement(contourTree.augmentarcs,
                               contourTree.augmentnodes.GetNumberOfValues());
  contourtree_maker_inc_ns::ComputeRegularStructure_SetAugmentArcs setAugmentArcsWorklet(
    contourTree.augmentarcs.GetNumberOfValues());
  this->Invoke(setAugmentArcsWorklet,
               augmentnodes_sorted,      // (input) arcSorter array
               superparents,             // (input)
               contourTree.superarcs,    // (input)
               contourTree.supernodes,   // (input)
               toCompressed,             // (input)
               contourTree.augmentarcs); // (output)
  DebugPrint("Regular Boundary Structure Computed", __FILE__, __LINE__);
} // ComputeRegularStructure()


// routine that augments the join & split tree with each other's supernodes
// the augmented trees will be stored in the joinSuperarcs / mergeSuperarcs arrays
// the sort IDs will be stored in the ContourTree's arrays, &c.
void ContourTreeMaker::AugmentMergeTrees()
{ // ContourTreeMaker::AugmentMergeTrees()
  // in this version, we know that only connectivity-critical points are used
  // so we want to combine the lists of supernodes.
  // but they are not in sorted order, so some juggling is required.

  // NOTE: The following code block is a direct port from the original PPP2 code using std::set_union
  //       In the main code below  we replaced steps 1-4 with a combination of VTKM copy, sort, and union operators instead
  /*
    // 1. Allocate an array that is guaranteed to be big enough
    //  - the sum of the sizes of the trees or the total size of the data
    vtkm::Id nJoinSupernodes = joinTree.supernodes.GetNumberOfValues();
    vtkm::Id nSplitSupernodes = splitTree.supernodes.GetNumberOfValues();
    vtkm::Id nSupernodes = nJoinSupernodes + nSplitSupernodes;
    if (nSupernodes > joinTree.arcs.GetNumberOfValues())
      nSupernodes = joinTree.arcs.GetNumberOfValues();
    contourTree.supernodes.Allocate(nSupernodes);

    // 2. Make copies of the lists of join & split supernodes & sort them
    IdArrayType joinSort;
    joinSort.Allocate(nJoinSupernodes);
    vtkm::cont::Algorithm::Copy(joinTree.supernodes, joinSort);
    vtkm::cont::Algorithm::Sort(joinSort);
    IdArrayType splitSort;
    splitSort.Allocate(nSplitSupernodes);
    vtkm::cont::Algorithm::Copy(splitTree.supernodes, splitSort);
    vtkm::cont::Algorithm::Sort(splitSort);

    // 3. Use set_union to combine the lists
    auto contTreeSuperNodesBegin = vtkm::cont::ArrayPortalToIteratorBegin(contourTree.supernodes.GetPortalControl());
    auto tail = std::set_union(vtkm::cont::ArrayPortalToIteratorBegin(joinSort.GetPortalControl()),
                               vtkm::cont::ArrayPortalToIteratorEnd(joinSort.GetPortalControl()),
                               vtkm::cont::ArrayPortalToIteratorBegin(splitSort.GetPortalControl()),
                               vtkm::cont::ArrayPortalToIteratorEnd(splitSort.GetPortalControl()),
                               contTreeSuperNodesBegin);
    // compute the true number of supernodes
    nSupernodes = tail - contTreeSuperNodesBegin;
    // and release the memory
    joinSort.ReleaseResources();
    splitSort.ReleaseResources();

    // 4. Resize the supernode array accordingly
    contourTree.supernodes.Shrink(nSupernodes);
    */

  // 1. Allocate an array that is guaranteed to be big enough
  //  - the sum of the sizes of the trees or the total size of the data
  vtkm::Id nJoinSupernodes = joinTree.supernodes.GetNumberOfValues();
  vtkm::Id nSplitSupernodes = splitTree.supernodes.GetNumberOfValues();
  vtkm::Id nSupernodes = nJoinSupernodes + nSplitSupernodes;

  // TODO Check whether this replacement for Step 2 to 4 is a problem in terms of performance
  //  Step 2 - 4 in original PPP2. Create a sorted list of all unique supernodes from the Join and Split tree.
  contourTree.supernodes.Allocate(nSupernodes);
  vtkm::cont::Algorithm::CopySubRange(
    joinTree.supernodes, 0, nJoinSupernodes, contourTree.supernodes, 0);
  vtkm::cont::Algorithm::CopySubRange(
    splitTree.supernodes, 0, nSplitSupernodes, contourTree.supernodes, nJoinSupernodes);

  // Need to sort before Unique because VTKM only guarantees to find neighboring duplicates
  vtkm::cont::Algorithm::Sort(contourTree.supernodes);
  vtkm::cont::Algorithm::Unique(contourTree.supernodes);
  nSupernodes = contourTree.supernodes.GetNumberOfValues();

  // 5. Create lookup arrays for the join & split supernodes' new IDs
  IdArrayType newJoinID;
  newJoinID.Allocate(nJoinSupernodes);
  IdArrayType newSplitID;
  newSplitID.Allocate(nSplitSupernodes);

  // 6. Each supernode is listed by it's regular ID, so we can use the regular arrays
  //    to look up the corresponding supernode IDs in the merge trees, and to transfer
  //    the superparent for each
  IdArrayType joinSuperparents;
  joinSuperparents.Allocate(nSupernodes);
  IdArrayType splitSuperparents;
  splitSuperparents.Allocate(nSupernodes);

  contourtree_maker_inc_ns::AugmentMergeTrees_InitNewJoinSplitIDAndSuperparents
    initNewJoinSplitIDAndSuperparentsWorklet;
  this->Invoke(initNewJoinSplitIDAndSuperparentsWorklet,
               contourTree.supernodes, //input
               joinTree.superparents,  //input
               splitTree.superparents, //input
               joinTree.supernodes,    //input
               splitTree.supernodes,   //input
               joinSuperparents,       //output
               splitSuperparents,      //output
               newJoinID,              //output
               newSplitID);            //output

  // 7. use the active supernodes array for sorting
  vtkm::cont::ArrayHandleIndex initActiveSupernodes(
    nSupernodes); // create linear sequence of numbers 0, 1, .. nSupernodes
  vtkm::cont::Algorithm::Copy(initActiveSupernodes, activeSupernodes);

  // 8. Once we have got the superparent for each, we can sort by superparents and set
  //      the augmented superarcs. We start with the join superarcs
  vtkm::cont::Algorithm::Sort(
    activeSupernodes,
    active_graph_inc_ns::SuperArcNodeComparator(joinSuperparents, joinTree.isJoinTree));

  // 9.   Set the augmented join superarcs
  augmentedJoinSuperarcs.Allocate(nSupernodes);
  contourtree_maker_inc_ns::AugmentMergeTrees_SetAugmentedMergeArcs setAugmentedJoinArcsWorklet;
  this->Invoke(setAugmentedJoinArcsWorklet,
               activeSupernodes,        // (input domain)
               joinSuperparents,        // (input)
               joinTree.superarcs,      // (input)
               newJoinID,               // (input)
               augmentedJoinSuperarcs); // (output)

  // 10. Now we repeat the process for the split superarcs
  vtkm::cont::Algorithm::Copy(initActiveSupernodes, activeSupernodes);
  // now sort by the split superparent
  vtkm::cont::Algorithm::Sort(
    activeSupernodes,
    active_graph_inc_ns::SuperArcNodeComparator(splitSuperparents, splitTree.isJoinTree));

  // 11.  Set the augmented split superarcs
  augmentedSplitSuperarcs.Allocate(nSupernodes);
  contourtree_maker_inc_ns::AugmentMergeTrees_SetAugmentedMergeArcs setAugmentedSplitArcsWorklet;
  this->Invoke(setAugmentedSplitArcsWorklet,
               activeSupernodes,         // (input domain)
               splitSuperparents,        // (input)
               splitTree.superarcs,      // (input)
               newSplitID,               // (input)
               augmentedSplitSuperarcs); // (output)

  // 12. Lastly, we can initialise all of the remaining arrays
  vtkm::cont::ArrayHandleConstant<vtkm::Id> noSuchElementArray((vtkm::Id)NO_SUCH_ELEMENT,
                                                               nSupernodes);
  vtkm::cont::Algorithm::Copy(noSuchElementArray, contourTree.superarcs);
  vtkm::cont::Algorithm::Copy(noSuchElementArray, contourTree.hyperparents);
  vtkm::cont::Algorithm::Copy(noSuchElementArray, contourTree.hypernodes);
  vtkm::cont::Algorithm::Copy(noSuchElementArray, contourTree.hyperarcs);
  vtkm::cont::Algorithm::Copy(noSuchElementArray, contourTree.whenTransferred);

  // TODO We should only need to allocate the updegree/downdegree arrays. We initialize them with 0 here to ensure consistency of debug output
  //updegree.Allocate(nSupernodes);
  //downdegree.Allocate(nSupernodes);
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, nSupernodes), updegree);
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, nSupernodes),
                              downdegree);

  DebugPrint("Supernodes Found", __FILE__, __LINE__);
} // ContourTreeMaker::AugmentMergeTrees()



namespace detail
{

struct LeafChainsToContourTree
{
  LeafChainsToContourTree(const vtkm::Id nIterations,
                          const bool isJoin,
                          const IdArrayType& outdegree,
                          const IdArrayType& indegree,
                          const IdArrayType& outbound,
                          const IdArrayType& inbound,
                          const IdArrayType& inwards)
    : NIterations(nIterations)
    , IsJoin(isJoin)
    , Outdegree(outdegree)
    , Indegree(indegree)
    , Outbound(outbound)
    , Inbound(inbound)
    , Inwards(inwards)
  {
  }

  template <typename DeviceAdapter, typename... Args>
  bool operator()(DeviceAdapter device, Args&&... args) const
  {
    contourtree_maker_inc_ns::TransferLeafChains_TransferToContourTree<DeviceAdapter> worklet(
      this->NIterations, // (input)
      this->IsJoin,      // (input)
      this->Outdegree,   // (input)
      this->Indegree,    // (input)
      this->Outbound,    // (input)
      this->Inbound,     // (input)
      this->Inwards);    // (input)
    vtkm::worklet::DispatcherMapField<decltype(worklet)> dispatcher(worklet);
    dispatcher.SetDevice(device);
    dispatcher.Invoke(std::forward<Args>(args)...);
    return true;
  }


  const vtkm::Id NIterations;
  const bool IsJoin;
  const IdArrayType& Outdegree;
  const IdArrayType& Indegree;
  const IdArrayType& Outbound;
  const IdArrayType& Inbound;
  const IdArrayType& Inwards;
};
}

// routine to transfer leaf chains to contour tree
void ContourTreeMaker::TransferLeafChains(bool isJoin)
{ // ContourTreeMaker::TransferLeafChains()
  // we need to compute the chains in both directions, so we have two vectors:
  // TODO below we initialize the outbound and inbound arrays with 0 to ensure consistency of debug output. Check if this is needed.
  IdArrayType outbound;
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, contourTree.supernodes.GetNumberOfValues()),
    outbound);
  //outbound.Allocate(contourTree.supernodes.GetNumberOfValues());
  IdArrayType inbound;
  //inbound.Allocate(contourTree.supernodes.GetNumberOfValues());
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, contourTree.supernodes.GetNumberOfValues()),
    inbound);


  // a reference for the inwards array we use to initialise
  IdArrayType& inwards = isJoin ? augmentedJoinSuperarcs : augmentedSplitSuperarcs;
  // and references for the degrees
  IdArrayType& indegree = isJoin ? downdegree : updegree;
  IdArrayType& outdegree = isJoin ? updegree : downdegree;

  // loop to each active node to copy join/split to outbound and inbound arrays
  contourtree_maker_inc_ns::TransferLeafChains_InitInAndOutbound initInAndOutboundWorklet;
  this->Invoke(initInAndOutboundWorklet,
               activeSupernodes, // (input)
               inwards,          // (input)
               outdegree,        // (input)
               indegree,         // (input)
               outbound,         // (output)
               inbound);         // (output)

  DebugPrint("Init in and outbound -- Step 1", __FILE__, __LINE__);

  // Compute the number of log steps required in this pass
  vtkm::Id nLogSteps = 1;
  for (vtkm::Id shifter = activeSupernodes.GetNumberOfValues(); shifter != 0; shifter >>= 1)
  {
    nLogSteps++;
  }


  // loop to find the now-regular vertices and collapse past them without altering
  // the existing join & split arcs
  for (vtkm::Id iteration = 0; iteration < nLogSteps; iteration++)
  { // per iteration
    // loop through the vertices, updating outbound
    contourtree_maker_inc_ns::TransferLeafChains_CollapsePastRegular collapsePastRegularWorklet;
    this->Invoke(collapsePastRegularWorklet,
                 activeSupernodes, // (input)
                 outbound,         // (input/output)
                 inbound);         // (input/output)

  } // per iteration

  DebugPrint("Init in and outbound -- Step 2", __FILE__, __LINE__);

  // at this point, the outbound vector chains everything outwards to the leaf
  // any vertices on the last outbound leaf superarc point to the leaf
  // and the leaf itself will point to its saddle, identifying the hyperarc

  // what we want to do is:
  // a. for leaves (tested by degree),
  //              i.      we use inbound as the hyperarc
  //              ii.     we use inwards as the superarc
  //              iii.we use self as the hyperparent
  // b. for regular vertices pointing to a leaf (test by outbound's degree),
  //              i.      we use outbound as the hyperparent
  //              ii. we use inwards as the superarc
  // c. for all other vertics
  //              ignore


  // loop through the active vertices
  detail::LeafChainsToContourTree task(nIterations, // (input)
                                       isJoin,      // (input)
                                       outdegree,   // (input)
                                       indegree,    // (input)
                                       outbound,    // (input)
                                       inbound,     // (input)
                                       inwards);    // (input)
  vtkm::cont::TryExecute(task,
                         activeSupernodes,             // (input)
                         contourTree.hyperparents,     // (output)
                         contourTree.hyperarcs,        // (output)
                         contourTree.superarcs,        // (output)
                         contourTree.whenTransferred); // (output)

  DebugPrint(isJoin ? "Upper Regular Chains Transferred" : "Lower Regular Chains Transferred",
             __FILE__,
             __LINE__);
} // ContourTreeMaker::TransferLeafChains()


// routine to compress trees by removing regular vertices as well as hypernodes
void ContourTreeMaker::CompressTrees()
{ // ContourTreeMaker::CompressTrees()

  // Compute the number of log steps required in this pass
  vtkm::Id nLogSteps = 1;
  for (vtkm::Id shifter = activeSupernodes.GetNumberOfValues(); shifter != 0; shifter >>= 1)
  {
    nLogSteps++;
  }

  // loop to update the merge trees
  for (vtkm::Id logStep = 0; logStep < nLogSteps; logStep++)
  { // iteration log times
    contourtree_maker_inc_ns::CompressTrees_Step compressTreesStepWorklet;
    this->Invoke(compressTreesStepWorklet,
                 activeSupernodes,       // (input)
                 contourTree.superarcs,  // (input)
                 augmentedJoinSuperarcs, // (input/output)
                 augmentedSplitSuperarcs // (input/output)
                 );

  } // iteration log times

  DebugPrint("Trees Compressed", __FILE__, __LINE__);
} // ContourTreeMaker::CompressTrees()


// compresses trees to remove transferred vertices
void ContourTreeMaker::CompressActiveSupernodes()
{ // ContourTreeMaker::CompressActiveSupernodes()
  // copy only if contourTree.whenTransferred has been set
  IdArrayType compressedActiveSupernodes;

  // Transform the whenTransferred array to return 1 if the index was not transferred and 0 otherwise
  auto wasNotTransferred =
    vtkm::cont::ArrayHandleTransform<IdArrayType, contourtree_maker_inc_ns::WasNotTransferred>(
      contourTree.whenTransferred, contourtree_maker_inc_ns::WasNotTransferred());
  // Permute the wasNotTransferred array handle so that the lookup is based on the value of the indices in the active supernodes array
  auto notTransferredActiveSupernodes =
    vtkm::cont::make_ArrayHandlePermutation(activeSupernodes, wasNotTransferred);
  // Keep only the indices of the active supernodes that have not been transferred yet
  vtkm::cont::Algorithm::CopyIf(
    activeSupernodes, notTransferredActiveSupernodes, compressedActiveSupernodes);
  // Copy the data into the active supernodes
  activeSupernodes.ReleaseResources();
  activeSupernodes =
    compressedActiveSupernodes; // vtkm ArrayHandles are smart, so we can just swap it in without having to copy

  DebugPrint("Active Supernodes Compressed", __FILE__, __LINE__);
} // ContourTreeMaker::CompressActiveSupernodes()


void ContourTreeMaker::FindDegrees()
{ // ContourTreeMaker::FindDegrees()
  using PermuteIndexArray = vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  // retrieve the size to register for speed
  vtkm::Id nActiveSupernodes = activeSupernodes.GetNumberOfValues();

  // reset the updegree & downdegree
  contourtree_maker_inc_ns::FindDegrees_ResetUpAndDowndegree resetUpAndDowndegreeWorklet;
  this->Invoke(resetUpAndDowndegreeWorklet, activeSupernodes, updegree, downdegree);

  DebugPrint("Degrees Set to 0", __FILE__, __LINE__);

  // now we loop through every join & split arc, updating degrees
  // to minimise memory footprint, we will do two separate loops
  // but we could combine the two into paired loops
  // first we establish an array of destination vertices (since outdegree is always 1)
  IdArrayType inNeighbour;
  //inNeighbour.Allocate(nActiveSupernodes);
  //PermuteIndexArray permuteInNeighbour(activeSupernodes, inNeighbour);
  PermuteIndexArray permuteAugmentedJoinSuperarcs(activeSupernodes, augmentedJoinSuperarcs);
  vtkm::cont::Algorithm::Copy(permuteAugmentedJoinSuperarcs, inNeighbour);
  // now sort to group copies together
  vtkm::cont::Algorithm::Sort(inNeighbour);

  // there's probably a smarter scatter-gather solution to this, but this should work
  // find the RHE of each segment
  contourtree_maker_inc_ns::FindDegrees_FindRHE joinFindRHEWorklet(nActiveSupernodes);
  this->Invoke(joinFindRHEWorklet, inNeighbour, updegree);

  // now subtract the LHE to get the size
  contourtree_maker_inc_ns::FindDegrees_SubtractLHE joinSubractLHEWorklet;
  this->Invoke(joinSubractLHEWorklet, inNeighbour, updegree);

  // now repeat the same process for the split neighbours
  PermuteIndexArray permuteAugmentedSplitSuperarcs(activeSupernodes, augmentedSplitSuperarcs);
  vtkm::cont::Algorithm::Copy(permuteAugmentedSplitSuperarcs, inNeighbour);
  // now sort to group copies together
  vtkm::cont::Algorithm::Sort(inNeighbour);

  // there's probably a smarter scatter-gather solution to this, but this should work
  // find the RHE of each segment
  contourtree_maker_inc_ns::FindDegrees_FindRHE splitFindRHEWorklet(nActiveSupernodes);
  this->Invoke(splitFindRHEWorklet, inNeighbour, downdegree);

  // now subtract the LHE to get the size
  contourtree_maker_inc_ns::FindDegrees_SubtractLHE splitSubractLHEWorklet;
  this->Invoke(splitSubractLHEWorklet, inNeighbour, downdegree);

  DebugPrint("Degrees Computed", __FILE__, __LINE__);
} // ContourTreeMaker::FindDegrees()



void ContourTreeMaker::DebugPrint(const char* message, const char* fileName, long lineNum)
{ // ContourTreeMaker::DebugPrint()
#ifdef DEBUG_PRINT
  std::string childString = std::string(message);
  std::cout
    << "==========================================================================================="
       "==============================================="
    << "=============================================="
    << //============================================================================================" <<
    "=============================================================================================="
    "============================================"
    << std::endl;
  std::cout
    << "==========================================================================================="
       "==============================================="
    << "=============================================="
    << //============================================================================================" <<
    "=============================================================================================="
    "============================================"
    << std::endl;
  std::cout
    << "==========================================================================================="
       "==============================================="
    << "=============================================="
    << //============================================================================================" <<
    "=============================================================================================="
    "============================================"
    << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;

  // joinTree.DebugPrint((childString + std::string(": Join Tree")).c_str(), fileName, lineNum);
  // splitTree.DebugPrint((childString + std::string(": Split Tree")).c_str(), fileName, lineNum);
  contourTree.DebugPrint((childString + std::string(": Contour Tree")).c_str(), fileName, lineNum);
  std::cout
    << "==========================================================================================="
       "==============================================="
    << "=============================================="
    << //============================================================================================" <<
    "=============================================================================================="
    "============================================"
    << std::endl;

  // std::cout << "------------------------------------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Contour Tree Maker Contains:                          " << std::endl;
  std::cout << "------------------------------------------------------" << std::endl;
  std::cout << "nIterations: " << nIterations << std::endl;

  printHeader(updegree.GetNumberOfValues());
  printIndices("Updegree", updegree);
  printIndices("Downdegree", downdegree);
  printIndices("Aug Join SArcs", augmentedJoinSuperarcs);
  printIndices("Aug Split SArcs", augmentedSplitSuperarcs);

  printHeader(activeSupernodes.GetNumberOfValues());
  printIndices("Active SNodes", activeSupernodes);
#else
  // Avoid unused parameter warnings
  (void)message;
  (void)fileName;
  (void)lineNum;
#endif
} // ContourTreeMaker::DebugPrint()


} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
