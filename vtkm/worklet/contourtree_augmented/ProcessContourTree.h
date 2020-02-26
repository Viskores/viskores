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
#include <vtkm/worklet/contourtree_augmented/ArrayTransforms.h>
#include <vtkm/worklet/contourtree_augmented/ContourTree.h>
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/Branch.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/SuperArcVolumetricComparator.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/SuperNodeBranchComparator.h>

#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/ComputeBestUpDown.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/ComputeEulerTourFirstNext.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/ComputeEulerTourList.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/ComputeMinMaxValues.h>

#include <vtkm/worklet/contourtree_augmented/processcontourtree/EulerTour.h>



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
      //      sArc                     s11 s12 s13 s14 s21 s22 s23 s31
      //      hyperparent sNode ID     s11 s11 s11 s11 s21 s21 s21 s31
      //      transfer weight           0   1   2   1   2   3   1   0
      //      intrinsic weight          1   2   1   5   2   6   1   1
      //      sum(xfer + intrinsic)     1   3   3   6   4   9   2   1
      //  prefix sum (xfer + int)       1   4   7  13  14  26  28  29

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
    auto superarcsPortal = contourTree.Superarcs.ReadPortal();
    auto superarcDependentWeightPortal = superarcDependentWeight.ReadPortal();
    auto superarcIntrinsicWeightPortal = superarcIntrinsicWeight.ReadPortal();

    // cache the number of non-root supernodes & superarcs
    vtkm::Id nSupernodes = contourTree.Supernodes.GetNumberOfValues();
    vtkm::Id nSuperarcs = nSupernodes - 1;

    // STAGE I:  Find the upward and downwards weight for each superarc, and set up arrays
    IdArrayType upWeight;
    upWeight.Allocate(nSuperarcs);
    auto upWeightPortal = upWeight.WritePortal();
    IdArrayType downWeight;
    downWeight.Allocate(nSuperarcs);
    auto downWeightPortal = downWeight.WritePortal();
    IdArrayType bestUpward;
    auto noSuchElementArray =
      vtkm::cont::ArrayHandleConstant<vtkm::Id>((vtkm::Id)NO_SUCH_ELEMENT, nSupernodes);
    vtkm::cont::ArrayCopy(noSuchElementArray, bestUpward);
    IdArrayType bestDownward;
    vtkm::cont::ArrayCopy(noSuchElementArray, bestDownward);
    vtkm::cont::ArrayCopy(noSuchElementArray, whichBranch);
    auto bestUpwardPortal = bestUpward.WritePortal();
    auto bestDownwardPortal = bestDownward.WritePortal();

    // STAGE II: Pick the best (largest volume) edge upwards and downwards
    // II A. Pick the best upwards weight by sorting on lower vertex then processing by segments
    // II A 1.  Sort the superarcs by lower vertex
    // II A 2.  Per segment, best superarc writes to the best upwards array
    vtkm::cont::ArrayHandle<EdgePair> superarcList;
    vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<EdgePair>(EdgePair(-1, -1), nSuperarcs),
                          superarcList);
    auto superarcListPortal = superarcList.WritePortal();
    vtkm::Id totalVolume = contourTree.Nodes.GetNumberOfValues();
#ifdef DEBUG_PRINT
    std::cout << "Total Volume: " << totalVolume << std::endl;
