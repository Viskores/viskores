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

#ifndef vtk_m_worklet_contourtree_distributed_boundary_restricted_augmented_contour_tree_maker_h
#define vtk_m_worklet_contourtree_distributed_boundary_restricted_augmented_contour_tree_maker_h

// augmented contour tree includes
#include <vtkm/worklet/contourtree_augmented/ContourTree.h>
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

// distibuted contour tree includes
#include <vtkm/worklet/contourtree_distributed/BoundaryRestrictedAugmentedContourTree.h>
#include <vtkm/worklet/contourtree_distributed/HierarchicalContourTree.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/AddTerminalFlagsToUpDownNeighboursWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/ArraySumFunctor.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/AugmentBoundaryWithNecessaryInteriorSupernodesAppendNecessarySupernodesWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/AugmentBoundaryWithNecessaryInteriorSupernodesUnsetBoundarySupernodesWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/BRACTNodeComparator.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/BoundaryVerticesPerSuperArcWorklets.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/CompressRegularisedNodesCopyNecessaryRegularNodesWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/CompressRegularisedNodesFillBractSuperarcsWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/CompressRegularisedNodesFindNewSuperarcsWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/CompressRegularisedNodesResolveRootWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/CompressRegularisedNodesTransferVerticesWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/ContourTreeNodeHyperArcComparator.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/FindBractSuperarcsSuperarcToWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/FindNecessaryInteriorSetSuperparentNecessaryWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/FindNecessaryInteriorSupernodesFindNodesWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/HyperarcComparator.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/IdentifyRegularisedSupernodesStepOneWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/IdentifyRegularisedSupernodesStepTwoWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/NoSuchElementFunctor.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/PointerDoubleUpDownNeighboursWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/PropagateBoundaryCountsComputeGroupTotalsWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/PropagateBoundaryCountsSubtractDependentCountsWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/PropagateBoundaryCountsTransferCumulativeCountsWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/PropagateBoundaryCountsTransferDependentCountsWorklet.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/SelectRangeFunctor.h>
#include <vtkm/worklet/contourtree_distributed/bract_maker/SetUpAndDownNeighboursWorklet.h>


// vtkm includes
#include <vtkm/Types.h>

// std includes
#include <sstream>
#include <string>
#include <utility>


namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{
/// \brief Class to compute the Boundary Restricted Augmented Contour Tree (BRACT)
///
///
template <typename MeshType, typename MeshBoundaryExecObjType>
class BoundaryRestrictedAugmentedContourTreeMaker
{
public:
  // pointers to underlying data structures
  /// Pointer to the input mesh
  MeshType* Mesh;
  MeshBoundaryExecObjType& MeshBoundaryExecutionObject;

  /// Pointer to the contour tree for the mesh
  vtkm::worklet::contourtree_augmented::ContourTree* ContourTree;
  /// Data structure for storing the results from this class
  BoundaryRestrictedAugmentedContourTree* Bract;

  /// how many vertices ARE on the boundary
  vtkm::Id NumBoundary;

  /// how many interior vertices are necessary
  vtkm::Id NumNecessary;

  /// how many vertices are kept in the BRACT
  vtkm::Id NumKept;

  /// arrays for computation - stored here to simplify debug print

  //  arrays sized to all regular vertices - this may not be necessary, but is robust
  /// array for ID in boundary tree
  vtkm::worklet::contourtree_augmented::IdArrayType BoundaryTreeId;
  vtkm::worklet::contourtree_augmented::IdArrayType HierarchicalTreeId;

  // arrays sized to number of boundary vertices
  /// the regular IDs of the boundary vertices (a conservative over-estimate, needed for hierarchical computation)
  vtkm::worklet::contourtree_augmented::IdArrayType BractVertexSuperset;
  /// their sort indices (may be redundant, but . . .)
  vtkm::worklet::contourtree_augmented::IdArrayType BoundaryIndices;
  /// the superparents for each boundary vertex
  vtkm::worklet::contourtree_augmented::IdArrayType BoundarySuperparents;

  // arrays sized to number of supernodes/superarcs
  /// these are essentially the same as the transfer/intrinsic/dependent weights
  /// probably about time to refactor and do a generic hyperarc propagation routine (owch!)
  /// array of flags for whether necessary
  vtkm::cont::ArrayHandle<bool> IsNecessary;

  /// mapping from tree super ID to bract superset ID (could potentially be combined with isNecessary)
  vtkm::worklet::contourtree_augmented::IdArrayType TreeToSuperset;

  /// vector with flags for type of supernode
  vtkm::worklet::contourtree_augmented::IdArrayType SupernodeType;

  /// vector with the new supernode IDs for each supernode
  vtkm::worklet::contourtree_augmented::IdArrayType NewSupernodeID;

  /// count of boundary nodes on each superarc
  vtkm::worklet::contourtree_augmented::IdArrayType SuperarcIntrinsicBoundaryCount;
  /// count of boundary nodes being transferred at eah supernode
  vtkm::worklet::contourtree_augmented::IdArrayType SupernodeTransferBoundaryCount;
  /// count of dependent boundary nodes for each superarc
  vtkm::worklet::contourtree_augmented::IdArrayType SuperarcDependentBoundaryCount;
  /// count of dependent boundary nodes for each hyperarc
  vtkm::worklet::contourtree_augmented::IdArrayType HyperarcDependentBoundaryCount;

  /// maps supernode IDs to regular IDs in parent hierarchical tree, if any
  vtkm::worklet::contourtree_augmented::IdArrayType HierarchicalRegularId;
  /// does the same to supernode IDs, if any
  vtkm::worklet::contourtree_augmented::IdArrayType HierarchicalSuperId;
  /// maps superparents to superparens in the parent hierarchical tree
  vtkm::worklet::contourtree_augmented::IdArrayType HierarchicalSuperparent;
  ///  maps hypernode IDs to regular IDs in the parent hierarchical tree, if any
  vtkm::worklet::contourtree_augmented::IdArrayType HierarchicalHyperId;
  /// this array tracks which superarc we insert into / belong on
  vtkm::worklet::contourtree_augmented::IdArrayType HierarchicalHyperparent;
  /// this array tracks what the hyperarc points to
  vtkm::worklet::contourtree_augmented::IdArrayType HierarchicalHyperarc;
  /// this array is for tracking when we are transferred
  vtkm::worklet::contourtree_augmented::IdArrayType WhenTransferred;

  // vectors needed for collapsing out "regular" supernodes
  // up- and down- neighbours in the tree (unique for regular nodes)
  // sized to BRACT
  vtkm::worklet::contourtree_augmented::IdArrayType UpNeighbour;
  vtkm::worklet::contourtree_augmented::IdArrayType DownNeighbour;
  /// array needed for compression
  vtkm::worklet::contourtree_augmented::IdArrayType NewVertexId;

  /// active supernode set used for re-constructing hyperstructure
  vtkm::worklet::contourtree_augmented::EdgePairArray ActiveSuperarcs;

  /// arrays holding the nodes, supernodes and hypernodes that need to be transferred
  vtkm::worklet::contourtree_augmented::IdArrayType NewNodes;
  vtkm::worklet::contourtree_augmented::IdArrayType NewSupernodes;
  vtkm::worklet::contourtree_augmented::IdArrayType NewHypernodes;

  /// variable for tracking # of iterations needed in transfer
  vtkm::Id NumTransferIterations;

  VTKM_CONT
  BoundaryRestrictedAugmentedContourTreeMaker(
    MeshType* inputMesh,
    vtkm::worklet::contourtree_augmented::ContourTree* inputTree,
    BoundaryRestrictedAugmentedContourTree* bract,
    MeshBoundaryExecObjType* meshBoundaryExecObj)
    : Mesh(inputMesh)
    , ContourTree(inputTree)
    , Bract(bract)
    , MeshBoundaryExecutionObject(meshBoundaryExecObj)
    , NumBoundary(0)
    , NumNecessary(0)
  {
  }

  /// computes a BRACT from a contour tree for a known block
  /// note the block ID for debug purposes
  VTKM_CONT
  void Construct();

  /// routine to find the set of boundary vertices
  VTKM_CONT
  void FindBoundaryVertices();

  /// routine to compute the initial dependent counts (i.e. along each superarc)
  /// in preparation for hyper-propagation
  VTKM_CONT
  void ComputeDependentBoundaryCounts();

  /// routine for hyper-propagation to compute dependent boundary counts
  VTKM_CONT
  void PropagateBoundaryCounts();

  /// routine to find the necessary interior supernodes for the BRACT
  VTKM_CONT
  void FindNecessaryInteriorSupernodes();

  /// routine to add the necessary interior supernodes to the boundary array
  VTKM_CONT
  void AugmentBoundaryWithNecessaryInteriorSupernodes();

  /// routine that sorts on hyperparent to find BRACT superarcs
  VTKM_CONT
  void FindBractSuperarcs();

  /// compresses out supernodes in the interior that have become regular in the BRACT
  VTKM_CONT
  void SuppressRegularisedInteriorSupernodes();

  /// routine to find *AN* up/down neighbour for each vertex
  /// this is deliberately non-canonical and exploits write-conflicts
  VTKM_CONT
  void SetUpAndDownNeighbours();

  /// routine to set a flag for each vertex that has become regular in the interior of the BRACT
  VTKM_CONT
  void IdentifyRegularisedSupernodes();

  /// this routine sets a flag on every up/down neighbour that points to a critical point
  /// to force termination of pointer-doubling
  VTKM_CONT
  void AddTerminalFlagsToUpDownNeighbours();

  /// routine that uses pointer-doubling to collapse regular nodes in the BRACT
  VTKM_CONT
  void PointerDoubleUpDownNeighbours();

  /// routine that compresses the regular nodes out of the BRACT
  VTKM_CONT
  void CompressRegularisedNodes();

  /// routine to graft the residue from a BRACT into the tree
  VTKM_CONT
  void GraftResidue(int theRound, HierarchicalContourTree& hierarchicalTree);

  /// routine to convert supernode IDs from global to IDs in the existing hierarchical tree
  VTKM_CONT
  void GetHierarchicalIds(HierarchicalContourTree& hierarchicalTree);

  /// sets up an active superarc set
  VTKM_CONT
  void InitializeActiveSuperarcs();

  /// find the critical points in what's left
  VTKM_CONT
  void FindCriticalPoints();

  /// pointer-double to collapse chains
  VTKM_CONT
  void CollapseRegularChains();

  /// routine to identify one iteration worth of leaves
  VTKM_CONT
  void IdentifyLeafHyperarcs();

  /// Compress arrays & repeat
  VTKM_CONT
  void CompressActiveArrays();

  /// Makes a list of new hypernodes, and maps their old IDs to their new ones
  VTKM_CONT
  void ListNewHypernodes(HierarchicalContourTree& hierarchicalTree);

  /// Makes a list of new supernodes, and maps their old IDs to their new ones
  VTKM_CONT
  void ListNewSupernodes(HierarchicalContourTree& hierarchicalTree);

  /// Makes a list of new nodes, and maps their old IDs to their new ones
  VTKM_CONT
  void ListNewNodes(HierarchicalContourTree& hierarchicalTree);

  /// Copies in the hypernodes, now that we have correct super IDs
  VTKM_CONT
  void CopyNewHypernodes(HierarchicalContourTree& hierarchicalTree);

  /// Copies in the supernodes, now that we have correct regular IDs
  VTKM_CONT
  void CopyNewSupernodes(HierarchicalContourTree& hierarchicalTree, vtkm::Id theRound);

  /// Copies the regular nodes in, setting all arrays except superparents
  /// Must be called LAST since it depends on the hypernodes & supernodes that have just been added
  /// in order to resolve the superparents
  VTKM_CONT
  void CopyNewNodes(HierarchicalContourTree& hierarchicalTree);

  /// Transfers the details of nodes used in each iteration
  VTKM_CONT
  void CopyIterationDetails(HierarchicalContourTree& hierarchicalTree, vtkm::Id theRound);

  /// prints the contents of the restrictor object in a standard format
  VTKM_CONT
  std::string DebugPrint(const char* message, const char* fileName, long lineNum) const;

}; // BoundaryRestrictedAugmentedContourTreeMaker class



template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::Construct()
{ // Construct

  // 0.  Retrieve the number of iterations used to construct the contour tree
  //    NB: There may be sense in reusing transfer & dependent weight arrays,
  //    and nIterations, and the arrays for the super/hyper arc subsegments per pass
  //    but for now, burn extra memory
  //    The +1 is because this is 0-indexed

  // Step I: Initialise the bract to hold the set of boundary vertices
  //        & save how many for later
  FindBoundaryVertices();

  // Step II: For each supernode / superarc, compute the dependent boundary counts
  ComputeDependentBoundaryCounts();

  // Step III: We now have initial weights and do the standard inward propagation through
  // the hyperstructure to compute properly over the entire tree
  PropagateBoundaryCounts();

  // Step IV:  We now use the dependent weight to identify which internal supernodes are necessary
  FindNecessaryInteriorSupernodes();

  // Step V: Add the necessary interior nodes to the end of the boundary set
  AugmentBoundaryWithNecessaryInteriorSupernodes();

  // Step VI: Use the hyperparents to sort these vertices into contiguous chunks as usual
  //          We will store the BRACT ID for the superarc target this to simplify the next step
  FindBractSuperarcs();

  // Step VII: Suppress interior supernodes that are regular in the BRACT
  //           At the end, we will reset the superarc target from BRACT ID to block ID
  SuppressRegularisedInteriorSupernodes();

#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             this->Bract->DebugPrint("All Completed\n", __FILE__, __LINE__));
#endif
} // Construct


/// routine to find the set of boundary vertices
///
/// Side-effects: This function updates:
///   - this->this->BractVertexSuperset
///   - this->BoundaryIndices
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType,
                                                 MeshBoundaryExecObjType>::FindBoundaryVertices()
{ // FindBoundaryVertices
  // ask the mesh to give us a list of boundary verticels (with their regular indices)
  this->Mesh->GetBoundaryVertices(
    this->BractVertexSuperset, this->BoundaryIndices, this->MeshBoundaryExecutionObject);
  // pull a local copy of the size (they can diverge)
  this->NumBoundary = this->BractVertexSuperset.GetNumberOfValues();
#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             this->DebugPrint("Boundary Vertices Set", __FILE__, __LINE__));
#endif
} // FindBoundaryVertices


/// routine to compute the initial dependent counts (i.e. along each superarc)
/// in preparation for hyper-propagation
///
/// Side-effects: This function updates:
///   - this->BoundarySuperparents
///   - this->SuperarcIntrinsicBoundaryCount
///   - this->BoundarySuperparents
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  ComputeDependentBoundaryCounts()
{ // ComputeDependentBoundaryCounts
  // 1.  copy in the superparent from the regular arrays in the contour tree
  auto permutedContourTreeSuperparents =
    vtkm::cont::make_ArrayHandlePermutation(this->BoundaryIndices, this->ContourTree->Superparents);
  vtkm::cont::Algorithm::Copy(permutedContourTreeSuperparents, this->BoundarySuperparents);

#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, this->DebugPrint("Superparents Set", __FILE__, __LINE__));
#endif

  // 2. Sort this set & count by superarc to set initial intrinsic boundary counts
  //    Note that this is in the parallel style, but can be done more efficiently in serial
  //    a. Allocate space for the count & initialise to zero
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleConstant(0, this->ContourTree->Superarcs.GetNumberOfValues()),
    this->SuperarcIntrinsicBoundaryCount);
  //   b. sort the superparents
  vtkm::cont::Algorithm::Sort(this->BoundarySuperparents);

  // c.  Now compute the number of boundary vertices per superarc
  //    This *could* be done with a prefix sum, but it's cheaper to do it this way with 2 passes
  //    NB: first superarc's beginning is always 0, so we can omit it, which simplifies the IF logic
  //    i. Start by detecting the high end of the range and then ii) setting the last element explicitly
  bract_maker::BoundaryVerticiesPerSuperArcStepOneWorklet tempWorklet1(this->NumBoundary);
  this->Invoke(tempWorklet1,
               this->BoundarySuperparents,
               this->SuperarcIntrinsicBoundaryCount // output
  );

  // iii.Repeat to subtract and compute the extent lengths (i.e. the counts)
  //     Note that the 0th element will subtract 0 and can be omitted
  bract_maker::BoundaryVerticiesPerSuperArcStepTwoWorklet tempWorklet2;
  this->Invoke(tempWorklet2,
               this->BoundarySuperparents,
               this->SuperarcIntrinsicBoundaryCount // output
  );

  // resize local array
  this->BoundarySuperparents.ReleaseResources();

#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             this->DebugPrint("Initial Counts Set", __FILE__, __LINE__));
#endif
} // ComputeDependentBoundaryCounts


