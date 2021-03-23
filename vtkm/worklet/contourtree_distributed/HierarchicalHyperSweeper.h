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
//	Parallel Peak Pruning v. 2.0
//
//	Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// HierarchicalHyperSweeper.h
//
//=======================================================================================
//
// COMMENTS:
//
//	This class encapsulates a hypersweep over the hierarchical contour tree.  It is a
//	separate class primarily to keep the post-processing separate from the main tree
//	construction, but it should also make it easier to generalise to arbitrary computations
//
//	Basically, the way that this operates is:
//	1.	First, we do a local (standard) hypersweep over the hierarchical tree
//	2.	We then fan-in one round at a time.  In each round,
//		a.	We trade the prefix of the array with our logical partner, then
//		b.	Combine the array prefix with our own
//
//	Tactically, when we do MPI, we can either embed it in this unit, or leave it in the
//	calling unit.  For ease of porting, we will leave all MPI in the calling unit, so
//	this unit only needs to do the combination.
//
//	Note that we could define an operator to be passed in, and probably want to template it
//	that way in the future, but for now, we'll do the first version directly with addition
//
//	By assumption, we need a commutative property, since we do not guarantee that we have
//	strict ordering along superarcs (which would require sharing a supernode sort with our
//	partner, if not globally)
//
//=======================================================================================

#ifndef vtk_m_worklet_contourtree_distributed_hierarchical_hyper_sweeper_h
#define vtk_m_worklet_contourtree_distributed_hierarchical_hyper_sweeper_h

#include <iomanip>
#include <string>
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/data_set_mesh/IdRelabeler.h>
#include <vtkm/worklet/contourtree_distributed/HierarchicalContourTree.h>
#include <vtkm/worklet/contourtree_distributed/PrintGraph.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_hyper_sweeper/ComputeSuperarcDependentWeightsWorklet.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_hyper_sweeper/ComputeSuperarcTransferWeightsWorklet.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_hyper_sweeper/InitializeIntrinsicVertexCountComputeSuperparentIdsWorklet.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_hyper_sweeper/InitializeIntrinsicVertexCountInitalizeCountsWorklet.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_hyper_sweeper/InitializeIntrinsicVertexCountSubtractLowEndWorklet.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_hyper_sweeper/TransferTargetComperator.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_hyper_sweeper/TransferWeightsUpdateLHEWorklet.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_hyper_sweeper/TransferWeightsUpdateRHEWorklet.h>


namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{

// the class itself
template <typename MeshType, typename FieldType>
class HierarchicalHyperSweeper
{ // class HierarchicalHyperSweeper
public:
  // the tree that it hypersweeps over
  const HierarchicalContourTree<FieldType>& HierarchicalTree;

  // the underlying mesh base block type
  const MeshType& BaseBlock;

  // the Id of the base block (used for debug output)
  vtkm::Id BlockId;

  // array of values being operated over (same size as supernode set)
  // keep both intrinsic & dependent values
  // the intrinsic values are just stored but not modifid here
  const vtkm::cont::ArrayHandle<FieldType>& IntrinsicValues;
  // the dependent values are what is being sweeped and are updated here
  const vtkm::cont::ArrayHandle<FieldType>& DependentValues;
  // and to avoid an extra log summation, store the number of logical nodes for the underlying block
  // (computed when initializing the regular vertex list)
  vtkm::Id NumOwnedRegularVertices;


  // these are working arrays, lifted up here for ease of debug code
  // Subranges of these arrays will be reused in the rounds / iterations rather than being reallocated
  // an array for temporary storage of the prefix sums
  vtkm::cont::ArrayHandle<FieldType> ValuePrefixSum;
  // two arrays for collecting targets of transfers
  vtkm::worklet::contourtree_augmented::IdArrayType TransferTarget;
  vtkm::worklet::contourtree_augmented::IdArrayType SortedTransferTarget;
  // an array for indirect sorting of sets of superarcs
  vtkm::worklet::contourtree_augmented::IdArrayType SuperSortPermute;

  /// Constructor
  /// @param[in] blockId  The Id of the base block (used for debug output)
  /// @param[in] hierarchicalTree the tree that to hypersweeps over
  /// @param[in] baseBlock the underlying mesh base block type
  /// @param[in] intrinsicValues array of values of intrinisic nodes are just being stored here but not modified
  /// @param[in] dependentValues array of values being operated over (same size as supernode set)
  HierarchicalHyperSweeper<MeshType, FieldType>(
    vtkm::Id blockId,
    const HierarchicalContourTree<FieldType>& hierarchicalTree,
    const MeshType& baseBlock,
    const vtkm::cont::ArrayHandle<FieldType>& intrinsicValues,
    const vtkm::cont::ArrayHandle<FieldType>& dependentValues);

  /// Our routines to initialize the sweep need to be static (or externa)l if we are going to use the constructor
  /// to run the actual hypersweep
  /// @param[in] hierarchicalTree the tree that to hypersweeps over
  /// @param[in] baseBlock the underlying mesh base block  to initialize from
  /// @param[in] localToGlobalIdRelabeler Id relabeler used to compute global indices from local mesh indices
  /// @param[out] superarcRegularCounts   arrray for the output superarc regular counts
  static void InitializeIntrinsicVertexCount(
    const HierarchicalContourTree<FieldType>& hierarchicalTree,
    const MeshType& baseBlock,
    const vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler* localToGlobalIdRelabeler,
    vtkm::worklet::contourtree_augmented::IdArrayType& superarcRegularCounts);

  /// routine to do the local hypersweep using addition / subtraction
  /// The funtion use the ComputeSuperarcDependentWeights, ComputeSuperarcTransferWeights,
  /// and TransferWeights functions to carry out the local hyper sweep
  void LocalHyperSweep();

  /// Debug routine to print contents of the HiearchicalHyperSweep
  /// @param[in] message Message to print along the debug output
  /// @param[in] fileName Name of the file the message is printed from. Usually set to __FILE__
  /// @param[in] lineNum Line number in the file where the message is printed from. Usually set to __LINE__
  std::string DebugPrint(std::string message, const char* fileName, long lineNum) const;

  /// Routine to save the HierarchicalContourTree of this HierarchicalHyperSweeper to a Dot file
  /// @param[in] message Message included in the file
  /// @param[in] outFileName The name of the file to write the
  void SaveHierarchicalContourTreeDot(std::string message, const char* outFileName) const;

protected:
  // Functions used internally be LocalHyperSweep to compute the local hyper sweep

  /// Routine to compute the correct weights dependent on each superarc in a subrange (defined by the round & iteration)
  void ComputeSuperarcDependentWeights(vtkm::Id round,
                                       vtkm::Id iteration,
                                       vtkm::Id firstSupernode,
                                       vtkm::Id lastSupernode);

  /// routine to compute the weights to transfer to superarcs (defined by the round & iteration)
  void ComputeSuperarcTransferWeights(vtkm::Id round,
                                      vtkm::Id iteration,
                                      vtkm::Id firstSupernode,
                                      vtkm::Id lastSupernode);

  /// routine to transfer the weights
  void TransferWeights(vtkm::Id round,
                       vtkm::Id iteration,
                       vtkm::Id firstSupernode,
                       vtkm::Id lastSupernode);

private:
  /// Used internally to Invoke worklets
  vtkm::cont::Invoker Invoke;

}; // class HierarchicalHyperSweeper


template <typename MeshType, typename FieldType>
HierarchicalHyperSweeper<MeshType, FieldType>::HierarchicalHyperSweeper(
  vtkm::Id blockId,
  const HierarchicalContourTree<FieldType>& hierarchicalTree,
  const MeshType& baseBlock,
  const vtkm::cont::ArrayHandle<FieldType>& intrinsicValues,
  const vtkm::cont::ArrayHandle<FieldType>& dependentValues)
  : HierarchicalTree(hierarchicalTree)
  , BaseBlock(baseBlock)
  , BlockId(blockId)
  , IntrinsicValues(intrinsicValues)
  , DependentValues(dependentValues)
  , NumOwnedRegularVertices(static_cast<vtkm::Id>(0))
{ // constructor
  // Initalize arrays with 0s
  vtkm::cont::ArrayHandleConstant<vtkm::Id> tempZeroArray(
    0, this->HierarchicalTree.Supernodes.GetNumberOfValues());
  vtkm::cont::Algorithm::Copy(tempZeroArray, this->ValuePrefixSum);
  vtkm::cont::Algorithm::Copy(tempZeroArray, this->TransferTarget);
  vtkm::cont::Algorithm::Copy(tempZeroArray, this->SortedTransferTarget);
  // initialise the supersortPermute to the identity
  vtkm::cont::ArrayHandleIndex tempIndexArray(
    this->HierarchicalTree.Supernodes.GetNumberOfValues());
  vtkm::cont::Algorithm::Copy(tempIndexArray, this->SuperSortPermute);
} // constructor


// static function used to compute the initial superarc regular counts
template <typename MeshType, typename FieldType>
void HierarchicalHyperSweeper<MeshType, FieldType>::InitializeIntrinsicVertexCount(
  const HierarchicalContourTree<FieldType>& hierarchicalTree,
  const MeshType& baseBlock,
  const vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler* localToGlobalIdRelabeler,
  vtkm::worklet::contourtree_augmented::IdArrayType& superarcRegularCounts)
{ // InitializeIntrinsicVertexCount()
  vtkm::cont::Invoker
    localInvoke; // Needed because this a static function so we can't use the invoke from the object
  // I.  Call the mesh to get a list of all regular vertices belonging to the block by global Id
  vtkm::worklet::contourtree_augmented::IdArrayType globalIds;
  baseBlock.GetOwnedVerticesByGlobalId(localToGlobalIdRelabeler, globalIds);
  // and store the size for later reference
  hierarchicalTree.NumOwnedRegularVertices = globalIds.GetNumberOfValues();

#ifdef DEBUG_PRINT
  {
    std::stringstream debugStream;
    debugStream << std::endl << "Owned Regular Vertex List";
    vtkm::worklet::contourtree_augmented::PrintHeader(globalIds.GetNumberOfValues(), debugStream);
    vtkm::worklet::contourtree_augmented::PrintIndices("GlobalId", globalIds, debugStream);
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, debugStream.str());
  }
#endif

  // II.  Look up the global Ids in the hierarchical tree & convert to superparent Ids
  vtkm::worklet::contourtree_augmented::IdArrayType superparents;
  { // scope to make sure temporary variables are deleted
    auto findRegularByGlobal = hierarchicalTree.GetFindRegularByGlobal();
    auto computeSuperparentIdsWorklet = vtkm::worklet::contourtree_distributed::
      hierarchical_hyper_sweeper::InitializeIntrinsicVertexCountComputeSuperparentIdsWorklet();
    localInvoke(computeSuperparentIdsWorklet,       // worklet to run
                globalIds,                          // input
                findRegularByGlobal,                // input
                hierarchicalTree.Regular2Supernode, // input
                hierarchicalTree.Superparents,      // input
                superparents                        // output
    );
  }

#ifdef DEBUG_PRINT
  {
    std::stringstream debugStream;
    vtkm::worklet::contourtree_augmented::PrintIndices("Superparents", superparents, debugStream);
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, debugStream.str());
  }
