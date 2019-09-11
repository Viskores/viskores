//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

//=======================================================================================
//
// COMMENTS:
//
//  The old ChainGraph has been abstracted a little further - it still does the same job
//  of carrying most of the intermediate stages.  However, since the chain building is
//  also needed by the mesh to set up the initial graph input, it has been moved (for now
//  to Types.h)
//
//	There will be no explicit constructor - instead, it's the mesh's job to initialise
//	a valid object of this type
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_chaingraph_h
#define vtkm_worklet_contourtree_chaingraph_h

#include <vtkm/worklet/contourtree/ActiveEdgeTransferrer.h>
#include <vtkm/worklet/contourtree/ChainDoubler.h>
#include <vtkm/worklet/contourtree/EdgePeakComparator.h>
#include <vtkm/worklet/contourtree/GoverningSaddleFinder.h>
#include <vtkm/worklet/contourtree/JoinTreeTransferrer.h>
#include <vtkm/worklet/contourtree/PrintVectors.h>
#include <vtkm/worklet/contourtree/RegularPointTransferrer.h>
#include <vtkm/worklet/contourtree/SaddleAscentFunctor.h>
#include <vtkm/worklet/contourtree/SaddleAscentTransferrer.h>
#include <vtkm/worklet/contourtree/TrunkBuilder.h>
#include <vtkm/worklet/contourtree/VertexDegreeUpdater.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/worklet/DispatcherMapField.h>

//#define DEBUG_PRINT 1
//#define DEBUG_FUNCTION_ENTRY 1
//#define DEBUG_TIMING 1

namespace vtkm
{
namespace worklet
{
namespace contourtree
{

#define DEBUG_STRING_TRANSFER_GOVERNING_SADDLES "Extrema should now be assigned"
#define DEBUG_STRING_TRANSFER_SADDLE_STARTS "Transfer Saddle Starts "
#define DEBUG_STRING_TRANSFERRED_SADDLE_STARTS "Saddle Starts Transferred"
#define DEBUG_STRING_TRANSFER_TO_MERGE_TREE "Transfer to Merge Tree"
#define DEBUG_STRING_OUTDEGREE "Outdegree"
#define DEBUG_STRING_CHAINEXT "Chain Ext"
#define DEBUG_STRING_ACTIVE_OUTDEGREE "Active Outdegree"
#define DEBUG_STRING_ACTIVE_CHAINEXT "Active Chain Ext"
#define DEBUG_STRING_FAR_ID "Far"
#define DEBUG_STRING_FAR_INDEX "Far Index"
#define DEBUG_STRING_FAR_VALUE "Far Value"
#define DEBUG_STRING_NEAR_ID "Near"
#define DEBUG_STRING_NEAR_INDEX "Near Index"
#define DEBUG_STRING_NEAR_VALUE "Near Value"
#define DEBUG_STRING_EDGE_FAR_ID "Edge Far"
#define DEBUG_STRING_EDGE_NEAR_ID "Edge Near"
#define DEBUG_STRING_EDGE_NEAR_INDEX "Edge Near Index"
#define DEBUG_STRING_EDGE_NEAR_VALUE "Edge Near Value"
#define DEBUG_STRING_SORTED_NEAR_ID "Sorted Near"
#define DEBUG_STRING_SORTED_NEAR_INDEX "Sorted Near Index"
#define DEBUG_STRING_SORTED_NEAR_VALUE "Sorted Near Value"
#define DEBUG_STRING_SORTED_FAR_ID "Sorted Far"

template <typename T, typename StorageType>
class ChainGraph
{
public:
  // we will want a reference to the original data array
  const vtkm::cont::ArrayHandle<T, StorageType>& values;

  // we will also want a reference to the arc array where we write the output
  vtkm::cont::ArrayHandle<vtkm::Id>& arcArray;

  // for each vertex, we need to know where it is in the original data array
  vtkm::cont::ArrayHandle<vtkm::Id> valueIndex;

