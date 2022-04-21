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
//  Parallel Peak Pruning v. 2.0
//
//  Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// HierarchicalContourTree.cpp - Hierarchical version of contour tree that captures all of the
// superarcs relevant for a particular block.  It is constructed by grafting missing edges
// into the tree at all levels
//
//=======================================================================================
//
// COMMENTS:
//
//  There are several significant differences from the ContourTree class, in particular the
// semantics of storage:
// i.  Hyper arcs are processed inside to outside instead of outside to inside
//     This is to allow the superarcs in higher blocks to be a prefix of those in lower blocks
//    We can do this by inverting the loop order and processing each level separately, so
//    we don't need to renumber (whew!)
// ii.  If the superarc is -1, it USED to mean the root of the tree. Now it can also mean
//    the root of a lower-level subtree. in this case, the superparent will show which
//    existing superarc it inserts into.
//
//=======================================================================================

#ifndef vtk_m_worklet_contourtree_distributed_hierarchical_contour_tree_h
#define vtk_m_worklet_contourtree_distributed_hierarchical_contour_tree_h

#define VOLUME_PRINT_WIDTH 8

#include <vtkm/Types.h>
#include <vtkm/worklet/contourtree_augmented/ContourTree.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_contour_tree/FindRegularByGlobal.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_contour_tree/FindSuperArcForUnknownNode.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_contour_tree/InitalizeSuperchildrenWorklet.h>
#include <vtkm/worklet/contourtree_distributed/hierarchical_contour_tree/PermuteComparator.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{


/// \brief Hierarchical Contour Tree data structure
///
/// This class contains all the structures to construct/store the HierarchicalContourTree.
/// Functions used on the Device are then implemented in the HierarchicalContourTreeDeviceData
/// class which stores the prepared array portals we need for those functions.
///
template <typename FieldType>
class HierarchicalContourTree
{
public:
  VTKM_CONT
  HierarchicalContourTree();

  // REGULAR arrays: i.e. over all nodes in the tree, including regular
  // the full list of global IDs for the regular nodes
  vtkm::worklet::contourtree_augmented::IdArrayType RegularNodeGlobalIds;
  // we will also need to track the data values
  vtkm::cont::ArrayHandle<FieldType> DataValues;

  // an array to support searching by global ID
  // given a global ID, find its position in the regular node index
  // To do so, we keep an index by global ID of their positions in the array
  vtkm::worklet::contourtree_augmented::IdArrayType RegularNodeSortOrder;
  // the supernode ID for each regular node: for most, this will be NO_SUCH_ELEMENT
  // but this makes lookups for supernode ID a lot easier
  vtkm::worklet::contourtree_augmented::IdArrayType Regular2Supernode;
  // the superparent for each regular node
  vtkm::worklet::contourtree_augmented::IdArrayType Superparents;

  // SUPER arrays: i.e. over all supernodes in the tree
  // the ID in the globalID array
  vtkm::worklet::contourtree_augmented::IdArrayType Supernodes;
  // where the supernode connects to
  vtkm::worklet::contourtree_augmented::IdArrayType Superarcs;
  // the hyperparent for each supernode
  vtkm::worklet::contourtree_augmented::IdArrayType Hyperparents;
  // the hypernode ID for each supernode: often NO_SUCH_ELEMENT
  // but it makes lookups easier
  vtkm::worklet::contourtree_augmented::IdArrayType Super2Hypernode;

  // which iteration & round the vertex is transferred in
  // the second of these is the same as "whenTransferred", but inverted order
  vtkm::worklet::contourtree_augmented::IdArrayType WhichRound;
  vtkm::worklet::contourtree_augmented::IdArrayType WhichIteration;

  // HYPER arrays: i.e. over all hypernodes in the tree
  // the ID in the supernode array
  vtkm::worklet::contourtree_augmented::IdArrayType Hypernodes;
  // where the hypernode connects to
  vtkm::worklet::contourtree_augmented::IdArrayType Hyperarcs;
  // the number of child supernodes on the superarc (including the start node)
  // and not including any inserted in the hierarchy
  vtkm::worklet::contourtree_augmented::IdArrayType Superchildren;

  // how many rounds of fan-in were used to construct it
  vtkm::Id NumRounds;

  // use for debugging? -> This makes more sense in hyper sweeper?
  // vtkm::Id NumOwnedRegularVertices;

  // The following arrays store the numbers of reg/super/hyper nodes at each level of the hierarchy
  // They are filled in from the top down, and are fundamentally CPU side control variables
  // They will be needed for hypersweeps.

  // May be for hypersweeps later on. SHOULD be primarily CPU side
  /// arrays holding the logical size of the arrays at each level
  vtkm::worklet::contourtree_augmented::IdArrayType NumRegularNodesInRound;
  vtkm::worklet::contourtree_augmented::IdArrayType NumSupernodesInRound;
  vtkm::worklet::contourtree_augmented::IdArrayType NumHypernodesInRound;

  /// how many iterations needed for the hypersweep at each level
  vtkm::worklet::contourtree_augmented::IdArrayType NumIterations;