/// routine for hyper-propagation to compute dependent boundary counts
///
///  Side-effects: This function updates:
///    - this->SupernodeTransferBoundaryCount
///    - this->SuperarcDependentBoundaryCount
///    - this->HyperarcDependentBoundaryCount
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType,
                                                 MeshBoundaryExecObjType>::PropagateBoundaryCounts()
{ // PropagateBoundaryCounts
  //   1.  Propagate boundary counts inwards along super/hyperarcs (same as ComputeWeights)
  //    a.  Initialise arrays for transfer & dependent counts
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleConstant(0, this->ContourTree->Supernodes.GetNumberOfValues()),
    this->SupernodeTransferBoundaryCount);
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleConstant(0, this->ContourTree->Superarcs.GetNumberOfValues()),
    this->SuperarcDependentBoundaryCount);
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleConstant(0, this->ContourTree->Hyperarcs.GetNumberOfValues()),
    this->HyperarcDependentBoundaryCount);

  // b.  Iterate, propagating counts inwards
  auto firstSupernodePerIterationReadPortal =
    this->ContourTree->FirstSupernodePerIteration.ReadPortal();
  auto firstHypernodePerIterationReadPortal =
    this->ContourTree->FirstHypernodePerIteration.ReadPortal();
  for (vtkm::Id iteration = 0; iteration < this->ContourTree->NumIterations; iteration++)
  { // b. per iteration
    // i. Pull the array bounds into register
    vtkm::Id firstSupernode = firstSupernodePerIterationReadPortal.Get(iteration);
    vtkm::Id lastSupernode = firstSupernodePerIterationReadPortal.Get(iteration + 1);
    vtkm::Id firstHypernode = firstHypernodePerIterationReadPortal.Get(iteration);
    vtkm::Id lastHypernode = firstHypernodePerIterationReadPortal.Get(iteration + 1);

    //  ii.  Add xfer + int & store in dependent count
    //      Compute the sum of this->SupernodeTransferBoundaryCount and this->SuperarcIntrinsicBoundaryCount
    //      for the [firstSupernodex, lastSupernode) subrange and copy to the this->SuperarcDependentBoundaryCount
    { // make local context ot fancyTempSumArray gets deleted
      auto fancyTempSumArray = vtkm::cont::make_ArrayHandleImplicit(
        bract_maker::ArraySumFunctor(this->SupernodeTransferBoundaryCount,
                                     this->SuperarcIntrinsicBoundaryCount),
        this->SupernodeTransferBoundaryCount.GetNumberOfValues());
      vtkm::cont::Algorithm::CopySubRange(
        fancyTempSumArray,                    // input array
        firstSupernode,                       // start index for the copy
        lastSupernode - firstSupernode,       // number of values to copy
        this->SuperarcDependentBoundaryCount, // target output array
        firstSupernode // index in the output array where we start writing values to
      );
    } // end local context for step ii

    // iii.Perform prefix sum on dependent count range
    { // make local context so tempArray and fancyRangeArraySuperarcDependentBoundaryCountget  deleted
      auto fancyRangeArraySuperarcDependentBoundaryCount = vtkm::cont::make_ArrayHandleImplicit(
        bract_maker::SelectRangeFunctor(this->SuperarcDependentBoundaryCount, firstSupernode),
        lastSupernode - firstSupernode);
      // Write to temporary array first as it is not clear whether ScanInclusive is safe to read and write
      // to the same array and range
      vtkm::worklet::contourtree_augmented::IdArrayType tempArray;
      vtkm::cont::Algorithm::ScanInclusive(fancyRangeArraySuperarcDependentBoundaryCount,
                                           tempArray);
      vtkm::cont::Algorithm::CopySubRange(
        tempArray,
        0,
        tempArray.GetNumberOfValues(), // copy all of tempArray
        this->SuperarcDependentBoundaryCount,
        firstSupernode // to the SuperarcDependentBoundaryCound starting at firstSupernode
      );
    } // end local context for step iii

    //  iv.  Subtract out the dependent count of the prefix to the entire hyperarc
    { // make local context for iv so newSuperArcDependentBoundaryCount and the worklet gets deleted
      // Storage for the vector portion that will be modified
      vtkm::worklet::contourtree_augmented::IdArrayType newSuperArcDependentBoundaryCount;
      vtkm::cont::Algorithm::CopySubRange(this->SuperarcDependentBoundaryCount,
                                          firstSupernode,
                                          lastSupernode - firstSupernode,
                                          newSuperArcDependentBoundaryCount);
      auto subtractDependentCountsWorklet =
        bract_maker::PropagateBoundaryCountsSubtractDependentCountsWorklet(firstSupernode,
                                                                           firstHypernode);
      // per supenode
      this->Invoke(
        subtractDependentCountsWorklet,
        // use backwards counting array for consistency with original code, but if forward or backward doesn't matter
        vtkm::cont::ArrayHandleCounting<vtkm::Id>(
          lastSupernode - 1, -1, lastSupernode - firstSupernode - 1), // input
        this->ContourTree->Hyperparents,                              // input
        this->ContourTree->Hypernodes,                                // input
        this->SuperarcDependentBoundaryCount,                         // input
        newSuperArcDependentBoundaryCount                             // (input/output)
      );
      // copy the results back into our main array
      vtkm::cont::Algorithm::CopySubRange(newSuperArcDependentBoundaryCount,
                                          0,
                                          newSuperArcDependentBoundaryCount.GetNumberOfValues(),
                                          this->SuperarcDependentBoundaryCount,
                                          firstSupernode);
    } // end local context of iv

    //  v.  Transfer the dependent count to the hyperarc's target supernode
    { // local context of v.
      auto transferDependentCountsWorklet =
        bract_maker::PropagateBoundaryCountsTransferDependentCountsWorklet(
          this->ContourTree->Supernodes.GetNumberOfValues(),
          this->ContourTree->Hypernodes.GetNumberOfValues());
      this->Invoke(
        transferDependentCountsWorklet,
        // for (vtkm::Id hypernode = firstHypernode; hypernode < lastHypernode; hypernode++)
        vtkm::cont::ArrayHandleCounting<vtkm::Id>(
          firstHypernode, 1, lastHypernode - firstHypernode), // input
        this->ContourTree->Hypernodes,                        // input
        this->SuperarcDependentBoundaryCount,                 // input
        this->HyperarcDependentBoundaryCount                  // output
      );
      // transferring the count is done as a separate reduction
    } // end local context for step iv


    // next we want to end up summing transfer count & storing in the target.
    // Unfortunately, there may be multiple hyperarcs in a given
    // pass that target the same supernode, so we have to do this separately.
    // 1. permute so that all hypernodes with the same target are contiguous
    { // local context for summing transfer count & storing in the target.
      vtkm::worklet::contourtree_augmented::IdArrayType hyperarcTargetSortPermutation;
      vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleCounting<vtkm::Id>(
                                    firstHypernode, 1, lastHypernode - firstHypernode),
                                  hyperarcTargetSortPermutation);

      // 2. sort the elements to cluster by hyperarc target, using a lambda function for comparator
      //    we need a reference that a lambda function can use
      auto hyperarcComparator = bract_maker::HyperarcComparator(this->ContourTree->Hyperarcs);
      vtkm::cont::Algorithm::Sort(hyperarcTargetSortPermutation, hyperarcComparator);

      // 3. now compute the partial sum for the properly permuted boundary counts
      // total boundary count at each node
      vtkm::worklet::contourtree_augmented::IdArrayType accumulatedBoundaryCount;
      auto permutedHyperarcDependentCount =
        vtkm::cont::make_ArrayHandlePermutation(hyperarcTargetSortPermutation,       // id array
                                                this->HyperarcDependentBoundaryCount // value array
        );
      vtkm::cont::Algorithm::ScanInclusive(permutedHyperarcDependentCount,
                                           accumulatedBoundaryCount);

      // 4. The partial sum is now over ALL hypertargets, so within each group we need to subtract the first from the last
      // To do so, the last hyperarc in each cluster copies its cumulative count to the output array
      auto transferCumulativeCountsWorklet =
        bract_maker::PropagateBoundaryCountsTransferCumulativeCountsWorklet();
      this->Invoke(transferCumulativeCountsWorklet,
                   this->ContourTree->Hyperarcs,
                   hyperarcTargetSortPermutation,
                   accumulatedBoundaryCount,
                   this->SupernodeTransferBoundaryCount);

      // 5. Finally, we subtract the beginning of the group to get the total for each group
      // Note that we start the loop from 1, which means we don't need a special case if statement
      // because the prefix sum of the first element is the correct value anyway
      auto computeGroupTotalsWorklet =
        bract_maker::PropagateBoundaryCountsComputeGroupTotalsWorklet();
      this->Invoke(computeGroupTotalsWorklet,
                   this->ContourTree->Hyperarcs,
                   hyperarcTargetSortPermutation,
                   accumulatedBoundaryCount,
                   this->SupernodeTransferBoundaryCount);

    } // end  local context for summing transfer count & storing in the target.
  }   // b. per iteration

  // when we are done, we need to force the summation for the root node, JUST IN CASE it is a boundary node itself
  // BTW, the value *SHOULD* be the number of boundary nodes, anyway
  vtkm::Id rootSuperId = this->ContourTree->Supernodes.GetNumberOfValues() - 1;
  this->SuperarcDependentBoundaryCount.WritePortal().Set(
    rootSuperId,
    this->SupernodeTransferBoundaryCount.ReadPortal().Get(rootSuperId) +
      this->SuperarcIntrinsicBoundaryCount.ReadPortal().Get(rootSuperId));
  this->HyperarcDependentBoundaryCount.WritePortal().Set(
    this->ContourTree->Hypernodes.GetNumberOfValues() - 1,
    this->SuperarcDependentBoundaryCount.ReadPortal().Get(rootSuperId));