  // and we also need the orientation of the edges (i.e. is it join or split)
  bool isJoinGraph;

  // and we will store the number of iterations the computation took here
  vtkm::Id nIterations;

  // array recording pruning sequence
  // pseudo-extrema prune to pseudo-saddles
  // all others prune to pseudo-extrema
  vtkm::cont::ArrayHandle<vtkm::Id> prunesTo;

  // we also want to keep track of the first edge for each vertex
  vtkm::cont::ArrayHandle<vtkm::Id> firstEdge;

  // and the outdegree for each vertex
  vtkm::cont::ArrayHandle<vtkm::Id> outdegree;

  // finally, we need to keep track of the chain extremum for each vertex
  vtkm::cont::ArrayHandle<vtkm::Id> chainExtremum;

  // we will also need to keep track of both near and far ends of each edge
  vtkm::cont::ArrayHandle<vtkm::Id> edgeFar;
  vtkm::cont::ArrayHandle<vtkm::Id> edgeNear;

  // we will also keep track of the currently active set of vertices and edges
  vtkm::cont::ArrayHandle<vtkm::Id> activeVertices;
  vtkm::cont::ArrayHandle<vtkm::Id> activeEdges;

  // and an array for sorting edges
  vtkm::cont::ArrayHandle<vtkm::Id> edgeSorter;

  // constructor takes necessary references
  ChainGraph(const vtkm::cont::ArrayHandle<T, StorageType>& Values,
             vtkm::cont::ArrayHandle<vtkm::Id>& ArcArray,
             bool IsJoinGraph)
    : values(Values)
    , arcArray(ArcArray)
    , isJoinGraph(IsJoinGraph)
  {
  }

  // sets initial size of vertex arrays
  void AllocateVertexArrays(vtkm::Id Size);

  // sets initial size of edge arrays
  void AllocateEdgeArrays(vtkm::Id Size);

  // routine that builds the merge graph once the initial vertices & edges are set
  void Compute(vtkm::cont::ArrayHandle<vtkm::Id>& saddles);

  // sorts saddle starts to find governing saddles
  void FindGoverningSaddles();

  // marks now regular points for removal
  void TransferRegularPoints();

  // compacts the active vertex list
  void CompactActiveVertices();

  // compacts the active edge list
  void CompactActiveEdges();

  // builds the chains for the new active vertices
  void BuildChains();

  // suppresses non-saddles for the governing saddles pass
  void TransferSaddleStarts();

  // sets all remaining active vertices
  void BuildTrunk();

  // transfers partial results to merge tree array
  void TransferToMergeTree(vtkm::cont::ArrayHandle<vtkm::Id>& saddles);

  // prints the contents of the topology graph in a standard format
  void DebugPrint(const char* message);
}; // class ChainGraph

// sets initial size of vertex arrays
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::AllocateVertexArrays(vtkm::Id Size)
{
  valueIndex.Allocate(Size);
  prunesTo.Allocate(Size);
  firstEdge.Allocate(Size);
  outdegree.Allocate(Size);
  chainExtremum.Allocate(Size);
  activeVertices.Allocate(Size);
} // AllocateVertexArrays()

// sets initial size of edge arrays
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::AllocateEdgeArrays(vtkm::Id Size)
{
  edgeFar.Allocate(Size);
  edgeNear.Allocate(Size);
  activeEdges.Allocate(Size);
} // AllocateEdgeArrays()

// routine that builds the merge graph once the initial vertices & edges are set
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::Compute(vtkm::cont::ArrayHandle<vtkm::Id>& saddles)
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "===================" << std::endl;
  std::cout << "Compute Chain Graph" << std::endl;
  std::cout << "===================" << std::endl;
  std::cout << std::endl;
#endif

#ifdef DEBUG_PRINT
  DebugPrint("Chain Graph Computation Starting");
#endif

