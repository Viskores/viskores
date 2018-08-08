//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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

#ifndef vtkm_worklet_contourtree_ppp2_contourtreemaker_h
#define vtkm_worklet_contourtree_ppp2_contourtreemaker_h

#include "Types.h"
#include <iomanip>

// local includes
#include <vtkm/worklet/contourtree_ppp2/ArrayTransforms.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTree.h>
#include <vtkm/worklet/contourtree_ppp2/MergeTree.h>
#include <vtkm/worklet/contourtree_ppp2/MeshExtrema.h>
#include <vtkm/worklet/contourtree_ppp2/PrintVectors.h>
#include <vtkm/worklet/contourtree_ppp2/Types.h>

// contourtree_maker_inc includes
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/AugmentMergeTrees_InitNewJoinSplitIDAndSuperparents.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/AugmentMergeTrees_SetAugmentedMergeArcs.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/CompressTrees_Step.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/ComputeHyperAndSuperStructure_HypernodesSetFirstSuperchild.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/ComputeHyperAndSuperStructure_PermuteArcs.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/ComputeHyperAndSuperStructure_ResetHyperparentsId.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/ComputeHyperAndSuperStructure_SetNewHypernodesAndArcs.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/ComputeRegularStructure_LocateSuperarcs.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/ComputeRegularStructure_SetArcs.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/ContourTreeNodeComparator.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/ContourTreeSuperNodeComparator.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/FindDegrees_FindRHE.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/FindDegrees_ResetUpAndDowndegree.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/FindDegrees_SubtractLHE.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/TransferLeafChains_CollapsePastRegular.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/TransferLeafChains_InitInAndOutbound.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/TransferLeafChains_TransferToContourTree.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTreeMaker_Inc/WasNotTransferred.h>

#include <vtkm/worklet/contourtree_ppp2/ActiveGraph_Inc/SuperArcNodeComparator.h>


//VTKM includes
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>



namespace contourtree_maker_inc_ns = vtkm::worklet::contourtree_ppp2::contourtree_maker_inc;
namespace active_graph_inc_ns = vtkm::worklet::contourtree_ppp2::active_graph_inc;

namespace vtkm
{
namespace worklet
{
namespace contourtree_ppp2
{


template <typename DeviceAdapter>
class ContourTreeMaker
{ // class MergeTree
public:
  // Typedef for using vtkm device adapter algorithms
  typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;

  // the contour tree, join tree & split tree to use
  ContourTree& contourTree;
  MergeTree<DeviceAdapter> &joinTree, &splitTree;

  // vectors of up and down degree kept during the computation
  IdArrayType updegree, downdegree;

  // vectors for tracking merge superarcs
  IdArrayType augmentedJoinSuperarcs, augmentedSplitSuperarcs;

  // vector for the active set of supernodes
  IdArrayType activeSupernodes;

  // counter for the number of iterations it took
  vtkm::Id nIterations;

  // constructor does the real work does the real work but mostly, it just calls the following two routines
  ContourTreeMaker(ContourTree& theContourTree,
                   MergeTree<DeviceAdapter>& theJoinTree,
                   MergeTree<DeviceAdapter>& theSplitTree);

  // computes the hyperarcs in the contour tree
  void ComputeHyperAndSuperStructure();

  // computes the regular arcs in the contour tree
  void ComputeRegularStructure(MeshExtrema<DeviceAdapter>& meshExtrema);

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
template <typename DeviceAdapter>
ContourTreeMaker<DeviceAdapter>::ContourTreeMaker(ContourTree& theContourTree,
                                                  MergeTree<DeviceAdapter>& theJoinTree,
                                                  MergeTree<DeviceAdapter>& theSplitTree)
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


template <typename DeviceAdapter>
void ContourTreeMaker<DeviceAdapter>::ComputeHyperAndSuperStructure()
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
    contourTree.superarcs.GetPortalControl().Set(superID, NO_SUCH_ELEMENT);
    contourTree.hyperarcs.GetPortalControl().Set(superID, NO_SUCH_ELEMENT);
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
  DeviceAlgorithm::Copy(initContourTreeHypernodes, contourTree.hypernodes);