#endif
    // NB: Last element in array is guaranteed to be root superarc to infinity,
    // so we can easily skip it by not indexing to the full size
    for (vtkm::Id superarc = 0; superarc < nSuperarcs; superarc++)
    { // per superarc
      if (IsAscending(superarcsPortal.Get(superarc)))
      { // ascending superarc
        superarcListPortal.Set(superarc,
                               EdgePair(superarc, MaskedIndex(superarcsPortal.Get(superarc))));
        upWeightPortal.Set(superarc, superarcDependentWeightPortal.Get(superarc));
        // at the inner end, dependent weight is the total in the subtree.  Then there are vertices along the edge itself (intrinsic weight), including the supernode at the outer end
        // So, to get the "dependent" weight in the other direction, we start with totalVolume - dependent, then subtract (intrinsic - 1)
        downWeightPortal.Set(superarc,
                             (totalVolume - superarcDependentWeightPortal.Get(superarc)) +
                               (superarcIntrinsicWeightPortal.Get(superarc) - 1));
      } // ascending superarc
      else
      { // descending superarc
        superarcListPortal.Set(superarc,
                               EdgePair(MaskedIndex(superarcsPortal.Get(superarc)), superarc));
        downWeightPortal.Set(superarc, superarcDependentWeightPortal.Get(superarc));
        // at the inner end, dependent weight is the total in the subtree.  Then there are vertices along the edge itself (intrinsic weight), including the supernode at the outer end
        // So, to get the "dependent" weight in the other direction, we start with totalVolume - dependent, then subtract (intrinsic - 1)
        upWeightPortal.Set(superarc,
                           (totalVolume - superarcDependentWeightPortal.Get(superarc)) +
                             (superarcIntrinsicWeightPortal.Get(superarc) - 1));
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
    auto superarcSorterPortal = superarcSorter.WritePortal();
    for (vtkm::Id superarc = 0; superarc < nSuperarcs; superarc++)
      superarcSorterPortal.Set(superarc, superarc);

    vtkm::cont::Algorithm::Sort(
      superarcSorter,
      process_contourtree_inc_ns::SuperArcVolumetricComparator(upWeight, superarcList, false));

    // II B 2.  Per segment, best superarc writes to the best upward array
    for (vtkm::Id superarc = 0; superarc < nSuperarcs; superarc++)
    { // per superarc
      vtkm::Id superarcID = superarcSorterPortal.Get(superarc);
      const EdgePair& edge = superarcListPortal.Get(superarcID);
      // if it's the last one
      if (superarc == nSuperarcs - 1)
        bestDownwardPortal.Set(edge.second, edge.first);
      else
      { // not the last one
        const EdgePair& nextEdge = superarcListPortal.Get(superarcSorterPortal.Get(superarc + 1));
        // if the next edge belongs to another, we're the highest
        if (nextEdge.second != edge.second)
          bestDownwardPortal.Set(edge.second, edge.first);
      } // not the last one
    }   // per superarc

    // II B 3.  Repeat for lower vertex
    vtkm::cont::Algorithm::Sort(
      superarcSorter,
      process_contourtree_inc_ns::SuperArcVolumetricComparator(downWeight, superarcList, true));

    // II B 2.  Per segment, best superarc writes to the best upward array
    for (vtkm::Id superarc = 0; superarc < nSuperarcs; superarc++)
    { // per superarc
      vtkm::Id superarcID = superarcSorterPortal.Get(superarc);
      const EdgePair& edge = superarcListPortal.Get(superarcID);
      // if it's the last one
      if (superarc == nSuperarcs - 1)
        bestUpwardPortal.Set(edge.first, edge.second);
      else
      { // not the last one
        const EdgePair& nextEdge = superarcListPortal.Get(superarcSorterPortal.Get(superarc + 1));
        // if the next edge belongs to another, we're the highest
        if (nextEdge.first != edge.first)
          bestUpwardPortal.Set(edge.first, edge.second);
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

    // Set up portals
    auto bestUpwardPortal = bestUpward.WritePortal();
    auto bestDownwardPortal = bestDownward.WritePortal();
    auto whichBranchPortal = whichBranch.WritePortal();

    // STAGE III: For each vertex, identify which neighbours are on same branch
    // Let v = BestUp(u). Then if u = BestDown(v), copy BestUp(u) to whichBranch(u)
    // Otherwise, let whichBranch(u) = BestUp(u) | TERMINAL to mark the end of the side branch
    // NB 1: Leaves already have the flag set, but it's redundant so its safe
    // NB 2: We don't need to do it downwards because it's symmetric
    for (vtkm::Id supernode = 0; supernode != nSupernodes; supernode++)
    { // per supernode
      vtkm::Id bestUp = bestUpwardPortal.Get(supernode);
      if (NoSuchElement(bestUp))
        // flag it as an upper leaf
        whichBranchPortal.Set(supernode, TERMINAL_ELEMENT | supernode);
      else if (bestDownwardPortal.Get(bestUp) == supernode)
        whichBranchPortal.Set(supernode, bestUp);
      else
        whichBranchPortal.Set(supernode, TERMINAL_ELEMENT | supernode);
    } // per supernode

#ifdef DEBUG_PRINT
    std::cout << "III. Branch Neighbours Identified" << std::endl;
    PrintHeader(whichBranch.GetNumberOfValues());
    PrintIndices("Which Branch", whichBranch);
    std::cout << std::endl;
#endif

    // STAGE IV: Use pointer-doubling on whichBranch to propagate branches
    // Compute the number of log steps required in this pass
    vtkm::Id numLogSteps = 1;
    for (vtkm::Id shifter = nSupernodes; shifter != 0; shifter >>= 1)
      numLogSteps++;

    // use pointer-doubling to build the branches
    for (vtkm::Id iteration = 0; iteration < numLogSteps; iteration++)
    { // per iteration
      // loop through the vertices, updating the chaining array
      for (vtkm::Id supernode = 0; supernode < nSupernodes; supernode++)
        if (!IsTerminalElement(whichBranchPortal.Get(supernode)))
          whichBranchPortal.Set(supernode, whichBranchPortal.Get(whichBranchPortal.Get(supernode)));
    } // per iteration

#ifdef DEBUG_PRINT
    std::cout << "IV. Branch Chains Propagated" << std::endl;
    PrintHeader(whichBranch.GetNumberOfValues());
    PrintIndices("Which Branch", whichBranch);
    std::cout << std::endl;
#endif

    // STAGE V:  Create an array of branches. To do this, first we need a temporary array storing
    // which existing leaf corresponds to which branch.  It is possible to estimate the correct number
    // by counting leaves, but for parallel, we'll need a compression anyway
    // V A.  Set up the ID lookup for branches
    vtkm::Id nBranches = 0;
    IdArrayType chainToBranch;
    vtkm::cont::ArrayCopy(noSuchElementArray, chainToBranch);
    auto chainToBranchPortal = chainToBranch.WritePortal();
    for (vtkm::Id supernode = 0; supernode < nSupernodes; supernode++)
    {
      // test whether the supernode points to itself to find the top ends
      if (MaskedIndex(whichBranchPortal.Get(supernode)) == supernode)
      {
        chainToBranchPortal.Set(supernode, nBranches++);
      }
    }

    // V B.  Create the arrays for the branches
    auto noSuchElementArrayNBranches =
      vtkm::cont::ArrayHandleConstant<vtkm::Id>((vtkm::Id)NO_SUCH_ELEMENT, nBranches);
    vtkm::cont::ArrayCopy(noSuchElementArrayNBranches, branchMinimum);
    vtkm::cont::ArrayCopy(noSuchElementArrayNBranches, branchMaximum);
    vtkm::cont::ArrayCopy(noSuchElementArrayNBranches, branchSaddle);
    vtkm::cont::ArrayCopy(noSuchElementArrayNBranches, branchParent);
    auto branchMinimumPortal = branchMinimum.WritePortal();
    auto branchMaximumPortal = branchMaximum.WritePortal();
    auto branchSaddlePortal = branchSaddle.WritePortal();
    auto branchParentPortal = branchParent.WritePortal();

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
    // STAGE VI:  Sort all supernodes by [whichBranch, regular index] to get the sequence along the branch
    // Assign the upper end of the branch as an ID (for now).
    // VI A.  Create the sorting array, then sort
    IdArrayType supernodeSorter;
    supernodeSorter.Allocate(nSupernodes);
    auto supernodeSorterPortal = supernodeSorter.WritePortal();
    for (vtkm::Id supernode = 0; supernode < nSupernodes; supernode++)
    {
      supernodeSorterPortal.Set(supernode, supernode);
    }

    vtkm::cont::Algorithm::Sort(
      supernodeSorter,
      process_contourtree_inc_ns::SuperNodeBranchComparator(whichBranch, contourTree.Supernodes));
    IdArrayType permutedBranches;
    permutedBranches.Allocate(nSupernodes);
    PermuteArray<vtkm::Id>(whichBranch, supernodeSorter, permutedBranches);

    IdArrayType permutedRegularID;
    permutedRegularID.Allocate(nSupernodes);
    PermuteArray<vtkm::Id>(contourTree.Supernodes, supernodeSorter, permutedRegularID);

#ifdef DEBUG_PRINT
    std::cout << "VI A. Sorted into Branches" << std::endl;
    PrintHeader(nSupernodes);
    PrintIndices("Supernode IDs", supernodeSorter);
    PrintIndices("Branch", permutedBranches);
    PrintIndices("Regular ID", permutedRegularID);
#endif

    // VI B. And reset the whichBranch to use the new branch IDs
    for (vtkm::Id supernode = 0; supernode < nSupernodes; supernode++)
    {
      whichBranchPortal.Set(supernode,
                            chainToBranchPortal.Get(MaskedIndex(whichBranchPortal.Get(supernode))));
    }

    // VI C.  For each segment, the highest element sets up the upper end, the lowest element sets the low end
    for (vtkm::Id supernode = 0; supernode < nSupernodes; supernode++)
    { // per supernode
      // retrieve supernode & branch IDs
      vtkm::Id supernodeID = supernodeSorterPortal.Get(supernode);
      vtkm::Id branchID = whichBranchPortal.Get(supernodeID);
      // save the branch ID as the owner
      // use LHE of segment to set branch minimum
      if (supernode == 0)
      { // sn = 0
        branchMinimumPortal.Set(branchID, supernodeID);
      } // sn = 0
      else if (branchID != whichBranchPortal.Get(supernodeSorterPortal.Get(supernode - 1)))
      { // LHE
        branchMinimumPortal.Set(branchID, supernodeID);
      } // LHE
      // use RHE of segment to set branch maximum
      if (supernode == nSupernodes - 1)
      { // sn = max
        branchMaximumPortal.Set(branchID, supernodeID);
      } // sn = max
      else if (branchID != whichBranchPortal.Get(supernodeSorterPortal.Get(supernode + 1)))
      { // RHE
        branchMaximumPortal.Set(branchID, supernodeID);
      } // RHE
    }   // per supernode

#ifdef DEBUG_PRINT
    std::cout << "VI. Branches Set" << std::endl;
    PrintHeader(nBranches);
    PrintIndices("Branch Maximum", branchMaximum);
    PrintIndices("Branch Minimum", branchMinimum);
    PrintIndices("Branch Saddle", branchSaddle);
    PrintIndices("Branch Parent", branchParent);
#endif

    // STAGE VII: For each branch, set its parent (initially) to NO_SUCH_ELEMENT
    // Then test upper & lower ends of each branch (in their segments) to see whether they are leaves
    // At most one is a leaf. In the case of the master branch, both are
    // So, non-leaf ends set the parent branch to the branch owned by the BestUp/BestDown corresponding
    // while leaf ends do nothing. At the end of this, the master branch still has -1 as the parent,
    // while all other branches have their parents correctly set
    // BTW: This is inefficient, and we need to compress down the list of branches
    for (vtkm::Id branchID = 0; branchID < nBranches; branchID++)
    { // per branch
      vtkm::Id branchMax = branchMaximumPortal.Get(branchID);
      // check whether the maximum is NOT a leaf
      if (!NoSuchElement(bestUpwardPortal.Get(branchMax)))
      { // points to a saddle
        branchSaddlePortal.Set(branchID, MaskedIndex(bestUpwardPortal.Get(branchMax)));
        // if not, then the bestUp points to a saddle vertex at which we join the parent
        branchParentPortal.Set(branchID, whichBranchPortal.Get(bestUpwardPortal.Get(branchMax)));
      } // points to a saddle
      // now do the same with the branch minimum
      vtkm::Id branchMin = branchMinimumPortal.Get(branchID);
      // test whether NOT a lower leaf
      if (!NoSuchElement(bestDownwardPortal.Get(branchMin)))
      { // points to a saddle
        branchSaddlePortal.Set(branchID, MaskedIndex(bestDownwardPortal.Get(branchMin)));
        // if not, then the bestUp points to a saddle vertex at which we join the parent
        branchParentPortal.Set(branchID, whichBranchPortal.Get(bestDownwardPortal.Get(branchMin)));
      } // points to a saddle
    }   // per branch

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
                                          whichBranch,
                                          branchMinimum,
                                          branchMaximum,
                                          branchSaddle,
                                          branchParent,
                                          bestUpward,
                                          bestDownward);

  } // ComputeHeightBranchDecomposition()

}; // class ProcessContourTree
} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