  // loop until we run out of active edges
  nIterations = 0;
  while (edgeSorter.GetNumberOfValues() > 0)
  {
    // find & label the extrema with their governing saddles
    FindGoverningSaddles();

    // label the regular points
    TransferRegularPoints();

    // compact the active set of vertices & edges
    CompactActiveVertices();
    CompactActiveEdges();

    // rebuild the chains
    BuildChains();

    // choose the subset of edges for the governing saddles
    TransferSaddleStarts();

    // increment the iteration count
    nIterations++;
  } // main loop

  // final pass to label the trunk vertices
  BuildTrunk();

  // we can now release many of the arrays to free up space
  firstEdge.ReleaseResources();
  outdegree.ReleaseResources();
  edgeNear.ReleaseResources();
  edgeFar.ReleaseResources();
  activeEdges.ReleaseResources();
  activeVertices.ReleaseResources();
  edgeSorter.ReleaseResources();

  // and transfer results to mergearcs
  TransferToMergeTree(saddles);

  // then release the remaining memory
  chainExtremum.ReleaseResources();
  prunesTo.ReleaseResources();

#ifdef DEBUG_PRINT
  DebugPrint("Chain Graph Computed");
#endif
} // Compute()

// sorts saddle ascents to find governing saddles
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::FindGoverningSaddles()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "======================" << std::endl;
  std::cout << "Find Governing Saddles" << std::endl;
  std::cout << "======================" << std::endl;
  std::cout << std::endl;
#endif

  // sort with the comparator
  vtkm::cont::Algorithm::Sort(edgeSorter,
                              EdgePeakComparator<T, StorageType>(
                                values, valueIndex, edgeFar, edgeNear, arcArray, isJoinGraph));

#ifdef DEBUG_PRINT
  DebugPrint("After Sorting");
#endif

  // now loop through the edges
  GoverningSaddleFinder governingSaddleFinder;
  vtkm::worklet::DispatcherMapField<GoverningSaddleFinder> governingSaddleFinderDispatcher(
    governingSaddleFinder);
  vtkm::Id nEdges = edgeSorter.GetNumberOfValues();
  vtkm::cont::ArrayHandleIndex edgeIndexArray(nEdges);

  governingSaddleFinderDispatcher.Invoke(edgeIndexArray, // input
                                         edgeSorter,     // input (whole array)
                                         edgeFar,        // input (whole array)
                                         edgeNear,       // input (whole array)
                                         prunesTo,       // output (whole array)
                                         outdegree);     // output (whole array)
#ifdef DEBUG_PRINT
  DebugPrint(DEBUG_STRING_TRANSFER_GOVERNING_SADDLES);
#endif
} // FindGoverningSaddles()

// marks now regular points for removal
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::TransferRegularPoints()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << "Transfer Regular Points" << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << std::endl;
#endif
  RegularPointTransferrer<T> regularPointTransferrer(isJoinGraph);
  vtkm::worklet::DispatcherMapField<RegularPointTransferrer<T>> regularPointTransferrerDispatcher(
    regularPointTransferrer);

  regularPointTransferrerDispatcher.Invoke(activeVertices, // input
                                           chainExtremum,  // input (whole array)
                                           values,         // input (whole array)
                                           valueIndex,     // input (whole array)
                                           prunesTo,       // i/o (whole array)
                                           outdegree);     // output (whole array)
#ifdef DEBUG_PRINT
  DebugPrint("Regular Points Should Now Be Labelled");
#endif
} // TransferRegularPoints()

// compacts the active vertex list
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::CompactActiveVertices()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << "Compact Active Vertices" << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << std::endl;
#endif
  using IdArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using PermuteIndexType = vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  // create a temporary array the same size
  vtkm::cont::ArrayHandle<vtkm::Id> newActiveVertices;

  // Use only the current activeVertices outdegree to match size on CopyIf
  vtkm::cont::ArrayHandle<vtkm::Id> outdegreeLookup;
  vtkm::cont::ArrayCopy(PermuteIndexType(activeVertices, outdegree), outdegreeLookup);

  // compact the activeVertices array to keep only the ones of interest
  vtkm::cont::Algorithm::CopyIf(activeVertices, outdegreeLookup, newActiveVertices);

  activeVertices.ReleaseResources();
  vtkm::cont::Algorithm::Copy(newActiveVertices, activeVertices);

