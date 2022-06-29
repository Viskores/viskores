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

#ifndef vtk_m_worklet_contourtree_distributed_hierarchical_augmenter_create_superarcs_worklet_h
#define vtk_m_worklet_contourtree_distributed_hierarchical_augmenter_create_superarcs_worklet_h

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

/// Worklet used to implement the main part of HierarchicalAugmenter::CreateSuperarcs
/// Connect superarcs for the level & set hyperparents & superchildren count, whichRound,
/// whichIteration, super2hypernode
class CreateSuperarcsWorklet : public vtkm::worklet::WorkletMapField
{
public:
  // TODO: Check if augmentedTreeFirstSupernodePerIteration could be changed to WholeArrayOut or if we need the In to preserve orignal values

  /// Control signature for the worklet
  /// @param[in] supernodeSorter input domain. We need access to InputIndex and InputIndex+1,
  ///                           therefore this is a WholeArrayIn transfer.
  /// @param[in] superparentSet WholeArrayIn because we need access to superparentSet[supernodeSorter[InputIndex]]
  ///                           and superparentSet[supernodeSorter[InputIndex+1]].
  /// @param[in] baseTreeSuperarcs WholeArrayIn because we need access to baseTreeSuperarcsPortal.Get(superparentOldSuperId)
  ///                           While this could be done with fancy array magic, it would require a sequence of multiple
  ///                           fancy arrays and would likely not be cheaper then computing things in the worklet.
  /// @param[in] newSupernodeIds WholeArrayIn because we need to access newSupernodeIdsPortal.Get(oldTargetSuperId)
  ///                           where oldTargetSuperId is the unmasked baseTreeSuperarcsPortal.Get(superparentOldSuperId)
  /// @param[in] baseTreeSupernodes WholeArrayIn because we need to access baseTreeSupernodesPortal.Get(superparentOldSuperId);
  /// @param[in] baseTreeRegularNodeGlobalIds WholeArrayIn because we need to access
  ///                           baseTreeRegularNodeGlobalIdsPortal.Get(superparentOldSuperId);
  /// @param[in] globalRegularIdSet FieldInd. Permute globalRegularIdSet with supernodeSorter in order to allow this to be a FieldIn.
  /// @param[in] baseTreeSuper2Hypernode WholeArrayIn because we need to access
  ///                           baseTreeSuper2HypernodePortal.Get(superparentOldSuperId)
  /// @param[in] baseTreeWhichIteration WholeArrayIn because we need to access baseTreeWhichIterationPortal.Get(superparentOldSuperId)
  ///                           and baseTreeWhichIterationPortal.Get(superparentOldSuperId+1)
  /// @param[in] augmentedTreeSuperarcsView  output view of  this->AugmentedTree->Superarcs with
  ///                           vtkm::cont::make_ArrayHandleView(this->AugmentedTree->Superarcs,
  ///                           numSupernodesAlready, this->SupernodeSorter.GetNumberOfValues()).
  ///                           By using this view allows us to do this one as a FieldOut and it effectively the
  ///                           same as accessing the array at the newSuppernodeId location.
  /// @param[in] augmentedTreeFirstSupernodePerIteration WholeArrayInOut because we need to update multiple locations.
  ///                           In is used to preseve original values. Set to augmentedTree->firstSupernodePerIteration[roundNumber].
  /// @param[in] augmentedTreeSuper2hypernode FieldOut. Output view of this->AugmentedTree->Super2Hypernode
  ///                           vtkm::cont::make_ArrayHandleView(this->AugmentedTree->Super2Hypernode,
  ///                           numSupernodesAlready, this->SupernodeSorter.GetNumberOfValues()).
  ///                           By using this view allows us to do this one as a FieldOut and it effectively the
  ///                           same as accessing the array at the newSuppernodeId location.
  using ControlSignature = void(
    WholeArrayIn supernodeSorter,
    WholeArrayIn superparentSet,                             // input
    WholeArrayIn baseTreeSuperarcs,                          // input
    WholeArrayIn newSupernodeIds,                            // input
    WholeArrayIn baseTreeSupernodes,                         // input
    WholeArrayIn baseTreeRegularNodeGlobalIds,               // input
    FieldIn globalRegularIdSet,                              // input
    WholeArrayIn baseTreeSuper2Hypernode,                    // input
    WholeArrayIn baseTreeWhichIteration,                     // input
    FieldOut augmentedTreeSuperarcsView,                     // output
    WholeArrayInOut augmentedTreeFirstSupernodePerIteration, // input/output
    FieldOut augmentedTreeSuper2hypernode                    // ouput
  );
  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12);
  using InputDomain = _1;

  /// Default Constructor
  /// @param[in] numSupernodesAlready Set to vtkm::cont::ArrayGetValue(0, this->AugmentedTree->FirstSupernodePerIteration[roundNumber]);
  /// @param[in] baseTreeNumRounds Set to this->BaseTree->NumRounds
  /// @param[in] augmentedTreeNumIterations Set to  vtkm::cont::ArrayGetValue(roundNumber, this->AugmentedTree->NumIterations);
  /// @param[in] roundNumber Set the current round
  /// @param[in] numAugmentedTreeSupernodes Set to augmentedTreeSupernodes this->AugmentedTree->Supernodes.GetNumberOfValues();
  VTKM_EXEC_CONT
  CreateSuperarcsWorklet(const vtkm::Id& numSupernodesAlready,
                         const vtkm::Id& baseTreeNumRounds,
                         const vtkm::Id& augmentedTreeNumIterations,
                         const vtkm::Id& roundNumber,
                         const vtkm::Id& numAugmentedTreeSupernodes)
    : NumSupernodesAlready(numSupernodesAlready)
    , BaseTreeNumRounds(baseTreeNumRounds)
    , AugmentedTreeNumIterations(augmentedTreeNumIterations)
    , RoundNumber(roundNumber)
    , NumAugmentedTreeSupernodes(numAugmentedTreeSupernodes)
  {
  }

  /// operator() of the workelt
  template <typename InFieldPortalType, typename InOutFieldPortalType>
  VTKM_EXEC void operator()(
    const vtkm::Id& supernode, // InputIndex of supernodeSorter
    const InFieldPortalType& supernodeSorterPortal,
    const InFieldPortalType& superparentSetPortal,
    const InFieldPortalType& baseTreeSuperarcsPortal,
    const InFieldPortalType& newSupernodeIdsPortal,
    const InFieldPortalType& baseTreeSupernodesPortal,
    const InFieldPortalType& baseTreeRegularNodeGlobalIdsPortal,
    const vtkm::Id& globalRegularIdSetValue,
    const InFieldPortalType& baseTreeSuper2HypernodePortal,
    const InFieldPortalType& baseTreeWhichIterationPortal,
    vtkm::Id& augmentedTreeSuperarcsValue, // same as augmentedTree->superarcs[newSupernodeId]
    const InOutFieldPortalType&
      augmentedTreeFirstSupernodePerIterationPortal, // augmentedTree->firstSupernodePerIteration[roundNumber]
    vtkm::Id& augmentedTreeSuper2hypernodeValue) const
  {
    // per supernode in the set
    // retrieve the index from the sorting index array
    vtkm::Id supernodeSetIndex = supernodeSorterPortal.Get(supernode);

    // work out the new supernode Id. We have this defined on the outside as a fancy array handle,
    // however, using the fancy handle here would not really make a performance differnce and
    // computing it here is more readable
    vtkm::Id newSupernodeId = this->NumSupernodesAlready + supernode;

    // NOTE: The newRegularId is no longer needed here since all parts
    //       that used it in the worklet have been moved outside
    // vtkm::Id newRegularId = newSupernodeId;

    // NOTE: This part has been moved out of the worklet and is performed using standard vtkm copy constructs
    // // setting the supernode's regular Id is now trivial
    // augmentedTreeSupernodesPortal.Set(newSupernodeId, newRegularId);

    // retrieve the ascending flag from the superparent
    vtkm::Id superparentSetVal = superparentSetPortal.Get(supernodeSetIndex);
    // get the ascending flag from the parent
    bool superarcAscends = vtkm::worklet::contourtree_augmented::IsAscending(superparentSetVal);
    // strip the ascending flag from the superparent.
    vtkm::Id superparentOldSuperId =
      vtkm::worklet::contourtree_augmented::MaskedIndex(superparentSetVal);

    // setting the superarc is done the usual way.  Our sort routine has ended up
    // with the supernodes arranged in either ascending or descending order
    // inwards along the parent superarc (as expressed by the superparent Id).
    // Each superarc except the last in the segment points to the next one:
    // the last one points to the target of the original superarc.
    // first test to see if we're the last in the array
    if (supernode == supernodeSorterPortal.GetNumberOfValues() - 1)
    { // last in the array
      // special case for root of entire tree at end of top level
      if (RoundNumber == this->BaseTreeNumRounds)
      {
        augmentedTreeSuperarcsValue = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
      }
      else
      { // not the tree root
        // retrieve the target of the superarc from the base tree (masking to strip out the ascending flag)
        vtkm::Id oldTargetSuperId = vtkm::worklet::contourtree_augmented::MaskedIndex(
          baseTreeSuperarcsPortal.Get(superparentOldSuperId));
        // convert to a new supernode Id
        vtkm::Id newTargetSuperId = newSupernodeIdsPortal.Get(oldTargetSuperId);
        // add the ascending flag back in and store in the array
        augmentedTreeSuperarcsValue = newTargetSuperId |
          (superarcAscends ? vtkm::worklet::contourtree_augmented::IS_ASCENDING : 0x00);
      } // not the tree root
      // since there's an extra entry in the firstSupernode array as a sentinel, set it
      augmentedTreeFirstSupernodePerIterationPortal.Set(this->AugmentedTreeNumIterations,
                                                        NumAugmentedTreeSupernodes);
    } // last in the array
    else if (superparentOldSuperId !=
             vtkm::worklet::contourtree_augmented::MaskedIndex(
               superparentSetPortal.Get(supernodeSorterPortal.Get(supernode + 1))))
    { // last in the segment
      // retrieve the target of the superarc from the base tree (masking to strip out the ascending flag)
      vtkm::Id oldTargetSuperId = vtkm::worklet::contourtree_augmented::MaskedIndex(
        baseTreeSuperarcsPortal.Get(superparentOldSuperId));
      // convert to a new supernode Id
      vtkm::Id newTargetSuperId = newSupernodeIdsPortal.Get(oldTargetSuperId);
      // add the ascending flag back in and store in the array
      augmentedTreeSuperarcsValue = newTargetSuperId |
        (superarcAscends ? vtkm::worklet::contourtree_augmented::IS_ASCENDING : 0x00);

      // since we're the last in the segment, we check to see if we are at the end of an iteration
      vtkm::Id iterationNumber = vtkm::worklet::contourtree_augmented::MaskedIndex(
        baseTreeWhichIterationPortal.Get(superparentOldSuperId));
      vtkm::Id iterationNumberOfNext = vtkm::worklet::contourtree_augmented::MaskedIndex(
        baseTreeWhichIterationPortal.Get(superparentOldSuperId + 1));

      if (iterationNumber != iterationNumberOfNext)
      { // boundary of iterations
        // If so, we set the "firstSupernodePerIteration" for the next
        augmentedTreeFirstSupernodePerIterationPortal.Set(iterationNumberOfNext,
                                                          newSupernodeId + 1);
      } // boundary of iterations
    }   // last in the segment
    else
    { // not last in the segment
      // the target is always the next one, so just store it with the ascending flag
      augmentedTreeSuperarcsValue = (newSupernodeId + 1) |
        (superarcAscends ? vtkm::worklet::contourtree_augmented::IS_ASCENDING : 0x00);
    } // not last in the segment

    // set the first supernode in the first iteration to the beginning of the round
    augmentedTreeFirstSupernodePerIterationPortal.Set(0, this->NumSupernodesAlready);


    // NOTE: This part has been moved out of the worklet and is performed using standard vtkm copy constructs
    // // setting the hyperparent is straightforward since the hyperstructure is preserved
    // // we take the superparent (which is guaranteed to be in the baseTree), find it's hyperparent and use that
    // augmentedTreeHyperparentsPortal.Set(newSupernodeId, baseTreeHyperparentsPortal.Get(superparentOldSuperId));

    // NOTE: This part could potentially be made a separate worklet but it does not seem necessary
    // similarly, the super2hypernode should carry over, but it's harder to test because of the attachment points which
    // do not have valid old supernode Ids.  Instead, we check their superparent's regular global Id against them: if it
    // matches, then it must be the start of the superarc, in which case it does have an old Id, and we can then use the
    // existing hypernode Id
    vtkm::Id superparentOldRegularId = baseTreeSupernodesPortal.Get(superparentOldSuperId);
    vtkm::Id superparentGlobalId = baseTreeRegularNodeGlobalIdsPortal.Get(superparentOldRegularId);
    // Here: globalRegularIdSetValue is the same as globalRegularIdSetPortal.Get(supernodeSetIndex)
    if (superparentGlobalId == globalRegularIdSetValue)
    {
      // augmentedTreeSuper2hypernodePortal.Set(newSupernodeId, baseTreeSuper2HypernodePortal.Get(superparentOldSuperId));
      augmentedTreeSuper2hypernodeValue = baseTreeSuper2HypernodePortal.Get(superparentOldSuperId);
    }
    else
    {
      // augmentedTreeSuper2hypernodePortal.Set(newSupernodeId, vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT);
      augmentedTreeSuper2hypernodeValue = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    }

    // NOTE: This part has been moved out of the worklet and is performed using standard vtkm copy constructs
    // // which round and iteration carry over
    // augmentedTreeWhichRoundPortal.Set(newSupernodeId, baseTreeWhichRoundPortal.Get(superparentOldSuperId));
    // augmentedTreeWhichIterationPortal.Set(newSupernodeId, baseTreeWhichIterationPortal.Get(superparentOldSuperId));

    // now we deal with the regular-sized arrays

    // NOTE: This part has been moved out of the worklet and is performed using standard vtkm copy constructs
    // // copy the global regular Id and data value
    // augmentedTreeRegularNodeGlobalIdsPortal.Set(newRegularId, globalRegularIdSetPortal.Get(supernodeSetIndex));
    // augmentedTreeDataValuesPortal.Set(newRegularId, dataValueSetPortal.Get(supernodeSetIndex));

    // NOTE: This part has been moved out of the worklet and is performed using standard vtkm copy constructs
    // // the sort order will be dealt with later
    // // since all of these nodes are supernodes, they will be their own superparent, which means that:
    // //  a.  the regular2node can be set immediately
    // augmentedTreeRegular2SupernodePortal.Set(newRegularId, newSupernodeId);
    // //  b.  as can the superparent
    // augmentedTreeSuperparentsPortal.Set(newRegularId, newSupernodeId);

    // In serial this worklet implements the following operation
    /*
    for (vtkm::Id supernode = 0; supernode < supernodeSorter.size(); supernode++)
    { // per supernode in the set
      // retrieve the index from the sorting index array
      vtkm::Id supernodeSetIndex = supernodeSorter[supernode];

      // work out the new supernode ID
      vtkm::Id newSupernodeID = numSupernodesAlready + supernode;

      //  At all levels above 0, we used to keep regular vertices in case they are attachment points.  After augmentation, we don't need to.
      //  Instead, at all levels above 0, the regular nodes in each round are identical to the supernodes
      //  In order to avoid confusion, we will copy the ID into a separate variable
      vtkm::Id newRegularID = newSupernodeID;

      // setting the supernode's regular ID is now trivial
      augmentedTree->supernodes      [newSupernodeID] = newRegularID;

      // retrieve the ascending flag from the superparent
      bool superarcAscends = isAscending(superparentSet[supernodeSetIndex]);

      // strip the ascending flag from the superparent
      vtkm::Id superparentOldSuperID = maskedIndex(superparentSet[supernodeSetIndex]);

      // setting the superarc is done the usual way.  Our sort routine has ended up with the supernodes arranged in either ascending or descending order
      // inwards along the parent superarc (as expressed by the superparent ID).  Each superarc except the last in the segment points to the next one:
      // the last one points to the target of the original superarc.
      // first test to see if we're the last in the array
      if (supernode == supernodeSorter.size() - 1)
      { // last in the array
        // special case for root of entire tree at end of top level
        if (roundNumber == baseTree->nRounds)
        {
          augmentedTree->superarcs[newSupernodeID] = NO_SUCH_ELEMENT;
        }
        else
        { // not the tree root
          // retrieve the target of the superarc from the base tree (masking to strip out the ascending flag)
          vtkm::Id oldTargetSuperID = maskedIndex(baseTree->superarcs[superparentOldSuperID]);
          // convert to a new supernode ID
          vtkm::Id newTargetSuperID = newSupernodeIDs[oldTargetSuperID];
          // add the ascending flag back in and store in the array
          augmentedTree->superarcs[newSupernodeID] = newTargetSuperID | (superarcAscends ? IS_ASCENDING : 0x00);
        } // not the tree root
        // since there's an extra entry in the firstSupernode array as a sentinel, set it
        augmentedTree->firstSupernodePerIteration[roundNumber][augmentedTree->nIterations[roundNumber]] = augmentedTree->supernodes.size();
      } // last in the array
      else if (superparentOldSuperID != maskedIndex(superparentSet[supernodeSorter[supernode+1]]))
      { // last in the segment
        // retrieve the target of the superarc from the base tree (masking to strip out the ascending flag)
        vtkm::Id oldTargetSuperID = maskedIndex(baseTree->superarcs[superparentOldSuperID]);
        // convert to a new supernode ID
        vtkm::Id newTargetSuperID = newSupernodeIDs[oldTargetSuperID];
        // add the ascending flag back in and store in the array
        augmentedTree->superarcs[newSupernodeID] = newTargetSuperID | (superarcAscends ? IS_ASCENDING : 0x00);

        // since we're the last in the segment, we check to see if we are at the end of an iteration
        vtkm::Id iterationNumber     = maskedIndex(baseTree->whichIteration[superparentOldSuperID]);
        vtkm::Id iterationNumberOfNext = maskedIndex(baseTree->whichIteration[superparentOldSuperID + 1]);

        if (iterationNumber != iterationNumberOfNext)
        { // boundary of iterations
          // If so, we set the "firstSupernodePerIteration" for the next
          augmentedTree->firstSupernodePerIteration[roundNumber][iterationNumberOfNext] = newSupernodeID + 1;
        } // boundary of iterations
      } // last in the segment
      else
      { // not last in the segment
        // the target is always the next one, so just store it with the ascending flag
        augmentedTree->superarcs[newSupernodeID] = (newSupernodeID+1) | (superarcAscends ? IS_ASCENDING : 0x00);
      } // not last in the segment

      // set the first supernode in the first iteration to the beginning of the round
      augmentedTree->firstSupernodePerIteration[roundNumber][0] = numSupernodesAlready;

      // setting the hyperparent is straightforward since the hyperstructure is preserved
      // we take the superparent (which is guaranteed to be in the baseTree), find it's hyperparent and use that
      augmentedTree->hyperparents      [newSupernodeID] = baseTree->hyperparents      [superparentOldSuperID];

      // similarly, the super2hypernode should carry over, but it's harder to test because of the attachment points which
      // do not have valid old supernode IDs.  Instead, we check their superparent's regular global ID against them: if it
      // matches, then it must be the start of the superarc, in which case it does have an old ID, and we can then use the
      // existing hypernode ID
      vtkm::Id superparentOldRegularID = baseTree->supernodes[superparentOldSuperID];
      vtkm::Id superparentGlobalID = baseTree->regularNodeGlobalIDs[superparentOldRegularID];
      if (superparentGlobalID == globalRegularIDSet[supernodeSetIndex])
      {
        augmentedTree->super2hypernode  [newSupernodeID]   = baseTree->super2hypernode[superparentOldSuperID];
      }
      else
      {
        augmentedTree->super2hypernode  [newSupernodeID]   = NO_SUCH_ELEMENT;
      }

      // which round and iteration carry over
      augmentedTree->whichRound      [newSupernodeID]   = baseTree->whichRound[superparentOldSuperID];
      augmentedTree->whichIteration    [newSupernodeID]   = baseTree->whichIteration[superparentOldSuperID];

      // now we deal with the regular-sized arrays

      // copy the global regular ID and data value
      augmentedTree->regularNodeGlobalIDs  [newRegularID]    = globalRegularIDSet[supernodeSetIndex];
      augmentedTree->dataValues      [newRegularID]    = dataValueSet[supernodeSetIndex];

      // the sort order will be dealt with later
      // since all of these nodes are supernodes, they will be their own superparent, which means that:
      //  a.  the regular2node can be set immediately
      augmentedTree->regular2supernode  [newRegularID]     = newSupernodeID;
      //  b.  as can the superparent
      augmentedTree->superparents      [newRegularID]     = newSupernodeID;
    } // per supernode in the set
    */
  } // operator()()

private:
  const vtkm::Id NumSupernodesAlready;
  const vtkm::Id BaseTreeNumRounds;
  const vtkm::Id AugmentedTreeNumIterations;
  const vtkm::Id RoundNumber;
  const vtkm::Id NumAugmentedTreeSupernodes;

}; // CreateSuperarcsWorklet

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