  /// vectors tracking the segments used in each iteration of the hypersweep
  std::vector<vtkm::worklet::contourtree_augmented::IdArrayType> FirstSupernodePerIteration;
  std::vector<vtkm::worklet::contourtree_augmented::IdArrayType> FirstHypernodePerIteration;

  /// routine to create a FindRegularByGlobal object that we can use as an input for worklets to call the function
  VTKM_CONT
  FindRegularByGlobal GetFindRegularByGlobal() const
  {
    return FindRegularByGlobal(this->RegularNodeSortOrder, this->RegularNodeGlobalIds);
  }

  /// routine to create a FindSuperArcForUnknownNode object that we can use as an input for worklets to call the function
  VTKM_CONT
  FindSuperArcForUnknownNode<FieldType> GetFindSuperArcForUnknownNode()
  {
    return FindSuperArcForUnknownNode<FieldType>(this->Superparents,
                                                 this->Supernodes,
                                                 this->Superarcs,
                                                 this->Superchildren,
                                                 this->WhichRound,
                                                 this->WhichIteration,
                                                 this->Hyperparents,
                                                 this->Hypernodes,
                                                 this->Hyperarcs,
                                                 this->RegularNodeGlobalIds,
                                                 this->DataValues);
  }

  ///  routine to initialize the hierarchical tree with the top level tree
  VTKM_CONT
  void Initialize(vtkm::Id numRounds,
                  vtkm::worklet::contourtree_augmented::ContourTree& tree,
                  vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType>& mesh);

  /// utility routines for the path probes
  VTKM_CONT
  std::string RegularString(const vtkm::Id regularId) const;

  VTKM_CONT
  std::string SuperString(const vtkm::Id superId) const;

  VTKM_CONT
  std::string HyperString(const vtkm::Id hyperId) const;

  /// routine to probe a given node and trace it's hyperpath to the root
  VTKM_CONT
  std::string ProbeHyperPath(const vtkm::Id regularId, const vtkm::Id maxLength = -1) const;

  /// routine to probe a given node and trace it's superpath to the root
  VTKM_CONT
  std::string ProbeSuperPath(const vtkm::Id regularId, const vtkm::Id maxLength = -1) const;

  /// Outputs the Hierarchical Tree in Dot format for visualization
  VTKM_CONT
  std::string PrintDotSuperStructure(const char* label) const;

  /// Print hierarchical tree construction stats, usually used for logging
  VTKM_CONT
  std::string PrintTreeStats() const;

  /// debug routine
  VTKM_CONT
  std::string DebugPrint(std::string message, const char* fileName, long lineNum) const;

  // modified version of dumpSuper() that also gives volume counts
  VTKM_CONT
  static std::string DumpVolumes(
    const vtkm::worklet::contourtree_augmented::IdArrayType& supernodes,
    const vtkm::worklet::contourtree_augmented::IdArrayType& superarcs,
    const vtkm::worklet::contourtree_augmented::IdArrayType& regularNodeGlobalIds,
    vtkm::Id totalVolume,
    const vtkm::worklet::contourtree_augmented::IdArrayType& intrinsicVolume,
    const vtkm::worklet::contourtree_augmented::IdArrayType& dependentVolume);

private:
  /// Used internally to Invoke worklets
  vtkm::cont::Invoker Invoke;
};

template <typename FieldType>
HierarchicalContourTree<FieldType>::HierarchicalContourTree()
//: NumOwnedRegularVertices(static_cast<vtkm::Id>(0))
{ // constructor
  NumRegularNodesInRound.ReleaseResources();
  NumSupernodesInRound.ReleaseResources();
  NumHypernodesInRound.ReleaseResources();
  NumIterations.ReleaseResources();
} // constructor