#ifdef DEBUG_PRINT
  DebugPrint("Active Vertex List Compacted");
#endif
} // CompactActiveVertices()

// compacts the active edge list
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::CompactActiveEdges()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "====================" << std::endl;
  std::cout << "Compact Active Edges" << std::endl;
  std::cout << "====================" << std::endl;
  std::cout << std::endl;
#endif
  // grab the size of the array for easier reference
  vtkm::Id nActiveVertices = activeVertices.GetNumberOfValues();

  // first, we have to work out the first edge for each active vertex
  // we start with a temporary new updegree
  vtkm::cont::ArrayHandle<vtkm::Id> newOutdegree;
  newOutdegree.Allocate(nActiveVertices);

  // do a parallel computation using the vertex degree updater
  // WARNING: Using chainMaximum for I/O in parallel loop
  // See functor description for algorithmic justification of safety
  VertexDegreeUpdater vertexDegreeUpdater;
  vtkm::worklet::DispatcherMapField<VertexDegreeUpdater> vertexDegreeUpdaterDispatcher(
    vertexDegreeUpdater);

  vertexDegreeUpdaterDispatcher.Invoke(activeVertices, // input
                                       activeEdges,    // input (whole array)
                                       edgeFar,        // input (whole array)
                                       firstEdge,      // input (whole array)
                                       prunesTo,       // input (whole array)
                                       outdegree,      // input (whole array)
                                       chainExtremum,  // i/o (whole array)
                                       newOutdegree);  // output

  // now we do a reduction to compute the offsets of each vertex
  vtkm::cont::ArrayHandle<vtkm::Id> newPosition;
  vtkm::cont::Algorithm::ScanExclusive(newOutdegree, newPosition);
  vtkm::Id nNewEdges = vtkm::cont::ArrayGetValue(nActiveVertices - 1, newPosition) +
    vtkm::cont::ArrayGetValue(nActiveVertices - 1, newOutdegree);

  // create a temporary vector for copying
  vtkm::cont::ArrayHandle<vtkm::Id> newActiveEdges;
  newActiveEdges.Allocate(nNewEdges);

  // now copy the relevant edges into the active edge array
  // WARNING: Using chainMaximum, edgeHigh, firstEdge, updegree for I/O in parallel loop
  // See functor description for algorithmic justification of safety
  vtkm::worklet::DispatcherMapField<ActiveEdgeTransferrer>().Invoke(
    activeVertices,  // input
    newPosition,     // input
    newOutdegree,    // input
    activeEdges,     // input (whole array)
    prunesTo,        // input (whole array)
    firstEdge,       // i/o (whole array)
    outdegree,       // i/o (whole array)
    chainExtremum,   // i/o (whole array)
    edgeFar,         // i/o (whole array)
    newActiveEdges); // output (whole array)

  // resize the original array and recopy
  vtkm::cont::ArrayCopy(newActiveEdges, activeEdges);

#ifdef DEBUG_PRINT
  DebugPrint("Active Edges Now Compacted");
#endif
} // CompactActiveEdges()

// builds the chains for the new active vertices
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::BuildChains()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "============" << std::endl;
  std::cout << "Build Chains" << std::endl;
  std::cout << "============" << std::endl;
  std::cout << std::endl;