#endif

  // III.  Sort the superparent Ids & count the copies of each
  vtkm::cont::Algorithm ::Sort(superparents);

#ifdef DEBUG_PRINT
  {
    std::stringstream debugStream;
    vtkm::worklet::contourtree_augmented::PrintIndices("Sorted SP", superparents, debugStream);
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, debugStream.str());
  }
#endif

  // initialize the counts to zero.
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleConstant<vtkm::Id>(static_cast<vtkm::Id>(0),
                                              hierarchicalTree.Supernodes.GetNumberOfValues()),
    superarcRegularCounts);
  { // scope to make sure temporary variables are deleted
    vtkm::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      InitializeIntrinsicVertexCountInitalizeCountsWorklet initalizeCountsWorklet;
    // set the count to the Id one off the high end of each range
    localInvoke(initalizeCountsWorklet, // worklet
                superparents,           // input domain
                superarcRegularCounts   // output
    );
  }

  // now repeat to subtract out the low end
  {
    vtkm::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      InitializeIntrinsicVertexCountSubtractLowEndWorklet subtractLowEndWorklet;
    localInvoke(subtractLowEndWorklet, // worklet
                superparents,          // input domain
                superarcRegularCounts  // output
    );
  }
  // and that is that
#ifdef DEBUG_PRINT
  {
    std::stringstream debugStream;
    vtkm::worklet::contourtree_augmented::PrintIndices(
      "SuperarcRegularCounts", superarcRegularCounts, debugStream);
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, debugStream.str());
  }