#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             this->DebugPrint("Iterations Complete", __FILE__, __LINE__));
#endif
} // PropagateBoundaryCounts


/// routine to find the necessary interior supernodes for the BRACT
///
/// INVARIANT:
/// We have now computed the dependent weight for each supernode
///    For boundary nodes, we will ignore this
///    For non-boundary nodes, if the dependent weight is 0 or nBoundary
///    then all boundary nodes are in one direction, and the node is unnecessary
/// We have decided that if a superarc has any boundary nodes, the entire superarc
/// should be treated as necessary. This extends the criteria so that the superparent and superparent's
/// superarc of any boundary node are necessary
///
/// Side-effects: This function updates:
///    - this->IsNecessary
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  FindNecessaryInteriorSupernodes()
{ // FindNecessaryInteriorSupernodes
  //  1. Identify the necessary supernodes (between two boundary points & still critical)
  //  1.A.  Start by setting all of them to "unnecessary"
  // Initalize isNecessary with False
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleConstant(false, this->ContourTree->Supernodes.GetNumberOfValues()),
    this->IsNecessary);
  // 1.B.  Our condition is that if the superarc dependent count is neither 0 nor the # of boundary
  //       points, the superarc target is necessary. Note that there may be write conflicts, but it's
  //       an OR operation, so it doesn't matter
  auto findNodesWorklet =
    bract_maker::FindNecessaryInteriorSupernodesFindNodesWorklet(this->NumBoundary);
  this->Invoke(findNodesWorklet,
               this->ContourTree->Superarcs,         // input
               this->SuperarcDependentBoundaryCount, // input
               this->IsNecessary                     // output
  );

  // separate pass to set the superparent of every boundary node to be necessary
  auto setSuperparentNecessaryWorklet =
    bract_maker::FindNecessaryInteriorSetSuperparentNecessaryWorklet();
  this->Invoke(setSuperparentNecessaryWorklet,
               this->BoundaryIndices,
               this->ContourTree->Superparents,
               this->ContourTree->Superarcs,
               this->IsNecessary);
