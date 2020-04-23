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


#ifndef vtk_m_worklet_contourtree_augmented_process_contourtree_h
#define vtk_m_worklet_contourtree_augmented_process_contourtree_h

// global includes
#include "vtkm/BinaryOperators.h"
#include "vtkm/BinaryPredicates.h"
#include "vtkm/cont/ArrayHandle.h"
#include "vtkm/cont/ArrayHandleCounting.h"
#include <algorithm>
#include <iomanip>
#include <iostream>

// local includes
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

//VTKM includes
#include <vtkm/Pair.h>
#include <vtkm/Types.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/worklet/contourtree_augmented/ArrayTransforms.h>
#include <vtkm/worklet/contourtree_augmented/ContourTree.h>
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/Branch.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/SuperArcVolumetricComparator.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/SuperNodeBranchComparator.h>

#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/AddDependentWeightHypersweep.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/ComputeBestUpDown.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/ComputeEulerTourFirstNext.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/ComputeEulerTourList.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/ComputeMinMaxValues.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/EulerTour.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/HypwersweepWorklets.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/PointerDoubling.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/PrefixScanHyperarcs.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/SweepHyperarcs.h>

//#define DEBUG_PRINT


namespace process_contourtree_inc_ns =
  vtkm::worklet::contourtree_augmented::process_contourtree_inc;

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

// TODO Many of the post processing routines still need to be parallelized
// Class with routines for post processing the contour tree
class ProcessContourTree
{ // class ProcessContourTree
public:
  // initialises contour tree arrays - rest is done by another class
  ProcessContourTree()
  { // ProcessContourTree()
  } // ProcessContourTree()

  // collect the sorted arcs
  void static CollectSortedArcs(const ContourTree& contourTree,
                                const IdArrayType& sortOrder,
                                EdgePairArray& sortedArcs)
  { // CollectSortedArcs
    // create an array for sorting the arcs
    std::vector<EdgePair> arcSorter;

    // fill it up
    auto arcsPortal = contourTree.Arcs.ReadPortal();
    auto sortOrderPortal = sortOrder.ReadPortal();

    for (vtkm::Id node = 0; node < contourTree.Arcs.GetNumberOfValues(); node++)
    { // per node
      // retrieve ID of target supernode
      vtkm::Id arcTo = arcsPortal.Get(node);

      // if this is true, it is the last pruned vertex & is omitted
      if (NoSuchElement(arcTo))
        continue;

      // otherwise, strip out the flags
      arcTo = MaskedIndex(arcTo);

      // now convert to mesh IDs from sort IDs
      // otherwise, we need to convert the IDs to regular mesh IDs
      vtkm::Id regularID = sortOrderPortal.Get(node);

      // retrieve the regular ID for it
      vtkm::Id regularTo = sortOrderPortal.Get(arcTo);

      // how we print depends on which end has lower ID
      if (regularID < regularTo)
        arcSorter.push_back(EdgePair(regularID, regularTo));
      else
        arcSorter.push_back(EdgePair(regularTo, regularID));
    } // per vertex

    // now sort it
    // Setting saddlePeak reference to the make_ArrayHandle directly does not work
    EdgePairArray tempArray = vtkm::cont::make_ArrayHandle(arcSorter);
    vtkm::cont::Algorithm::Sort(tempArray, SaddlePeakSort());
    vtkm::cont::ArrayCopy(tempArray, sortedArcs);
  } // CollectSortedArcs

  // collect the sorted superarcs
  void static CollectSortedSuperarcs(const ContourTree& contourTree,
                                     const IdArrayType& sortOrder,
                                     EdgePairArray& saddlePeak)
  { // CollectSortedSuperarcs()
    // create an array for sorting the arcs
    std::vector<EdgePair> superarcSorter;

    // fill it up
    auto supernodesPortal = contourTree.Supernodes.ReadPortal();
    auto superarcsPortal = contourTree.Superarcs.ReadPortal();
    auto sortOrderPortal = sortOrder.ReadPortal();

    for (vtkm::Id supernode = 0; supernode < contourTree.Supernodes.GetNumberOfValues();
         supernode++)
    { // per supernode
      // sort ID of the supernode
      vtkm::Id sortID = supernodesPortal.Get(supernode);

      // retrieve ID of target supernode
      vtkm::Id superTo = superarcsPortal.Get(supernode);

      // if this is true, it is the last pruned vertex & is omitted
      if (NoSuchElement(superTo))
        continue;

      // otherwise, strip out the flags
      superTo = MaskedIndex(superTo);

      // otherwise, we need to convert the IDs to regular mesh IDs
      vtkm::Id regularID = sortOrderPortal.Get(MaskedIndex(sortID));

      // retrieve the regular ID for it
      vtkm::Id regularTo = sortOrderPortal.Get(MaskedIndex(supernodesPortal.Get(superTo)));

      // how we print depends on which end has lower ID
      if (regularID < regularTo)
      { // from is lower
        // extra test to catch duplicate edge
        if (superarcsPortal.Get(superTo) != supernode)
        {
          superarcSorter.push_back(EdgePair(regularID, regularTo));
        }
      } // from is lower
      else
      {
        superarcSorter.push_back(EdgePair(regularTo, regularID));
      }
    } // per vertex

    // Setting saddlePeak reference to the make_ArrayHandle directly does not work
    EdgePairArray tempArray = vtkm::cont::make_ArrayHandle(superarcSorter);

    // now sort it
    vtkm::cont::Algorithm::Sort(tempArray, SaddlePeakSort());
    vtkm::cont::ArrayCopy(tempArray, saddlePeak);
  } // CollectSortedSuperarcs()