#endif
  // a temporary array the full size of the graph
  vtkm::cont::ArrayHandle<vtkm::Id> tempChainExtremum;
  tempChainExtremum.Allocate(edgeNear.GetNumberOfValues());

  // compute the number of log steps required in this pass
  vtkm::Id nActiveVertices = activeVertices.GetNumberOfValues();
  vtkm::Id nLogSteps = 1;
  for (vtkm::Id shifter = nActiveVertices; shifter != 0; shifter >>= 1)
    nLogSteps++;

  ChainDoubler chainDoubler;
  vtkm::worklet::DispatcherMapField<ChainDoubler> chainDoublerDispatcher(chainDoubler);

  // 2.	Use path compression / step doubling to collect vertices along ascending chains
  //		until every vertex has been assigned to *an* extremum
  //		Step two at a time, so that we rock between the original and the temp
  for (vtkm::Id logStep = 0; logStep < nLogSteps; logStep++)
  {
    chainDoublerDispatcher.Invoke(activeVertices, // input
                                  chainExtremum); // i/o (whole array)
  }

#ifdef DEBUG_PRINT
  DebugPrint("Chains Built");
#endif
} // BuildChains()

// transfers saddle ascent edges into edge sorter
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::TransferSaddleStarts()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << DEBUG_STRING_TRANSFER_SADDLE_STARTS << std::endl;
  std::cout << "=======================" << std::endl;
  std::cout << std::endl;
#endif

  // grab the size of the array for easier reference
  vtkm::Id nActiveVertices = activeVertices.GetNumberOfValues();

  // reset number of edges to sort
  vtkm::Id nEdgesToSort = 0;

  // in parallel, we need to create a vector to count the first edge for each vertex
  vtkm::cont::ArrayHandle<vtkm::Id> newFirstEdge;
  vtkm::cont::ArrayHandle<vtkm::Id> newOutdegree;
  newFirstEdge.Allocate(nActiveVertices);
  newOutdegree.Allocate(nActiveVertices);

  // 2. now test all active vertices to see if they have only one chain maximum
  SaddleAscentFunctor saddleAscentFunctor;
  vtkm::worklet::DispatcherMapField<SaddleAscentFunctor> saddleAscentFunctorDispatcher(
    saddleAscentFunctor);

  saddleAscentFunctorDispatcher.Invoke(activeVertices, // input
                                       firstEdge,      // input (whole array)
                                       outdegree,      // input (whole array)
                                       activeEdges,    // input (whole array)
                                       chainExtremum,  // input (whole array)
                                       edgeFar,        // input (whole array)
                                       newOutdegree);  // output

  // 3. now compute the new offsets in the newFirstEdge array
  vtkm::cont::Algorithm::ScanExclusive(newOutdegree, newFirstEdge);
  nEdgesToSort = vtkm::cont::ArrayGetValue(nActiveVertices - 1, newFirstEdge) +
    vtkm::cont::ArrayGetValue(nActiveVertices - 1, newOutdegree);

  edgeSorter.ReleaseResources();
  edgeSorter.Allocate(nEdgesToSort);

  SaddleAscentTransferrer saddleAscentTransferrer;
  vtkm::worklet::DispatcherMapField<SaddleAscentTransferrer> saddleAscentTransferrerDispatcher(
    saddleAscentTransferrer);

  saddleAscentTransferrerDispatcher.Invoke(activeVertices, // input
                                           newOutdegree,   // input
                                           newFirstEdge,   // input
                                           activeEdges,    // input (whole array)
                                           firstEdge,      // input (whole array)
                                           edgeSorter);    // output (whole array)

#ifdef DEBUG_PRINT
  DebugPrint(DEBUG_STRING_TRANSFERRED_SADDLE_STARTS);
#endif
} // TransferSaddleStarts()

// sets all remaining active vertices
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::BuildTrunk()
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "===========" << std::endl;
  std::cout << "Build Trunk" << std::endl;
  std::cout << "============" << std::endl;
  std::cout << std::endl;
#endif

  TrunkBuilder trunkBuilder;
  vtkm::worklet::DispatcherMapField<TrunkBuilder> trunkBuilderDispatcher(trunkBuilder);

  trunkBuilderDispatcher.Invoke(activeVertices, // input
                                chainExtremum,  // input (whole array)
                                prunesTo);      // output (whole array)
#ifdef DEBUG_PRINT
  DebugPrint("Trunk Built");
#endif
} // BuildTrunk()