#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, this->DebugPrint("Is Necessary Set", __FILE__, __LINE__));
#endif
} // FindNecessaryInteriorSupernodes()

/// routine to add the necessary interior supernodes to the boundary array
///
/// Side effects: This function updates:
/// - this->NumNecessary
/// - this->BoundaryIndices,
/// - this->BractVertexSuperset
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  AugmentBoundaryWithNecessaryInteriorSupernodes()
{ // AugmentBoundaryWithNecessaryInteriorSupernodes
  //  1.  Collect the necessary supernodes & boundary vertices & suppress duplicates
  vtkm::worklet::contourtree_augmented::IdArrayType isNecessaryAndInterior;
  vtkm::cont::Algorithm::Copy(this->IsNecessary, isNecessaryAndInterior);

  //  a.  To do this, first *UNSET* the necessary flags for all supernodes that are also on the boundary
  auto unsetBoundarySupernodesWorklet =
    bract_maker::AugmentBoundaryWithNecessaryInteriorSupernodesUnsetBoundarySupernodesWorklet();
  this->Invoke(unsetBoundarySupernodesWorklet,
               this->BoundaryIndices,           // input
               this->ContourTree->Superparents, // input
               this->ContourTree->Supernodes,   // input
               isNecessaryAndInterior           // output
  );

  //  b.  Now append all necessary supernodes to the boundary vertex array
  // first count how many are needed, then resize the arrays
  this->NumNecessary = vtkm::cont::Algorithm::Reduce(isNecessaryAndInterior, 0);
  // TODO Check if Allocate is enough or if we need to do ReleseResources first as well?
  this->BractVertexSuperset.Allocate(this->NumBoundary + this->NumNecessary);
  this->BoundaryIndices.Allocate(this->NumBoundary + this->NumNecessary);

  // create a temporary array for transfer IDs
  vtkm::worklet::contourtree_augmented::IdArrayType boundaryNecessaryId;
  boundaryNecessaryId.Allocate(this->ContourTree->Supernodes.GetNumberOfValues());
  // and partial sum them in place
  vtkm::cont::Algorithm::ScanInclusive(isNecessaryAndInterior, boundaryNecessaryId);

  // now do a parallel for loop to copy them
  auto appendNecessarySupernodesWorklet =
    bract_maker::AugmentBoundaryWithNecessaryInteriorSupernodesAppendNecessarySupernodesWorklet(
      this->NumBoundary);
  this->Invoke(appendNecessarySupernodesWorklet,
               this->ContourTree->Supernodes,
               isNecessaryAndInterior,
               boundaryNecessaryId,
               this->Mesh->SortOrder,
               this->BoundaryIndices,
               this->BractVertexSuperset);
#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             this->DebugPrint("Necessary Appended", __FILE__, __LINE__));
#endif
} // AugmentBoundaryWithNecessaryInteriorSupernodes


/// routine that sorts on hyperparent to find BRACT superarcs
///
///  Side-effects: This function updates:
///  - this->TreeToSuperset
///  - this->BoundaryIndices
///  - this->BractVertexSuperset
///  - this->Bract->Superarcs
///  - this->BoundaryTreeId
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType,
                                                 MeshBoundaryExecObjType>::FindBractSuperarcs()
{ // FindBractSuperarcs
  //  0.  Allocate memory for the tree2superset map
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleConstant(vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                                         this->ContourTree->Supernodes.GetNumberOfValues()),
    this->TreeToSuperset);

  //  1.  Sort the boundary set by hyperparent
  auto contourTreeNodeHyperArcComparator = bract_maker::ContourTreeNodeHyperArcComparator(
    this->ContourTree->Superarcs, this->ContourTree->Superparents);
  vtkm::cont::Algorithm::Sort(this->BoundaryIndices, contourTreeNodeHyperArcComparator);

  //  2.  Reset the order of the vertices in the BRACT
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandlePermutation(this->BoundaryIndices, // index array to permute with
                                            this->Mesh->SortOrder  // value array to be permuted
                                            ),
    this->BractVertexSuperset // Copy the permuted Mesh->SortOrder to BractVertexSuperset
  );

  // allocate memory for the superarcs (same size as supernodes for now)
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleConstant(vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                                         this->BractVertexSuperset.GetNumberOfValues()),
    this->Bract->Superarcs);

  // We would like to connect vertices to their neighbour on the hyperarc as usual
  // The problem here is that the root of the tree may be unnecessary
  // and if that is the case we will need to adjust

  // The test will be:
  //     i.  if there is a "next", we will take it
  //    ii.  if we are dangling at the end of the hyperarc, two possibilities exist
  //      a.  the supernode target of the hyperarc is in the BRACT anyway
  //      b.  the supernode target is not in the BRACT

  // To resolve all this, we will need to have an array the size of all the regular nodes
  // in order to find the boundary ID of each vertex transferred
  {
    auto tempPermutedBoundaryTreeId =
      vtkm::cont::make_ArrayHandlePermutation(this->BractVertexSuperset, // index array
                                              this->BoundaryTreeId       // value array
      );
    vtkm::cont::Algorithm::Copy(
      vtkm::cont::ArrayHandleIndex(this->BractVertexSuperset.GetNumberOfValues), // copy 0,1, ... n
      tempPermutedBoundaryTreeId // copy to BoundaryTreeId permuted by BractVertexIndex
    );
  }
  // We now compute the superarc "to" for every bract node
  auto superarcToWorklet = bract_maker::FindBractSuperarcsSuperarcToWorklet();
  this->Invoke(superarcToWorklet,
               this->BractVertexSuperset,       // input
               this->BoundaryIndices,           // input
               this->BoundaryTreeId,            // input
               this->ContourTree->Superparents, // input
               this->ContourTree->Hyperparents, // input
               this->ContourTree->Hyperarcs,    // input
               this->ContourTree->Supernodes,   // input
               this->Mesh->SortOrder,           // input
               this->TreeToSuperset,            // output
               this->Bract->Superarcs           // output
  );

