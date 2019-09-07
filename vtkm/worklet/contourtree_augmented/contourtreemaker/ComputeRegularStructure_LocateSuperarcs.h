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

#ifndef vtkm_worklet_contourtree_augmented_contourtree_maker_inc_compute_regular_structure_locate_superarcs_h
#define vtkm_worklet_contourtree_augmented_contourtree_maker_inc_compute_regular_structure_locate_superarcs_h

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

// Worklet for the second step of ContourTreeMaker::ComputeRegularStructure ---
// for all remaining (regular) nodes, locate the superarc to which they belong
class ComputeRegularStructure_LocateSuperarcs : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayInOut contourTreeSuperparents, // (input/output)
                                WholeArrayIn contourTreeWhenTransferred, // (input)
                                WholeArrayIn contourTreeHyperparents,    // (input)
                                WholeArrayIn contourTreeHyperarcs,       // (input)
                                WholeArrayIn contourTreeHypernodes,      // (input)
                                WholeArrayIn contourTreeSupernodes,      // (input)
                                WholeArrayIn meshExtremaPeaks,           // (input)
                                WholeArrayIn meshExtremaPits);           // (input)

  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  vtkm::Id numHypernodes; // contourTree.hypernodes.GetNumberOfValues()
  vtkm::Id numSupernodes; // contourTree.supernodes.GetNumberOfValues()

  // Default Constructor
  VTKM_EXEC_CONT
  ComputeRegularStructure_LocateSuperarcs(vtkm::Id NumHypernodes, vtkm::Id NumSupernodes)
    : numHypernodes(NumHypernodes)
    , numSupernodes(NumSupernodes)
  {
  }

  template <typename InOutFieldPortalType, typename InFieldPortalType>
  VTKM_EXEC void operator()(const InOutFieldPortalType& contourTreeSuperparentsPortal,
                            const vtkm::Id node,
                            const InFieldPortalType& contourTreeWhenTransferredPortal,
                            const InFieldPortalType& contourTreeHyperparentsPortal,
                            const InFieldPortalType& contourTreeHyperarcsPortal,
                            const InFieldPortalType& contourTreeHypernodesPortal,
                            const InFieldPortalType& contourTreeSupernodesPortal,
                            const InFieldPortalType& meshExtremaPeaksPortal,
                            const InFieldPortalType& meshExtremaPitsPortal) const
  {
    // per node
    // if the superparent is already set, it's a supernode, so skip it.
    if (noSuchElement(contourTreeSuperparentsPortal.Get(node)))
    { // regular nodes only
      // we will need to prune top and bottom until one of them prunes past the node
      vtkm::Id top = meshExtremaPeaksPortal.Get(node);
      vtkm::Id bottom = meshExtremaPitsPortal.Get(node);
      // these are the regular IDs of supernodes, so their superparents are already set
      vtkm::Id topSuperparent = contourTreeSuperparentsPortal.Get(maskedIndex(top));
      vtkm::Id bottomSuperparent = contourTreeSuperparentsPortal.Get(maskedIndex(bottom));
      // and we can also find out when they transferred
      vtkm::Id topWhen = contourTreeWhenTransferredPortal.Get(topSuperparent);
      vtkm::Id bottomWhen = contourTreeWhenTransferredPortal.Get(bottomSuperparent);
      // and their hyperparent
      vtkm::Id topHyperparent = contourTreeHyperparentsPortal.Get(topSuperparent);
      vtkm::Id bottomHyperparent = contourTreeHyperparentsPortal.Get(bottomSuperparent);
      // our goal is to work out the true hyperparent of the node
      vtkm::Id hyperparent = (vtkm::Id)NO_SUCH_ELEMENT;

      // now we loop until one of them goes past the vertex
      // the invariant here is that the first direction to prune past the vertex prunes it
      while (noSuchElement(hyperparent))
      { // loop to find pruner
        // we test the one that prunes first
        if (maskedIndex(topWhen) < maskedIndex(bottomWhen))
        { // top pruned first
          // we prune down to the bottom of the hyperarc in either case, by updating the top superparent
          topSuperparent = contourTreeHyperarcsPortal.Get(maskedIndex(topHyperparent));
          top = contourTreeSupernodesPortal.Get(maskedIndex(topSuperparent));

          topWhen = contourTreeWhenTransferredPortal.Get(maskedIndex(topSuperparent));
          // test to see if we've passed the node
          if (top < node)
          { // just pruned past
            hyperparent = topHyperparent;
          } // just pruned past
          // == is not possible, since node is regular
          else // top < node
          {    // not pruned past
            topHyperparent = contourTreeHyperparentsPortal.Get(maskedIndex(topSuperparent));
          } // not pruned past
        }   // top pruned first
        else if (maskedIndex(topWhen) > maskedIndex(bottomWhen))
        { // bottom pruned first
          // we prune up to the top of the hyperarc in either case, by updating the bottom superparent
          bottomSuperparent = contourTreeHyperarcsPortal.Get(maskedIndex(bottomHyperparent));
          bottom = contourTreeSupernodesPortal.Get(maskedIndex(bottomSuperparent));
          bottomWhen = contourTreeWhenTransferredPortal.Get(maskedIndex(bottomSuperparent));
          // test to see if we've passed the node
          if (bottom > node)
          { // just pruned past
            hyperparent = bottomHyperparent;
          } // just pruned past
          // == is not possible, since node is regular
          else // bottom > node
          {    // not pruned past
            bottomHyperparent = contourTreeHyperparentsPortal.Get(maskedIndex(bottomSuperparent));
          } // not pruned past
        }   // bottom pruned first
        else
        { // both prune simultaneously
          // this can happen when both top & bottom prune in the same pass because they belong to the same hyperarc
          // but this means that they must have the same hyperparent, so we know the correct hyperparent & can check whether it ascends
          hyperparent = bottomHyperparent;
        } // both prune simultaneously
      }   // loop to find pruner
      // we have now set the hyperparent correctly, so we retrieve it's hyperarc to find whether it ascends or descends
      if (isAscending(contourTreeHyperarcsPortal.Get(hyperparent)))
      { // ascending hyperarc
        // the supernodes on the hyperarc are in sorted low-high order
        vtkm::Id lowSupernode = contourTreeHypernodesPortal.Get(hyperparent);
        vtkm::Id highSupernode;
        // if it's at the right hand end, take the last supernode in the array
        if (maskedIndex(hyperparent) == numHypernodes - 1)
          highSupernode = numSupernodes - 1;
        // otherwise, take the supernode just before the next hypernode
        else
          highSupernode = contourTreeHypernodesPortal.Get(maskedIndex(hyperparent) + 1) - 1;
        // now, the high supernode may be lower than the element, because the node belongs
        // between it and the high end of the hyperarc
        if (contourTreeSupernodesPortal.Get(highSupernode) < node)
          contourTreeSuperparentsPortal.Set(node, highSupernode);
        // otherwise, we do a binary search of the superarcs
        else
        { // node between high & low
          // keep going until we span exactly
          while (highSupernode - lowSupernode > 1)
          { // binary search
            // find the midway supernode
            vtkm::Id midSupernode = (lowSupernode + highSupernode) / 2;
            // test against the node
            if (contourTreeSupernodesPortal.Get(midSupernode) > node)
              highSupernode = midSupernode;
            // == can't happen since node is regular
            else
              lowSupernode = midSupernode;
          } // binary search

          // now we can use the low node as the superparent
          contourTreeSuperparentsPortal.Set(node, lowSupernode);
        } // node between high & low
      }   // ascending hyperarc
      else
      { // descending hyperarc
        // the supernodes on the hyperarc are in sorted high-low order
        vtkm::Id highSupernode = contourTreeHypernodesPortal.Get(hyperparent);
        vtkm::Id lowSupernode;
        // if it's at the right hand end, take the last supernode in the array
        if (maskedIndex(hyperparent) == numHypernodes - 1)
        { // last hyperarc
          lowSupernode = numSupernodes - 1;
        } // last hyperarc
        // otherwise, take the supernode just before the next hypernode
        else
        { // other hyperarc
          lowSupernode = contourTreeHypernodesPortal.Get(maskedIndex(hyperparent) + 1) - 1;
        } // other hyperarc
        // now, the low supernode may be higher than the element, because the node belongs
        // between it and the low end of the hyperarc
        if (contourTreeSupernodesPortal.Get(lowSupernode) > node)
          contourTreeSuperparentsPortal.Set(node, lowSupernode);
        // otherwise, we do a binary search of the superarcs
        else
        { // node between low & high
          // keep going until we span exactly
          while (lowSupernode - highSupernode > 1)
          { // binary search
            // find the midway supernode
            vtkm::Id midSupernode = (highSupernode + lowSupernode) / 2;
            // test against the node
            if (contourTreeSupernodesPortal.Get(midSupernode) > node)
              highSupernode = midSupernode;
            // == can't happen since node is regular
            else
              lowSupernode = midSupernode;
          } // binary search
          // now we can use the high node as the superparent
          contourTreeSuperparentsPortal.Set(node, highSupernode);
        } // node between low & high
      }   // descending hyperarc
    }     // regular nodes only
    /*
    // In serial this worklet implements the following operation
    for (indexType node = 0; node < contourTree.arcs.size(); node++)
    { // per node
        // if the superparent is already set, it's a supernode, so skip it.
        if (noSuchElement(contourTree.superparents[node]))
        { // regular nodes only
            // we will need to prune top and bottom until one of them prunes past the node
            indexType top = meshExtrema.peaks[node];
            indexType bottom = meshExtrema.pits[node];
            // these are the regular IDs of supernodes, so their superparents are already set
            indexType topSuperparent = contourTree.superparents[top];
            indexType bottomSuperparent = contourTree.superparents[bottom];
            // and we can also find out when they transferred
            indexType topWhen = contourTree.whenTransferred[topSuperparent];
            indexType bottomWhen = contourTree.whenTransferred[bottomSuperparent];
            // and their hyperparent
            indexType topHyperparent = contourTree.hyperparents[topSuperparent];
            indexType bottomHyperparent = contourTree.hyperparents[bottomSuperparent];

            // our goal is to work out the true hyperparent of the node
            indexType hyperparent = NO_SUCH_ELEMENT;

            // now we loop until one of them goes past the vertex
            // the invariant here is that the first direction to prune past the vertex prunes it
            while (noSuchElement(hyperparent))
            { // loop to find pruner

                // we test the one that prunes first
                if (maskedIndex(topWhen) < maskedIndex(bottomWhen))
                { // top pruned first
                    // we prune down to the bottom of the hyperarc in either case, by updating the top superparent
                    topSuperparent = contourTree.hyperarcs[maskedIndex(topHyperparent)];
                    top = contourTree.supernodes[maskedIndex(topSuperparent)];

                    topWhen = contourTree.whenTransferred[maskedIndex(topSuperparent)];
                    // test to see if we've passed the node
                    if (top < node)
                    { // just pruned past
                        hyperparent = topHyperparent;
                    } // just pruned past
                    // == is not possible, since node is regular
                    else // top < node
                    { // not pruned past
                        topHyperparent = contourTree.hyperparents[maskedIndex(topSuperparent)];
                    } // not pruned past
                } // top pruned first
                else if (maskedIndex(topWhen) > maskedIndex(bottomWhen))
                { // bottom pruned first
                    // we prune up to the top of the hyperarc in either case, by updating the bottom superparent
                    bottomSuperparent = contourTree.hyperarcs[maskedIndex(bottomHyperparent)];
                    bottom = contourTree.supernodes[maskedIndex(bottomSuperparent)];
                    bottomWhen = contourTree.whenTransferred[maskedIndex(bottomSuperparent)];
                    // test to see if we've passed the node
                    if (bottom > node)
                    { // just pruned past
                        hyperparent = bottomHyperparent;
                    } // just pruned past
                    // == is not possible, since node is regular
                    else // bottom > node
                    { // not pruned past
                        bottomHyperparent = contourTree.hyperparents[maskedIndex(bottomSuperparent)];
                    } // not pruned past
                } // bottom pruned first
                else
                { // both prune simultaneously
                    // this can happen when both top & bottom prune in the same pass because they belong to the same hyperarc
                    // but this means that they must have the same hyperparent, so we know the correct hyperparent & can check whether it ascends
                    hyperparent = bottomHyperparent;
                } // both prune simultaneously
            } // loop to find pruner


            // we have now set the hyperparent correctly, so we retrieve it's hyperarc to find whether it ascends or descends
            if (isAscending(contourTree.hyperarcs[hyperparent]))
            { // ascending hyperarc
                // the supernodes on the hyperarc are in sorted low-high order
                indexType lowSupernode = contourTree.hypernodes[hyperparent];
                indexType highSupernode;
                // if it's at the right hand end, take the last supernode in the array
                if (maskedIndex(hyperparent) == contourTree.hypernodes.size() - 1)
                    highSupernode = contourTree.supernodes.size() - 1;
                // otherwise, take the supernode just before the next hypernode
                else
                    highSupernode = contourTree.hypernodes[maskedIndex(hyperparent) + 1] - 1;
                // now, the high supernode may be lower than the element, because the node belongs
                // between it and the high end of the hyperarc
                if (contourTree.supernodes[highSupernode] < node)
                    contourTree.superparents[node] = highSupernode;
                // otherwise, we do a binary search of the superarcs
                else
                { // node between high & low
                    // keep going until we span exactly
                    while (highSupernode - lowSupernode > 1)
                    { // binary search
                        // find the midway supernode
                        indexType midSupernode = (lowSupernode + highSupernode) / 2;
                        // test against the node
                        if (contourTree.supernodes[midSupernode] > node)
                            highSupernode = midSupernode;
                        // == can't happen since node is regular
                        else
                            lowSupernode = midSupernode;
                    } // binary search
                    // now we can use the low node as the superparent
                    contourTree.superparents[node] = lowSupernode;
                } // node between high & low
            } // ascending hyperarc
            else
            { // descending hyperarc
                // the supernodes on the hyperarc are in sorted high-low order
                indexType highSupernode = contourTree.hypernodes[hyperparent];
                indexType lowSupernode;
                // if it's at the right hand end, take the last supernode in the array
                if (maskedIndex(hyperparent) == contourTree.hypernodes.size() - 1)
                { // last hyperarc
                    lowSupernode = contourTree.supernodes.size() - 1;
                } // last hyperarc
                // otherwise, take the supernode just before the next hypernode
                else
                { // other hyperarc
                    lowSupernode = contourTree.hypernodes[maskedIndex(hyperparent) + 1] - 1;
                } // other hyperarc
                // now, the low supernode may be higher than the element, because the node belongs
                // between it and the low end of the hyperarc
                if (contourTree.supernodes[lowSupernode] > node)
                    contourTree.superparents[node] = lowSupernode;
                // otherwise, we do a binary search of the superarcs
                else
                { // node between low & high
                    // keep going until we span exactly
                    while (lowSupernode - highSupernode > 1)
                    { // binary search
                        // find the midway supernode
                        indexType midSupernode = (highSupernode + lowSupernode) / 2;
                        // test against the node
                        if (contourTree.supernodes[midSupernode] > node)
                            highSupernode = midSupernode;
                        // == can't happen since node is regular
                        else
                            lowSupernode = midSupernode;
                    } // binary search
                    // now we can use the high node as the superparent
                    contourTree.superparents[node] = highSupernode;
                } // node between low & high
            } // descending hyperarc
        } // regular nodes only
    } // per node
    */
  }

}; // ComputeRegularStructure_LocateSuperarcs.h



