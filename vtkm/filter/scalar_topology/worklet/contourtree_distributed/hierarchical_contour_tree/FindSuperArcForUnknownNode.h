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
// THIS SOFTWARE IS PROVIdED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIdENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
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

#ifndef vtk_m_worklet_contourtree_distributed_find_superarc_for_unknown_node_h
#define vtk_m_worklet_contourtree_distributed_find_superarc_for_unknown_node_h

#include <vtkm/Types.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>


namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{


/// Device implementation of FindSuperArcForUnknownNode for the HierarchicalContourTree
template <typename FieldType>
class FindSuperArcForUnknownNodeDeviceData
{
public:
  using IndicesPortalType =
    typename vtkm::worklet::contourtree_augmented::IdArrayType::ReadPortalType;
  using DataPortalType = typename vtkm::cont::ArrayHandle<FieldType>::ReadPortalType;

  VTKM_CONT
  FindSuperArcForUnknownNodeDeviceData(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token,
    const vtkm::worklet::contourtree_augmented::IdArrayType& superparents,
    const vtkm::worklet::contourtree_augmented::IdArrayType& supernodes,
    const vtkm::worklet::contourtree_augmented::IdArrayType& superarcs,
    const vtkm::worklet::contourtree_augmented::IdArrayType& superchildren,
    const vtkm::worklet::contourtree_augmented::IdArrayType& whichRound,
    const vtkm::worklet::contourtree_augmented::IdArrayType& whichIteration,
    const vtkm::worklet::contourtree_augmented::IdArrayType& hyperparents,
    const vtkm::worklet::contourtree_augmented::IdArrayType& hypernodes,
    const vtkm::worklet::contourtree_augmented::IdArrayType& hyperarcs,
    const vtkm::worklet::contourtree_augmented::IdArrayType& regularNodeGlobalIds,
    const vtkm::cont::ArrayHandle<FieldType>& dataValues)
  {
    // Prepare the arrays for input and store the array portals
    // so that they can be used inside a workelt
    this->Superparents = superparents.PrepareForInput(device, token);
    this->Supernodes = supernodes.PrepareForInput(device, token);
    this->Superarcs = superarcs.PrepareForInput(device, token);
    this->Superchildren = superchildren.PrepareForInput(device, token);
    this->WhichRound = whichRound.PrepareForInput(device, token);
    this->WhichIteration = whichIteration.PrepareForInput(device, token);
    this->Hyperparents = hyperparents.PrepareForInput(device, token);
    this->Hypernodes = hypernodes.PrepareForInput(device, token);
    this->Hyperarcs = hyperarcs.PrepareForInput(device, token);
    this->RegularNodeGlobalIds = regularNodeGlobalIds.PrepareForInput(device, token);
    this->DataValues = dataValues.PrepareForInput(device, token);
  }

  /// routine to find the superarc to which a given global Id/value pair maps
  /// given a known pair of vertices by their regular Ids, one above, one below
  /// assumes that the vertex being searched for is NOT in the hierarchical tree (at all)
  /// and that the above/below pair ARE in the hierarchical tree
  VTKM_EXEC
  vtkm::Id FindSuperArcForUnknownNode(vtkm::Id nodeGlobalId,
                                      FieldType nodeValue,
                                      vtkm::Id above,
                                      vtkm::Id below) const
  { // FindSuperArcForUnknownNode()
    // the hyperparent which we need to search along
    vtkm::Id hyperparent = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT;

    // to find the superarc, we will first have to convert the above / below to a pair of super/hypernodes
    vtkm::Id aboveSuperparent = this->Superparents.Get(above);
    vtkm::Id belowSuperparent = this->Superparents.Get(below);

    // if the two superparents match, we must be on the same superarc, and we are done
    if (aboveSuperparent == belowSuperparent)
    {
      return aboveSuperparent;
    }

    // now it gets slightly tricky.  While we know that the above / below pair straddle the node of interest, it is not guaranteed that
    // their superparents will.  What we do is to take the two ends of the "above" superarc - one of which is guaranteed to be at least as high
    // as the above node.  We choose that, and the inverse at the lower end.  We can determine this from the ascending flag for the superarc
    vtkm::Id aboveSuperarc = this->Superarcs.Get(aboveSuperparent);
    // there are two possibilities here.
    //  I.    It could be null, which means that "aboveSuperparent" is either the root of the tree or an attachment point.  This can only happen
    //      if "above" IS the root/attachment point, so we can safely keep it.
    //  II.    In all other cases, there is a superarc, and if it's ascending, we take the destination instead of the source.
    if (!vtkm::worklet::contourtree_augmented::NoSuchElement(aboveSuperarc) &&
        (vtkm::worklet::contourtree_augmented::IsAscending(aboveSuperarc)))
    {
      aboveSuperparent = vtkm::worklet::contourtree_augmented::MaskedIndex(aboveSuperarc);
    }

    // and the same at the lower end
    vtkm::Id belowSuperarc = this->Superarcs.Get(belowSuperparent);
    if (!vtkm::worklet::contourtree_augmented::NoSuchElement(belowSuperarc) &&
        (!vtkm::worklet::contourtree_augmented::IsAscending(belowSuperarc)))
    {
      belowSuperparent = vtkm::worklet::contourtree_augmented::MaskedIndex(belowSuperarc);
    }

    // we now have as an invariant that the above, below supernodes straddle the node of interest

    // retrieve the corresponding hyperparents: we no longer need to worry whether we straddle, as the iteration takes care of pruning in
    // the correct direction
    vtkm::Id aboveHyperparent = this->Hyperparents.Get(aboveSuperparent);
    vtkm::Id belowHyperparent = this->Hyperparents.Get(belowSuperparent);

    // now test to see if we have the same hyperparent:
    // if we do, choose it and fall through the following while loop
    if (aboveHyperparent == belowHyperparent)
    {
      hyperparent = aboveHyperparent;
    }

    // loop until we have matching hyperparents - i.e. until we're on the same hyperarc
    while (aboveHyperparent != belowHyperparent)
    { // different hyperparents
      // otherwise, they must be different, and we can ask which prunes first
      // the rule is that we do it by hierarchical round first, iteration second, so
      vtkm::Id belowRound = this->WhichRound.Get(this->Hypernodes.Get(belowHyperparent));
      vtkm::Id belowIteration = vtkm::worklet::contourtree_augmented::MaskedIndex(
        this->WhichIteration.Get(this->Hypernodes.Get(belowHyperparent)));

      vtkm::Id aboveRound = this->WhichRound.Get(this->Hypernodes.Get(aboveHyperparent));
      vtkm::Id aboveIteration = vtkm::worklet::contourtree_augmented::MaskedIndex(
        this->WhichIteration.Get(this->Hypernodes.Get(aboveHyperparent)));

      // and a variable for which end prunes first
      vtkm::Id pruningEnd = PRUNE_LOW;

      // now search until we find a hyperarc
      while (vtkm::worklet::contourtree_augmented::NoSuchElement(hyperparent))
      { // until we have set a hyperparent
        // decide which end prunes first
        // low round #'s prune first
        if (belowRound < aboveRound)
        {
          pruningEnd = PRUNE_LOW;
        }
        else if (belowRound > aboveRound)
        {
          pruningEnd = PRUNE_HIGH;
        }
        // in the same round, low iterations prune first
        else if (belowIteration < aboveIteration)
        {
          pruningEnd = PRUNE_LOW;
        }
        else if (belowIteration > aboveIteration)
        {
          pruningEnd = PRUNE_HIGH;
        }
        // perfect match
        else if (aboveHyperparent == belowHyperparent)
        {
          pruningEnd = PRUNE_FINAL;
        }
        else // prune either end first
        {
          pruningEnd = PRUNE_LOW;
        }

        // now, depending on the case
        switch (pruningEnd)
        { // switch on pruning end
          case PRUNE_FINAL:
          { // last hyperarc left can prune both ends simultaneously
            // in this case, both have the same hyperparent set (& its arbitrary between them)
            // this will cause the loop to exit
            hyperparent = aboveHyperparent;
            break;
          } // last hyperarc left can prune both ends simultaneously
          case PRUNE_LOW:
          { // low end prunes first
            // here, we test the hyperarc to see if the upper end is higher than the target
            // if it is, we've overshot, but at least we now know which hyperarc
            vtkm::Id hyperTarget = vtkm::worklet::contourtree_augmented::MaskedIndex(
              this->Hyperarcs.Get(belowHyperparent));
            vtkm::Id hyperTargetRegularId = this->Supernodes.Get(hyperTarget);
            vtkm::Id hyperTargetGlobalId = this->RegularNodeGlobalIds.Get(hyperTargetRegularId);
            FieldType hyperTargetValue = this->DataValues.Get(hyperTargetRegularId);

            // now compare, with simulation of simplicity
            // success means we've found the hyperarc
            if ((hyperTargetValue > nodeValue) ||
                ((hyperTargetValue == nodeValue) && (hyperTargetGlobalId > nodeGlobalId)))
            { // overshoot
              hyperparent = belowHyperparent;
            } // overshoot
            // failure means we update the low end and keep going
            else
            { // no overshoot
              // the next hyperarc is always the hyperparent, even for attachment points
              belowHyperparent = this->Hyperparents.Get(hyperTarget);
              // the round and iteration, however, need to be set from the hyperparent
              // since an attachment point will have a different round / iteration from it's hyperparent
              belowSuperparent = this->Hypernodes.Get(belowHyperparent);
              belowRound = this->WhichRound.Get(belowSuperparent);
              belowIteration = vtkm::worklet::contourtree_augmented::MaskedIndex(
                this->WhichIteration.Get(belowSuperparent));
            } // no overshoot
            break;
          } // low end prunes first
          case PRUNE_HIGH:
          { // high end prunes first
            // here, we test the hyperarc to see if the lower end is lower than the target
            // if it is, we've overshot, but at least we now know which hyperarc
            // this differs from the hypersweep logic in the regular tree
            vtkm::Id hyperTarget = vtkm::worklet::contourtree_augmented::MaskedIndex(
              this->Hyperarcs.Get(aboveHyperparent));
            vtkm::Id hyperTargetRegularId = this->Supernodes.Get(hyperTarget);
            vtkm::Id hyperTargetGlobalId = this->RegularNodeGlobalIds.Get(hyperTargetRegularId);
            FieldType hyperTargetValue = this->DataValues.Get(hyperTargetRegularId);

            // now compare, with simulation of simplicity
            // success means we've found the hyperarc
            if ((hyperTargetValue < nodeValue) ||
                ((hyperTargetValue == nodeValue) && (hyperTargetGlobalId < nodeGlobalId)))
            { // overshoot
              hyperparent = aboveHyperparent;
            } // overshoot
            // failure means we update the low end and keep going
            else
            { // no overshoot
              // the next hyperarc is always the hyperparent, even for attachment points
              aboveHyperparent = this->Hyperparents.Get(hyperTarget);
              // the round and iteration, however, need to be set from the hyperparent
              // since an attachment point will have a different round / iteration from it's hyperparent
              // this differs from the hypersweep logic in the regular tree
              aboveSuperparent = this->Hypernodes.Get(aboveHyperparent);
              aboveRound = this->WhichRound.Get(aboveSuperparent);
              aboveIteration = vtkm::worklet::contourtree_augmented::MaskedIndex(
                this->WhichIteration.Get(aboveSuperparent));
            } // no overshoot
            break;
          } // high end prunes first
        }   // switch on pruning end
      }     // until we have set a hyperparent

      // if we found one, then we exit this loop too
      if (!vtkm::worklet::contourtree_augmented::NoSuchElement(hyperparent))
      {
        break;
      }
    } // different hyperparents

    // We are now on the correct hyperarc and "merely" need to find the correct superarc with a binary search.
    // We are, however, guaranteed to have a data value strictly in the range of the hyperarc
    // Moreover, we are already guaranteed that the data value is strictly in the range on the hyperarc
    if (vtkm::worklet::contourtree_augmented::IsAscending(this->Hyperarcs.Get(hyperparent)))
    { // ascending hyperarc
      // the supernodes on the hyperarc are in sorted low-high order
      vtkm::Id lowSupernode = this->Hypernodes.Get(hyperparent);
      // now that we have stored "superchildren", this next bit is easier than it used to be
      vtkm::Id highSupernode =
        this->Hypernodes.Get(hyperparent) + this->Superchildren.Get(hyperparent) - 1;

      // now, the high supernode may be lower than the element, because the node belongs
      // between it and the high end of the hyperarc. In this case, the high supernode's ascending superarc is the correct one
      vtkm::Id highSupernodeRegularId = this->Supernodes.Get(highSupernode);
      vtkm::Id highSupernodeGlobalId = this->RegularNodeGlobalIds.Get(highSupernodeRegularId);
      FieldType highValue = this->DataValues.Get(highSupernodeRegularId);
      // simulation of simplicity
      if ((highValue < nodeValue) ||
          ((highValue == nodeValue) && (highSupernodeGlobalId < nodeGlobalId)))
      { // last superarc
        return highSupernode;
      } // last superarc
      // otherwise, we do a binary search of the superarcs
      else
      { // node between high & low
        // keep going until we span exactly
        while (highSupernode - lowSupernode > 1)
        { // binary search
          // find the midway supernode
          vtkm::Id midSupernode = (lowSupernode + highSupernode) / 2;
          vtkm::Id midSupernodeRegularId = this->Supernodes.Get(midSupernode);
          vtkm::Id midSupernodeGlobalId = this->RegularNodeGlobalIds.Get(midSupernodeRegularId);
          FieldType midValue = this->DataValues.Get(midSupernodeRegularId);

          // test against the node (with simulation of simplicity)
          if ((midValue > nodeValue) ||
              ((midValue == nodeValue) && (midSupernodeGlobalId > nodeGlobalId)))
          { // mid higher
            highSupernode = midSupernode;
          } // mid higher
          // == can't happen since node is regular
          else
          { // mid lower
            lowSupernode = midSupernode;
          } // mid lower
        }   // binary search
        // we've now narrowed down the search and found the superarc we wanted, so return it
        // for an ascending arc, the Id is that of the lower end
        return lowSupernode;
      } // node between high & low
    }   // ascending hyperarc
    else
    { // descending hyperarc
      // the supernodes on the hyperarc are in sorted high-low order
      vtkm::Id highSupernode = this->Hypernodes.Get(hyperparent);
      // now that we have stored "superchildren", this next bit is easier than it used to be
      vtkm::Id lowSupernode =
        this->Hypernodes.Get(hyperparent) + this->Superchildren.Get(hyperparent) - 1;

      // now, the low supernode may be higher than the element, because the node belongs
      // between it and the low end of the hyperarc. In this case, the low supernode's descending superarc is the correct one
      vtkm::Id lowSupernodeRegularId = this->Supernodes.Get(lowSupernode);
      vtkm::Id lowSupernodeGlobalId = this->RegularNodeGlobalIds.Get(lowSupernodeRegularId);
      FieldType lowValue = this->DataValues.Get(lowSupernodeRegularId);
      // simulation of simplicity
      if ((lowValue > nodeValue) ||
          ((lowValue == nodeValue) && (lowSupernodeGlobalId >= nodeGlobalId)))
      {
        return lowSupernode;
      }
      // otherwise, we do a binary search of the superarcs
      else
      { // node between low & high
        // keep going until we span exactly
        while (lowSupernode - highSupernode > 1)
        { // binary search
          // find the midway supernode
          vtkm::Id midSupernode = (highSupernode + lowSupernode) / 2;
          vtkm::Id midSupernodeRegularId = this->Supernodes.Get(midSupernode);
          vtkm::Id midSupernodeGlobalId = this->RegularNodeGlobalIds.Get(midSupernodeRegularId);
          FieldType midValue = this->DataValues.Get(midSupernodeRegularId);
          // test against the node (with simulation of simplicity)
          if ((midValue > nodeValue) ||
              ((midValue == nodeValue) && (midSupernodeGlobalId > nodeGlobalId)))
          { // mid higher
            highSupernode = midSupernode;
          } // mid higher
          // == can't happen since node is regular
          else
          { // mid lower
            lowSupernode = midSupernode;
          } // mid lower
        }   // binary search
        // we've now narrowed down the search and found the superarc we wanted, so return it
        // for an ascending arc, the Id is that of the lower end
        return highSupernode;
      } // node between low & high
    }   // descending hyperarc
  }     // FindSuperArcForUnknownNode()

private:
  // these are used to make it simpler to search through the hierarchy
  static constexpr vtkm::Id PRUNE_LOW = static_cast<vtkm::Id>(0);
  static constexpr vtkm::Id PRUNE_HIGH = static_cast<vtkm::Id>(1);
  static constexpr vtkm::Id PRUNE_FINAL = static_cast<vtkm::Id>(2);

  // Array portals needed by FindSuperArcForUnknownNode
  // These arrays all originate from the HierarchicalContourTree
  IndicesPortalType Superparents;
  IndicesPortalType Supernodes;
  IndicesPortalType Superarcs;
  IndicesPortalType Superchildren;
  IndicesPortalType WhichRound;
  IndicesPortalType WhichIteration;
  IndicesPortalType Hyperparents;
  IndicesPortalType Hypernodes;
  IndicesPortalType Hyperarcs;
  IndicesPortalType RegularNodeGlobalIds;
  DataPortalType DataValues;
};


/// ExecutionObject to generate a device object to use FindSuperArcForUnknownNode for the HierarchicalContourTree
///
/// This is a  routine to find the superarc to which a given global Id/value pair maps
/// given a known pair of vertices by their regular Ids, one above, one below
/// assumes that the vertex being searched for is NOT in the hierarchical tree (at all)
/// and that the above/below pair ARE in the hierarchical tree
template <typename FieldType>
class FindSuperArcForUnknownNode : public vtkm::cont::ExecutionObjectBase
{
public:
  /// constructor
  VTKM_CONT
  FindSuperArcForUnknownNode(
    const vtkm::worklet::contourtree_augmented::IdArrayType& superparents,
    const vtkm::worklet::contourtree_augmented::IdArrayType& supernodes,
    const vtkm::worklet::contourtree_augmented::IdArrayType& superarcs,
    const vtkm::worklet::contourtree_augmented::IdArrayType& superchildren,
    const vtkm::worklet::contourtree_augmented::IdArrayType& whichRound,
    const vtkm::worklet::contourtree_augmented::IdArrayType& whichIteration,
    const vtkm::worklet::contourtree_augmented::IdArrayType& hyperparents,
    const vtkm::worklet::contourtree_augmented::IdArrayType& hypernodes,
    const vtkm::worklet::contourtree_augmented::IdArrayType& hyperarcs,
    const vtkm::worklet::contourtree_augmented::IdArrayType& regularNodeGlobalIds,
    const vtkm::cont::ArrayHandle<FieldType>& dataValues)
    : Superparents(superparents)
    , Supernodes(supernodes)
    , Superarcs(superarcs)
    , Superchildren(superchildren)
    , WhichRound(whichRound)
    , WhichIteration(whichIteration)
    , Hyperparents(hyperparents)
    , Hypernodes(hypernodes)
    , Hyperarcs(hyperarcs)
    , RegularNodeGlobalIds(regularNodeGlobalIds)
    , DataValues(dataValues)
  {
  }

  VTKM_CONT FindSuperArcForUnknownNodeDeviceData<FieldType> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    return FindSuperArcForUnknownNodeDeviceData<FieldType>(device,
                                                           token,
                                                           this->Superparents,
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

private:
  // Arrays needed by FindSuperArcForUnknownNode
  // These arrays all originate from the HierarchicalContourTree
  vtkm::worklet::contourtree_augmented::IdArrayType Superparents;
  vtkm::worklet::contourtree_augmented::IdArrayType Supernodes;
  vtkm::worklet::contourtree_augmented::IdArrayType Superarcs;
  vtkm::worklet::contourtree_augmented::IdArrayType Superchildren;
  vtkm::worklet::contourtree_augmented::IdArrayType WhichRound;
  vtkm::worklet::contourtree_augmented::IdArrayType WhichIteration;
  vtkm::worklet::contourtree_augmented::IdArrayType Hyperparents;
  vtkm::worklet::contourtree_augmented::IdArrayType Hypernodes;
  vtkm::worklet::contourtree_augmented::IdArrayType Hyperarcs;
  vtkm::worklet::contourtree_augmented::IdArrayType RegularNodeGlobalIds;
  vtkm::cont::ArrayHandle<FieldType> DataValues;
};


} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