  // now we sort the array with a comparator
  DeviceAlgorithm::Sort(
    contourTree.hypernodes,
    contourtree_maker_inc_ns::ContourTreeSuperNodeComparator<DeviceAdapter>(
      contourTree.hyperparents, contourTree.supernodes, contourTree.whenTransferred));


  // we have to permute a bunch of arrays, so let's have some temporaries to store them
  IdArrayType permutedHyperparents;
  permuteArray<vtkm::Id, IdArrayType, DeviceAdapter>(
    contourTree.hyperparents, contourTree.hypernodes, permutedHyperparents);
  IdArrayType permutedSupernodes;
  permuteArray<vtkm::Id, IdArrayType, DeviceAdapter>(
    contourTree.supernodes, contourTree.hypernodes, permutedSupernodes);
  IdArrayType permutedSuperarcs;
  permuteArray<vtkm::Id, IdArrayType, DeviceAdapter>(
    contourTree.superarcs, contourTree.hypernodes, permutedSuperarcs);

  // now we establish the reverse index array
  IdArrayType superSortIndex;
  superSortIndex.Allocate(contourTree.supernodes.GetNumberOfValues());
  // The following copy is equivilant to
  // for (vtkm::Id supernode = 0; supernode < contourTree.supernodes.size(); supernode++)
  //   superSortIndex[contourTree.hypernodes[supernode]] = supernode;

  //typedef vtkm::cont::ArrayHandlePermutation<IdArrayType, vtkm::cont::ArrayHandleIndex> PermuteArrayHandleIndex;
  vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> permutedSuperSortIndex(
    contourTree.hypernodes, // index array
    superSortIndex);        // value array
  DeviceAlgorithm::Copy(
    vtkm::cont::ArrayHandleIndex(contourTree.supernodes.GetNumberOfValues()), // source value array
    permutedSuperSortIndex);                                                  // target array

  // we then copy the supernodes & hyperparents back to the main array
  DeviceAlgorithm::Copy(permutedSupernodes, contourTree.supernodes);
  DeviceAlgorithm::Copy(permutedHyperparents, contourTree.hyperparents);