// TODO This algorithm looks to be a 3d/2d volume algorithm that is iterating 'points' and concerned about being on the 'boundary'. This would be better suited as WorkletMapPointNeighborhood as it can provide the boundary condition logic for you.

// Worklet for the second step of template <class Mesh> void ContourTreeMaker::ComputeRegularStructure ---
// for all remaining (regular) nodes on the boundary, locate the superarc to which they belong
class ComputeRegularStructure_LocateSuperarcsOnBoundary : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayInOut contourTreeSuperparents, // (input/output)
                                WholeArrayIn contourTreeWhenTransferred, // (input)
                                WholeArrayIn contourTreeHyperparents,    // (input)
                                WholeArrayIn contourTreeHyperarcs,       // (input)
                                WholeArrayIn contourTreeHypernodes,      // (input)
                                WholeArrayIn contourTreeSupernodes,      // (input)
                                WholeArrayIn meshExtremaPeaks,           // (input)
                                WholeArrayIn meshExtremaPits,            // (input)
                                ExecObject meshBoundary);                // (input)

  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6, _7, _8, _9);
  using InputDomain = _1;

  vtkm::Id numHypernodes; // contourTree.hypernodes.GetNumberOfValues()
  vtkm::Id numSupernodes; // contourTree.supernodes.GetNumberOfValues()

  vtkm::Id nRows, nCols, nSlices; // Mesh 2D or 3D - nRows, nCols, nSlices
  vtkm::Id nDims;                 // Mesh 2D or 3D - 2 or 3

  // Default Constructor
  VTKM_EXEC_CONT
  ComputeRegularStructure_LocateSuperarcsOnBoundary(vtkm::Id NumHypernodes, vtkm::Id NumSupernodes)
    : numHypernodes(NumHypernodes)
    , numSupernodes(NumSupernodes)
  {
  }

  template <typename InOutFieldPortalType, typename InFieldPortalType, typename MeshBoundaryType>
  VTKM_EXEC void operator()(const InOutFieldPortalType& contourTreeSuperparentsPortal,
                            const vtkm::Id node,
                            const InFieldPortalType& contourTreeWhenTransferredPortal,
                            const InFieldPortalType& contourTreeHyperparentsPortal,
                            const InFieldPortalType& contourTreeHyperarcsPortal,
                            const InFieldPortalType& contourTreeHypernodesPortal,
                            const InFieldPortalType& contourTreeSupernodesPortal,
                            const InFieldPortalType& meshExtremaPeaksPortal,
                            const InFieldPortalType& meshExtremaPitsPortal,
                            const MeshBoundaryType& meshBoundary) const
  {
    // per node
    // if the superparent is already set, it's a supernode, so skip it.
    if (noSuchElement(contourTreeSuperparentsPortal.Get(node)) && meshBoundary.liesOnBoundary(node))
    { // regular nodes only
      // we will need to prune top and bottom until one of them prunes past the node
      vtkm::Id top = meshExtremaPeaksPortal.Get(node);
      vtkm::Id bottom = meshExtremaPitsPortal.Get(node);
      // these are the regular IDs of supernodes, so their superparents are already set
      vtkm::Id topSuperparent = contourTreeSuperparentsPortal.Get(maskedIndex(top));
      vtkm::Id bottomSuperparent = contourTreeSuperparentsPortal.Get(maskedIndex(bottom));
      // and we can also find out when they transferred
      vtkm::Id topWhen = contourTreeWhenTransferredPortal.Get(topSuperparent);
      vtkm::Id bottomWhen = contourTreeWhenTransferredPortal.Get(bottomSuperparent);
      // and their hyperparent
      vtkm::Id topHyperparent = contourTreeHyperparentsPortal.Get(topSuperparent);
      vtkm::Id bottomHyperparent = contourTreeHyperparentsPortal.Get(bottomSuperparent);
      // our goal is to work out the true hyperparent of the node
      vtkm::Id hyperparent = (vtkm::Id)NO_SUCH_ELEMENT;

      // now we loop until one of them goes past the vertex
      // the invariant here is that the first direction to prune past the vertex prunes it
      while (noSuchElement(hyperparent))
      { // loop to find pruner
        // we test the one that prunes first
        if (maskedIndex(topWhen) < maskedIndex(bottomWhen))
        { // top pruned first
          // we prune down to the bottom of the hyperarc in either case, by updating the top superparent
          topSuperparent = contourTreeHyperarcsPortal.Get(maskedIndex(topHyperparent));
          top = contourTreeSupernodesPortal.Get(maskedIndex(topSuperparent));

          topWhen = contourTreeWhenTransferredPortal.Get(maskedIndex(topSuperparent));
          // test to see if we've passed the node
          if (top < node)
          { // just pruned past
            hyperparent = topHyperparent;
          } // just pruned past
          // == is not possible, since node is regular
          else // top < node
          {    // not pruned past
            topHyperparent = contourTreeHyperparentsPortal.Get(maskedIndex(topSuperparent));
          } // not pruned past
        }   // top pruned first
        else if (maskedIndex(topWhen) > maskedIndex(bottomWhen))
        { // bottom pruned first
          // we prune up to the top of the hyperarc in either case, by updating the bottom superparent
          bottomSuperparent = contourTreeHyperarcsPortal.Get(maskedIndex(bottomHyperparent));
          bottom = contourTreeSupernodesPortal.Get(maskedIndex(bottomSuperparent));
          bottomWhen = contourTreeWhenTransferredPortal.Get(maskedIndex(bottomSuperparent));
          // test to see if we've passed the node
          if (bottom > node)
          { // just pruned past
            hyperparent = bottomHyperparent;
          } // just pruned past
          // == is not possible, since node is regular
          else // bottom > node
          {    // not pruned past
            bottomHyperparent = contourTreeHyperparentsPortal.Get(maskedIndex(bottomSuperparent));
          } // not pruned past
        }   // bottom pruned first
        else
        { // both prune simultaneously
          // this can happen when both top & bottom prune in the same pass because they belong to the same hyperarc
          // but this means that they must have the same hyperparent, so we know the correct hyperparent & can check whether it ascends
          hyperparent = bottomHyperparent;
        } // both prune simultaneously
      }   // loop to find pruner
      // we have now set the hyperparent correctly, so we retrieve it's hyperarc to find whether it ascends or descends
      if (isAscending(contourTreeHyperarcsPortal.Get(hyperparent)))
      { // ascending hyperarc
        // the supernodes on the hyperarc are in sorted low-high order
        vtkm::Id lowSupernode = contourTreeHypernodesPortal.Get(hyperparent);
        vtkm::Id highSupernode;
        // if it's at the right hand end, take the last supernode in the array
        if (maskedIndex(hyperparent) == numHypernodes - 1)
          highSupernode = numSupernodes - 1;
        // otherwise, take the supernode just before the next hypernode
        else
          highSupernode = contourTreeHypernodesPortal.Get(maskedIndex(hyperparent) + 1) - 1;
        // now, the high supernode may be lower than the element, because the node belongs
        // between it and the high end of the hyperarc
        if (contourTreeSupernodesPortal.Get(highSupernode) < node)
          contourTreeSuperparentsPortal.Set(node, highSupernode);
        // otherwise, we do a binary search of the superarcs
        else
        { // node between high & low
          // keep going until we span exactly
          while (highSupernode - lowSupernode > 1)
          { // binary search
            // find the midway supernode
            vtkm::Id midSupernode = (lowSupernode + highSupernode) / 2;
            // test against the node
            if (contourTreeSupernodesPortal.Get(midSupernode) > node)
              highSupernode = midSupernode;
            // == can't happen since node is regular
            else
              lowSupernode = midSupernode;
          } // binary search

          // now we can use the low node as the superparent
          contourTreeSuperparentsPortal.Set(node, lowSupernode);
        } // node between high & low
      }   // ascending hyperarc
      else
      { // descending hyperarc
        // the supernodes on the hyperarc are in sorted high-low order
        vtkm::Id highSupernode = contourTreeHypernodesPortal.Get(hyperparent);
        vtkm::Id lowSupernode;
        // if it's at the right hand end, take the last supernode in the array
        if (maskedIndex(hyperparent) == numHypernodes - 1)
        { // last hyperarc
          lowSupernode = numSupernodes - 1;
        } // last hyperarc
        // otherwise, take the supernode just before the next hypernode
        else
        { // other hyperarc
          lowSupernode = contourTreeHypernodesPortal.Get(maskedIndex(hyperparent) + 1) - 1;
        } // other hyperarc
        // now, the low supernode may be higher than the element, because the node belongs
        // between it and the low end of the hyperarc
        if (contourTreeSupernodesPortal.Get(lowSupernode) > node)
          contourTreeSuperparentsPortal.Set(node, lowSupernode);
        // otherwise, we do a binary search of the superarcs
        else
        { // node between low & high
          // keep going until we span exactly
          while (lowSupernode - highSupernode > 1)
          { // binary search
            // find the midway supernode
            vtkm::Id midSupernode = (highSupernode + lowSupernode) / 2;
            // test against the node
            if (contourTreeSupernodesPortal.Get(midSupernode) > node)
              highSupernode = midSupernode;
            // == can't happen since node is regular
            else
              lowSupernode = midSupernode;
          } // binary search
          // now we can use the high node as the superparent
          contourTreeSuperparentsPortal.Set(node, highSupernode);
        } // node between low & high
      }   // descending hyperarc
    }     // regular nodes only
    /*
    // In serial this worklet implements the following operation
    for (indexType node = 0; node < contourTree.arcs.size(); node++)
    { // per node
        // if the superparent is already set, it's a supernode, so skip it.
        if (noSuchElement(contourTree.superparents[node]) && mesh.liesOnBoundary(node))
        { // regular nodes only
            // we will need to prune top and bottom until one of them prunes past the node
            indexType top = meshExtrema.peaks[node];
            indexType bottom = meshExtrema.pits[node];
            // these are the regular IDs of supernodes, so their superparents are already set
            indexType topSuperparent = contourTree.superparents[top];
            indexType bottomSuperparent = contourTree.superparents[bottom];
            // and we can also find out when they transferred
            indexType topWhen = contourTree.whenTransferred[topSuperparent];
            indexType bottomWhen = contourTree.whenTransferred[bottomSuperparent];
            // and their hyperparent
            indexType topHyperparent = contourTree.hyperparents[topSuperparent];
            indexType bottomHyperparent = contourTree.hyperparents[bottomSuperparent];

            // our goal is to work out the true hyperparent of the node
            indexType hyperparent = NO_SUCH_ELEMENT;

            // now we loop until one of them goes past the vertex
            // the invariant here is that the first direction to prune past the vertex prunes it
            while (noSuchElement(hyperparent))
            { // loop to find pruner

                // we test the one that prunes first
                if (maskedIndex(topWhen) < maskedIndex(bottomWhen))
                { // top pruned first
                    // we prune down to the bottom of the hyperarc in either case, by updating the top superparent
                    topSuperparent = contourTree.hyperarcs[maskedIndex(topHyperparent)];
                    top = contourTree.supernodes[maskedIndex(topSuperparent)];

                    topWhen = contourTree.whenTransferred[maskedIndex(topSuperparent)];
                    // test to see if we've passed the node
                    if (top < node)
                    { // just pruned past
                        hyperparent = topHyperparent;
                    } // just pruned past
                    // == is not possible, since node is regular
                    else // top < node
                    { // not pruned past
                        topHyperparent = contourTree.hyperparents[maskedIndex(topSuperparent)];
                    } // not pruned past
                } // top pruned first
                else if (maskedIndex(topWhen) > maskedIndex(bottomWhen))
                { // bottom pruned first
                    // we prune up to the top of the hyperarc in either case, by updating the bottom superparent
                    bottomSuperparent = contourTree.hyperarcs[maskedIndex(bottomHyperparent)];
                    bottom = contourTree.supernodes[maskedIndex(bottomSuperparent)];
                    bottomWhen = contourTree.whenTransferred[maskedIndex(bottomSuperparent)];
                    // test to see if we've passed the node
                    if (bottom > node)
                    { // just pruned past
                        hyperparent = bottomHyperparent;
                    } // just pruned past
                    // == is not possible, since node is regular
                    else // bottom > node
                    { // not pruned past
                        bottomHyperparent = contourTree.hyperparents[maskedIndex(bottomSuperparent)];
                    } // not pruned past
                } // bottom pruned first
                else
                { // both prune simultaneously
                    // this can happen when both top & bottom prune in the same pass because they belong to the same hyperarc
                    // but this means that they must have the same hyperparent, so we know the correct hyperparent & can check whether it ascends
                    hyperparent = bottomHyperparent;
                } // both prune simultaneously
            } // loop to find pruner


            // we have now set the hyperparent correctly, so we retrieve it's hyperarc to find whether it ascends or descends
            if (isAscending(contourTree.hyperarcs[hyperparent]))
            { // ascending hyperarc
                // the supernodes on the hyperarc are in sorted low-high order
                indexType lowSupernode = contourTree.hypernodes[hyperparent];
                indexType highSupernode;
                // if it's at the right hand end, take the last supernode in the array
                if (maskedIndex(hyperparent) == contourTree.hypernodes.size() - 1)
                    highSupernode = contourTree.supernodes.size() - 1;
                // otherwise, take the supernode just before the next hypernode
                else
                    highSupernode = contourTree.hypernodes[maskedIndex(hyperparent) + 1] - 1;
                // now, the high supernode may be lower than the element, because the node belongs
                // between it and the high end of the hyperarc
                if (contourTree.supernodes[highSupernode] < node)
                    contourTree.superparents[node] = highSupernode;
                // otherwise, we do a binary search of the superarcs
                else
                { // node between high & low
                    // keep going until we span exactly
                    while (highSupernode - lowSupernode > 1)
                    { // binary search
                        // find the midway supernode
                        indexType midSupernode = (lowSupernode + highSupernode) / 2;
                        // test against the node
                        if (contourTree.supernodes[midSupernode] > node)
                            highSupernode = midSupernode;
                        // == can't happen since node is regular
                        else
                            lowSupernode = midSupernode;
                    } // binary search
                    // now we can use the low node as the superparent
                    contourTree.superparents[node] = lowSupernode;
                } // node between high & low
            } // ascending hyperarc
            else
            { // descending hyperarc
                // the supernodes on the hyperarc are in sorted high-low order
                indexType highSupernode = contourTree.hypernodes[hyperparent];
                indexType lowSupernode;
                // if it's at the right hand end, take the last supernode in the array
                if (maskedIndex(hyperparent) == contourTree.hypernodes.size() - 1)
                { // last hyperarc
                    lowSupernode = contourTree.supernodes.size() - 1;
                } // last hyperarc
                // otherwise, take the supernode just before the next hypernode
                else
                { // other hyperarc
                    lowSupernode = contourTree.hypernodes[maskedIndex(hyperparent) + 1] - 1;
                } // other hyperarc
                // now, the low supernode may be higher than the element, because the node belongs
                // between it and the low end of the hyperarc
                if (contourTree.supernodes[lowSupernode] > node)
                    contourTree.superparents[node] = lowSupernode;
                // otherwise, we do a binary search of the superarcs
                else
                { // node between low & high
                    // keep going until we span exactly
                    while (lowSupernode - highSupernode > 1)
                    { // binary search
                        // find the midway supernode
                        indexType midSupernode = (highSupernode + lowSupernode) / 2;
                        // test against the node
                        if (contourTree.supernodes[midSupernode] > node)
                            highSupernode = midSupernode;
                        // == can't happen since node is regular
                        else
                            lowSupernode = midSupernode;
                    } // binary search
                    // now we can use the high node as the superparent
                    contourTree.superparents[node] = highSupernode;
                } // node between low & high
            } // descending hyperarc
        } // regular nodes only
    } // per node
    */
  }

}; // ComputeRegularStructure_LocateSuperarcsOnBoundary.h

} // namespace contourtree_maker_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