// transfers partial results to merge tree array
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::TransferToMergeTree(vtkm::cont::ArrayHandle<vtkm::Id>& saddles)
{
#ifdef DEBUG_FUNCTION_ENTRY
  std::cout << std::endl;
  std::cout << "=====================" << std::endl;
  std::cout << DEBUG_STRING_TRANSFER_TO_MERGE_TREE << std::endl;
  std::cout << "=====================" << std::endl;
  std::cout << std::endl;
#endif

  // first allocate memory for the target array
  saddles.ReleaseResources();

  // initialise it to the arcArray
  vtkm::cont::ArrayCopy(arcArray, saddles);

  JoinTreeTransferrer joinTreeTransferrer;
  vtkm::worklet::DispatcherMapField<JoinTreeTransferrer> joinTreeTransferrerDispatcher(
    joinTreeTransferrer);
  vtkm::cont::ArrayHandleIndex valueIndexArray(valueIndex.GetNumberOfValues());

  joinTreeTransferrerDispatcher.Invoke(valueIndexArray, // input
                                       prunesTo,        // input
                                       valueIndex,      // input (whole array)
                                       chainExtremum,   // input (whole array)
                                       saddles,         // output (whole array)
                                       arcArray);       // output (whole array)
} // TransferToMergeTree()