///  routine to initialize the hierarchical tree with the top level tree
template <typename FieldType>
void HierarchicalContourTree<FieldType>::Initialize(
  vtkm::Id numRounds,
  vtkm::worklet::contourtree_augmented::ContourTree& tree,
  vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType>& mesh)
{ // Initialize(..)
  // TODO: If any other arrays are only copied in this function but will not be modified then we could just assign instead of copy them and make them const
  // set the initial logical size of the arrays: note that we need to keep level 0 separate, so have an extra level at the top
  this->NumRounds = numRounds;
  {
    auto tempZeroArray = vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, this->NumRounds + 1);
    vtkm::cont::Algorithm::Copy(tempZeroArray, this->NumIterations);
    vtkm::cont::Algorithm::Copy(tempZeroArray, this->NumRegularNodesInRound);
    vtkm::worklet::contourtree_augmented::IdArraySetValue(
      this->NumRounds, tree.Nodes.GetNumberOfValues(), this->NumRegularNodesInRound);
    vtkm::cont::Algorithm::Copy(tempZeroArray, this->NumSupernodesInRound);
    vtkm::worklet::contourtree_augmented::IdArraySetValue(
      this->NumRounds, tree.Supernodes.GetNumberOfValues(), this->NumSupernodesInRound);
    vtkm::cont::Algorithm::Copy(tempZeroArray, this->NumHypernodesInRound);
    vtkm::worklet::contourtree_augmented::IdArraySetValue(
      this->NumRounds, tree.Hypernodes.GetNumberOfValues(), this->NumHypernodesInRound);
  }
  // copy the iterations of the top level hypersweep - this is +1: one because we are counting inclusively
  // HAC JAN 15, 2020: In order to make this consistent with grafting rounds for hybrid hypersweeps, we add one to the logical number of
  // iterations instead of the prior version which stored an extra extra element (ie +2)
  // WARNING! WARNING! WARNING!  This is a departure from the treatment in the contour tree, where the last iteration to the NULL root was
  // treated as an implicit round.
  {
    vtkm::Id tempSizeVal = vtkm::cont::ArrayGetValue(this->NumRounds, this->NumIterations) + 1;
    vtkm::worklet::contourtree_augmented::IdArraySetValue(
      this->NumRounds, tree.NumIterations + 1, this->NumIterations);
    this->FirstSupernodePerIteration.resize(static_cast<std::size_t>(this->NumRounds + 1));
    this->FirstSupernodePerIteration[static_cast<std::size_t>(this->NumRounds)].Allocate(
      tempSizeVal);
    this->FirstHypernodePerIteration.resize(static_cast<std::size_t>(this->NumRounds + 1));
    this->FirstHypernodePerIteration[static_cast<std::size_t>(this->NumRounds)].Allocate(
      tempSizeVal);
  }
  // now copy in the details. Use CopySubRagnge to ensure that the Copy does not shrink the size
  // of the array as the arrays are in this case allocated above to the approbriate size
  vtkm::cont::Algorithm::CopySubRange(
    tree.FirstSupernodePerIteration,                     // copy this
    0,                                                   // start at index 0
    tree.FirstSupernodePerIteration.GetNumberOfValues(), // copy all values
    this->FirstSupernodePerIteration[static_cast<std::size_t>(this->NumRounds)]);
  vtkm::cont::Algorithm::CopySubRange(
    tree.FirstHypernodePerIteration,
    0,                                                   // start at index 0
    tree.FirstHypernodePerIteration.GetNumberOfValues(), // copy all values
    this->FirstHypernodePerIteration[static_cast<std::size_t>(this->NumRounds)]);

  // set the sizes for the arrays
  this->RegularNodeGlobalIds.Allocate(tree.Nodes.GetNumberOfValues());
  this->DataValues.Allocate(mesh.SortedValues.GetNumberOfValues());
  this->RegularNodeSortOrder.Allocate(tree.Nodes.GetNumberOfValues());
  this->Superparents.Allocate(tree.Superparents.GetNumberOfValues());
  {
    auto tempNSE = vtkm::cont::ArrayHandleConstant<vtkm::Id>(
      vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT, tree.Nodes.GetNumberOfValues());
    vtkm::cont::Algorithm::Copy(tempNSE, this->Regular2Supernode);
  }

  this->Supernodes.Allocate(tree.Supernodes.GetNumberOfValues());
  this->Superarcs.Allocate(tree.Superarcs.GetNumberOfValues());
  this->Hyperparents.Allocate(tree.Hyperparents.GetNumberOfValues());
  {
    auto tempNSE = vtkm::cont::ArrayHandleConstant<vtkm::Id>(
      vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT, tree.Supernodes.GetNumberOfValues());
    vtkm::cont::Algorithm::Copy(tempNSE, this->Super2Hypernode);
  }
  this->WhichRound.Allocate(tree.Supernodes.GetNumberOfValues());
  this->WhichIteration.Allocate(tree.Supernodes.GetNumberOfValues());

  this->Hypernodes.Allocate(tree.Hypernodes.GetNumberOfValues());
  this->Hyperarcs.Allocate(tree.Hyperarcs.GetNumberOfValues());
  this->Superchildren.Allocate(tree.Hyperarcs.GetNumberOfValues());

  //copy the regular nodes
  vtkm::cont::Algorithm::Copy(mesh.GlobalMeshIndex, this->RegularNodeGlobalIds);
  vtkm::cont::Algorithm::Copy(mesh.SortedValues, this->DataValues);

  // we want to be able to search by global mesh index.  That means we need to have an index array, sorted indirectly on globalMeshIndex
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleIndex(RegularNodeSortOrder.GetNumberOfValues()), RegularNodeSortOrder);
  vtkm::cont::Algorithm::Sort(RegularNodeSortOrder, PermuteComparator(this->RegularNodeGlobalIds));
  vtkm::cont::Algorithm::Copy(tree.Superparents, this->Superparents);

  // copy in the supernodes
  vtkm::cont::Algorithm::Copy(tree.Supernodes, this->Supernodes);
  vtkm::cont::Algorithm::Copy(tree.Superarcs, this->Superarcs);
  vtkm::cont::Algorithm::Copy(tree.Hyperparents, this->Hyperparents);

  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleConstant<vtkm::Id>(numRounds, this->WhichRound.GetNumberOfValues()),
    this->WhichRound);
  vtkm::cont::Algorithm::Copy(tree.WhenTransferred, this->WhichIteration);

  // now set the regular to supernode array up: it's already been set to NO_SUCH_ELEMENT
  {
    auto regular2SupernodePermuted =
      vtkm::cont::make_ArrayHandlePermutation(this->Supernodes, this->Regular2Supernode);
    vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleIndex(this->Supernodes.GetNumberOfValues()),
                                regular2SupernodePermuted);
  }
  // copy in the hypernodes
  vtkm::cont::Algorithm::Copy(tree.Hypernodes, this->Hypernodes);
  vtkm::cont::Algorithm::Copy(tree.Hyperarcs, this->Hyperarcs);

  // now set the supernode to hypernode array up: it's already been set to NO_SUCH_ELEMENT
  {
    auto super2HypernodePermuted =
      vtkm::cont::make_ArrayHandlePermutation(this->Hypernodes, this->Super2Hypernode);
    vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleIndex(this->Hypernodes.GetNumberOfValues()),
                                super2HypernodePermuted);
  }
  {
    auto initalizeSuperchildrenWorklet = InitalizeSuperchildrenWorklet();
    this->Invoke(initalizeSuperchildrenWorklet,
                 this->Hyperarcs,    // Input
                 this->Hypernodes,   // Input
                 this->Superchildren // Output
    );
  }
} // Initialize(..)


