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

#ifndef vtk_m_worklet_contourtree_distributed_hierarchical_augmenter_update_hyperstructure_set_hyperarcs_and_nodes_worklet_h
#define vtk_m_worklet_contourtree_distributed_hierarchical_augmenter_update_hyperstructure_set_hyperarcs_and_nodes_worklet_h

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

/// Worklet used in HierarchicalAugmenter::UpdateHyperstructure to set the hyperarcs and hypernodes
class UpdateHyperstructureSetHyperarcsAndNodesWorklet : public vtkm::worklet::WorkletMapField
{
public:
  /// Control signature for the worklet
  using ControlSignature = void(FieldIn baseTreeHypernodes,       // input
                                FieldIn baseTreeHyperarcs,        // input
                                WholeArrayIn newSupernodeIds,     // input
                                FieldOut augmentedTreeHypernodes, // output
                                FieldOut augmentedTreeHyperarcs   // output
  );
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  // Default Constructor
  VTKM_EXEC_CONT
  UpdateHyperstructureSetHyperarcsAndNodesWorklet() {}

  template <typename InFieldPortalType>
  VTKM_EXEC void operator()(
    const vtkm::Id& oldHypernodeSuperId,    // same as baseTree->hypernodes[hypernode]
    const vtkm::Id& oldTargetSuperIdMasked, // same as baseTree->hyperarcs[hypernode]
    const InFieldPortalType& newSupernodeIdsPortal,
    vtkm::Id& outAugmentedTreeHypernodesValue, // same as augmentedTree->hypernodes[hypernode] = ...
    vtkm::Id& outAugmentedTreeHyperarcsValue   // same as augmentedTree->hyperarcs[hypernode] = ...
  ) const
  {
    // per hypernode
    // retrieve existing values which are in old supernode Ids
    // oldHypernodeSuperId and oldTargetSuperIdMasked are set by the worklt
    // strip out the ascending flag & the flag for root hyperarc
    bool isRootHyperarc =
      vtkm::worklet::contourtree_augmented::NoSuchElement(oldTargetSuperIdMasked);
    bool hyperarcAscends =
      vtkm::worklet::contourtree_augmented::IsAscending(oldTargetSuperIdMasked);
    vtkm::Id oldTargetSuperId =
      vtkm::worklet::contourtree_augmented::MaskedIndex(oldTargetSuperIdMasked);

    // lookup new values
    vtkm::Id newHypernodeSuperId = newSupernodeIdsPortal.Get(oldHypernodeSuperId);
    vtkm::Id newTargetSuperId = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    if (!isRootHyperarc)
    { // not the root
      // lookup the new ID
      newTargetSuperId = newSupernodeIdsPortal.Get(oldTargetSuperId);
      if (hyperarcAscends)
      {
        newTargetSuperId |= vtkm::worklet::contourtree_augmented::IS_ASCENDING;
      }
    } // not the root

    // now store them
    outAugmentedTreeHypernodesValue = newHypernodeSuperId;
    outAugmentedTreeHyperarcsValue = newTargetSuperId;

    // In serial this worklet implements the following operation
    /*
    for (vtkm::Id hypernode = 0; hypernode < augmentedTree->hypernodes.size(); hypernode++)
    { // per hypernode
      // retrieve existing values which are in old supernode IDs
      vtkm::Id oldHypernodeSuperID = baseTree->hypernodes[hypernode];
      vtkm::Id oldTargetSuperID = baseTree->hyperarcs[hypernode];
      // strip out the ascending flag & the flag for root hyperarc
      bool isRootHyperarc = noSuchElement(oldTargetSuperID);
      bool hyperarcAscends = isAscending(oldTargetSuperID);
      oldTargetSuperID = maskedIndex(oldTargetSuperID);

      // lookup new values
      vtkm::Id newHypernodeSuperID = newSupernodeIDs[oldHypernodeSuperID];
      vtkm::Id newTargetSuperID = NO_SUCH_ELEMENT;
      if (!isRootHyperarc)
      { // not the root
        // lookup the new ID
        newTargetSuperID = newSupernodeIDs[oldTargetSuperID];
        if (hyperarcAscends)
          newTargetSuperID |= IS_ASCENDING;
      } // not the root

      // now store them
      augmentedTree->hypernodes[hypernode] = newHypernodeSuperID;
      augmentedTree->hyperarcs[hypernode] = newTargetSuperID;
    } // per hypernode

    */
  } // operator()()
};  // UpdateHyperstructureSetHyperarcsAndNodesWorklet

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