  // routine to compute the volume for each hyperarc and superarc
  void static ComputeVolumeWeights(const ContourTree& contourTree,
                                   const vtkm::Id nIterations,
                                   IdArrayType& superarcIntrinsicWeight,
                                   IdArrayType& superarcDependentWeight,
                                   IdArrayType& supernodeTransferWeight,
                                   IdArrayType& hyperarcDependentWeight)
  { // ContourTreeMaker::ComputeWeights()
    // start by storing the first sorted vertex ID for each superarc
    IdArrayType firstVertexForSuperparent;
    firstVertexForSuperparent.Allocate(contourTree.Superarcs.GetNumberOfValues());
    superarcIntrinsicWeight.Allocate(contourTree.Superarcs.GetNumberOfValues());
    auto superarcIntrinsicWeightPortal = superarcIntrinsicWeight.WritePortal();
    auto firstVertexForSuperparentPortal = firstVertexForSuperparent.WritePortal();
    auto superparentsPortal = contourTree.Superparents.ReadPortal();
    auto hyperparentsPortal = contourTree.Hyperparents.ReadPortal();
    auto hypernodesPortal = contourTree.Hypernodes.ReadPortal();
    auto hyperarcsPortal = contourTree.Hyperarcs.ReadPortal();
    // auto superarcsPortal = contourTree.Superarcs.ReadPortal();
    auto nodesPortal = contourTree.Nodes.ReadPortal();
    auto whenTransferredPortal = contourTree.WhenTransferred.ReadPortal();
    for (vtkm::Id sortedNode = 0; sortedNode < contourTree.Arcs.GetNumberOfValues(); sortedNode++)
    { // per node in sorted order
      vtkm::Id sortID = nodesPortal.Get(sortedNode);
      vtkm::Id superparent = superparentsPortal.Get(sortID);
      if (sortedNode == 0)
        firstVertexForSuperparentPortal.Set(superparent, sortedNode);
      else if (superparent != superparentsPortal.Get(nodesPortal.Get(sortedNode - 1)))
        firstVertexForSuperparentPortal.Set(superparent, sortedNode);
    } // per node in sorted order
    // now we use that to compute the intrinsic weights
    for (vtkm::Id superarc = 0; superarc < contourTree.Superarcs.GetNumberOfValues(); superarc++)
      if (superarc == contourTree.Superarcs.GetNumberOfValues() - 1)
        superarcIntrinsicWeightPortal.Set(superarc,
                                          contourTree.Arcs.GetNumberOfValues() -
                                            firstVertexForSuperparentPortal.Get(superarc));
      else
        superarcIntrinsicWeightPortal.Set(superarc,
                                          firstVertexForSuperparentPortal.Get(superarc + 1) -
                                            firstVertexForSuperparentPortal.Get(superarc));

    // now initialise the arrays for transfer & dependent weights
    vtkm::cont::ArrayCopy(
      vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, contourTree.Superarcs.GetNumberOfValues()),
      superarcDependentWeight);
    vtkm::cont::ArrayCopy(
      vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, contourTree.Supernodes.GetNumberOfValues()),
      supernodeTransferWeight);
    vtkm::cont::ArrayCopy(
      vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, contourTree.Hyperarcs.GetNumberOfValues()),
      hyperarcDependentWeight);

    // set up the array which tracks which supernodes to deal with on which iteration
    IdArrayType firstSupernodePerIteration;
    vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, nIterations + 1),
                          firstSupernodePerIteration);
    auto firstSupernodePerIterationPortal = firstSupernodePerIteration.WritePortal();
    for (vtkm::Id supernode = 0; supernode < contourTree.Supernodes.GetNumberOfValues();
         supernode++)
    { // per supernode
      vtkm::Id when = MaskedIndex(whenTransferredPortal.Get(supernode));
      if (supernode == 0)
      { // zeroth supernode
        firstSupernodePerIterationPortal.Set(when, supernode);
      } // zeroth supernode
      else if (when != MaskedIndex(whenTransferredPortal.Get(supernode - 1)))
      { // non-matching supernode
        firstSupernodePerIterationPortal.Set(when, supernode);
      } // non-matching supernode
    }   // per supernode
    for (vtkm::Id iteration = 1; iteration < nIterations; ++iteration)
      if (firstSupernodePerIterationPortal.Get(iteration) == 0)
        firstSupernodePerIterationPortal.Set(iteration,
                                             firstSupernodePerIterationPortal.Get(iteration + 1));

    // set the sentinel at the end of the array
    firstSupernodePerIterationPortal.Set(nIterations, contourTree.Supernodes.GetNumberOfValues());

    // now use that array to construct a similar array for hypernodes
    IdArrayType firstHypernodePerIteration;
    firstHypernodePerIteration.Allocate(nIterations + 1);
    auto firstHypernodePerIterationPortal = firstHypernodePerIteration.WritePortal();
    auto supernodeTransferWeightPortal = supernodeTransferWeight.WritePortal();
    auto superarcDependentWeightPortal = superarcDependentWeight.WritePortal();
    auto hyperarcDependentWeightPortal = hyperarcDependentWeight.WritePortal();
    for (vtkm::Id iteration = 0; iteration < nIterations; iteration++)
      firstHypernodePerIterationPortal.Set(
        iteration, hyperparentsPortal.Get(firstSupernodePerIterationPortal.Get(iteration)));
    firstHypernodePerIterationPortal.Set(nIterations, contourTree.Hypernodes.GetNumberOfValues());

    // now iterate, propagating weights inwards
    for (vtkm::Id iteration = 0; iteration < nIterations; iteration++)
    { // per iteration
      // pull the array bounds into register
      vtkm::Id firstSupernode = firstSupernodePerIterationPortal.Get(iteration);
      vtkm::Id lastSupernode = firstSupernodePerIterationPortal.Get(iteration + 1);
      vtkm::Id firstHypernode = firstHypernodePerIterationPortal.Get(iteration);
      vtkm::Id lastHypernode = firstHypernodePerIterationPortal.Get(iteration + 1);

      // Recall that the superarcs are sorted by (iteration, hyperarc), & that all superarcs for a given hyperarc are processed
      // in the same iteration.  Assume therefore that:
      //      i. we now have the intrinsic weight assigned for each superarc, and
      // ii. we also have the transfer weight assigned for each supernode.
      //
      // Suppose we have a sequence of superarcs
      //                      s11 s12 s13 s14 s21 s22 s23 s31
      // with transfer weights at their origins and intrinsic weights along them
      //      sArc                     s11 s12 s13 s14 s21 s22 s23 s31
      //      transfer wt               0   1   2   1   2   3   1   0
      //      intrinsic wt              1   2   1   5   2   6   1   1
      //
      //  now, if we do a prefix sum on each of these and add the two sums together, we get:
      //      sArc                                  s11 s12 s13 s14 s21 s22 s23 s31
      //      hyperparent sNode ID                  s11 s11 s11 s11 s21 s21 s21 s31
      //      transfer weight                       0   1   2   1   2   3   1   0
      //      intrinsic weight                      1   2   1   5   2   6   1   1
      //      sum(xfer + intrinsic)                 1   3   3   6   4   9   2   1
      //  prefix sum (xfer + int)                   1   4   7  13  17  26  28  29
      //  prefix sum (xfer + int - previous hArc)   1   4   7  13  4   13  15  16

      // so, step 1: add xfer + int & store in dependent weight
      for (vtkm::Id supernode = firstSupernode; supernode < lastSupernode; supernode++)
      {
        superarcDependentWeightPortal.Set(supernode,
                                          supernodeTransferWeightPortal.Get(supernode) +
                                            superarcIntrinsicWeightPortal.Get(supernode));
      }

      // step 2: perform prefix sum on the dependent weight range
      for (vtkm::Id supernode = firstSupernode + 1; supernode < lastSupernode; supernode++)
        superarcDependentWeightPortal.Set(supernode,
                                          superarcDependentWeightPortal.Get(supernode) +
                                            superarcDependentWeightPortal.Get(supernode - 1));

      // step 3: subtract out the dependent weight of the prefix to the entire hyperarc. This will be a transfer, but for now, it's easier
      // to show it in serial. NB: Loops backwards so that computation uses the correct value
      // As a bonus, note that we test > firstsupernode, not >=.  This is because we've got unsigned integers, & otherwise it will not terminate
      // But the first is always correct anyway (same reason as the short-cut termination on hyperparent), so we're fine
      for (vtkm::Id supernode = lastSupernode - 1; supernode > firstSupernode; supernode--)
      { // per supernode
        // retrieve the hyperparent & convert to a supernode ID
        vtkm::Id hyperparent = hyperparentsPortal.Get(supernode);
        vtkm::Id hyperparentSuperID = hypernodesPortal.Get(hyperparent);

        // if the hyperparent is the first in the sequence, dependent weight is already correct
        if (hyperparent == firstHypernode)
          continue;

        // otherwise, subtract out the dependent weight *immediately* before the hyperparent's supernode
        superarcDependentWeightPortal.Set(
          supernode,
          superarcDependentWeightPortal.Get(supernode) -
            superarcDependentWeightPortal.Get(hyperparentSuperID - 1));
      } // per supernode

      // step 4: transfer the dependent weight to the hyperarc's target supernode
      for (vtkm::Id hypernode = firstHypernode; hypernode < lastHypernode; hypernode++)
      { // per hypernode
        // last superarc for the hyperarc
        vtkm::Id lastSuperarc;
        // special case for the last hyperarc
        if (hypernode == contourTree.Hypernodes.GetNumberOfValues() - 1)
          // take the last superarc in the array
          lastSuperarc = contourTree.Supernodes.GetNumberOfValues() - 1;
        else
          // otherwise, take the next hypernode's ID and subtract 1
          lastSuperarc = hypernodesPortal.Get(hypernode + 1) - 1;

        // now, given the last superarc for the hyperarc, transfer the dependent weight
        hyperarcDependentWeightPortal.Set(hypernode,
                                          superarcDependentWeightPortal.Get(lastSuperarc));

        // note that in parallel, this will have to be split out as a sort & partial sum in another array
        vtkm::Id hyperarcTarget = MaskedIndex(hyperarcsPortal.Get(hypernode));
        supernodeTransferWeightPortal.Set(hyperarcTarget,
                                          supernodeTransferWeightPortal.Get(hyperarcTarget) +
                                            hyperarcDependentWeightPortal.Get(hypernode));
      } // per hypernode
    }   // per iteration
  }     // ContourTreeMaker::ComputeWeights()

  // routine to compute the branch decomposition by volume
  void static ComputeVolumeBranchDecomposition(const ContourTree& contourTree,
                                               const IdArrayType& superarcDependentWeight,
                                               const IdArrayType& superarcIntrinsicWeight,
                                               IdArrayType& whichBranch,
                                               IdArrayType& branchMinimum,
                                               IdArrayType& branchMaximum,
                                               IdArrayType& branchSaddle,
                                               IdArrayType& branchParent)
  { // ComputeVolumeBranchDecomposition()
    //auto superarcsPortal = contourTree.Superarcs.ReadPortal();
    //auto superarcDependentWeightPortal = superarcDependentWeight.ReadPortal();
    //auto superarcIntrinsicWeightPortal = superarcIntrinsicWeight.ReadPortal();

    // cache the number of non-root supernodes & superarcs
    vtkm::Id nSupernodes = contourTree.Supernodes.GetNumberOfValues();
    vtkm::Id nSuperarcs = nSupernodes - 1;

    // STAGE I:  Find the upward and downwards weight for each superarc, and set up arrays
    IdArrayType upWeight;
    upWeight.Allocate(nSuperarcs);
    //auto upWeightPortal = upWeight.WritePortal();
    IdArrayType downWeight;
    downWeight.Allocate(nSuperarcs);
    //auto downWeightPortal = downWeight.WritePortal();
    IdArrayType bestUpward;
    auto noSuchElementArray =
      vtkm::cont::ArrayHandleConstant<vtkm::Id>((vtkm::Id)NO_SUCH_ELEMENT, nSupernodes);
    vtkm::cont::ArrayCopy(noSuchElementArray, bestUpward);
    IdArrayType bestDownward;
    vtkm::cont::ArrayCopy(noSuchElementArray, bestDownward);
    vtkm::cont::ArrayCopy(noSuchElementArray, whichBranch);
    //auto bestUpwardPortal = bestUpward.WritePortal();
    //auto bestDownwardPortal = bestDownward.WritePortal();

    // STAGE II: Pick the best (largest volume) edge upwards and downwards
    // II A. Pick the best upwards weight by sorting on lower vertex then processing by segments
    // II A 1.  Sort the superarcs by lower vertex
    // II A 2.  Per segment, best superarc writes to the best upwards array
    vtkm::cont::ArrayHandle<EdgePair> superarcList;
    vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<EdgePair>(EdgePair(-1, -1), nSuperarcs),
                          superarcList);
    //auto superarcListPortal = superarcList.WritePortal();
    vtkm::Id totalVolume = contourTree.Nodes.GetNumberOfValues();