#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             this->DebugPrint("Restricted to Boundary", __FILE__, __LINE__));
#endif
} // FindBractSuperarcs


/// compresses out supernodes in the interior that have become regular in the BRACT
///
/// Side-effects: This function has the cummulative side effects of
/// - this->SetUpAndDownNeighbours();
/// - this->IdentifyRegularisedSupernodes();
/// - this->AddTerminalFlagsToUpDownNeighbours();
/// - this->PointerDoubleUpDownNeighbours();
/// - this->CompressRegularisedNodes();
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  SuppressRegularisedInteriorSupernodes()
{ // SuppressRegularisedInteriorSupernodes
  // 1.   We have to suppress regular vertices that were interior critical points
  //      We can't get rid of them earlier because we need them to connect super-/hyper- arcs

  //  STEP I:    Find a (non-canonical) up/down neighbour for each vertex
  SetUpAndDownNeighbours();

  //  STEP II:  Find the critical points
  IdentifyRegularisedSupernodes();

  //  STEP III:  Set flags to indicate which pointers are terminal
  AddTerminalFlagsToUpDownNeighbours();

  //  STEP IV:  Use pointer-doubling to collapse past regular nodes
  PointerDoubleUpDownNeighbours();

  //  STEP V:    Get rid of the now regular interior supernodes
  CompressRegularisedNodes();
} // SuppressRegularisedInteriorSupernodes


/// routine to find *AN* up/down neighbour for each vertex
/// this is deliberately non-canonical and exploits write-conflicts
///
/// Side effect: This function updates
/// - this->UpNeighbour
/// - this->DownNeighbour
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType,
                                                 MeshBoundaryExecObjType>::SetUpAndDownNeighbours()
{ // SetUpAndDownNeighbours
  //  So, we will set an up- and down-neighbour for each one (for critical points, non-canonical)
  { // local context
    auto tempNoSuchElementArray =
      vtkm::cont::make_ArrayHandleConstant(vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                                           this->BractVertexSuperset.GetNumberOfValues());
    vtkm::cont::Algorithm::Copy(tempNoSuchElementArray, this->UpNeighbour);
    vtkm::cont::Algorithm::Copy(tempNoSuchElementArray, this->DownNeighbour);
  } // end local context

  auto setUpAndDownNeighboursWorklet = bract_maker::SetUpAndDownNeighboursWorklet();
  this->Invoke(setUpAndDownNeighboursWorklet,
               this->BractVertexSuperset, // input
               this->Bract->Superarcs,    // input
               this->Mesh->SortIndex,     // input
               this->UpNeighbour,         // output
               this->DownNeighbour        // output
  );

#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             this->DebugPrint("Initial Up/Down Neighbours Set", __FILE__, __LINE__));
#endif
} // SetUpAndDownNeighbours


/// routine to set a flag for each vertex that has become regular in the interior of the BRACT
///
/// Side effect: This function updates
/// - this->NewVertexId
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  IdentifyRegularisedSupernodes()
{ // IdentifyRegularisedSupernodes
  // here, if any edge detects the up/down neighbours mismatch, we must have had a write conflict
  // it then follows we have a critical point and can set a flag accordingly
  // we will use a vector that stores NO_SUCH_ELEMENT for false, anything else for true
  // it gets reused as an ID
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleConstant(vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                                         this->BractVertexSuperset.GetNumberOfValues()),
    this->NewVertexId);

  auto stepOneWorklet = bract_maker::IdentifyRegularisedSupernodesStepOneWorklet();
  this->Invoke(stepOneWorklet,
               this->BractVertexSuperset, // input
               this->Bract->Superarcs,    // input
               this->Mesh->SortIndex,     // input
               this->UpNeighbour,         // input
               this->DownNeighbour,       // input
               this->NewVertexId          // output
  );

  //  c.  We also want to flag the leaves and boundary nodes as necessary
  auto stepTwoWorklet = bract_maker::IdentifyRegularisedSupernodesStepTwoWorklet();
  this->Invoke(stepTwoWorklet,
               this->BractVertexSuperset,         // input
               this->UpNeighbour,                 // input
               this->DownNeighbour,               // input
               this->MeshBoundaryExecutionObject, // input
               this->NewVertexId                  // output
  );

#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             this->DebugPrint("Boundaries & Leaves Set", __FILE__, __LINE__));
#endif
} // IdentifyRegularisedSupernodes


/// this routine sets a flag on every up/down neighbour that points to a critical point
/// to force termination of pointer-doubling
///
/// Side effects: This function updates
/// - this->UpNeighbour
/// - this->DownNeighbour
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  AddTerminalFlagsToUpDownNeighbours()
{ //
  //  d.  Now that we know which vertices are necessary, we can set the upNeighbour & downNeighbour flags
  auto addTerminalFlagsToUpDownNeighboursWorklet =
    bract_maker::AddTerminalFlagsToUpDownNeighboursWorklet();
  this->Invoke(addTerminalFlagsToUpDownNeighboursWorklet,
               this->NewVertexId,  // input
               this->UpNeighbour,  // output
               this->DownNeighbour // output
  );

#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             this->DebugPrint("Up/Down Neighbours Terminated", __FILE__, __LINE__));
#endif
} //


/// routine that uses pointer-doubling to collapse regular nodes in the BRACT
///
/// Side effects: This functions updates
/// - this->UpNeighbour,
/// - this->DownNeighbour
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  PointerDoubleUpDownNeighbours()
{ // PointerDoubleUpDownNeighbours
  // e. Use pointer-doubling to eliminate regular nodes
  // 1. Compute the number of log steps
  vtkm::Id nLogSteps = 1;
  for (vtkm::Id shifter = this->BractVertexSuperset.GetNumberOfValues(); shifter != 0;
       shifter >>= 1)
  {
    nLogSteps++;
  }

  //  2. loop that many times to do the compression
  for (vtkm::Id iteration = 0; iteration < nLogSteps; iteration++)
  { // per iteration
    // loop through the vertices, updating both ends
    auto pointerDoubleUpDownNeighboursWorklet = bract_maker::PointerDoubleUpDownNeighboursWorklet();
    this->Invoke(pointerDoubleUpDownNeighboursWorklet, this->UpNeighbour, this->DownNeighbour);
  } // per iteration
#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             this->DebugPrint("Pointer Doubling Done", __FILE__, __LINE__));
#endif
} // PointerDoubleUpDownNeighbours