  // we need an extra permutation to get the superarcs correct
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_PermuteArcs permuteSuperarcsWorklet;
  vtkm::worklet::DispatcherMapField<
    contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_PermuteArcs>
    permuteSuperarcsDispatcher(permuteSuperarcsWorklet);
  permuteSuperarcsDispatcher.Invoke(permutedSuperarcs,      // (input)
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
  permuteArray<vtkm::Id, IdArrayType, DeviceAdapter>(
    contourTree.hyperarcs, contourTree.hypernodes, permutedHyperarcs);
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_PermuteArcs permuteHyperarcsWorklet;
  vtkm::worklet::DispatcherMapField<
    contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_PermuteArcs>
    permuteHyperarcsDispatcher(permuteHyperarcsWorklet);
  permuteSuperarcsDispatcher.Invoke(permutedHyperarcs, superSortIndex, contourTree.hyperarcs);

  // now swizzle the whenTransferred value
  IdArrayType permutedWhenTransferred;
  permuteArray<vtkm::Id, IdArrayType, DeviceAdapter>(
    contourTree.whenTransferred, contourTree.hypernodes, permutedWhenTransferred);
  DeviceAlgorithm::Copy(permutedWhenTransferred, contourTree.whenTransferred);

  // now we compress both the hypernodes & hyperarcs
  // The following commented code block is variant ported directly from PPP2 using std::partial_sum. This has been replaced here with vtkm's ScanExclusive.
  /*IdArrayType newHypernodePosition;
    newHypernodePosition.Allocate(contourTree.whenTransferred.GetNumberOfValues());
    newHypernodePosition.GetPortalControl().Set(0,  0);
    auto oneIfHypernode = [](vtkm::Id v) { return isHypernode(v) ? 1 : 0; };
    std::partial_sum(
                boost::make_transform_iterator(vtkm::cont::ArrayPortalToIteratorBegin(contourTree.whenTransferred.GetPortalControl()), oneIfHypernode),
                boost::make_transform_iterator(vtkm::cont::ArrayPortalToIteratorEnd(contourTree.whenTransferred.GetPortalControl()) - 1, oneIfHypernode),
                vtkm::cont::ArrayPortalToIteratorBegin(newHypernodePosition.GetPortalControl()) + 1);*/
  IdArrayType newHypernodePosition;
  onefIfHypernode oneIfHypernodeFunctor;
  auto oneIfHypernodeArrayHandle = vtkm::cont::ArrayHandleTransform<IdArrayType, onefIfHypernode>(
    contourTree.whenTransferred, oneIfHypernodeFunctor);
  DeviceAlgorithm::ScanExclusive(oneIfHypernodeArrayHandle, newHypernodePosition);

  vtkm::Id nHypernodes =
    newHypernodePosition.GetPortalConstControl().Get(newHypernodePosition.GetNumberOfValues() - 1) +
    oneIfHypernodeFunctor(contourTree.whenTransferred.GetPortalConstControl().Get(
      contourTree.whenTransferred.GetNumberOfValues() - 1));

  IdArrayType newHypernodes;
  newHypernodes.Allocate(nHypernodes);
  IdArrayType newHyperarcs;
  newHyperarcs.Allocate(nHypernodes);

  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_SetNewHypernodesAndArcs
    setNewHypernodesAndArcsWorklet;
  vtkm::worklet::DispatcherMapField<
    contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_SetNewHypernodesAndArcs>
    setNewHypernodesAndArcsDispatcher(setNewHypernodesAndArcsWorklet);
  setNewHypernodesAndArcsDispatcher.Invoke(contourTree.supernodes,
                                           contourTree.whenTransferred,
                                           contourTree.hypernodes,
                                           contourTree.hyperarcs,
                                           newHypernodePosition,
                                           newHypernodes,
                                           newHyperarcs);
  // swap in the new computed arrays.
  // vtkm ArrayHandles are smart so we can just swap the new data in here rather than copy
  //DeviceAlgorithm::Copy(newHypernodes, contourTree.hypernodes);
  //DeviceAlgorithm::Copy(newHyperarcs, contourTree.hyperarcs);
  contourTree.hypernodes.ReleaseResources();
  contourTree.hypernodes = newHypernodes;
  contourTree.hyperarcs.ReleaseResources();
  contourTree.hyperarcs = newHyperarcs;


  // now reuse the superSortIndex array for hypernode IDs
  // The following copy is equivilant to
  // for (vtkm::Id hypernode = 0; hypernode < contourTree.hypernodes.size(); hypernode++)
  //            superSortIndex[contourTree.hypernodes[hypernode]] = hypernode;
  // source data array is a simple linear index from 0 to #hypernodes
  vtkm::cont::ArrayHandleIndex tempHypernodeIndexArray(contourTree.hypernodes.GetNumberOfValues());
  // target data array for the copy operation is superSortIndex permuted by contourTree.hypernodes
  permutedSuperSortIndex = vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>(
    contourTree.hypernodes, superSortIndex);
  DeviceAlgorithm::Copy(tempHypernodeIndexArray, permutedSuperSortIndex);

  // loop through the hyperparents array, setting the first one for each
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_HypernodesSetFirstSuperchild
    hypernodesSetFirstSuperchildWorklet;
  vtkm::worklet::DispatcherMapField<
    contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_HypernodesSetFirstSuperchild>
    hypernodesSetFirstSuperchildDispatcher(hypernodesSetFirstSuperchildWorklet);
  hypernodesSetFirstSuperchildDispatcher.Invoke(
    contourTree.hyperparents, superSortIndex, contourTree.hypernodes);

  // do a separate loop to reset the hyperparent's ID
  // This does the following
  // for (vtkm::Id supernode = 0; supernode < contourTree.supernodes.size(); supernode++)
  //    contourTree.hyperparents[supernode] = superSortIndex[maskedIndex(contourTree.hyperparents[supernode])];
  contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_ResetHyperparentsId
    resetHyperparentsIdWorklet;
  vtkm::worklet::DispatcherMapField<
    contourtree_maker_inc_ns::ComputeHyperAndSuperStructure_ResetHyperparentsId>
    resetHyperparentsIdDispatcher(resetHyperparentsIdWorklet);
  resetHyperparentsIdDispatcher.Invoke(superSortIndex, contourTree.hyperparents);

  DebugPrint("Contour Tree Super Structure Constructed", __FILE__, __LINE__);
} // ComputeHyperAndSuperStructure()


// computes the regular arcs in the contour tree
template <typename DeviceAdapter>
void ContourTreeMaker<DeviceAdapter>::ComputeRegularStructure(
  MeshExtrema<DeviceAdapter>& meshExtrema)
{ // ComputeRegularStructure()
  // First step - use the superstructure to set the superparent for all supernodes
  auto supernodesIndex = vtkm::cont::ArrayHandleIndex(
    contourTree.supernodes.GetNumberOfValues()); // Counting array of lenght #supernodes to
  auto permutedSuperparents = vtkm::cont::make_ArrayHandlePermutation(
    contourTree.supernodes,
    contourTree.superparents); // superparents array permmuted by the supernodes array
  DeviceAlgorithm::Copy(supernodesIndex, permutedSuperparents);
  // The above copy is equivlant to
  // for (indexType supernode = 0; supernode < contourTree.supernodes.size(); supernode++)
  //    contourTree.superparents[contourTree.supernodes[supernode]] = supernode;

  // Second step - for all remaining (regular) nodes, locate the superarc to which they belong
  contourtree_maker_inc_ns::ComputeRegularStructure_LocateSuperarcs locateSuperarcsWorklet(
    contourTree.hypernodes.GetNumberOfValues(), contourTree.supernodes.GetNumberOfValues());
  vtkm::worklet::DispatcherMapField<
    contourtree_maker_inc_ns::ComputeRegularStructure_LocateSuperarcs>
    locateSuperarcsDispatcher(locateSuperarcsWorklet);
  locateSuperarcsDispatcher.Invoke(contourTree.superparents,    // (input/output)
                                   contourTree.whenTransferred, // (input)
                                   contourTree.hyperparents,    // (input)
                                   contourTree.hyperarcs,       // (input)
                                   contourTree.hypernodes,      // (input)
                                   contourTree.supernodes,      // (input)
                                   meshExtrema.peaks,           // (input)
                                   meshExtrema.pits);           // (input)
  // We have now set the superparent correctly for each node, and need to sort them to get the correct regular arcs
  DeviceAlgorithm::Copy(vtkm::cont::ArrayHandleIndex(contourTree.arcs.GetNumberOfValues()),
                        contourTree.nodes);
  DeviceAlgorithm::Sort(contourTree.nodes,
                        contourtree_maker_inc_ns::ContourTreeNodeComparator<DeviceAdapter>(
                          contourTree.superparents, contourTree.superarcs));

  // now set the arcs based on the array
  contourtree_maker_inc_ns::ComputeRegularStructure_SetArcs setArcsWorklet(
    contourTree.arcs.GetNumberOfValues());
  vtkm::worklet::DispatcherMapField<contourtree_maker_inc_ns::ComputeRegularStructure_SetArcs>
    setArcsDispatcher(setArcsWorklet);
  setArcsDispatcher.Invoke(contourTree.nodes,        // (input) arcSorter array
                           contourTree.superparents, // (input)
                           contourTree.superarcs,    // (input)
                           contourTree.supernodes,   // (input)
                           contourTree.arcs);        // (output)

  DebugPrint("Regular Structure Computed", __FILE__, __LINE__);
} // ComputeRegularStructure()


// routine that augments the join & split tree with each other's supernodes
// the augmented trees will be stored in the joinSuperarcs / mergeSuperarcs arrays
// the sort IDs will be stored in the ContourTree's arrays, &c.
template <typename DeviceAdapter>
void ContourTreeMaker<DeviceAdapter>::AugmentMergeTrees()
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
    DeviceAlgorithm::Copy(joinTree.supernodes, joinSort);
    DeviceAlgorithm::Sort(joinSort);
    IdArrayType splitSort;
    splitSort.Allocate(nSplitSupernodes);
    DeviceAlgorithm::Copy(splitTree.supernodes, splitSort);
    DeviceAlgorithm::Sort(splitSort);

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
  DeviceAlgorithm::CopySubRange(joinTree.supernodes, 0, nJoinSupernodes, contourTree.supernodes, 0);
  DeviceAlgorithm::CopySubRange(
    splitTree.supernodes, 0, nSplitSupernodes, contourTree.supernodes, nJoinSupernodes);
  DeviceAlgorithm::Sort(
    contourTree
      .supernodes); // Need to sort before Unique because VTKM only guarntees to find neighbouring duplicates
  DeviceAlgorithm::Unique(contourTree.supernodes);
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
  vtkm::worklet::DispatcherMapField<
    contourtree_maker_inc_ns::AugmentMergeTrees_InitNewJoinSplitIDAndSuperparents>
    initNewJoinSplitIDAndSuperparentsDispatcher(initNewJoinSplitIDAndSuperparentsWorklet);
  initNewJoinSplitIDAndSuperparentsDispatcher.Invoke(contourTree.supernodes, //input
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
  DeviceAlgorithm::Copy(initActiveSupernodes, activeSupernodes);

  // 8. Once we have got the superparent for each, we can sort by superparents and set
  //      the augmented superarcs. We start with the join superarcs
  DeviceAlgorithm::Sort(activeSupernodes,
                        active_graph_inc_ns::SuperArcNodeComparator<DeviceAdapter>(
                          joinSuperparents, joinTree.isJoinTree));


  // 9.   Set the augmented join superarcs
  augmentedJoinSuperarcs.Allocate(nSupernodes);
  contourtree_maker_inc_ns::AugmentMergeTrees_SetAugmentedMergeArcs setAugmentedJoinArcsWorklet;
  vtkm::worklet::DispatcherMapField<
    contourtree_maker_inc_ns::AugmentMergeTrees_SetAugmentedMergeArcs>
    setAugmentedJoinArcsDispatcher(setAugmentedJoinArcsWorklet);
  setAugmentedJoinArcsDispatcher.Invoke(activeSupernodes,        // (input domain)
                                        joinSuperparents,        // (input)
                                        joinTree.superarcs,      // (input)
                                        newJoinID,               // (input)
                                        augmentedJoinSuperarcs); // (output)

  // 10. Now we repeat the process for the split superarcs
  DeviceAlgorithm::Copy(initActiveSupernodes, activeSupernodes);
  // now sort by the split superparent
  DeviceAlgorithm::Sort(activeSupernodes,
                        active_graph_inc_ns::SuperArcNodeComparator<DeviceAdapter>(
                          splitSuperparents, splitTree.isJoinTree));

  // 11.  Set the augmented split superarcs
  augmentedSplitSuperarcs.Allocate(nSupernodes);
  contourtree_maker_inc_ns::AugmentMergeTrees_SetAugmentedMergeArcs setAugmentedSplitArcsWorklet;
  vtkm::worklet::DispatcherMapField<
    contourtree_maker_inc_ns::AugmentMergeTrees_SetAugmentedMergeArcs>
    setAugmentedSplitArcsDispatcher(setAugmentedSplitArcsWorklet);
  setAugmentedSplitArcsDispatcher.Invoke(activeSupernodes,         // (input domain)
                                         splitSuperparents,        // (input)
                                         splitTree.superarcs,      // (input)
                                         newSplitID,               // (input)
                                         augmentedSplitSuperarcs); // (output)

  // 12. Lastly, we can initialise all of the remaining arrays
  vtkm::cont::ArrayHandleConstant<vtkm::Id> noSuchElementArray(NO_SUCH_ELEMENT, nSupernodes);
  DeviceAlgorithm::Copy(noSuchElementArray, contourTree.superarcs);
  DeviceAlgorithm::Copy(noSuchElementArray, contourTree.hyperparents);
  DeviceAlgorithm::Copy(noSuchElementArray, contourTree.hypernodes);
  DeviceAlgorithm::Copy(noSuchElementArray, contourTree.hyperarcs);
  DeviceAlgorithm::Copy(noSuchElementArray, contourTree.whenTransferred);

  // TODO We should only need to allocate the updegree/downdegree arrays. We initalize them with 0 here to ensure consistency of debug output
  //updegree.Allocate(nSupernodes);
  //downdegree.Allocate(nSupernodes);
  DeviceAlgorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, nSupernodes), updegree);
  DeviceAlgorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, nSupernodes), downdegree);