/// utility routine for the path probes
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::RegularString(const vtkm::Id regularId) const
{ // RegularString()
  std::stringstream resultStream;
  // this can get called before the regular ID is fully stored
  if (regularId >= this->DataValues.GetNumberOfValues())
  {
    resultStream << "Regular ID: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(regularId, resultStream);
    resultStream << " Value: N/A Global ID: N/A Regular ID: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(regularId, resultStream);
    resultStream << " SNode ID:    N/A Superparent: N/A";
  }
  else
  {
    resultStream << "Regular ID: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(regularId, resultStream);
    resultStream << "  Value: " << vtkm::cont::ArrayGetValue(regularId, this->DataValues);
    resultStream << " Global ID: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(
      vtkm::cont::ArrayGetValue(regularId, this->RegularNodeGlobalIds), resultStream);
    resultStream << " Regular ID: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(regularId, resultStream);
    resultStream << " SNode ID: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(
      vtkm::cont::ArrayGetValue(regularId, this->Regular2Supernode), resultStream);
    resultStream << "Superparents: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(
      vtkm::cont::ArrayGetValue(regularId, this->Superparents));
  }
  return resultStream.str();
} // RegularString()


/// utility routine for the path probes
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::SuperString(const vtkm::Id superId) const
{ // SuperString()
  std::stringstream resultStream;
  if (vtkm::worklet::contourtree_augmented::NoSuchElement(superId))
  {
    resultStream << "Super ID:   ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(superId, resultStream);
  }
  else
  {
    vtkm::Id unmaskedSuperId = vtkm::worklet::contourtree_augmented::MaskedIndex(superId);
    vtkm::Id tempSupernodeOfSuperId = vtkm::cont::ArrayGetValue(unmaskedSuperId, this->Supernodes);
    resultStream << "Super ID:   ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(superId, resultStream);
    resultStream << "  Value: "
                 << vtkm::cont::ArrayGetValue(tempSupernodeOfSuperId, this->DataValues);
    resultStream << " Global ID: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(
      vtkm::cont::ArrayGetValue(tempSupernodeOfSuperId, this->RegularNodeGlobalIds), resultStream);
    resultStream << " Regular Id: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(tempSupernodeOfSuperId, resultStream);
    resultStream << " Superarc:    ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(
      vtkm::cont::ArrayGetValue(unmaskedSuperId, this->Superarcs), resultStream);
    resultStream << " HNode ID: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(
      vtkm::cont::ArrayGetValue(unmaskedSuperId, this->Super2Hypernode), resultStream);
    resultStream << " Hyperparent:   ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(
      vtkm::cont::ArrayGetValue(unmaskedSuperId, this->Hyperparents), resultStream);
    resultStream << " Round: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(
      vtkm::cont::ArrayGetValue(unmaskedSuperId, this->WhichRound), resultStream);
    resultStream << " Iteration: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(
      vtkm::cont::ArrayGetValue(unmaskedSuperId, this->WhichIteration), resultStream);
  }
  return resultStream.str();
} // SuperString()


/// utility routine for the path probes
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::HyperString(const vtkm::Id hyperId) const
{ // HyperString()
  std::stringstream resultStream;
  if (vtkm::worklet::contourtree_augmented::NoSuchElement(hyperId))
  {
    resultStream << "Hyper ID:   ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(hyperId, resultStream);
  }
  else
  {
    vtkm::Id unmaskedHyperId = vtkm::worklet::contourtree_augmented::MaskedIndex(hyperId);
    vtkm::Id hypernodeOfHyperId = vtkm::cont::ArrayGetValue(unmaskedHyperId, this->Hypernodes);
    vtkm::Id supernodeOfHyperId = vtkm::cont::ArrayGetValue(hypernodeOfHyperId, this->Supernodes);
    resultStream << "Hyper Id:    ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(hyperId, resultStream);
    resultStream << "  Value: " << vtkm::cont::ArrayGetValue(supernodeOfHyperId, this->DataValues);
    resultStream << " Global ID: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(
      vtkm::cont::ArrayGetValue(supernodeOfHyperId, this->RegularNodeGlobalIds), resultStream);
    resultStream << " Regular ID: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(supernodeOfHyperId, resultStream);
    resultStream << " Super ID: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(hypernodeOfHyperId, resultStream);
    resultStream << " Hyperarc: ";
    vtkm::worklet::contourtree_augmented::PrintIndexType(
      vtkm::cont::ArrayGetValue(unmaskedHyperId, this->Hyperarcs), resultStream);
    resultStream << " Superchildren: "
                 << vtkm::cont::ArrayGetValue(unmaskedHyperId, this->Superchildren);
  }
  return resultStream.str();
} // HyperString()