/// routine that compresses the regular nodes out of the BRACT
///
/// Side effects: This function updates:
///  - this->>NewVertexId
///  - this->NumKept
///  - this->Bract->VertexIndex
///  - this->Bract->Superarcs
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  CompressRegularisedNodes()
{ // CompressRegularisedNodes
  //  f.  Compress the regular nodes out of the tree
  //  1.  Assign new indices
  //      Copy the necessary ones only - this is a compression call in parallel
  // HAC: to use a different array, keeping isNecessary indexed on everything
  //      We now reset it to the size of the return tree
  vtkm::worklet::contourtree_augmented::IdArrayType keptInBract;
  //    Start by creating the ID #s with a partial sum (these will actually start from 1, not 0
  vtkm::cont::Algorithm::ScanInclusive(
    vtkm::cont::make_ArrayHandleTransform(this->NewVertexId, bract_maker::NoSuchElementFunctor()),
    keptInBract);
  // Update newVertexID, i.e., for each element set:
  // if (!noSuchElement(newVertexID[returnIndex]))
  //      newVertexID[returnIndex] = keptInBract[returnIndex]-1;

  auto copyNecessaryRegularNodesWorklet =
    bract_maker::CompressRegularisedNodesCopyNecessaryRegularNodesWorklet();
  this->Invoke(copyNecessaryRegularNodesWorklet, this->NewVertexId, keptInBract);

  //    2.  Work out the new superarcs, which is slightly tricky, since they point inbound.
  //      For each necessary vertex N, the inbound vertex I in the original contour tree can point to:
  //      i.    Another necessary vertex (in which case we keep the superarc)
  //      ii.    Nothing (in the case of the root) - again, we keep it, since we already know it's necessar
  //      iii.  An unnecessary vertex (i.e. any other case).  Here, the treatment is more complex.
  //          In this case, we know that the pointer-doubling has forced the up/down neighbours of I
  //          to point to necessary vertices (or nothing).  And since we know that there is an inbound
  //          edge from N, N must therefore be either the up or down neighbour of I after doubling.
  //          We therefore go the other way to find which necessary vertex V that N must connect to.
  //
  //      Just to make it interesting, the root vertex can become unnecessary. If this is the case, we end up with two
  //       necessary vertices each with a superarc to the other.  In serial, we can deal with this by checking to see whether
  //      the far end has already been set to ourself, but this doesn't parallelise properly.  So we will have an extra pass
  //      just to get this correct.
  //  We will compute the new superarcs directly with the new size
  //
  //  To do this in parallel, we compute for every vertex

  // first create the array: start by observing that the last entry is guaranteed
  // to hold the total number of necessary vertices
  this->NumKept = keptInBract.ReadPortal().Get(keptInBract.GetNumberOfValues() - 1);
  // create an array to store the new superarc Ids and initalize it with NO_SUCH_ELEMENT
  vtkm::worklet::contourtree_augmented::IdArrayType newSuperarc;
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandleConstant(vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
                                         this->NumKept),
    newSuperarc);

  auto findNewSuperarcsWorklet = bract_maker::CompressRegularisedNodesFindNewSuperarcsWorklet();
  this->Invoke(findNewSuperarcsWorklet,
               this->NewVertexId,   //input
               this->Superarcs,     //input
               this->UpNeighbour,   //input
               this->DownNeighbour, //input
               newSuperarc          //output
  );

  //  3.  Now do the pass to resolve the root: choose the direction with decreasing index
  auto resolveRootWorklet = bract_maker::CompressRegularisedNodesResolveRootWorklet();
  this->Invoke(resolveRootWorklet,
               vtkm::cont::ArrayHandleIndex(this->NumKept), // input
               newSuperarc                                  // output
  );

  // 4.  Now transfer the vertices & resize
  vtkm::worklet::contourtree_augmented::IdArrayType newVertexIndex;
  newVertexIndex.Allocate(this->NumKept);
  auto transferVerticesWorklet = bract_maker::CompressRegularisedNodesTransferVerticesWorklet();
  this->Invoke(transferVerticesWorklet,
               this->BractVertexSuperset, // input
               this->NewVertexId,         // input
               newVertexIndex             // output
  );

  // 5.  Create an index array and sort it indirectly by sortOrder
  vtkm::worklet::contourtree_augmented::IdArrayType vertexSorter;
  vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleIndex(this->NumKept), vertexSorter);
  auto bractNodeComparator = bract_maker::BRACTNodeComparator(newVertexIndex, this->Mesh.SortIndex);
  vtkm::cont::Algorithm::Sort(vertexSorter, bractNodeComparator);
  vtkm::worklet::contourtree_augmented::IdArrayType reverseSorter;
  {
    auto permutedReverseSorter =
      vtkm::cont::make_ArrayHandlePermutation(vertexSorter, reverseSorter);
    vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleIndex(this->NumKept), permutedReverseSorter);
  }

  //    6.  Resize both vertex IDs and superarcs, and copy in by sorted order
  // copy the vertex index with indirection, and using the sort order NOT the regular ID
  // the following Copy operation is equivilant to
  // bract->vertexIndex[bractID] = mesh->SortIndex(newVertexIndex[vertexSorter[bractID]]);
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::make_ArrayHandlePermutation(
      vtkm::cont::make_ArrayHandlePermutation(vertexSorter, newVertexIndex), this->Mesh->SortIndex),
    this->Bract->VertexIndex);
  // now copy the this->Bract->Superarcs
  auto fillBractSuperarcsWorklet = bract_maker::CompressRegularisedNodesFillBractSuperarcsWorklet();
  this->Invoke(
    fillBractSuperarcsWorklet, newSuperarc, reverseSorter, vertexSorter, this->Bract->Superarcs);

#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             this->DebugPrint("Regularised Nodes Compressed", __FILE__, __LINE__));
#endif
} // CompressRegularisedNodes


/// routine to graft the residue from a BRACT into the tree
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::GraftResidue(
  int theRound,
  HierarchicalContourTree& hierarchicalTree)
{ // GraftResidue
  // TODO
  (void)theRound;
  (void)hierarchicalTree;
} // GraftResidue


/// routine to convert supernode IDs from global to IDs in the existing hierarchical tree
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  GetHierarchicalIds(HierarchicalContourTree& hierarchicalTree)
{ // GetHierarchicalIds
  // TODO
  (void)hierarchicalTree;
} // GetHierarchicalIds


/// sets up an active superarc set
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  InitializeActiveSuperarcs()
{ // InitializeActiveSuperarcs
  // TODO
} // InitializeActiveSuperarcs


/// find the critical points in what's left
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType,
                                                 MeshBoundaryExecObjType>::FindCriticalPoints()
{ // FindCriticalPoints
  // TODO
} // FindCriticalPoints


/// pointer-double to collapse chains
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType,
                                                 MeshBoundaryExecObjType>::CollapseRegularChains()
{ // CollapseRegularChains
  // TODO
} // CollapseRegularChains


/// routine to identify one iteration worth of leaves
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType,
                                                 MeshBoundaryExecObjType>::IdentifyLeafHyperarcs()
{ // IdentifyLeafHyperarcs
  // TODO
} // IdentifyLeafHyperarcs


/// Compress arrays & repeat
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType,
                                                 MeshBoundaryExecObjType>::CompressActiveArrays()
{ // CompressActiveArrays
  // TODO
} // CompressActiveArrays