  DebugPrint("Supernodes Found", __FILE__, __LINE__);
} // ContourTreeMaker::AugmentMergeTrees()


// routine to transfer leaf chains to contour tree
template <typename DeviceAdapter>
void ContourTreeMaker<DeviceAdapter>::TransferLeafChains(bool isJoin)
{ // ContourTreeMaker::TransferLeafChains()
  // we need to compute the chains in both directions, so we have two vectors:
  // TODO below we initalize the outbound and inbound arrays with 0 to ensure consistency of debug output. Check if this is needed.
  IdArrayType outbound;
  DeviceAlgorithm::Copy(
    vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, contourTree.supernodes.GetNumberOfValues()),
    outbound);
  //outbound.Allocate(contourTree.supernodes.GetNumberOfValues());
  IdArrayType inbound;
  //inbound.Allocate(contourTree.supernodes.GetNumberOfValues());
  DeviceAlgorithm::Copy(
    vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, contourTree.supernodes.GetNumberOfValues()),
    inbound);


  // a reference for the inwards array we use to initialise
  IdArrayType& inwards = isJoin ? augmentedJoinSuperarcs : augmentedSplitSuperarcs;
  // and references for the degrees
  IdArrayType& indegree = isJoin ? downdegree : updegree;
  IdArrayType& outdegree = isJoin ? updegree : downdegree;

  // loop to each active node to copy join/split to outbound and inbound arrays
  contourtree_maker_inc_ns::TransferLeafChains_InitInAndOutbound initInAndOutboundWorklet;
  vtkm::worklet::DispatcherMapField<contourtree_maker_inc_ns::TransferLeafChains_InitInAndOutbound>
    initInAndOutboundDispatcher(initInAndOutboundWorklet);
  initInAndOutboundDispatcher.Invoke(activeSupernodes, // (input)
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
    vtkm::worklet::DispatcherMapField<
      contourtree_maker_inc_ns::TransferLeafChains_CollapsePastRegular>
      collapsePastRegularDispatcher(collapsePastRegularWorklet);
    collapsePastRegularDispatcher.Invoke(activeSupernodes, // (input)
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
  contourtree_maker_inc_ns::TransferLeafChains_TransferToContourTree<DeviceAdapter>
    transferToContourTreeWorklet(nIterations, // (input)
                                 isJoin,      // (input)
                                 outdegree,   // (input)
                                 indegree,    // (input)
                                 outbound,    // (input)
                                 inbound,     // (input)
                                 inwards);    // (input)

  vtkm::worklet::DispatcherMapField<
    contourtree_maker_inc_ns::TransferLeafChains_TransferToContourTree<DeviceAdapter>>
    transferTorContourTreeDispatcher(transferToContourTreeWorklet);
  transferTorContourTreeDispatcher.Invoke(activeSupernodes,           // (input)
                                          contourTree.hyperparents,   // (output)
                                          contourTree.hyperarcs,      // (output)
                                          contourTree.superarcs,      // (output)
                                          contourTree.whenTransferred // (output)
                                          );

  DebugPrint(isJoin ? "Upper Regular Chains Transferred" : "Lower Regular Chains Transferred",
             __FILE__,
             __LINE__);
} // ContourTreeMaker::TransferLeafChains()


// routine to compress trees by removing regular vertices as well as hypernodes
template <typename DeviceAdapter>
void ContourTreeMaker<DeviceAdapter>::CompressTrees()
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
    vtkm::worklet::DispatcherMapField<contourtree_maker_inc_ns::CompressTrees_Step>
      compressTreesStepDispatcher(compressTreesStepWorklet);
    compressTreesStepDispatcher.Invoke(activeSupernodes,       // (input)
                                       contourTree.superarcs,  // (input)
                                       augmentedJoinSuperarcs, // (input/output)
                                       augmentedSplitSuperarcs // (input/output)
                                       );

  } // iteration log times

  DebugPrint("Trees Compressed", __FILE__, __LINE__);
} // ContourTreeMaker::CompressTrees()