/// routine to probe a given node and trace it's hyperpath to the root
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::ProbeHyperPath(const vtkm::Id regularId,
                                                               const vtkm::Id maxLength) const
{ // ProbeHyperPath()
  std::stringstream resultStream;
  resultStream << "Probing HyperPath\n";
  resultStream << "Node:        " << this->RegularString(regularId) << std::endl;

  // find the superparent
  vtkm::Id superparent = vtkm::cont::ArrayGetValue(regularId, this->Superparents);
  resultStream << "Superparent: " << SuperString(superparent) << std::endl;

  // and the hyperparent
  vtkm::Id hyperparent = vtkm::cont::ArrayGetValue(superparent, this->Hyperparents);

  // now trace the path inwards: terminate on last round when we have null hyperarc
  vtkm::Id length = 0;
  while (true)
  { // loop inwards
    length++;
    if (length > maxLength && maxLength > 0)
    {
      break;
    }
    resultStream << "Hyperparent: " << this->HyperString(hyperparent) << std::endl;

    // retrieve the target of the hyperarc
    vtkm::Id hypertarget = vtkm::cont::ArrayGetValue(hyperparent, this->Hyperarcs);

    resultStream << "Hypertarget: "
                 << SuperString(vtkm::worklet::contourtree_augmented::MaskedIndex(hypertarget))
                 << std::endl;

    // mask the hypertarget
    vtkm::Id maskedHypertarget = vtkm::worklet::contourtree_augmented::MaskedIndex(hypertarget);

    // test for null superarc: can only be root or attachment point
    if (vtkm::worklet::contourtree_augmented::NoSuchElement(hypertarget))
    { // root or attachment point
      // we're done
      break;
    } // root or attachment point
    else
    { // ordinary supernode
      hyperparent = vtkm::cont::ArrayGetValue(maskedHypertarget, this->Hyperparents);
    } // ordinary supernode

    // now take the new superparent's hyperparent/hypertarget
    hypertarget = vtkm::cont::ArrayGetValue(hyperparent, this->Hyperarcs);
  } // loop inwards

  resultStream << "Probe Complete" << std::endl << std::endl;
  return resultStream.str();
} // ProbeHyperPath()


/// routine to probe a given node and trace it's superpath to the root
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::ProbeSuperPath(const vtkm::Id regularId,
                                                               const vtkm::Id maxLength) const
{
  std::stringstream resultStream;
  // find the superparent
  vtkm::Id superparent = vtkm::cont::ArrayGetValue(regularId, this->Superparents);
  // now trace the path inwards: terminate on last round when we have null hyperarc
  vtkm::Id length = 0;
  while (true)
  { // loop inwards
    length++;
    if (length > maxLength && maxLength > 0)
    {
      break;
    }
    // retrieve the target of the superarc
    vtkm::Id supertarget = vtkm::cont::ArrayGetValue(superparent, this->Superarcs);

    resultStream << "Superparent: " << this->SuperString(superparent) << std::endl;
    resultStream << "Supertarget: "
                 << this->SuperString(
                      vtkm::worklet::contourtree_augmented::MaskedIndex(supertarget))
                 << std::endl;

    // mask the supertarget
    vtkm::Id maskedSupertarget = vtkm::worklet::contourtree_augmented::MaskedIndex(supertarget);
    // and retrieve it's supertarget
    vtkm::Id nextSupertarget = vtkm::cont::ArrayGetValue(maskedSupertarget, this->Superarcs);
    vtkm::Id maskedNextSupertarget =
      vtkm::worklet::contourtree_augmented::MaskedIndex(nextSupertarget);
    resultStream << "Next target: " << this->SuperString(nextSupertarget) << std::endl;

    // test for null superarc: can only be root or attachment point
    if (vtkm::worklet::contourtree_augmented::NoSuchElement(nextSupertarget))
    { // root or attachment point
      // test round: if it's the last one, only the root has a null edge
      if (vtkm::cont::ArrayGetValue(maskedNextSupertarget, this->WhichRound) == this->NumRounds)
        // we're done
        break;
      else // attachment point
        superparent = maskedNextSupertarget;
    } // root or attachment point
    else
    { // ordinary supernode
      superparent = maskedSupertarget;
    } // ordinary supernode
  }   // loop inwards

  resultStream << "Probe Complete" << std::endl << std::endl;
  return resultStream.str();
}