#ifdef DEBUG_PRINT
    std::cout << "Total Volume: " << totalVolume << std::endl;
#endif
    // NB: Last element in array is guaranteed to be root superarc to infinity,
    // so we can easily skip it by not indexing to the full size
    for (vtkm::Id superarc = 0; superarc < nSuperarcs; superarc++)
    { // per superarc
      if (IsAscending(contourTree.Superarcs.ReadPortal().Get(superarc)))
      { // ascending superarc
        superarcList.WritePortal().Set(
          superarc,
          EdgePair(superarc, MaskedIndex(contourTree.Superarcs.ReadPortal().Get(superarc))));
        upWeight.WritePortal().Set(superarc, superarcDependentWeight.ReadPortal().Get(superarc));
        // at the inner end, dependent weight is the total in the subtree.  Then there are vertices along the edge itself (intrinsic weight), including the supernode at the outer end
        // So, to get the "dependent" weight in the other direction, we start with totalVolume - dependent, then subtract (intrinsic - 1)
        downWeight.WritePortal().Set(
          superarc,
          (totalVolume - superarcDependentWeight.ReadPortal().Get(superarc)) +
            (superarcIntrinsicWeight.ReadPortal().Get(superarc) - 1));
      } // ascending superarc
      else
      { // descending superarc
        superarcList.WritePortal().Set(
          superarc,
          EdgePair(MaskedIndex(contourTree.Superarcs.ReadPortal().Get(superarc)), superarc));
        downWeight.WritePortal().Set(superarc, superarcDependentWeight.ReadPortal().Get(superarc));
        // at the inner end, dependent weight is the total in the subtree.  Then there are vertices along the edge itself (intrinsic weight), including the supernode at the outer end
        // So, to get the "dependent" weight in the other direction, we start with totalVolume - dependent, then subtract (intrinsic - 1)
        upWeight.WritePortal().Set(
          superarc,
          (totalVolume - superarcDependentWeight.ReadPortal().Get(superarc)) +
            (superarcIntrinsicWeight.ReadPortal().Get(superarc) - 1));
      } // descending superarc
    }   // per superarc

#ifdef DEBUG_PRINT
    std::cout << "II A. Weights Computed" << std::endl;
    PrintHeader(upWeight.GetNumberOfValues());
    //PrintIndices("Intrinsic Weight", superarcIntrinsicWeight);
    //PrintIndices("Dependent Weight", superarcDependentWeight);
    PrintIndices("Upwards Weight", upWeight);
    PrintIndices("Downwards Weight", downWeight);
    std::cout << std::endl;
#endif

    // II B. Pick the best downwards weight by sorting on upper vertex then processing by segments
    // II B 1.      Sort the superarcs by upper vertex
    IdArrayType superarcSorter;
    superarcSorter.Allocate(nSuperarcs);
    //auto superarcSorterPortal = superarcSorter.WritePortal();
    for (vtkm::Id superarc = 0; superarc < nSuperarcs; superarc++)
      superarcSorter.WritePortal().Set(superarc, superarc);

    vtkm::cont::Algorithm::Sort(
      superarcSorter,
      process_contourtree_inc_ns::SuperArcVolumetricComparator(upWeight, superarcList, false));

    // II B 2.  Per segment, best superarc writes to the best upward array
    for (vtkm::Id superarc = 0; superarc < nSuperarcs; superarc++)
    { // per superarc
      vtkm::Id superarcID = superarcSorter.ReadPortal().Get(superarc);
      const EdgePair& edge = superarcList.ReadPortal().Get(superarcID);
      // if it's the last one
      if (superarc == nSuperarcs - 1)
        bestDownward.WritePortal().Set(edge.second, edge.first);
      else
      { // not the last one
        const EdgePair& nextEdge =
          superarcList.ReadPortal().Get(superarcSorter.ReadPortal().Get(superarc + 1));
        // if the next edge belongs to another, we're the highest
        if (nextEdge.second != edge.second)
          bestDownward.WritePortal().Set(edge.second, edge.first);
      } // not the last one
    }   // per superarc

    // II B 3.  Repeat for lower vertex
    vtkm::cont::Algorithm::Sort(
      superarcSorter,
      process_contourtree_inc_ns::SuperArcVolumetricComparator(downWeight, superarcList, true));

    // II B 2.  Per segment, best superarc writes to the best upward array
    for (vtkm::Id superarc = 0; superarc < nSuperarcs; superarc++)
    { // per superarc
      vtkm::Id superarcID = superarcSorter.ReadPortal().Get(superarc);
      const EdgePair& edge = superarcList.ReadPortal().Get(superarcID);
      // if it's the last one
      if (superarc == nSuperarcs - 1)
        bestUpward.WritePortal().Set(edge.first, edge.second);
      else
      { // not the last one
        const EdgePair& nextEdge =
          superarcList.ReadPortal().Get(superarcSorter.ReadPortal().Get(superarc + 1));
        // if the next edge belongs to another, we're the highest
        if (nextEdge.first != edge.first)
          bestUpward.WritePortal().Set(edge.first, edge.second);
      } // not the last one
    }   // per superarc

#ifdef DEBUG_PRINT
    std::cout << "II. Best Edges Selected" << std::endl;
    PrintHeader(bestUpward.GetNumberOfValues());
    PrintIndices("Best Upwards", bestUpward);
    PrintIndices("Best Downwards", bestDownward);
    std::cout << std::endl;
#endif

    ProcessContourTree::ComputeBranchData(contourTree,
                                          false,
                                          whichBranch,
                                          branchMinimum,
                                          branchMaximum,
                                          branchSaddle,
                                          branchParent,
                                          bestUpward,
                                          bestDownward);
  }

  // routine to compute the branch decomposition by volume
  void static ComputeBranchData(const ContourTree& contourTree,
                                const bool printTime,
                                IdArrayType& whichBranch,
                                IdArrayType& branchMinimum,
                                IdArrayType& branchMaximum,
                                IdArrayType& branchSaddle,
                                IdArrayType& branchParent,
                                IdArrayType& bestUpward,
                                IdArrayType& bestDownward)
  { // ComputeBranchData()

    // Set up constants
    vtkm::Id nSupernodes = contourTree.Supernodes.GetNumberOfValues();
    auto noSuchElementArray =
      vtkm::cont::ArrayHandleConstant<vtkm::Id>((vtkm::Id)NO_SUCH_ELEMENT, nSupernodes);
    vtkm::cont::ArrayCopy(noSuchElementArray, whichBranch);

    vtkm::cont::Timer timer;
    timer.Start();

    // STAGE III: For each vertex, identify which neighbours are on same branch
    // Let v = BestUp(u). Then if u = BestDown(v), copy BestUp(u) to whichBranch(u)
    // Otherwise, let whichBranch(u) = BestUp(u) | TERMINAL to mark the end of the side branch
    // NB 1: Leaves already have the flag set, but it's redundant so its safe
    // NB 2: We don't need to do it downwards because it's symmetric
    vtkm::cont::Invoker invoke;
    vtkm::worklet::contourtree_augmented::process_contourtree_inc::PropagateBestUpDown
      propagateBestUpDownWorklet;
    invoke(propagateBestUpDownWorklet, bestUpward, bestDownward, whichBranch);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "----------------//---------------- Propagating Branches took " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Propagating Branches took.\n", timer.GetElapsedTime());
    }

#ifdef DEBUG_PRINT
    std::cout << "III. Branch Neighbours Identified" << std::endl;
    PrintHeader(whichBranch.GetNumberOfValues());
    PrintIndices("Which Branch", whichBranch);
    std::cout << std::endl;