// prints the contents of the topology graph in standard format
template <typename T, typename StorageType>
void ChainGraph<T, StorageType>::DebugPrint(const char* message)
{
  std::cout << "---------------------------" << std::endl;
  std::cout << std::string(message) << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  using IdArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using ValueArrayType = vtkm::cont::ArrayHandle<T>;
  using PermuteIndexType = vtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;
  using PermuteValueType = vtkm::cont::ArrayHandlePermutation<IdArrayType, ValueArrayType>;

  // Full Vertex Arrays
  vtkm::Id nValues = valueIndex.GetNumberOfValues();
  vtkm::cont::ArrayHandle<T, StorageType> vertexValues;

  std::cout << "Full Vertex Arrays - Size:  " << nValues << std::endl;
  printHeader(nValues);
  printIndices("Index", valueIndex);
  vtkm::cont::ArrayCopy(PermuteValueType(valueIndex, values), vertexValues);
  printValues("Value", vertexValues);
  printIndices("First Edge", firstEdge);
  printIndices("Outdegree", outdegree);
  printIndices("Chain Ext", chainExtremum);
  printIndices("Prunes To", prunesTo);
  std::cout << std::endl;

  // Active Vertex Arrays
  vtkm::Id nActiveVertices = activeVertices.GetNumberOfValues();
  std::cout << "Active Vertex Arrays - Size: " << nActiveVertices << std::endl;
  if (nActiveVertices > 0)
  {
    vtkm::cont::ArrayHandle<vtkm::Id> tempIndex;
    vtkm::cont::ArrayHandle<T> tempValue;

    printHeader(nActiveVertices);
    printIndices("Active Vertices", activeVertices);
    vtkm::cont::ArrayCopy(PermuteIndexType(activeVertices, valueIndex), tempIndex);
    printIndices("Active Indices", tempIndex);
    vtkm::cont::ArrayCopy(PermuteValueType(activeVertices, vertexValues), tempValue);
    printValues("Active Values", tempValue);
    vtkm::cont::ArrayCopy(PermuteIndexType(activeVertices, firstEdge), tempIndex);
    printIndices("Active First Edge", tempIndex);
    vtkm::cont::ArrayCopy(PermuteIndexType(activeVertices, outdegree), tempIndex);
    printIndices("Active Outdegree", tempIndex);
    vtkm::cont::ArrayCopy(PermuteIndexType(activeVertices, chainExtremum), tempIndex);
    printIndices("Active Chain Ext", tempIndex);
    vtkm::cont::ArrayCopy(PermuteIndexType(activeVertices, prunesTo), tempIndex);
    printIndices("Active Prunes To", tempIndex);
    std::cout << std::endl;
  }

  // Full Edge Arrays
  vtkm::Id nEdges = edgeNear.GetNumberOfValues();
  std::cout << "Full Edge Arrays - Size:     " << nEdges << std::endl;
  vtkm::cont::ArrayHandle<vtkm::Id> farIndices;
  vtkm::cont::ArrayHandle<vtkm::Id> nearIndices;
  vtkm::cont::ArrayHandle<T, StorageType> farValues;
  vtkm::cont::ArrayHandle<T, StorageType> nearValues;
  if (nEdges > 0)
  {
    printHeader(nEdges);
    printIndices("Far", edgeFar);
    vtkm::cont::ArrayCopy(PermuteIndexType(edgeFar, valueIndex), farIndices);
    printIndices("Far Index", farIndices);
    vtkm::cont::ArrayCopy(PermuteValueType(farIndices, values), farValues);
    printValues("Far Value", farValues);

    printHeader(nEdges);
    printIndices("Near", edgeNear);
    vtkm::cont::ArrayCopy(PermuteIndexType(edgeNear, valueIndex), nearIndices);
    printIndices("Near Index", nearIndices);
    vtkm::cont::ArrayCopy(PermuteValueType(nearIndices, values), nearValues);
    printValues("Near Value", nearValues);
  }

  // Active Edge Arrays
  vtkm::Id nActiveEdges = activeEdges.GetNumberOfValues();
  std::cout << "Active Edge Arrays - Size:   " << nActiveEdges << std::endl;
  if (nActiveEdges > 0)
  {
    vtkm::cont::ArrayHandle<vtkm::Id> activeFarIndices;
    vtkm::cont::ArrayHandle<vtkm::Id> activeNearIndices;
    vtkm::cont::ArrayHandle<vtkm::Id> activeNearLookup;
    vtkm::cont::ArrayHandle<T, StorageType> activeNearValues;

    printHeader(nActiveEdges);
    printIndices("Active Edges", activeEdges);

    vtkm::cont::ArrayCopy(PermuteIndexType(activeEdges, edgeFar), activeFarIndices);
    printIndices("Edge Far", activeFarIndices);
    vtkm::cont::ArrayCopy(PermuteIndexType(activeEdges, edgeNear), activeNearIndices);
    printIndices("Edge Near", activeNearIndices);
    vtkm::cont::ArrayCopy(PermuteIndexType(activeNearIndices, valueIndex), activeNearLookup);
    printIndices("Edge Near Index", activeNearLookup);
    vtkm::cont::ArrayCopy(PermuteValueType(activeNearLookup, values), activeNearValues);
    printValues("Edge Near Value", activeNearValues);
    std::cout << std::endl;
  }

  // Edge Sorter Array
  vtkm::Id nEdgeSorter = edgeSorter.GetNumberOfValues();
  std::cout << "Edge Sorter - Size:          " << nEdgeSorter << std::endl;
  if (nEdgeSorter > 0)
  {
    vtkm::cont::ArrayHandle<vtkm::Id> tempSortIndex;
    vtkm::cont::ArrayHandle<T> tempSortValue;

    printHeader(nEdgeSorter);
    printIndices("Edge Sorter", edgeSorter);
    vtkm::cont::ArrayCopy(PermuteIndexType(edgeSorter, edgeNear), tempSortIndex);
    printIndices("Sorted Near", tempSortIndex);
    vtkm::cont::ArrayCopy(PermuteIndexType(edgeSorter, nearIndices), tempSortIndex);
    printIndices("Sorted Near Index", tempSortIndex);
    vtkm::cont::ArrayCopy(PermuteIndexType(edgeSorter, edgeFar), tempSortIndex);
    printIndices("Sorted Far", tempSortIndex);
    vtkm::cont::ArrayCopy(PermuteValueType(edgeSorter, nearValues), tempSortValue);
    printValues("Sorted Near Value", tempSortValue);
    std::cout << std::endl;
  }

  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;
} // DebugPrint()
}
}
}

#endif