/// Outputs the Hierarchical Tree in Dot format for visualization
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::PrintDotSuperStructure(const char* label) const
{ // PrintDotSuperStructure
  // make a copy of the label
  std::string filename("temp/");
  filename += label;

  // replace spaces with underscores
  for (unsigned int strChar = 0; strChar < filename.length(); strChar++)
    if (filename[strChar] == ' ')
      filename[strChar] = '_';

  // add the .gv suffix
  filename += ".gv";

  // generate an output stream
  std::ofstream outstream(filename);

  // print the header information
  outstream << "digraph RegularTree\n\t{\n";
  outstream << "\tsize=\"6.5, 9\"\n\tratio=\"fill\"\n";
  outstream << "\tlabel=\"" << label << "\"\n\tlabelloc=t\n\tfontsize=30\n";

  // create the NULL (root) node
  outstream << "\t// NULL node to use as a root for the tree\n";
  outstream << "\tNULL [style=filled,fillcolor=white,shape=point,label=\"NULL\"];\n";

  outstream << "\t// Supernodes\n";
  // loop through all supernodes
  // We use regular ReadPortals here since this requires access to many values anyways
  auto supernodesPortal = this->Supernodes.ReadPortal();
  auto hypernodesPortal = this->Hypernodes.ReadPortal();
  auto hyperparentsPortal = this->Hyperparents.ReadPortal();
  auto hyperarcsPortal = this->Hyperarcs.ReadPortal();
  auto regularNodeGlobalIdsPortal = this->RegularNodeGlobalIds.ReadPortal();
  auto whichIterationPortal = this->WhichIteration.ReadPortal();
  auto whichRoundPortal = this->whichRound.ReadPortal();
  auto superarcsPortal = this->Superarcs.ReadPortal();
  auto superparentsPortal = this->Superparents.ReadPortal();
  for (vtkm::Id supernode = 0; supernode < this->Supernodes.GetNumberOfValues(); supernode++)
  { // per supernode
    vtkm::Id regularID = supernodesPortal.Get(supernode);
    // print the supernode, making hypernodes double octagons
    outstream
      << "    SN" << std::setw(1) << supernode
      << " [style=filled,fillcolor=white,shape=" << std::setw(1)
      << ((hypernodesPortal.Get(hyperparentsPortal.Get(supernode)) == supernode)
            ? "doublecircle"
            : "circle") // hypernodes are double-circles
      << ",label=\"sn" << std::setw(4) << supernode << "    h" << std::setw(1)
      << ((hypernodesPortal.Get(hyperparentsPortal.Get(supernode)) == supernode)
            ? "n"
            : "p") // hypernodes show "hn001" (their own ID), supernodes show "hp001" (their hyperparent)
      << std::setw(4) << hyperparentsPortal.Get(supernode) << "\\nm" << std::setw(1) << regularID
      << "    g" << std::setw(4) << regularNodeGlobalIdsPortal.Get(regularID) << "\\nrd"
      << std::setw(1) << whichIterationPortal.Get(supernode) << "    it" << std::setw(4)
      << contourtree_augmented::MaskedIndex(whichIterationPortal.Get(supernode)) << "\"];\n";
  } // per supernode

  outstream << "\t// Superarc nodes\n";
  // now repeat to create nodes for the middle of each superarc (to represent the superarcs themselves)
  for (vtkm::Id superarc = 0; superarc < this->Superarcs.GetNumberOfValues(); superarc++)
  { // per superarc
    // print the superarc vertex
    outstream
      << "\tSA" << std::setw(1) << superarc
      << " [shape=circle,fillcolor=white,fixedsize=true,height=0.5,width=0.5,label=\"\"];\n";
  } // per superarc

  outstream << "\t// Superarc edges\n";
  // loop through all superarcs to draw them
  for (vtkm::Id superarc = 0; superarc < this->Superarcs.GetNumberOfValues(); superarc++)
  { // per superarc
    // retrieve ID of target supernode
    vtkm::Id superarcFrom = superarc;
    vtkm::Id superarcTo = superarcsPortal.Get(superarcFrom);

    // if this is true, it may be the last pruned vertex
    if (contourtree_augmented::NoSuchElement(superarcTo))
    { // no superarc
      // if it occurred on the final round, it's the global root and is shown as the NULL node
      if (whichRoundPortal.Get(superarcFrom) == this->NRounds)
      { // root node
        outstream << "\tSN" << std::setw(1) << superarcFrom << " -> SA" << std::setw(1) << superarc
                  << " [label=\"S" << std::setw(1) << superarc << "\",style=dotted]\n";
        outstream << "\tSN" << std::setw(1) << superarc << " -> NULL[label=\"S" << std::setw(1)
                  << superarc << "\",style=dotted]\n";
      } // root node
      else
      { // attachment point
        // otherwise, the target is actually a superarc vertex not a supernode vertex
        // so we use the regular ID to retrieve the superparent which tells us which superarc we insert into
        vtkm::Id regularFrom = supernodesPortal.Get(superarcFrom);
        superarcTo = superparentsPortal.Get(regularFrom);

        // output a suitable edge
        outstream << "\tSN" << std::setw(1) << superarcFrom << " -> SA" << std::setw(1)
                  << superarcTo << "[label=\"S" << std::setw(1) << superarc << "\",style=dotted]\n";
      } // attachment point
    }   // no superarc

    else
    { // there is a superarc
      // retrieve the ascending flag
      bool ascendingSuperarc = contourtree_augmented::IsAscending(superarcTo);

      // strip out the flags
      superarcTo = contourtree_augmented::MaskedIndex(superarcTo);

      // how we print depends on whether the superarc ascends
      outstream << "\tSN" << std::setw(1) << (ascendingSuperarc ? superarcTo : superarcFrom)
                << " -> SA" << std::setw(1) << superarc << " [label=\"S" << std::setw(1) << superarc
                << "\"" << (ascendingSuperarc ? ",dir=\"back\"" : "") << ",arrowhead=\"none\"]\n";
      outstream << "\tSA" << std::setw(1) << superarc << " -> SN" << std::setw(1)
                << (ascendingSuperarc ? superarcFrom : superarcTo) << " [label=\"S" << std::setw(1)
                << superarc << "\"" << (ascendingSuperarc ? ",dir=\"back\"" : "")
                << ",arrowhead=\"none\"]\n";
    } // there is a superarc
  }   // per superarc

  outstream << "\t// Hyperarcs\n";
  // now loop through the hyperarcs to draw them
  for (vtkm::Id hyperarc = 0; hyperarc < this->Hyperarcs.GetNumberOfValues(); hyperarc++)
  { // per hyperarc
    // retrieve ID of target hypernode
    vtkm::Id hyperarcFrom = hypernodesPortal.Get(hyperarc);
    vtkm::Id hyperarcTo = hyperarcsPortal.Get(hyperarc);

    // if this is true, it is the last pruned vertex & needs a hyperarc to the root
    if (contourtree_augmented::NoSuchElement(hyperarcTo))
      outstream << "\tSN" << std::setw(1) << hyperarcFrom << " -> NULL[label=\"H" << std::setw(1)
                << hyperarc << "\",penwidth=5.0,style=dotted]\n";
    else
    { // not the last one
      // otherwise, retrieve the ascending flag
      bool ascendingHyperarc = contourtree_augmented::IsAscending(hyperarcTo);

      // strip out the flags
      hyperarcTo = contourtree_augmented::MaskedIndex(hyperarcTo);

      // how we print depends on whether the hyperarc ascends
      outstream << "\tSN" << std::setw(1) << (ascendingHyperarc ? hyperarcTo : hyperarcFrom)
                << " -> SN" << std::setw(1) << (ascendingHyperarc ? hyperarcFrom : hyperarcTo)
                << "[label=\"H" << std::setw(1) << hyperarc << "\",penwidth=5.0,dir=\"back\"]\n";
    } // not the last one
  }   // per hyperarc

  // print the footer information
  outstream << "\t}\n";

  return std::string("HierarchicalContourTree<FieldType>::PrintDotSuperStructure() Complete");
} // PrintDotSuperStructure

