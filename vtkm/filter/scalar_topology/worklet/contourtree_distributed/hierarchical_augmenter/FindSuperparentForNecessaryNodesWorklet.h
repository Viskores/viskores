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
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef vtk_m_worklet_contourtree_distributed_hierarchical_augmenter_find_superparent_for_necessary_nodes_worklet_h
#define vtk_m_worklet_contourtree_distributed_hierarchical_augmenter_find_superparent_for_necessary_nodes_worklet_h

#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_augmenter
{

/// Worklet used in HierarchicalAugmenter::CopyBaseRegularStructure for
/// finding the superparent for each node needed
class FindSuperparentForNecessaryNodesWorklet : public vtkm::worklet::WorkletMapField
{
public:
  /// Control signature for the worklet
  using ControlSignature = void(
    FieldIn baseTreeRegularNodeGlobalIds, // input domain
    FieldIn baseTreeSuperparents,         // input
    FieldIn baseTreeDataValues,           // input
    WholeArrayIn baseTreeSuperarcs,       // input
    WholeArrayIn newSupernodeIds,         // input
    // Execution objects from the AugmentedTree to use the FindRegularByGlobal
    // and FindSuperArcForUnknownNode for the hierarchical tree.
    ExecObject findRegularByGlobal,
    ExecObject findSuperArcForUnknownNode,
    // Output arrays to populate
    FieldOut regularSuperparents, // output
    FieldOut regularNodesNeeded   // output
  );
  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9);
  using InputDomain = _1;

  /// Default Constructor
  VTKM_EXEC_CONT
  FindSuperparentForNecessaryNodesWorklet() {}

  /// operator() of the workelt
  template <typename InFieldPortalType,
            typename FieldType,
            typename ExecObjectType1,
            typename ExecObjectType2>
  VTKM_EXEC void operator()(
    const vtkm::Id& regularNode,     // InputIndex a.k.a out loop index
    const vtkm::Id& globalRegularId, // same as baseTree->regularNodeGlobalIDs[regularNode];
    const vtkm::Id& oldSuperparent,  // same as baseTree->superparents[regularNode];
    const FieldType& dataValue,      // same as baseTree->dataValues[regularNode]
    const InFieldPortalType& baseTreeSuperarcsPortal,
    const InFieldPortalType& newSupernodeIdsPortal,
    const ExecObjectType1& findRegularByGlobal, // Execution object to call FindRegularByGlobal
    const ExecObjectType2&
      findSuperArcForUnknownNode, // Execution object to call FindSuperArcForUnknownNode
    vtkm::Id&
      regularSuperparentsValue, // same as regularSuperparents[regularNode]  = ... (set on output)
    vtkm::Id&
      regularNodesNeededValue // same as regularNodesNeeded[regularNode]  = ... (set on output)
  ) const
  {
    // per regular node
    // retrieve the index (globalRegularId set on input)
    // first check to see if it is already present (newRegularId set on input)
    vtkm::Id newRegularId = findRegularByGlobal.FindRegularByGlobal(globalRegularId);

    // if it fails this test, then it's already in tree
    if (vtkm::worklet::contourtree_augmented::NoSuchElement(newRegularId))
    { // not yet in tree
      // since it's not in the tree, we want to find where it belongs
      // to do so, we need to find an "above" and "below" node for it. Since it exists in the old tree, it belongs to a superarc, and we
      // can use the ends of the superarc as above and below to do the searching
      // oldSuperparent set on Input vtkm::Id oldSuperparent = baseTree->superparents[regularNode];
      vtkm::Id oldSuperarc = baseTreeSuperarcsPortal.Get(oldSuperparent);

      // break the superarc into the flag and the target
      // NOTE that we do not test for NO_SUCH_ELEMENT as all attachment points and the root are guaranteed to be present already,
      // and have therefore been excluded by the if statement already
      vtkm::Id oldSuperTarget = vtkm::worklet::contourtree_augmented::MaskedIndex(oldSuperarc);
      bool ascendingSuperarc = vtkm::worklet::contourtree_augmented::IsAscending(oldSuperarc);

      // convert both from and to into new supernode IDs
      vtkm::Id newSuperparent = newSupernodeIdsPortal.Get(oldSuperparent);
      vtkm::Id newSuperTarget = newSupernodeIdsPortal.Get(oldSuperTarget);

      // retrieve the data value (dataValue set on input)
      // now test and retrieve, with above = target if ascending, &c.
      if (ascendingSuperarc)
      {
        regularSuperparentsValue = findSuperArcForUnknownNode.FindSuperArcForUnknownNode(
          globalRegularId, dataValue, newSuperTarget, newSuperparent);
      }
      else
      {
        regularSuperparentsValue = findSuperArcForUnknownNode.FindSuperArcForUnknownNode(
          globalRegularId, dataValue, newSuperparent, newSuperTarget);
      }
      // either way, we set the index array to the index
      regularNodesNeededValue = regularNode;
    } // not yet in tree
    // Set to NO_SUCH_ELEMENT by default. By doing this in the worklet we an avoid having to
    // initialize the output arrays first and we can use FieldIn instead of FieldInOut
    else
    {
      regularSuperparentsValue = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
      regularNodesNeededValue = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    }

    // In serial this worklet implements the following operation
    /*
    // now loop, finding the superparent for each node needed
      for (vtkm::Id regularNode = 0; regularNode < baseTree->regularNodeGlobalIDs.size(); regularNode++)
      { // per regular node
        // retrieve the index
        vtkm::Id globalRegularID = baseTree->regularNodeGlobalIDs[regularNode];

        // first check to see if it is already present
        vtkm::Id newRegularID = augmentedTree->FindRegularByGlobal(globalRegularID);

        // if it fails this test, then it's already in tree
        if (noSuchElement(newRegularID))
        { // not yet in tree
          //       std::cout << "Not yet in tree" << std::endl;
          // since it's not in the tree, we want to find where it belongs
          // to do so, we need to find an "above" and "below" node for it. Since it exists in the old tree, it belongs to a superarc, and we
          // can use the ends of the superarc as above and below to do the searching
          vtkm::Id oldSuperparent = baseTree->superparents[regularNode];
          vtkm::Id oldSuperarc = baseTree->superarcs[oldSuperparent];

          // break the superarc into the flag and the target
          // NOTE that we do not test for NO_SUCH_ELEMENT as all attachment points and the root are guaranteed to be present already,
          // and have therefore been excluded by the if statement already
          vtkm::Id oldSuperTarget = maskedIndex(oldSuperarc);
          bool ascendingSuperarc = isAscending(oldSuperarc);

          // convert both from and to into new supernode IDs
          vtkm::Id newSuperparent = newSupernodeIDs[oldSuperparent];
          vtkm::Id newSuperTarget = newSupernodeIDs[oldSuperTarget];

          // retrieve the data value
          dataType dataValue = baseTree->dataValues[regularNode];

          // now test and retrieve, with above = target if ascending, &c.
          if (ascendingSuperarc)
            regularSuperparents[regularNode] = augmentedTree->FindSuperArcForUnknownNode(globalRegularID, dataValue, newSuperTarget, newSuperparent);
          else
            regularSuperparents[regularNode] = augmentedTree->FindSuperArcForUnknownNode(globalRegularID, dataValue, newSuperparent, newSuperTarget);

          // either way, we set the index array to the index
          regularNodesNeeded[regularNode] = regularNode;
        } // not yet in tree
      } // per regular node
    */
  } // operator()()

}; // FindSuperparentForNecessaryNodesWorklet

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