#endif
    timer.Reset();
    timer.Start();

    // STAGE IV: Use pointer-doubling on whichBranch to propagate branches
    // Compute the number of log steps required in this pass
    vtkm::Id numLogSteps = 1;
    for (vtkm::Id shifter = nSupernodes; shifter != 0; shifter >>= 1)
      numLogSteps++;

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::PointerDoubling pointerDoubling(
      nSupernodes);

    // use pointer-doubling to build the branches
    for (vtkm::Id iteration = 0; iteration < numLogSteps; iteration++)
    { // per iteration
      invoke(pointerDoubling, whichBranch);
    } // per iteration


    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "----------------//---------------- Branch Point Doubling " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Branch Point Doubling.\n", timer.GetElapsedTime());
    }

#ifdef DEBUG_PRINT
    std::cout << "IV. Branch Chains Propagated" << std::endl;
    PrintHeader(whichBranch.GetNumberOfValues());
    PrintIndices("Which Branch", whichBranch);
    std::cout << std::endl;
#endif

    timer.Reset();
    timer.Start();

    // Initialise
    IdArrayType chainToBranch;
    vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, nSupernodes), chainToBranch);

    // Set 1 to every relevant
    vtkm::worklet::contourtree_augmented::process_contourtree_inc::PrepareChainToBranch
      prepareChainToBranchWorklet;
    invoke(prepareChainToBranchWorklet, whichBranch, chainToBranch);

    // Prefix scanto get IDs
    vtkm::cont::Algorithm::ScanInclusive(chainToBranch, chainToBranch);

    vtkm::Id nBranches = chainToBranch.ReadPortal().Get(chainToBranch.GetNumberOfValues() - 1);

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::FinaliseChainToBranch
      finaliseChainToBranchWorklet;
    invoke(finaliseChainToBranchWorklet, whichBranch, chainToBranch);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "----------------//---------------- Create Chain to Branch " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Create Chain to Branch.\n", timer.GetElapsedTime());
    }


    timer.Reset();
    timer.Start();
    // V B.  Create the arrays for the branches
    auto noSuchElementArrayNBranches =
      vtkm::cont::ArrayHandleConstant<vtkm::Id>((vtkm::Id)NO_SUCH_ELEMENT, nBranches);
    vtkm::cont::ArrayCopy(noSuchElementArrayNBranches, branchMinimum);
    vtkm::cont::ArrayCopy(noSuchElementArrayNBranches, branchMaximum);
    vtkm::cont::ArrayCopy(noSuchElementArrayNBranches, branchSaddle);
    vtkm::cont::ArrayCopy(noSuchElementArrayNBranches, branchParent);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "----------------//---------------- Array Coppying " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Array Coppying.\n", timer.GetElapsedTime());
    }

#ifdef DEBUG_PRINT
    std::cout << "V. Branch Arrays Created" << std::endl;
    PrintHeader(chainToBranch.GetNumberOfValues());
    PrintIndices("Chain To Branch", chainToBranch);
    PrintHeader(nBranches);
    PrintIndices("Branch Minimum", branchMinimum);
    PrintIndices("Branch Maximum", branchMaximum);
    PrintIndices("Branch Saddle", branchSaddle);
    PrintIndices("Branch Parent", branchParent);
#endif

    timer.Reset();
    timer.Start();

    IdArrayType supernodeSorter;
    vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, nSupernodes),
                          supernodeSorter);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "----------------//---------------- Supernode Sorter " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Supernode Sorter.\n", timer.GetElapsedTime());
    }

    timer.Reset();
    timer.Start();
    vtkm::cont::Algorithm::Sort(
      supernodeSorter,
      process_contourtree_inc_ns::SuperNodeBranchComparator(whichBranch, contourTree.Supernodes));

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "----------------//---------------- VTKM Sorting " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for VTKM Sorting.\n", timer.GetElapsedTime());
    }

    timer.Reset();
    timer.Start();
    IdArrayType permutedBranches;
    permutedBranches.Allocate(nSupernodes);
    PermuteArray<vtkm::Id>(whichBranch, supernodeSorter, permutedBranches);

    IdArrayType permutedRegularID;
    permutedRegularID.Allocate(nSupernodes);
    PermuteArray<vtkm::Id>(contourTree.Supernodes, supernodeSorter, permutedRegularID);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "----------------//---------------- Array Permuting " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Array Permuting.\n", timer.GetElapsedTime());
    }

#ifdef DEBUG_PRINT
    std::cout << "VI A. Sorted into Branches" << std::endl;
    PrintHeader(nSupernodes);
    PrintIndices("Supernode IDs", supernodeSorter);
    PrintIndices("Branch", permutedBranches);
    PrintIndices("Regular ID", permutedRegularID);
#endif

    timer.Reset();
    timer.Start();

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::WhichBranchNewId
      whichBranchNewIdWorklet;
    invoke(whichBranchNewIdWorklet, chainToBranch, whichBranch);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "----------------//---------------- Which Branch Initialisation " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Which Branch Initialisation.\n", timer.GetElapsedTime());
    }

    timer.Reset();
    timer.Start();

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::BranchMinMaxSet
      branchMinMaxSetWorklet(nSupernodes);
    invoke(branchMinMaxSetWorklet, supernodeSorter, whichBranch, branchMinimum, branchMaximum);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "----------------//---------------- Branch min/max setting " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Branch min/max setting.\n", timer.GetElapsedTime());
    }

#ifdef DEBUG_PRINT
    std::cout << "VI. Branches Set" << std::endl;
    PrintHeader(nBranches);
    PrintIndices("Branch Maximum", branchMaximum);
    PrintIndices("Branch Minimum", branchMinimum);
    PrintIndices("Branch Saddle", branchSaddle);
    PrintIndices("Branch Parent", branchParent);
#endif

    timer.Reset();
    timer.Start();

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::BranchSaddleParentSet
      branchSaddleParentSetWorklet;
    invoke(branchSaddleParentSetWorklet,
           whichBranch,
           branchMinimum,
           branchMaximum,
           bestDownward,
           bestUpward,
           branchSaddle,
           branchParent);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "----------------//---------------- Branch parents & Saddles setting " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Branch parents & Saddles setting.\n", timer.GetElapsedTime());
    }

#ifdef DEBUG_PRINT
    std::cout << "VII. Branches Constructed" << std::endl;
    PrintHeader(nBranches);
    PrintIndices("Branch Maximum", branchMaximum);
    PrintIndices("Branch Minimum", branchMinimum);
    PrintIndices("Branch Saddle", branchSaddle);
    PrintIndices("Branch Parent", branchParent);