/// Makes a list of new hypernodes, and maps their old IDs to their new ones
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  ListNewHypernodes(HierarchicalContourTree& hierarchicalTree)
{ // ListNewHypernodes
  // TODO
  (void)hierarchicalTree;
} // ListNewHypernodes


/// Makes a list of new supernodes, and maps their old IDs to their new ones
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  ListNewSupernodes(HierarchicalContourTree& hierarchicalTree)
{ // ListNewSupernodes
  // TODO
  (void)hierarchicalTree;
} // ListNewSupernodes


/// Makes a list of new nodes, and maps their old IDs to their new ones
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::ListNewNodes(
  HierarchicalContourTree& hierarchicalTree)
{ // ListNewNodes
  // TODO
  (void)hierarchicalTree;
} // ListNewNodes


/// Copies in the hypernodes, now that we have correct super Ids
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  CopyNewHypernodes(HierarchicalContourTree& hierarchicalTree)
{ // CopyNewHypernodes
  // TODO
  (void)hierarchicalTree;
} // CopyNewHypernodes


/// Copies in the supernodes, now that we have correct regular Ids
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  CopyNewSupernodes(HierarchicalContourTree& hierarchicalTree, vtkm::Id theRound)
{ // CopyNewSupernodes
  // TODO
  (void)hierarchicalTree;
  (void)theRound;
} // CopyNewSupernodes


/// Copies the regular nodes in, setting all arrays except superparents
/// Must be called LAST since it depends on the hypernodes & supernodes that have just been added
/// in order to resolve the superparents
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::CopyNewNodes(
  HierarchicalContourTree& hierarchicalTree)
{ // CopyNewNodes
  // TODO
  (void)hierarchicalTree;
} // CopyNewNodes


/// Transfers the details of nodes used in each iteration
template <typename MeshType, typename MeshBoundaryExecObjType>
void BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::
  CopyIterationDetails(HierarchicalContourTree& hierarchicalTree, vtkm::Id theRound)
{ //
  // TODO
  (void)theRound;
  (void)hierarchicalTree;
} //


/// prints the contents of the restrictor object in a standard format
template <typename MeshType, typename MeshBoundaryExecObjType>
std::string
BoundaryRestrictedAugmentedContourTreeMaker<MeshType, MeshBoundaryExecObjType>::DebugPrint(
  const char* message,
  const char* fileName,
  long lineNum) const
{ // DebugPrint
  // TODO
  std::stringstream resultStream;
  resultStream << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
               << lineNum << " ";
  resultStream << std::left << std::string(message) << std::endl;

  resultStream << "------------------------------------------------------" << std::endl;
  resultStream << "BRACT Contains:                                       " << std::endl;
  resultStream << "------------------------------------------------------" << std::endl;
  vtkm::worklet::contourtree_augmented::PrintHeader(this->Bract->VertexIndex.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "BRACT Vertices", this->Bract->VertexIndex, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "BRACT Superarcs", this->Bract->Superarcs, -1, resultStream);
  resultStream << "------------------------------------------------------" << std::endl;
  resultStream << "BRACT Maker Contains:                                 " << std::endl;
  resultStream << "------------------------------------------------------" << std::endl;
  resultStream << "nBoundary:  " << this->NumBoundary << std::endl;
  resultStream << "nNecessary: " << this->NumNecessary << std::endl;

  // Regular Vertex Arrays
  vtkm::worklet::contourtree_augmented::PrintHeader(this->BoundaryTreeId.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "ID in Boundary Tree", this->BoundaryTreeId, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "ID in Hierarchical Tree", this->HierarchicalTreeId, -1, resultStream);
  resultStream << std::endl;

  // Boundary Vertex Arrays
  vtkm::worklet::contourtree_augmented::PrintHeader(this->BoundaryIndices.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Boundary Sort Indices", this->BoundaryIndices, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Boundary Vertex Superset", this->BractVertexSuperset, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Boundary Superparents", this->BoundarySuperparents, -1, resultStream);
  resultStream << std::endl;

  // Per Supernode Arrays
  vtkm::worklet::contourtree_augmented::PrintHeader(
    this->SupernodeTransferBoundaryCount.GetNumberOfValues(), resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Supernode Transfer Count", this->SupernodeTransferBoundaryCount, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Superarc Intrinsic Count", this->SuperarcIntrinsicBoundaryCount, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Superarc Dependent Count", this->SuperarcDependentBoundaryCount, -1, resultStream);
  // Print IsNecessary as bool
  vtkm::worklet::contourtree_augmented::PrintValues(
    "isNecessary", this->IsNecessary, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Tree To Superset", this->TreeToSuperset, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Hierarchical Regular ID", this->HierarchicalRegularId, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Hierarchical Superparent", this->HierarchicalSuperparent, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Hierarchical Super ID", this->HierarchicalSuperId, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Hierarchical Hyperparent", this->HierarchicalHyperparent, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Hierarchical Hyper ID", this->HierarchicalHyperId, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Hierarchical Hyperarc", this->hierarchicalHyperarc, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "When Transferred", this->WhenTransferred, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Supernode Type", this->SupernodeType, -1, resultStream);
  resultStream << std::endl;

  // Per Hypernode Arrays
  vtkm::worklet::contourtree_augmented::PrintHeader(
    this->HyperarcDependentBoundaryCount.GetNumberOfValues(), resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Hyperarc Dependent Count", this->HyperarcDependentBoundaryCount, -1, resultStream);
  resultStream << std::endl;

  // BRACT sized Arrays
  vtkm::worklet::contourtree_augmented::PrintHeader(this->NewVertexId.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "New Vertex ID", this->NewVertexId, -1, resultStream);

  // arrays with double use & different sizes
  vtkm::worklet::contourtree_augmented::PrintHeader(this->pNeighbour.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Up Neighbour", this->UpNeighbour, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Down Neighbour", this->DownNeighbour, -1, resultStream);

  // Active Supernode Arrays
  vtkm::worklet::contourtree_augmented::PrintHeader(this->ActiveSuperarcs.GetNumberOfValues(),
                                                    resultStream);
  resultStream << "Active Superarcs" << std::endl;
  vtkm::worklet::contourtree_augmented::PrintEdgePairArray(this->ActiveSuperarcs, resultStream);

  // Arrays for transfer to hierarchical tree
  vtkm::worklet::contourtree_augmented::PrintHeader(this->NewHypernodes.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "New Hypernodes", this->NewHypernodes, -1, resultStream);

  vtkm::worklet::contourtree_augmented::PrintHeader(this->NewSupernodes.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "New Supernodes", this->NewSupernodes, -1, resultStream);

  vtkm::worklet::contourtree_augmented::PrintHeader(this->NewNodes.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices("New Nodes", this->NewNodes, -1, resultStream);

  resultStream << "------------------------------------------------------" << std::endl;
  resultStream << std::endl;
  resultStream << std::flush;
  return resultStream.str();
} // DebugPrint


} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