// compresses trees to remove transferred vertices
template <typename DeviceAdapter>
void ContourTreeMaker<DeviceAdapter>::CompressActiveSupernodes()
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
  // Keep only the indicies of the active supernodes that have not been transferred yet
  DeviceAlgorithm::CopyIf(
    activeSupernodes, notTransferredActiveSupernodes, compressedActiveSupernodes);
  // Copy the data into the active supernodes
  activeSupernodes.ReleaseResources();
  activeSupernodes =
    compressedActiveSupernodes; // vtkm ArrayHandles are smart, so we can just swap it in without having to copy

  DebugPrint("Active Supernodes Compressed", __FILE__, __LINE__);
} // ContourTreeMaker::CompressActiveSupernodes()


template <typename DeviceAdapter>
void ContourTreeMaker<DeviceAdapter>::FindDegrees()
{ // ContourTreeMaker::FindDegrees()
  typedef vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType> PermuteIndexArray;

  // retrieve the size to register for speed
  vtkm::Id nActiveSupernodes = activeSupernodes.GetNumberOfValues();

  // reset the updegree & downdegree
  contourtree_maker_inc_ns::FindDegrees_ResetUpAndDowndegree resetUpAndDowndegreeWorklet;
  vtkm::worklet::DispatcherMapField<contourtree_maker_inc_ns::FindDegrees_ResetUpAndDowndegree>
    resetUpAndDowndegreeDispatcher(resetUpAndDowndegreeWorklet);
  resetUpAndDowndegreeDispatcher.Invoke(activeSupernodes, updegree, downdegree);

  DebugPrint("Degrees Set to 0", __FILE__, __LINE__);

  // now we loop through every join & split arc, updating degrees
  // to minimise memory footprint, we will do two separate loops
  // but we could combine the two into paired loops
  // first we establish an array of destination vertices (since outdegree is always 1)
  IdArrayType inNeighbour;
  //inNeighbour.Allocate(nActiveSupernodes);
  //PermuteIndexArray permuteInNeighbour(activeSupernodes, inNeighbour);
  PermuteIndexArray permuteAugmentedJoinSuperarcs(activeSupernodes, augmentedJoinSuperarcs);
  DeviceAlgorithm::Copy(permuteAugmentedJoinSuperarcs, inNeighbour);
  // now sort to group copies together
  DeviceAlgorithm::Sort(inNeighbour);

  // there's probably a smarter scatter-gather solution to this, but this should work
  // find the RHE of each segment
  contourtree_maker_inc_ns::FindDegrees_FindRHE joinFindRHEWorklet(nActiveSupernodes);
  vtkm::worklet::DispatcherMapField<contourtree_maker_inc_ns::FindDegrees_FindRHE>
    joinFindRHEDispatcher(joinFindRHEWorklet);
  joinFindRHEDispatcher.Invoke(inNeighbour, updegree);

  // now subtract the LHE to get the size
  contourtree_maker_inc_ns::FindDegrees_SubtractLHE joinSubractLHEWorklet;
  vtkm::worklet::DispatcherMapField<contourtree_maker_inc_ns::FindDegrees_SubtractLHE>
    joinSubtractLHEDispatcher(joinSubractLHEWorklet);
  joinSubtractLHEDispatcher.Invoke(inNeighbour, updegree);

  // now repeat the same process for the split neighbours
  PermuteIndexArray permuteAugmentedSplitSuperarcs(activeSupernodes, augmentedSplitSuperarcs);
  DeviceAlgorithm::Copy(permuteAugmentedSplitSuperarcs, inNeighbour);
  // now sort to group copies together
  DeviceAlgorithm::Sort(inNeighbour);

  // there's probably a smarter scatter-gather solution to this, but this should work
  // find the RHE of each segment
  contourtree_maker_inc_ns::FindDegrees_FindRHE splitFindRHEWorklet(nActiveSupernodes);
  vtkm::worklet::DispatcherMapField<contourtree_maker_inc_ns::FindDegrees_FindRHE>
    splitFindRHEDispatcher(splitFindRHEWorklet);
  joinFindRHEDispatcher.Invoke(inNeighbour, downdegree);

  // now subtract the LHE to get the size
  contourtree_maker_inc_ns::FindDegrees_SubtractLHE splitSubractLHEWorklet;
  vtkm::worklet::DispatcherMapField<contourtree_maker_inc_ns::FindDegrees_SubtractLHE>
    splitSubtractLHEDispatcher(splitSubractLHEWorklet);
  joinSubtractLHEDispatcher.Invoke(inNeighbour, downdegree);

  DebugPrint("Degrees Computed", __FILE__, __LINE__);
} // ContourTreeMaker::FindDegrees()



template <typename DeviceAdapter>
void ContourTreeMaker<DeviceAdapter>::DebugPrint(const char* message,
                                                 const char* fileName,
                                                 long lineNum)
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


} // namespace contourtree_ppp2
} // worklet
} // vtkm

#endif