#endif

  } // ComputeBranchData()

  // Create branch decomposition from contour tree
  template <typename T, typename StorageType>
  static process_contourtree_inc_ns::Branch<T>* ComputeBranchDecomposition(
    const IdArrayType& contourTreeSuperparents,
    const IdArrayType& contourTreeSupernodes,
    const IdArrayType& whichBranch,
    const IdArrayType& branchMinimum,
    const IdArrayType& branchMaximum,
    const IdArrayType& branchSaddle,
    const IdArrayType& branchParent,
    const IdArrayType& sortOrder,
    const vtkm::cont::ArrayHandle<T, StorageType>& dataField,
    bool dataFieldIsSorted)
  {
    return process_contourtree_inc_ns::Branch<T>::ComputeBranchDecomposition(
      contourTreeSuperparents,
      contourTreeSupernodes,
      whichBranch,
      branchMinimum,
      branchMaximum,
      branchSaddle,
      branchParent,
      sortOrder,
      dataField,
      dataFieldIsSorted);
  }

  void static findMinMaxParallel(const IdArrayType supernodes,
                                 const cont::ArrayHandle<Vec<Id, 2>> tourEdges,
                                 const bool isMin,
                                 IdArrayType minMaxIndex,
                                 IdArrayType parents)
  {
    //
    // Set up some useful portals
    //
    auto parentsPortal = parents.WritePortal();
    auto tourEdgesPortal = tourEdges.ReadPortal();

    //
    // Set initial values
    //
    Id root = tourEdgesPortal.Get(0)[0];
    parentsPortal.Set(root, root);

    //
    // Find what the first and last occurence of a vertex is in the euler tour.
    //
    cont::ArrayHandle<vtkm::Pair<Id, Id>> firstLastVertex;
    firstLastVertex.Allocate(supernodes.GetNumberOfValues());
    for (int i = 0; i < firstLastVertex.GetNumberOfValues(); i++)
    {
      firstLastVertex.WritePortal().Set(i,
                                        { (vtkm::Id)NO_SUCH_ELEMENT, (vtkm::Id)NO_SUCH_ELEMENT });
    }

    auto firstLastVertexPortal = firstLastVertex.WritePortal();

    for (int i = 0; i < tourEdges.GetNumberOfValues(); i++)
    {
      // Forward Edge
      if (firstLastVertexPortal.Get(tourEdgesPortal.Get(i)[1]).first == NO_SUCH_ELEMENT)
      {
        //printf("The parent of %d is %d.\n", tourEdges[i][1], tourEdges[i][0]);
        parentsPortal.Set(tourEdgesPortal.Get(i)[1], tourEdgesPortal.Get(i)[0]);

        //firstLastVertex[tourEdges.Get(i)[1]].first = i;

        firstLastVertexPortal.Set(
          tourEdgesPortal.Get(i)[1],
          { i, firstLastVertexPortal.Get(tourEdgesPortal.Get(i)[1]).second });
      }

      //firstLastVertex[tourEdges.Get(i)[0]].second = i;
      firstLastVertexPortal.Set(tourEdgesPortal.Get(i)[1],
                                { firstLastVertexPortal.Get(tourEdgesPortal.Get(i)[1]).first, i });
    }

    firstLastVertexPortal.Set(root, { 0, supernodes.GetNumberOfValues() - 1 });

    //
    // For every vertex look at the subrray between the first and last occurence and find the min/max values in it
    //
    vtkm::worklet::contourtree_augmented::process_contourtree_inc::ComputeMinMaxValues
      computeMinMax(isMin);

    vtkm::cont::Invoker Invoke;
    Invoke(computeMinMax, supernodes, firstLastVertex, tourEdges, minMaxIndex);
  }

  void static findMinMax(const IdArrayType::ReadPortalType supernodes,
                         const cont::ArrayHandle<Vec<Id, 2>>::ReadPortalType tourEdges,
                         const bool isMin,
                         IdArrayType::WritePortalType minMaxIndex,
                         IdArrayType::WritePortalType parents)
  {
    Id root = tourEdges.Get(0)[0];

    struct VertexData
    {
      long long distance;
      unsigned long index;
    };

    std::vector<VertexData> vertexData(static_cast<unsigned long>(supernodes.GetNumberOfValues()),
                                       { -1, 0 });
    for (unsigned long i = 0; i < vertexData.size(); i++)
    {
      vertexData[i].index = i;
    }

    parents.Set(root, root);
    vertexData[static_cast<unsigned long>(root)].distance = 0;

    for (int i = 0; i < tourEdges.GetNumberOfValues(); i++)
    {
      const Vec<Id, 2> e = tourEdges.Get(i);
      if (-1 == vertexData[static_cast<unsigned long>(e[1])].distance)
      {
        parents.Set(e[1], e[0]);
        vertexData[static_cast<unsigned long>(e[1])].distance =
          vertexData[static_cast<unsigned long>(e[0])].distance + 1;
      }
    }

    std::sort(vertexData.begin(), vertexData.end(), [](const VertexData& a, const VertexData& b) {
      return a.distance > b.distance;
    });

    for (int i = 0; i < minMaxIndex.GetNumberOfValues(); i++)
    {
      minMaxIndex.Set(i, i);
    }

    for (unsigned int i = 0; i < vertexData.size(); i++)
    {
      Id vertex = static_cast<Id>(vertexData[i].index);
      Id parent = parents.Get(vertex);

      Id vertexValue = MaskedIndex(supernodes.Get(minMaxIndex.Get(vertex)));
      Id parentValue = MaskedIndex(supernodes.Get(minMaxIndex.Get(parent)));

      if ((true == isMin && vertexValue < parentValue) ||
          (false == isMin && vertexValue > parentValue))
      {
        minMaxIndex.Set(parent, minMaxIndex.Get(vertex));
      }
    }
  }

  // routine to compute the branch decomposition by volume
  void static ComputeHeightBranchDecomposition(const ContourTree& contourTree,
                                               const cont::ArrayHandle<Float64> fieldValues,
                                               const IdArrayType& ctSortOrder,
                                               const bool useParallelMinMaxSearch,
                                               IdArrayType& whichBranch,
                                               IdArrayType& branchMinimum,
                                               IdArrayType& branchMaximum,
                                               IdArrayType& branchSaddle,
                                               IdArrayType& branchParent)
  { // ComputeHeightBranchDecomposition()

    // Cache the number of non-root supernodes & superarcs
    vtkm::Id nSupernodes = contourTree.Supernodes.GetNumberOfValues();
    auto noSuchElementArray =
      vtkm::cont::ArrayHandleConstant<vtkm::Id>((vtkm::Id)NO_SUCH_ELEMENT, nSupernodes);

    // STAGE I:  Find the upward and downwards weight for each superarc, and set up arrays
    IdArrayType bestUpward;
    IdArrayType bestDownward;
    vtkm::cont::ArrayCopy(noSuchElementArray, bestUpward);
    vtkm::cont::ArrayCopy(noSuchElementArray, bestDownward);

    //
    // Compute Euler Tours
    //
    cont::ArrayHandle<Vec<Id, 2>> minTourEdges, maxTourEdges;

    minTourEdges.Allocate(2 * (nSupernodes - 1));
    maxTourEdges.Allocate(2 * (nSupernodes - 1));

    // Compute the Euler Tour
    process_contourtree_inc_ns::EulerTour tour;
    tour.computeEulerTour(contourTree.Superarcs.ReadPortal());

    // Reroot the Euler Tour at the global min
    tour.getTourAtRoot(MaskedIndex(contourTree.Superparents.ReadPortal().Get(0)),
                       minTourEdges.WritePortal());

    // Reroot the Euler Tour at the global max
    tour.getTourAtRoot(MaskedIndex(contourTree.Superparents.ReadPortal().Get(
                         contourTree.Nodes.GetNumberOfValues() - 1)),
                       maxTourEdges.WritePortal());

    //
    // Compute Min/Max per subtree
    //
    IdArrayType minValues, minParents, maxValues, maxParents;
    vtkm::cont::ArrayCopy(noSuchElementArray, minValues);
    vtkm::cont::ArrayCopy(noSuchElementArray, minParents);
    vtkm::cont::ArrayCopy(noSuchElementArray, maxValues);
    vtkm::cont::ArrayCopy(noSuchElementArray, maxParents);

    // Finding the min/max for every subtree can either be done in parallel or serial
    // The parallel implementation is not work efficient and will not work well on a single/few cores
    // This is why I have left the option to do a BFS style search in serial instead of doing an a prefix min/max for every subtree in the euler tour
    if (false == useParallelMinMaxSearch)
    {
      ProcessContourTree::findMinMax(contourTree.Supernodes.ReadPortal(),
                                     minTourEdges.ReadPortal(),
                                     true,
                                     minValues.WritePortal(),
                                     minParents.WritePortal());

      ProcessContourTree::findMinMax(contourTree.Supernodes.ReadPortal(),
                                     maxTourEdges.ReadPortal(),
                                     false,
                                     maxValues.WritePortal(),
                                     maxParents.WritePortal());
    }
    else
    {
      ProcessContourTree::findMinMaxParallel(
        contourTree.Supernodes, minTourEdges, true, minValues, minParents);

      ProcessContourTree::findMinMaxParallel(
        contourTree.Supernodes, maxTourEdges, false, maxValues, maxParents);
    }

    //
    // Compute bestUp and bestDown
    //
    vtkm::worklet::contourtree_augmented::process_contourtree_inc::ComputeBestUpDown
      bestUpDownWorklet;

    vtkm::cont::Invoker Invoke;
    Invoke(bestUpDownWorklet,
           tour.first,
           contourTree.Nodes,
           contourTree.Supernodes,
           minValues,
           minParents,
           maxValues,
           maxParents,
           ctSortOrder,
           tour.edges,
           fieldValues,
           bestUpward,  // output
           bestDownward // output
           );

    ProcessContourTree::ComputeBranchData(contourTree,
                                          false,
                                          whichBranch,
                                          branchMinimum,
                                          branchMaximum,
                                          branchSaddle,
                                          branchParent,
                                          bestUpward,
                                          bestDownward);

  } // ComputeHeightBranchDecomposition()

  // routine to compute the branch decomposition by volume
  void static ComputeHeightBranchDecompositionNew(const ContourTree& contourTree,
                                                  const cont::ArrayHandle<Float64> fieldValues,
                                                  const IdArrayType& ctSortOrder,
                                                  const int printTime,
                                                  const vtkm::Id nIterations,
                                                  IdArrayType& whichBranch,
                                                  IdArrayType& branchMinimum,
                                                  IdArrayType& branchMaximum,
                                                  IdArrayType& branchSaddle,
                                                  IdArrayType& branchParent)
  { // ComputeHeightBranchDecomposition()

    vtkm::cont::Timer timerTotal;
    timerTotal.Start();

    vtkm::cont::Timer timerTotalAll;
    timerTotalAll.Start();

    vtkm::cont::Timer timer;
    timer.Start();

    double minHypersweepTime = 0.0;
    double maxHypersweepTime = 0.0;
    double bothHypersweepTime = 0.0;

    // Cache the number of non-root supernodes & superarcs
    vtkm::Id nSupernodes = contourTree.Supernodes.GetNumberOfValues();
    auto noSuchElementArray =
      vtkm::cont::ArrayHandleConstant<vtkm::Id>((vtkm::Id)NO_SUCH_ELEMENT, nSupernodes);

    // Set up bestUpward and bestDownward array, these are the things we want to compute in this routine.
    IdArrayType bestUpward, bestDownward;
    vtkm::cont::ArrayCopy(noSuchElementArray, bestUpward);
    vtkm::cont::ArrayCopy(noSuchElementArray, bestDownward);

    // maxValues and minValues store the values from the max and min hypersweep respectively.
    IdArrayType minValues, maxValues;
    vtkm::cont::ArrayCopy(contourTree.Supernodes, maxValues);
    vtkm::cont::ArrayCopy(contourTree.Supernodes, minValues);

    // Store the direction of the superarcs in the min and max hypersweep (find a way to get rid of these, the only differing direction is on the path from the root to the min/max).
    IdArrayType minParents, maxParents;
    vtkm::cont::ArrayCopy(contourTree.Superarcs, minParents);
    vtkm::cont::ArrayCopy(contourTree.Superarcs, maxParents);

    // Cache the glonal minimum and global maximum (these will be the roots in the min and max hypersweep)
    Id minSuperNode = MaskedIndex(contourTree.Superparents.ReadPortal().Get(0));
    Id maxSuperNode = MaskedIndex(
      contourTree.Superparents.ReadPortal().Get(contourTree.Nodes.GetNumberOfValues() - 1));

    timer.Stop();
    vtkm::Float64 ellapsedTime = timer.GetElapsedTime();
    if (printTime >= 2)
    {
      //std::cout << "---------------- Initialising Array too  " << ellapsedTime << " seconds." << std::endl;
      printf("%.6f for Initialising Array.\n", timer.GetElapsedTime());
    }

    timer.Reset();
    timer.Start();

    // Find the path from the global minimum to the root, not parallelisable (but it's fast, no need to parallelise)
    auto minPath = findSuperPathToRoot(contourTree.Superarcs.ReadPortal(), minSuperNode);

    // Find the path from the global minimum to the root, not parallelisable (but it's fast, no need to parallelise)
    auto maxPath = findSuperPathToRoot(contourTree.Superarcs.ReadPortal(), maxSuperNode);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "---------------- Finding min/max paths to the root took \t\t" << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Finding min/max paths to the root.\n", timer.GetElapsedTime());
    }

    timer.Reset();
    timer.Start();

    // Reserve the direction of the superarcs on the min path.
    for (uint i = 1; i < minPath.size(); i++)
    {
      minParents.WritePortal().Set(minPath[i], minPath[i - 1]);
    }
    minParents.WritePortal().Set(minPath[0], 0);

    // Reserve the direction of the superarcs on the max path.
    for (uint i = 1; i < maxPath.size(); i++)
    {
      maxParents.WritePortal().Set(maxPath[i], maxPath[i - 1]);
    }
    maxParents.WritePortal().Set(maxPath[0], 0);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "---------------- Rerooting took " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Rerooting.\n", timer.GetElapsedTime());
    }

    timer.Reset();
    timer.Start();

    vtkm::cont::Invoker Invoke;
    vtkm::worklet::contourtree_augmented::process_contourtree_inc::UnmaskArray unmaskArrayWorklet;

    Invoke(unmaskArrayWorklet, minValues);
    Invoke(unmaskArrayWorklet, maxValues);

    // Thse arrays hold the changes hyperarcs in the min and max hypersweep respectively
    vtkm::cont::ArrayHandle<vtkm::Id> minHyperarcs, maxHyperarcs;
    vtkm::cont::ArrayCopy(contourTree.Hyperarcs, minHyperarcs);
    vtkm::cont::ArrayCopy(contourTree.Hyperarcs, maxHyperarcs);

    vtkm::cont::ArrayHandle<vtkm::Id> minHyperparents, maxHyperparents;
    vtkm::cont::ArrayCopy(contourTree.Hyperparents, minHyperparents);
    vtkm::cont::ArrayCopy(contourTree.Hyperparents, maxHyperparents);

    for (uint i = 0; i < minPath.size(); i++)
    {
      // Set a unique dummy Id (something that the prefix scan by key will leave alone)
      minHyperparents.WritePortal().Set(minPath[i],
                                        contourTree.Hypernodes.GetNumberOfValues() + minPath[i]);
    }

    for (uint i = 0; i < maxPath.size(); i++)
    {
      // Set a unique dummy Id (something that the prefix scan by key will leave alone)
      maxHyperparents.WritePortal().Set(maxPath[i],
                                        contourTree.Hypernodes.GetNumberOfValues() + maxPath[i]);
    }

    // These arrays hold the number of nodes in each hypearcs that are on the min or max path for the min and max hypersweep respectively.
    vtkm::cont::ArrayHandle<vtkm::Id> minHowManyUsed, maxHowManyUsed;
    vtkm::cont::ArrayCopy(
      vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, maxHyperarcs.GetNumberOfValues()),
      minHowManyUsed);
    vtkm::cont::ArrayCopy(
      vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, maxHyperarcs.GetNumberOfValues()),
      maxHowManyUsed);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "---------------- Initialising more stuff took " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Initialising more stuff.\n", timer.GetElapsedTime());
    }

    // Total HS Timer
    timer.Reset();
    timer.Start();


    //timer.Reset();
    //timer.Start();

    //
    // Min Hypersweep
    //
    const auto minOperator = vtkm::Minimum();

    editHyperarcs(contourTree.Hyperparents.ReadPortal(),
                  minPath,
                  minHyperarcs.WritePortal(),
                  minHowManyUsed.WritePortal());

    vtkm::cont::Timer hypersweepTimer;
    hypersweepTimer.Start();

    findMinMaxNewSimple<decltype(vtkm::Minimum())>(contourTree.Supernodes,
                                                   contourTree.Hypernodes,
                                                   minHyperarcs,
                                                   contourTree.Hyperparents,
                                                   minHyperparents,
                                                   contourTree.WhenTransferred,
                                                   minHowManyUsed,
                                                   nIterations,
                                                   vtkm::Minimum(),
                                                   minValues);

    hypersweepTimer.Stop();
    minHypersweepTime = hypersweepTimer.GetElapsedTime();
    if (printTime >= 1)
    {
      //std::cout << "---------------- Initialising more stuff took " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for the Min Hypersweep.\n", hypersweepTimer.GetElapsedTime());
    }

    hypersweepTimer.Reset();
    hypersweepTimer.Start();

    fixPath(vtkm::Minimum(), minPath, minValues.WritePortal());

    //
    // Max Hypersweep
    //
    const auto maxOperator = vtkm::Maximum();

    editHyperarcs(contourTree.Hyperparents.ReadPortal(),
                  maxPath,
                  maxHyperarcs.WritePortal(),
                  maxHowManyUsed.WritePortal());

    findMinMaxNewSimple<decltype(vtkm::Maximum())>(contourTree.Supernodes,
                                                   contourTree.Hypernodes,
                                                   maxHyperarcs,
                                                   contourTree.Hyperparents,
                                                   maxHyperparents,
                                                   contourTree.WhenTransferred,
                                                   maxHowManyUsed,
                                                   nIterations,
                                                   vtkm::Maximum(),
                                                   maxValues);

    fixPath(vtkm::Maximum(), maxPath, maxValues.WritePortal());

    timer.Stop();
    hypersweepTimer.Stop();
    maxHypersweepTime = hypersweepTimer.GetElapsedTime();
    bothHypersweepTime = timer.GetElapsedTime();
    if (printTime >= 1)
    {
      //std::cout << "---------------- HS TOTAL ---------------- Total Hypersweep took " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Max Hypersweep.\n", hypersweepTimer.GetElapsedTime());
      printf("%.6f for BOTH Hypersweeps.\n", timer.GetElapsedTime());
    }

    timer.Reset();
    timer.Start();

    IdArrayType maxValuesCopy, minValuesCopy;
    vtkm::cont::ArrayCopy(maxValues, maxValuesCopy);
    vtkm::cont::ArrayCopy(minValues, minValuesCopy);

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::IncorporateParent<decltype(
      vtkm::Minimum())>
      incorporateParentMinimumWorklet(minOperator);
    Invoke(incorporateParentMinimumWorklet, minParents, contourTree.Supernodes, minValues);

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::IncorporateParent<decltype(
      vtkm::Maximum())>
      incorporateParentMaximumWorklet(maxOperator);
    Invoke(incorporateParentMaximumWorklet, maxParents, contourTree.Supernodes, maxValues);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "---------------- Incorporating Parent took " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Incorporating Parent.\n", timer.GetElapsedTime());
    }

    timer.Reset();
    timer.Start();

    vtkm::cont::ArrayHandle<vtkm::worklet::contourtree_augmented::EdgeData> arcs;
    arcs.Allocate(contourTree.Superarcs.GetNumberOfValues() * 2 - 2);

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::InitialiseArcs initArcs(
      0, contourTree.Arcs.GetNumberOfValues() - 1, minPath[minPath.size() - 1]);

    Invoke(initArcs, minParents, maxParents, minValues, maxValues, contourTree.Superarcs, arcs);

    //
    // Set whether an arc is up or down arcs. Parallelisable. No HS.
    //

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "---------------- Initialising arcs took " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Initialising arcs.\n", timer.GetElapsedTime());
    }

    //
    // Compute the height of all subtrees using the min and max
    //
    timer.Reset();
    timer.Start();

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::ComputeSubtreeHeight
      computeSubtreeHeight;
    Invoke(computeSubtreeHeight, fieldValues, ctSortOrder, contourTree.Supernodes, arcs);

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "---------------- Computing subtree height took " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Computing subtree height.\n", timer.GetElapsedTime());
    }

    //
    // Sort to find the bestUp and Down
    //
    timer.Reset();
    timer.Start();

    vtkm::cont::Algorithm::Sort(arcs, vtkm::SortLess());

    timer.Stop();
    if (printTime >= 2)
    {
      //std::cout << "---------------- Sorting took " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Sorting.\n", timer.GetElapsedTime());
    }


    //
    // Set bestUp and bestDown. Parallelisable
    //
    timer.Reset();
    timer.Start();

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::SetBestUpDown setBestUpDown;
    Invoke(setBestUpDown, bestUpward, bestDownward, arcs);

    timer.Stop();
    timerTotal.Stop();
    if (printTime >= 2)
    {
      //std::cout << "---------------- Setting bestUp/Down took " << timer.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for Setting bestUp/Down.\n", timer.GetElapsedTime());
      //std::cout << "---------------- TOTAL TIME for 1st Part is  " << timerTotal.GetElapsedTime() << " seconds." << std::endl;
      printf("%.6f for TOTAL TIME for 1st Part.\n", timerTotal.GetElapsedTime());
      printf("\n\n");
    }


    timer.Reset();
    timer.Start();

    ProcessContourTree::ComputeBranchData(contourTree,
                                          printTime,
                                          whichBranch,
                                          branchMinimum,
                                          branchMaximum,
                                          branchSaddle,
                                          branchParent,
                                          bestUpward,
                                          bestDownward);

    timer.Stop();
    timerTotalAll.Stop();
    ellapsedTime = timer.GetElapsedTime();
    if (printTime >= 2)
    {
      printf("%.6f for Computing branch data.\n", timer.GetElapsedTime());
    }
    if (printTime >= 1)
    {
      printf("%.6f TOTAL Branch Decomposition.\n", timerTotalAll.GetElapsedTime());
    }

    if (printTime >= 0)
    {
      //printf("MinHypersweep, MaxHypersweep, BothHypersweep, TotalBD\n");
      printf("%.8f, %.8f, %.8f, %.8f",
             minHypersweepTime,
             maxHypersweepTime,
             bothHypersweepTime,
             timerTotalAll.GetElapsedTime());
    }

    //printf("Working!");

  } // ComputeHeightBranchDecomposition()

  void static findMinMaxNew(
    const vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType supernodesPortal,
    const vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType superarcsPortal,
    const bool isMin,
    const Id root,
    vtkm::cont::ArrayHandle<vtkm::Id>::PortalControl minMaxIndex)
  {
    using vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT;

    //
    // Holds the index and distance of every supernode. Used to sort vertices by their distance for processing.
    //
    struct VertexData
    {
      Id distance;
      Id index;
    };

    //
    // Set the initial values. Parallelisable.
    //
    std::vector<VertexData> vertexData(
      static_cast<unsigned long>(supernodesPortal.GetNumberOfValues()));
    for (unsigned long i = 0; i < vertexData.size(); i++)
    {
      vertexData[i].index = i;
      vertexData[i].distance = NO_SUCH_ELEMENT;
    }
    vertexData[root].distance = 0;

    //
    // Compute the distance to all vertices. With memorisation this is O(n). Not data parallelisable.
    //
    for (Id i = 0; i < superarcsPortal.GetNumberOfValues(); i++)
    {
      // If we have the distance skip this one
      if (vertexData[i].distance != (vtkm::Id)NO_SUCH_ELEMENT)
        continue;

      // Find the distance to the root
      vtkm::Id current = i;
      Id distanceToRoot = 0;
      while (vertexData[current].distance == (vtkm::Id)NO_SUCH_ELEMENT)
      {
        distanceToRoot++;
        current = MaskedIndex(superarcsPortal.Get(current));
      }

      Id miniRoot = current;

      // Set the distance to all nodes on the path to the root
      current = i;
      Id iteration = 0;
      while (vertexData[current].distance == (vtkm::Id)NO_SUCH_ELEMENT)
      {
        Id distance = vertexData[miniRoot].distance + distanceToRoot - iteration;
        vertexData[current].distance = distance;
        iteration++;
        current = MaskedIndex(superarcsPortal.Get(current));
      }
    }

    //
    // Sort all vertices based on distance. Parallelisable.
    //
    std::sort(vertexData.begin(), vertexData.end(), [](const VertexData& a, const VertexData& b) {
      return a.distance > b.distance;
    });

    //
    // Set an initial value, suppose every vertex is its own min/max. Parallelisable.
    //
    for (vtkm::Id i = 0; i < minMaxIndex.GetNumberOfValues(); i++)
    {
      minMaxIndex.Set(i, i);
    }

    //
    // For every vertex see if it's bigger than its parent. At the end the minMaxIndex with contain the min/max child of every vertex. Not data parallelisable.
    //
    for (Id i = 0; i < vertexData.size(); i++)
    {
      Id vertex = vertexData[i].index;
      Id parent = MaskedIndex(superarcsPortal.Get(vertex));

      if (vertex == root)
        continue;

      Id vertexValue = MaskedIndex(supernodesPortal.Get(minMaxIndex.Get(vertex)));
      Id parentValue = MaskedIndex(supernodesPortal.Get(minMaxIndex.Get(parent)));

      if ((true == isMin && vertexValue < parentValue) ||
          (false == isMin && vertexValue > parentValue))
      {
        minMaxIndex.Set(parent, minMaxIndex.Get(vertex));
      }
    }
  }

  std::vector<Id> static findSuperPathToRoot(
    vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType parentsPortal,
    vtkm::Id vertex)
  {
    // Initialise the empty path and starting vertex
    std::vector<vtkm::Id> path;
    vtkm::Id current = vertex;

    // Go up the parent list until we reach the root
    while (MaskedIndex(parentsPortal.Get(current)) != 0)
    {
      path.push_back(current);
      current = MaskedIndex(parentsPortal.Get(current));
    }
    path.push_back(current);

    return path;
  }

  void static fixPath(const std::function<vtkm::Id(vtkm::Id, vtkm::Id)> operation,
                      const std::vector<vtkm::Id> path,
                      vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType minMaxIndex)
  {
    using vtkm::worklet::contourtree_augmented::MaskedIndex;

    //
    // Fix path from the old root to the new root. Parallelisble with a prefix scan.
    // The root is correct so we need to start with the next vertex.
    //
    for (int i = path.size() - 2; i > 0; i--)
    {
      Id vertex = path[i + 1];
      Id parent = path[i];

      Id vertexValue = minMaxIndex.Get(vertex);
      Id parentValue = minMaxIndex.Get(parent);

      minMaxIndex.Set(parent, operation(vertexValue, parentValue));
    }
  }

  void static editHyperarcs(
    const vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType hyperparentsPortal,
    const std::vector<vtkm::Id> path,
    vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType hyperarcsPortal,
    vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType howManyUsedPortal)
  {
    using vtkm::worklet::contourtree_augmented::MaskedIndex;

    int i = 0;
    while (i < path.size())
    {
      //cout << "Node " << maxPath[i] << " is in hArc " << MaskedIndex(ct.Hyperparents.ReadPortal().Get(maxPath[i])) << endl;

      //
      // Cut the hyperacs at the first point
      //
      hyperarcsPortal.Set(MaskedIndex(hyperparentsPortal.Get(path[i])), path[i]);

      //cout << "Now pointing from " << MaskedIndex(ct.Hyperparents.ReadPortal().Get(maxPath[i])) << " -- " << maxPath[i] << endl;

      Id currentHyperparent = MaskedIndex(hyperparentsPortal.Get(path[i]));
      // Skip all others on the same hyperarc
      while (i < path.size() && MaskedIndex(hyperparentsPortal.Get(path[i])) == currentHyperparent)
      {
        auto value = howManyUsedPortal.Get(MaskedIndex(hyperparentsPortal.Get(path[i])));
        howManyUsedPortal.Set(MaskedIndex(hyperparentsPortal.Get(path[i])), value + 1);
        //cout << "Skipping over " << maxPath[i] << endl;
        i++;
      }
    }
  }

  template <class BinaryFunctor>
  void static findMinMaxNewSimple(const vtkm::cont::ArrayHandle<vtkm::Id> supernodes,
                                  const vtkm::cont::ArrayHandle<vtkm::Id> hypernodes,
                                  const vtkm::cont::ArrayHandle<vtkm::Id> hyperarcs,
                                  const vtkm::cont::ArrayHandle<vtkm::Id> hyperparents,
                                  const vtkm::cont::ArrayHandle<vtkm::Id> hyperparentKeys,
                                  const vtkm::cont::ArrayHandle<vtkm::Id> whenTransferred,
                                  const vtkm::cont::ArrayHandle<vtkm::Id> howManyUsed,
                                  const vtkm::Id nIterations,
                                  const BinaryFunctor operation,
                                  vtkm::cont::ArrayHandle<vtkm::Id> minMaxIndex)
  {
    using vtkm::worklet::contourtree_augmented::MaskedIndex;

    auto supernodesPortal = supernodes.ReadPortal();
    auto hypernodesPortal = hypernodes.ReadPortal();
    auto hyperarcsPortal = hyperarcs.ReadPortal();
    auto hyperparentsPortal = hyperparents.ReadPortal();
    auto whenTransferredPortal = whenTransferred.ReadPortal();
    auto howManyUsedPortal = howManyUsed.ReadPortal();

    //
    // Set the first supernode per iteration
    //

    vtkm::cont::ArrayHandle<vtkm::Id> firstSupernodePerIteration;
    vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, nIterations + 1),
                          firstSupernodePerIteration);
    auto firstSupernodePerIterationPortal = firstSupernodePerIteration.WritePortal();

    // The first different from the previous is the first in the iteration
    for (vtkm::Id supernode = 0; supernode < supernodesPortal.GetNumberOfValues(); supernode++)
    {
      vtkm::Id when = MaskedIndex(whenTransferredPortal.Get(supernode));
      if (supernode == 0)
      {
        firstSupernodePerIterationPortal.Set(when, supernode);
      }
      else if (when != MaskedIndex(whenTransferredPortal.Get(supernode - 1)))
      {
        firstSupernodePerIterationPortal.Set(when, supernode);
      }
    }

    // Why do we need this?
    for (vtkm::Id iteration = 1; iteration < nIterations; ++iteration)
    {
      if (firstSupernodePerIterationPortal.Get(iteration) == 0)
      {
        firstSupernodePerIterationPortal.Set(iteration,
                                             firstSupernodePerIterationPortal.Get(iteration + 1));
      }
    }

    // set the sentinel at the end of the array
    firstSupernodePerIterationPortal.Set(nIterations, supernodesPortal.GetNumberOfValues());

    //
    // Set the first hypernode per iteration
    //
    vtkm::cont::ArrayHandle<vtkm::Id> firstHypernodePerIteration;
    vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, nIterations + 1),
                          firstHypernodePerIteration);
    auto firstHypernodePerIterationPortal = firstHypernodePerIteration.WritePortal();

    for (vtkm::Id iteration = 0; iteration < nIterations; iteration++)
    {
      firstHypernodePerIterationPortal.Set(
        iteration, hyperparentsPortal.Get(firstSupernodePerIterationPortal.Get(iteration)));
    }

    // Set the sentinel at the end of the array
    firstHypernodePerIterationPortal.Set(nIterations, hypernodesPortal.GetNumberOfValues());

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::PrefixScanHyperarcs<
      BinaryFunctor>
      prefixScanHyperarcsWorklet(operation);
    vtkm::worklet::contourtree_augmented::process_contourtree_inc::AddDependentWeightHypersweep<
      BinaryFunctor>
      addDependentWeightHypersweepWorklet(operation);

    //
    // For every iteration do prefix scan on all hyperarcs and then transfer to their target supernode
    //
    for (vtkm::Id iteration = 0; iteration < nIterations; iteration++)
    {
      // Determine the first and last hypernode in the current iteration (all hypernodes between them are also in the current iteration)
      vtkm::Id firstHypernode = firstHypernodePerIterationPortal.Get(iteration);
      vtkm::Id lastHypernode = firstHypernodePerIterationPortal.Get(iteration + 1);
      lastHypernode = vtkm::Minimum()(lastHypernode, hypernodes.GetNumberOfValues() - 1);
      vtkm::Id firstSupernode = MaskedIndex(hypernodesPortal.Get(firstHypernode));
      vtkm::Id lastSupernode = MaskedIndex(hypernodesPortal.Get(lastHypernode));
      lastSupernode = vtkm::Minimum()(lastSupernode, hyperparents.GetNumberOfValues() - 1);

      // Prefix scan along all hyperarcs in the current iteration
      auto subarrayValues = vtkm::cont::make_ArrayHandleView(
        minMaxIndex, firstSupernode, lastSupernode - firstSupernode);
      auto subarrayKeys = vtkm::cont::make_ArrayHandleView(
        hyperparentKeys, firstSupernode, lastSupernode - firstSupernode);
      vtkm::cont::Algorithm::ScanInclusiveByKey(
        subarrayKeys, subarrayValues, subarrayValues, operation);

      // Array containing the Ids of the hyperarcs in the current iteration
      vtkm::cont::ArrayHandleCounting<vtkm::Id> iterationHyperarcs(
        firstHypernode, 1, lastHypernode - firstHypernode);

      // Transfer the value accumulated in the last entry of the prefix scan to the hypernode's targe supernode
      vtkm::cont::Invoker invoke;
      invoke(addDependentWeightHypersweepWorklet,
             iterationHyperarcs,
             hypernodes,
             hyperarcs,
             howManyUsed,
             minMaxIndex);


      //
      // Remnants of the serial version. Here for debugging.
      //

      //
      // For every hyperarc in the current iteration. Parallel.
      //
      //for (Id i = firstHypernode; i < lastHypernode; i++)
      //{
      //// If it's the last hyperacs (there's nothing to do it's just the root)
      //if (i >= hypernodesPortal.GetNumberOfValues() - 1) { continue; }

      //vtkm::Id firstSupernode = MaskedIndex(hypernodesPortal.Get(i));
      //vtkm::Id lastSupernode = MaskedIndex(hypernodesPortal.Get(i + 1)) - howManyUsedPortal.Get(i);

      ////
      //// Prefix scan along the hyperarc chain. Parallel prefix scan.
      ////
      //int threshold = 1000;
      //if (lastSupernode - firstSupernode > threshold)
      //{
      //auto subarray = vtkm::cont::make_ArrayHandleView(minMaxIndex, firstSupernode, lastSupernode - firstSupernode);
      //vtkm::cont::Algorithm::ScanInclusive(subarray, subarray, operation);
      //}

      //else
      //{
      //for (Id j = firstSupernode + 1; j < lastSupernode; j++)
      //{
      //Id vertex = j - 1;
      //Id parent = j;

      //Id vertexValue = minMaxIndex.ReadPortal().Get(vertex);
      //Id parentValue = minMaxIndex.ReadPortal().Get(parent);

      //minMaxIndex.WritePortal().Set(parent, operation(vertexValue, parentValue));
      //}
      //}
      //}


      //
      // Serial, but can be made parallel if we sort the hypearcs by destination and do a prefix sum on the destination.
      // The only problem is that figuring that out requries a serial pass anyway.
      // So we might as well do all of it in serial.
      // Furthermore the degree of the mesh is limiting this.
      //
      //for (Id i = firstHypernode; i < lastHypernode; i++)
      //{
      //// If it's the last hyperacs (there's nothing to do it's just the root)
      //if (i >= hypernodesPortal.GetNumberOfValues() - 1) { continue; }

      ////
      //// The value of the prefix scan is now accumulated in the last supernode of the hyperarc. Transfer is to the target
      ////
      //vtkm::Id lastSupernode = MaskedIndex(hypernodesPortal.Get(i + 1)) - howManyUsedPortal.Get(i);

      ////
      //// Transfer the accumulated value to the target supernode
      ////
      //Id vertex = lastSupernode - 1;
      //Id parent = MaskedIndex(hyperarcsPortal.Get(i));

      //Id vertexValue = minMaxIndex.ReadPortal().Get(vertex);
      //Id parentValue = minMaxIndex.ReadPortal().Get(parent);

      //minMaxIndex.WritePortal().Set(parent, operation(vertexValue, parentValue));
      //}
    }
  }




}; // class ProcessContourTree
} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