#endif
} // InitializeIntrinsicVertexCount()


// routine to do the local hypersweep using addition / subtraction
template <typename MeshType, typename FieldType>
void HierarchicalHyperSweeper<MeshType, FieldType>::LocalHyperSweep()
{ // LocalHyperSweep()
// TODO: Implement this function
#ifdef DEBUG_PRINT
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             DebugPrint(std::string("Hypersweep Block ") + std::to_string(blockId) +
                          std::string(" Starting Local HyperSweep"),
                        __FILE__,
                        __LINE__));
#endif

  // I.  Iterate over all rounds of the hyperstructure
  for (vtkm::Id round = 0; round <= this->hierarchicalTree.NumRounds; round++)
  { // per round
#ifdef DEBUG_PRINT
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               DebugPrint(std::string("Hypersweep Block ") + std::to_string(blockId) +
                            std::string(" Round ") + std::to_string(round) +
                            std::string(" Step 0 Starting Round"),
                          __FILE__,
                          __LINE__));
#endif
    //  A.  Iterate over all iterations of the round
    for (vtkm::Id iteration = 0; iteration < this->HierarchicalTree.NumIterations[round];
         iteration++)
    { // per iteration
#ifdef DEBUG_PRINT
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 DebugPrint(std::string("Hypersweep Block ") + std::to_string(blockId) +
                              std::string(" Round ") + std::to_string(round) +
                              std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                              std::string(" Step A Starting Iteration"),
                            __FILE__,
                            __LINE__));
#endif
      //  1.  Establish the range of supernode Ids that we want to process
      vtkm::Id firstSupernode = this->HierarchicalTree.FirstSupernodePerIteration[round][iteration];
      vtkm::Id lastSupernode =
        this->HierarchicalTree.FirstSupernodePerIteration[round][iteration + 1];

      // call the routine that computes the dependent weights for each superarc in that range
      this->ComputeSuperarcDependentWeights(round, iteration, firstSupernode, lastSupernode);

#ifdef DEBUG_PRINT
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 DebugPrint(std::string("Hypersweep Block ") + std::to_string(blockId) +
                              std::string(" Round ") + std::to_string(round) +
                              std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                              std::string(" Step B Dependent Weights Computed"),
                            __FILE__,
                            __LINE__));
#endif
      // now call the routine that computes the weights to be transferred and the superarcs to which they transfer
      this->ComputeSuperarcTransferWeights(round, iteration, firstSupernode, lastSupernode);

#ifdef DEBUG_PRINT
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 DebugPrint(std::string("Hypersweep Block ") + std::to_string(blockId) +
                              std::string(" Round ") + std::to_string(round) +
                              std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                              std::string(" Step C Transfer Weights Computed"),
                            __FILE__,
                            __LINE__));
#endif

      // transfer the weights
      this->TransferWeights(round, iteration, firstSupernode, lastSupernode);

#ifdef DEBUG_PRINT
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 DebugPrint(std::string("Hypersweep Block ") + std::to_string(blockId) +
                              std::string(" Round ") + std::to_string(round) +
                              std::string(" Step 1 Iteration ") + std::to_string(iteration) +
                              std::string(" Step D Weights Transferred"),
                            __FILE__,
                            __LINE__));
