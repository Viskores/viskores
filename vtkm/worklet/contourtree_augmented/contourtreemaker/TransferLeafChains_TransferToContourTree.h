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

#ifndef vtkm_worklet_contourtree_augmented_contourtree_maker_inc_transfer_leaf_chains_transfer_to_contour_tree_h
#define vtkm_worklet_contourtree_augmented_contourtree_maker_inc_transfer_leaf_chains_transfer_to_contour_tree_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace contourtree_maker_inc
{

// Worklet to transfer leaf chains to contour tree"
// a. for leaves (tested by degree),
//              i.      we use inbound as the hyperarc
//              ii.     we use inwards as the superarc
//              iii.we use self as the hyperparent
// b. for regular vertices pointing to a leaf (test by outbound's degree),
//              i.      we use outbound as the hyperparent
//              ii. we use inwards as the superarc
// c. for all other vertics
//              ignore
template <typename DeviceAdapter>
class TransferLeafChains_TransferToContourTree : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn activeSupernodes,                // (input)
                                WholeArrayOut contourTreeHyperparents,   // (output)
                                WholeArrayOut contourTreeHyperarcs,      // (output)
                                WholeArrayOut contourTreeSuperarcs,      // (output)
                                WholeArrayOut contourTreeWhenTransferred // (output)
                                );

  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5);
  using InputDomain = _1;

  // vtkm only allows 9 parameters for the operator so we need to do these inputs manually via the constructor
  using IdPortalType =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapter>::PortalConst;
  IdPortalType outdegreePortal;
  IdPortalType indegreePortal;
  IdPortalType outboundPortal;
  IdPortalType inboundPortal;
  IdPortalType inwardsPortal;
  vtkm::Id nIterations;
  bool isJoin;


  // Default Constructor
  TransferLeafChains_TransferToContourTree(const vtkm::Id NIterations,
                                           const bool IsJoin,
                                           const IdArrayType& outdegree,
                                           const IdArrayType& indegree,
                                           const IdArrayType& outbound,
                                           const IdArrayType& inbound,
                                           const IdArrayType& inwards)
    : nIterations(NIterations)
    , isJoin(IsJoin)
  {
    outdegreePortal = outdegree.PrepareForInput(DeviceAdapter());
    indegreePortal = indegree.PrepareForInput(DeviceAdapter());
    outboundPortal = outbound.PrepareForInput(DeviceAdapter());
    inboundPortal = inbound.PrepareForInput(DeviceAdapter());
    inwardsPortal = inwards.PrepareForInput(DeviceAdapter());
  }


  template <typename OutFieldPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& superID,
                            const vtkm::Id /*activeID*/, // FIXME: Remove unused parameter?
                            const OutFieldPortalType& contourTreeHyperparentsPortal,
                            const OutFieldPortalType& contourTreeHyperarcsPortal,
                            const OutFieldPortalType& contourTreeSuperarcsPortal,
                            const OutFieldPortalType& contourTreeWhenTransferredPortal) const
  {
    if ((outdegreePortal.Get(superID) == 0) && (indegreePortal.Get(superID) == 1))
    { // a leaf
      contourTreeHyperparentsPortal.Set(superID, superID | (isJoin ? 0 : IS_ASCENDING));
      contourTreeHyperarcsPortal.Set(
        superID, maskedIndex(inboundPortal.Get(superID)) | (isJoin ? 0 : IS_ASCENDING));
      contourTreeSuperarcsPortal.Set(
        superID, maskedIndex(inwardsPortal.Get(superID)) | (isJoin ? 0 : IS_ASCENDING));
      contourTreeWhenTransferredPortal.Set(superID, nIterations | IS_HYPERNODE);
    } // a leaf
    else
    { // not a leaf
      // retrieve the out neighbour
      vtkm::Id outNeighbour = maskedIndex(outboundPortal.Get(superID));

      // test whether outneighbour is a leaf
      if ((outdegreePortal.Get(outNeighbour) != 0) || (indegreePortal.Get(outNeighbour) != 1))
      {
      }
      else
      {
        // set superarc, &c.
        contourTreeSuperarcsPortal.Set(
          superID, maskedIndex(inwardsPortal.Get(superID)) | (isJoin ? 0 : IS_ASCENDING));
        contourTreeHyperparentsPortal.Set(superID, outNeighbour | (isJoin ? 0 : IS_ASCENDING));
        contourTreeWhenTransferredPortal.Set(superID, nIterations | IS_SUPERNODE);
      }
    } // not a leaf


    // In serial this worklet implements the following operation
    /*
      for (vtkm::Id activeID = 0; activeID < activeSupernodes.GetNumberOfValues(); activeID++)
      { // per active supernode
        // retrieve the supernode ID
        vtkm::Id superID = activeSupernodes[activeID];

        // test for leaf
        if ((outdegree[superID] == 0) && (indegree[superID] == 1))
                { // a leaf
                contourTree.hyperparents[superID] = superID | (isJoin ? 0 : IS_ASCENDING);
                contourTree.hyperarcs[superID] = maskedIndex(inbound[superID]) | (isJoin ? 0 : IS_ASCENDING);
                contourTree.superarcs[superID] = maskedIndex(inwards[superID]) | (isJoin ? 0 : IS_ASCENDING);
                contourTree.whenTransferred[superID] = nIterations | IS_HYPERNODE;
                } // a leaf
        else
                { // not a leaf
                // retrieve the out neighbour
                vtkm::Id outNeighbour = maskedIndex(outbound[superID]);

                // test whether outneighbour is a leaf
                if ((outdegree[outNeighbour] != 0) || (indegree[outNeighbour] != 1))
                        continue;

                // set superarc, &c.
                contourTree.superarcs[superID] = maskedIndex(inwards[superID]) | (isJoin ? 0 : IS_ASCENDING);
                contourTree.hyperparents[superID] = outNeighbour | (isJoin ? 0 : IS_ASCENDING);
                contourTree.whenTransferred[superID] = nIterations | IS_SUPERNODE;
                } // not a leaf
      } // per active supernode

      */
  }

}; // TransferLeafChains_TransferToContourTree

} // namespace contourtree_maker_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