/// debug routine
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::DebugPrint(std::string message,
                                                           const char* fileName,
                                                           long lineNum) const
{ // DebugPrint
  std::stringstream resultStream;
  resultStream << std::endl;
  resultStream << "[CUTHERE]-------------------------------" << std::endl;
  resultStream << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
               << lineNum << std::endl;
  resultStream << std::left << std::string(message) << std::endl;
  resultStream << "Hierarchical Contour Tree Contains:     " << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::endl;

  vtkm::worklet::contourtree_augmented::PrintHeader(this->RegularNodeGlobalIds.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Regular Nodes (global ID)", this->RegularNodeGlobalIds, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintValues(
    "Data Values", this->DataValues, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Regular Node Sort Order", this->RegularNodeSortOrder, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Superparents (unsorted)", this->Superparents, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Supernode ID (if any)", this->Regular2Supernode, -1, resultStream);
  resultStream << std::endl;
  vtkm::worklet::contourtree_augmented::PrintHeader(this->Supernodes.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Supernodes (regular index)", this->Supernodes, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Superarcs (supernode index)", this->Superarcs, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Hyperparents (hypernode index)", this->Hyperparents, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Hypernode ID (if any)", this->Super2Hypernode, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Which Round", this->WhichRound, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Which Iteration", this->WhichIteration, -1, resultStream);
  resultStream << std::endl;
  vtkm::worklet::contourtree_augmented::PrintHeader(this->Hypernodes.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Hypernodes (supernode index)", this->Hypernodes, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Hyperarcs (supernode index)", this->Hyperarcs, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Superchildren", this->Superchildren, -1, resultStream);
  resultStream << std::endl;
  resultStream << "nRounds: " << this->NumRounds << std::endl;
  vtkm::worklet::contourtree_augmented::PrintHeader(
    this->NumRegularNodesInRound.GetNumberOfValues(), resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "nRegular Nodes In Round", this->NumRegularNodesInRound, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "nSupernodes In Round", this->NumSupernodesInRound, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "nHypernodes In Round", this->NumHypernodesInRound, -1, resultStream);
  //resultStream << "Owned Regular Vertices: " << this->NumOwnedRegularVertices << std::endl;
  vtkm::worklet::contourtree_augmented::PrintHeader(this->NumIterations.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "nIterations", this->NumIterations, -1, resultStream);
  for (vtkm::Id whichRound = 0; whichRound < this->NumIterations.GetNumberOfValues(); whichRound++)
  { // per round
    resultStream << "Round " << whichRound << std::endl;
    vtkm::worklet::contourtree_augmented::PrintHeader(
      this->FirstSupernodePerIteration[static_cast<std::size_t>(whichRound)].GetNumberOfValues(),
      resultStream);
    vtkm::worklet::contourtree_augmented::PrintIndices(
      "First Supernode Per Iteration",
      this->FirstSupernodePerIteration[static_cast<std::size_t>(whichRound)],
      -1,
      resultStream);
    vtkm::worklet::contourtree_augmented::PrintIndices(
      "First Hypernode Per Iteration",
      this->FirstHypernodePerIteration[static_cast<std::size_t>(whichRound)],
      -1,
      resultStream);
    resultStream << std::endl;
  } // per round
  return resultStream.str();
} // DebugPrint

template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::PrintTreeStats() const
{ // PrintTreeStats
  std::stringstream resultStream;
  resultStream << std::setw(42) << std::left << "    NumRounds"
               << ": " << this->NumRounds << std::endl;
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "    NumIterations", this->NumIterations, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "    NumRegularNodesInRound", this->NumRegularNodesInRound, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "    NumSupernodesInRound", this->NumSupernodesInRound, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "    NumHypernodesInRound", this->NumHypernodesInRound, -1, resultStream);

  return resultStream.str();
} // PrintTreeStats


// modified version of dumpSuper() that also gives volume counts
template <typename FieldType>
std::string HierarchicalContourTree<FieldType>::DumpVolumes(
  const vtkm::worklet::contourtree_augmented::IdArrayType& supernodes,
  const vtkm::worklet::contourtree_augmented::IdArrayType& superarcs,
  const vtkm::worklet::contourtree_augmented::IdArrayType& regularNodeGlobalIds,
  vtkm::Id totalVolume,
  const vtkm::worklet::contourtree_augmented::IdArrayType& intrinsicVolume,
  const vtkm::worklet::contourtree_augmented::IdArrayType& dependentVolume)
{ // DumpVolumes()
  // a local string stream to build the output
  std::stringstream outStream;

  // header info
  outStream << "============" << std::endl;
  outStream << "Contour Tree" << std::endl;

  // loop through all superarcs.
  // We use regular ReadPortals here since this requires access to many values anyways
  auto supernodesPortal = supernodes.ReadPortal();
  auto regularNodeGlobalIdsPortal = regularNodeGlobalIds.ReadPortal();
  auto superarcsPortal = superarcs.ReadPortal();
  auto intrinsicVolumePortal = intrinsicVolume.ReadPortal();
  auto dependentVolumePortal = dependentVolume.ReadPortal();
  for (vtkm::Id supernode = 0; supernode < supernodes.GetNumberOfValues(); supernode++)
  { // per supernode
    // convert all the way down to global regular IDs
    vtkm::Id fromRegular = supernodesPortal.Get(supernode);
    vtkm::Id fromGlobal = regularNodeGlobalIdsPortal.Get(fromRegular);

    // retrieve the superarc target
    vtkm::Id toSuper = superarcsPortal.Get(supernode);

    // if it is NO_SUCH_ELEMENT, it is the root or an attachment point
    // for an augmented tree, it can only be the root
    // in any event, we don't want to print them
    if (vtkm::worklet::contourtree_augmented::NoSuchElement(toSuper))
    {
      continue;
    }
    // now break out the ascending flag & the underlying ID
    bool superarcAscends = vtkm::worklet::contourtree_augmented::IsAscending(toSuper);
    toSuper = vtkm::worklet::contourtree_augmented::MaskedIndex(toSuper);
    vtkm::Id toRegular = supernodesPortal.Get(toSuper);
    vtkm::Id toGlobal = regularNodeGlobalIdsPortal.Get(toRegular);

    // compute the weights
    vtkm::Id weight = dependentVolumePortal.Get(supernode);
    // -1 because the validation output does not count the supernode for the superarc
    vtkm::Id arcWeight = intrinsicVolumePortal.Get(supernode) - 1;
    vtkm::Id counterWeight = totalVolume - weight + arcWeight;

    // orient with high end first
    if (superarcAscends)
    { // ascending superarc
      outStream << "H: " << std::setw(VOLUME_PRINT_WIDTH) << toGlobal;
      outStream << " L: " << std::setw(VOLUME_PRINT_WIDTH) << fromGlobal;
      outStream << " VH: " << std::setw(VOLUME_PRINT_WIDTH) << weight;
      outStream << " VR: " << std::setw(VOLUME_PRINT_WIDTH) << arcWeight;
      outStream << " VL: " << std::setw(VOLUME_PRINT_WIDTH) << counterWeight;
      outStream << std::endl;
    } // ascending superarc
    else
    { // descending superarc
      outStream << "H: " << std::setw(VOLUME_PRINT_WIDTH) << fromGlobal;
      outStream << " L: " << std::setw(VOLUME_PRINT_WIDTH) << toGlobal;
      outStream << " VH: " << std::setw(VOLUME_PRINT_WIDTH) << counterWeight;
      outStream << " VR: " << std::setw(VOLUME_PRINT_WIDTH) << arcWeight;
      outStream << " VL: " << std::setw(VOLUME_PRINT_WIDTH) << weight;
      outStream << std::endl;
    } // descending superarc

  } // per supernode
  // return the string
  return outStream.str();
} // DumpVolumes()


} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