#endif
    } // per iteration

#ifdef DEBUG_PRINT
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               DebugPrint(std::string("Hypersweep Block ") + std::to_string(blockId) +
                            std::string(" Round ") + std::to_string(round) +
                            std::string(" Step 2 Ending Round"),
                          __FILE__,
                          __LINE__));
#endif
  } // per round
} // LocalHyperSweep()


// routine to compute the correct weights dependent on each superarc in a subrange (defined by the round & iteration)
template <typename MeshType, typename FieldType>
void HierarchicalHyperSweeper<MeshType, FieldType>::ComputeSuperarcDependentWeights(
  vtkm::Id round,
  vtkm::Id iteration,
  vtkm::Id firstSupernode,
  vtkm::Id lastSupernode)
{ // ComputeSuperarcDependentWeights()
  (void)
    iteration; // avoid compiler warning for unused parmeter. Kept parameter in case we need it for debugging.

  //  2.  Use sorted prefix sum to compute the total weight to contribute to the super/hypertarget
  // Same as std::partial_sum(sweepValues.begin() + firstSupernode, sweepValues.begin() + lastSupernode, valuePrefixSum.begin() + firstSupernode);
  {
    vtkm::Id numValuesToCopy = lastSupernode - firstSupernode;
    // DependentValues[firstSuperNode, lastSupernode)
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      dependentValuesView(this->DependentValues, // subset DependentValues
                          firstSupernode,        // start at firstSupernode
                          numValuesToCopy);      // until lastSuperNode (not inclued)
    // Target array
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      valuePrefixSumView(this->ValuePrefixSum, // subset ValuePrefixSum
                         firstSupernode,       // start at firstSupernode
                         numValuesToCopy);     // until lastSuperNode (not inclued)
    // Compute the partial sum for DependentValues[firstSuperNode, lastSupernode) and write to ValuePrefixSum[firstSuperNode, lastSupernode)
    vtkm::cont::Algorithm::ScanInclusive(dependentValuesView, // input
                                         valuePrefixSumView); // result of partial sum
  }

  // Since the prefix sum is over *all* supernodes in the iteration, we need to break it into segments
  // There are two cases we have to worry about:
  // a.  Hyperarcs made up of multiple supernodes
  // b.  Attachment points (which don't have a corresponding hyperarc)
  // and they can be mixed in any given iteration

  // Since we have the prefix sum in a separate array, we avoid read/write conflicts

  // 3.  Compute the segmented weights from the prefix sum array
  {
    // Create views of the subranges of the arrays we need to update
    vtkm::Id numValuesToProcess = lastSupernode - firstSupernode;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> supernodeIndex(
      firstSupernode, static_cast<vtkm::Id>(1), numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      hierarchicalTreeSuperarcsView(
        this->HierarchicalTree.Superarcs, firstSupernode, numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      hierarchicalTreeHyperparentsView(
        this->HierarchicalTree.Hyperparents, firstSupernode, numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      hierarchicalTreeHypernodesView(
        this->HierarchicalTree.Hypernodes, firstSupernode, numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      dependentValuesView(this->DependentValues, firstSupernode, numValuesToProcess);
    // create the worklet
    vtkm::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      ComputeSuperarcDependentWeightsWorklet<FieldType>
        computeSuperarcDependentWeightsWorklet(
          firstSupernode, round, this->HierarchicalTree.NumRounds);
    // Execute the worklet
    this->Invoke(
      computeSuperarcDependentWeightsWorklet, // the worklet
      supernodeIndex,                // input counting index [firstSupernode, lastSupernode)
      hierarchicalTreeSuperarcsView, // input view of  hierarchicalTree.Superarcs[firstSupernode, lastSupernode)
      hierarchicalTreeHyperparentsView, // input view of  hierarchicalTree.Hyperparents[firstSupernode, lastSupernode)
      this->hierarchicalTree.Hypernodes, // input full hierarchicalTree.Hypernodes array
      this->ValuePrefixSum,              // input full ValuePrefixSum array
      dependentValuesView                // output view of sweepValues[firstSu
    );
  }
} // ComputeSuperarcDependentWeights()


// routine to compute the weights to transfer to superarcs (defined by the round & iteration)
template <typename MeshType, typename FieldType>
void HierarchicalHyperSweeper<MeshType, FieldType>::ComputeSuperarcTransferWeights(
  vtkm::Id round,
  vtkm::Id iteration,
  vtkm::Id firstSupernode,
  vtkm::Id lastSupernode)
{ // ComputeSuperarcTransferWeights()
  (void)
    iteration; // avoid compiler warning for unused parmeter. Kept parameter in case we need it for debugging.

  // At this stage, we would otherwise transfer weights by hyperarc, but attachment points don't *have* hyperarcs
  // so we will do a transfer by superarc instead, making sure that we only transfer from the last superarc in each
  // hyperarc, plus for any attachment point

  // 4.  Set the amount each superarc wants to transfer, reusing the valuePrefixSum array for the purpose
  //    and the transfer target
  { // scope ComputeSuperarcTransferWeightsWorklet to make sure temp variables are cleared
    // Create ArrayHandleViews of the subrange of values that we need to update
    vtkm::Id numValuesToProcess = lastSupernode - firstSupernode;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> supernodeIndex(
      firstSupernode, static_cast<vtkm::Id>(1), numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      hierarchicalTreeSupernodesView(
        this->HierarchicalTree.Supernodes, firstSupernode, numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      hierarchicalTreeSuperarcsView(
        this->HierarchicalTree.Superarcs, firstSupernode, numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      transferTargetView(this->transferTarget, firstSupernode, numValuesToProcess);
    // instantiate the worklet
    vtkm::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      ComputeSuperarcTransferWeightsWorklet computeSuperarcTransferWeightsWorklet(
        round, this->HierarchicalTree.NumRounds, lastSupernode);
    // call the worklet
    this->Invoke(
      computeSuperarcTransferWeightsWorklet, // worklet
      supernodeIndex,                        // input counting array [firstSupernode, lastSupernode)
      hierarchicalTreeSupernodesView, // input view of hierarchicalTree.supernodes[firstSupernode, lastSupernode)
      this->HierarchicalTree.Superparents, // input whole array of hierarchicalTree.superparents
      this->HierarchicalTree.Hyperparents, // input whole array of hierarchicalTree.hyperparents
      hierarchicalTreeSuperarcsView, // input/output view of hierarchicalTree.superarcs[firstSupernode, lastSupernode)
      transferTargetView // input view of transferTarget[firstSupernode, lastSupernode)
    );
  } // scope ComputeSuperarcTransferWeightsWorklet

  // 5. Now we need to sort the transfer targets into contiguous segments
  {
    // create view of superSortPermute[firstSupernode, lastSupernode) for sorting
    vtkm::Id numValuesToProcess = lastSupernode - firstSupernode;
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      superSortPermuteView(this->SuperSortPermute, firstSupernode, numValuesToProcess);
    // create comperator for the sort
    vtkm::worklet::contourtree_distributed::hierarchical_hyper_sweeper::TransferTargetComperator
      transferTargetComperator(this->transferTarget);
    // sort the subrange of our array
    vtkm::cont::Algorithm::Sort(superSortPermuteView, transferTargetComperator);
  }

  // 6. The [first,last] subrange is now permuted, so we can copy the transfer targets and weights into arrays
  //    The following code block implements the following for loop using fancy array handles and copy
  //    for (vtkm::Id supernode = firstSupernode; supernode < lastSupernode; supernode++)
  //    {
  //       sortedTransferTarget[supernode] = transferTarget[superSortPermute[supernode]];
  //       valuePrefixSum[supernode] = sweepValues[superSortPermute[supernode]];
  //    }
  {
    // copy transfer target in the sorted order
    vtkm::Id numValuesToProcess = lastSupernode - firstSupernode;
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      sortedTransferTargetView(this->SortedTransferTarget, firstSupernode, numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      superSortPermuteView(this->SuperSortPermute, firstSupernode, numValuesToProcess);
    auto permutedTransferTarget =
      vtkm::cont::make_ArrayHandlePermutation(superSortPermuteView,  // idArray
                                              this->TransferTarget); // valueArray
    vtkm::cont::Algorithm::Copy(permutedTransferTarget, sortedTransferTargetView);
    // Note that any values associated with NO_SUCH_ELEMENT will be ignored
    // copy transfer weight in the sorted order
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      valuePrefixSumView(this->ValuePrefixSum, firstSupernode, numValuesToProcess);
    auto permutedDependentValues =
      vtkm::cont::make_ArrayHandlePermutation(superSortPermuteView,   // idArray
                                              this->DependentValues); // valueArray
    vtkm::cont::Algorithm::Copy(permutedDependentValues, valuePrefixSumView);
  }
} // ComputeSuperarcTransferWeights()


// routine to transfer the weights
template <typename MeshType, typename FieldType>
void HierarchicalHyperSweeper<MeshType, FieldType>::TransferWeights(vtkm::Id round,
                                                                    vtkm::Id iteration,
                                                                    vtkm::Id firstSupernode,
                                                                    vtkm::Id lastSupernode)
{ // TransferWeights()
  // avoid compiler warning for unused parmeters. Kept parameters in case we need it for debugging.
  (void)round;
  (void)iteration;

  // 7. Now perform a segmented prefix sum
  // Same as std::partial_sum(valuePrefixSum.begin() + firstSupernode, valuePrefixSum.begin() + lastSupernode, valuePrefixSum.begin() + firstSupernode);
  {
    vtkm::Id numValuesToCopy = lastSupernode - firstSupernode;
    // ValuePrefixSum[firstSuperNode, lastSupernode)
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      valuePrefixSumView(this->ValuePrefixSum, // subset ValuePrefixSum
                         firstSupernode,       // start at firstSupernode
                         numValuesToCopy);     // until lastSuperNode (not inclued)
    // TODO: If it is safe to use the same array as input and output for ScanInclusive then this code should be updated to avoid the extra copy
    // In this case our traget array is the same as our source array. For safety we
    // store the values of our prefix sum in a temporary arrya and then copy the values
    // back into our valuePrefixSumView at the end
    vtkm::cont::ArrayHandle<FieldType> tempScanInclusiveTarget;
    tempScanInclusiveTarget.Allocate(numValuesToCopy);
    // Compute the partial sum for DependentValues[firstSuperNode, lastSupernode) and write to ValuePrefixSum[firstSuperNode, lastSupernode)
    vtkm::cont::Algorithm::ScanInclusive(valuePrefixSumView,       // input
                                         tempScanInclusiveTarget); // result of partial sum
    // Now copy the values from our prefix sum back
    vtkm::cont::Algorithm::Copy(tempScanInclusiveTarget, valuePrefixSumView);
  }

  // 7a. and 7b.
  {
    // Prepare the approbriate array views for our worklet. This is done to allow us to
    // use FieldIn instead of having to transfer the entire array to the device when we
    // really only need a subrange
    vtkm::Id numValuesToProcess = lastSupernode - firstSupernode;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> supernodeIndex(
      firstSupernode, static_cast<vtkm::Id>(1), numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      sortedTransferTargetView(this->SortedTransferTarget, firstSupernode, numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      sortedTransferTargetShiftedView(
        this->SortedTransferTarget, firstSupernode + 1, numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      valuePrefixSumView(this->ValuePrefixSum, firstSupernode, numValuesToProcess);
    vtkm::cont::ArrayHandleView<vtkm::worklet::contourtree_augmented::IdArrayType>
      valuePrefixSumShiftedView(this->ValuePrefixSum, firstSupernode - 1, numValuesToProcess);
    auto sweepValuePermuted =
      vtkm::cont::make_ArrayHandlePermutation(sortedTransferTargetView, // idArray
                                              this->DependentValues);   // valueArray

    // 7a. Find the RHE of each group and transfer the prefix sum weight
    // Note that we do not compute the transfer weight separately, we add it in place instead
    // Instantiate the worklet
    vtkm::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      TransferWeightsUpdateRHEWorklet transferWeightsUpdateRHEWorklet(lastSupernode);
    // Invoke the worklet
    this->Invoke(
      transferWeightsUpdateRHEWorklet, // worklet
      supernodeIndex,                  // input counting array [firstSupernode, lastSupernode)
      sortedTransferTargetView, // input view of sortedTransferTarget[firstSupernode, lastSupernode)
      sortedTransferTargetShiftedView, // input view of sortedTransferTarget[firstSupernode+1, lastSupernode+1)
      valuePrefixSumView, // input view of valuePrefixSum[firstSupernode, lastSupernode)
      sweepValuePermuted // output view of sweepValues permuted by sortedTransferTarget[firstSupernode, lastSupernode). Use FieldInOut since we don't overwrite all values.
    );

    // 7b. Now find the LHE of each group and subtract out the prior weight
    vtkm::worklet::contourtree_distributed::hierarchical_hyper_sweeper::
      TransferWeightsUpdateLHEWorklet transferWeightsUpdateLHEWorklet(firstSupernode);
    this->Invoke(
      transferWeightsUpdateLHEWorklet, // worklet
      supernodeIndex,                  // input counting array [firstSupernode, lastSupernode)
      sortedTransferTargetView, // input view of sortedTransferTarget[firstSupernode, lastSupernode)
      sortedTransferTargetShiftedView, // input view of sortedTransferTarget[firstSupernode+1, lastSupernode+1)
      valuePrefixSumShiftedView, // input view of valuePrefixSum[firstSupernode-1, lastSupernode-1)
      sweepValuePermuted // output view of sweepValues permuted by sortedTransferTarget[firstSupernode, lastSupernode). Use FieldInOut since we don't overwrite all values.
    );
  }
} // TransferWeights()


// debug routine
template <typename MeshType, typename FieldType>
std::string HierarchicalHyperSweeper<MeshType, FieldType>::DebugPrint(std::string message,
                                                                      const char* fileName,
                                                                      long lineNum) const
{ // DebugPrint()
  std::stringstream resultStream;
  resultStream << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
               << lineNum << std::endl;
  resultStream << std::left << message << std::endl;
  resultStream << "Hypersweep Value Array Contains:        " << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::endl;

  vtkm::worklet::contourtree_augmented::PrintHeader(this->DependentValues.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Intrinsic", this->IntrinsicValues, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Dependent", this->DependentValues, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Prefix Sum", this->ValuePrefixSum - 1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Transfer To", this->TransferTarget - 1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Sorted Transfer", this->SortedTransferTarget - 1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Sort Permute", this->SuperSortPermute - 1, resultStream);
  return resultStream.str();
} // DebugPrint()


// Routine to save the hierarchical tree to file
template <typename MeshType, typename FieldType>
void HierarchicalHyperSweeper<MeshType, FieldType>::SaveHierarchicalContourTreeDot(
  std::string message,
  const char* outFileName) const
{ // SaveHierarchicalContourTreeDot()
  std::string hierarchicalTreeDotString =
    HierarchicalContourTreeDotGraphPrint<vtkm::worklet::contourtree_augmented::IdArrayType>(
      message,
      this->HierarchicalTree,
      SHOW_SUPER_STRUCTURE | SHOW_HYPER_STRUCTURE | SHOW_ALL_IDS | SHOW_ALL_SUPERIDS |
        SHOW_ALL_HYPERIDS | SHOW_EXTRA_DATA, //|GV_NODE_NAME_USES_GLOBAL_ID
      this->BlockId,
      this->DependentValues);
  std::ofstream hierarchicalTreeFile(outFileName);
  hierarchicalTreeFile << hierarchicalTreeDotString;
} // SaveHierarchicalContourTreeDot


} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
