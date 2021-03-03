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

#ifndef vtk_m_worklet_contourtree_distributed_hierarchical_hyper_sweeper_compute_superarc_dependent_weights_worklet_h
#define vtk_m_worklet_contourtree_distributed_hierarchical_hyper_sweeper_compute_superarc_dependent_weights_worklet_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_hyper_sweeper
{

/// Worklet used in HierarchicalHyperSweeper.ComputeSuperarcDependentWeightsWorklet(...) to
/// compute the superarc dependent weights
template <typename FieldType>
class ComputeSuperarcDependentWeightsWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn supernodeIndex, // counting index [firstSupernode, lastSupernode)
    FieldIn
      hierarchicalTreeSuperarcsView, // view of  hierarchicalTree.Superarcs[firstSupernode, lastSupernode)
    FieldIn
      hierarchicalTreeHyperparentsView, // view of  hierarchicalTree.Hyperparents[firstSupernode, lastSupernode)
    WholeArrayIn hierarchicalTreeHypernodes, // whole hierarchicalTree.Hypernodes array
    WholeArrayIn valuePrefixSum,             // whole valuePrefixSum array
    FieldInOut sweepValuesView // output view of sweepValues[firstSupernode, lastSupernode)
  );
  using ExecutionSignature = void(InputIndex, _1, _2);
  using InputDomain = _1;

  // Default Constructor
  VTKM_EXEC_CONT
  ComputeSuperarcDependentWeightsWorklet(const vtkm::Id& firstSupernode,
                                         const vtkm::Id& round,
                                         const vtkm::Id& hierarchicalTreeNumRounds)
    : FirstSupernode(firstSupernode)
    , Round(round)
    , HierarchicalTreeNumRounds(hierarchicalTreeNumRounds)
  {
  }

  template <typename InFieldPortalType, typename InOutFieldPortalType>
  VTKM_EXEC void operator()(
    const vtkm::Id& supernode,
    const vtkm::Id& superarcTo,  // same as hierarchicalTree.superarcs[supernode];
    const vtkm::Id& hyperparent, // same as hierarchicalTree.hyperparents[supernode];
    const InFieldPortalType& hierarchicalTreeHypernodesPortal,
    const InFieldPortalType& valuePrefixSumPortal,
    vtkm::Id& sweepValue) const
  {
    // per supernode
    // if there is no superarc, it is either the root of the tree or an attachment point
    if (vtkm::worklet::contourtree_augmented::NoSuchElement(superarcTo))
    { // null superarc
      // next we test for whether it is the global root
      if (this->Round == this->HierarchicalTreeNumRounds)
      { // global root
        // this is guaranteed to be the only element in it's iteration
        // so the prefix sum is good as it stands
        sweepValue = valuePrefixSumPortal.Get(supernode);
      } // global root
      else
      { // attachment point
        // could be the first in the iteration, in which case it is correct
        if (supernode == this->FirstSupernode)
        {
          sweepValue = valuePrefixSumPortal.Get(supernode);
        }
        // otherwise, we are guaranteed that it's a length one chain, so subtract predecessor
        else
        {
          sweepValue =
            valuePrefixSumPortal.Get(supernode) - valuePrefixSumPortal.Get(supernode - 1);
        }
      } // attachment point
    }   // null superarc
    else
    { // actual superarc
      // use the hyperparent to find the hypernode at the beginning of the chain
      vtkm::Id hyperparentSuperId = hierarchicalTreeHypernodesPortal.Get(hyperparent);

      // now we check to see which value we subtract
      FieldType baseValue = 0;
      if (hyperparentSuperId != this->FirstSupernode)
      {
        baseValue = valuePrefixSumPortal.Get(hyperparentSuperId - 1);
      }
      // for all others, remove the hyperparent's prefix sum to get the "relative" prefix sum
      sweepValue = valuePrefixSumPortal.Get(supernode) - baseValue;
    } // actual superarc

    // In serial this worklet implements the following operation
    /*
    for (vtkm::Id supernode = firstSupernode; supernode < lastSupernode; supernode++)
    { // per supernode
      // we need to know the superarc first
      vtkm::Id superarcTo = hierarchicalTree.superarcs[supernode];

      // if there is no superarc, it is either the root of the tree or an attachment point
      if (noSuchElement(superarcTo))
      { // null superarc
        // next we test for whether it is the global root
        if (round == hierarchicalTree.nRounds)
        { // global root
          // this is guaranteed to be the only element in it's iteration
          // so the prefix sum is good as it stands
          sweepValues[supernode] = valuePrefixSum[supernode];
        } // global root
        else
        { // attachment point
          // could be the first in the iteration, in which case it is correct
          if (supernode == firstSupernode)
            sweepValues[supernode] = valuePrefixSum[supernode];
          // otherwise, we are guaranteed that it's a length one chain, so subtract predecessor
          else
            sweepValues[supernode] = valuePrefixSum[supernode] - valuePrefixSum[supernode-1];
        } // attachment point
      } // null superarc
      else
      { // actual superarc
        // use the hyperparent to find the hypernode at the beginning of the chain
        vtkm::Id hyperparent = hierarchicalTree.hyperparents[supernode];
        vtkm::Id hyperparentSuperId = hierarchicalTree.hypernodes[hyperparent];

        // now we check to see which value we subtract
        dataType baseValue =  0;
        if (hyperparentSuperId != firstSupernode)
          baseValue = valuePrefixSum[hyperparentSuperId - 1];

        // for all others, remove the hyperparent's prefix sum to get the "relative" prefix sum
        sweepValues[supernode] = valuePrefixSum[supernode] - baseValue;
      } // actual superarc
    } // per supernode
    */
  } // operator()()

private:
  const vtkm::Id FirstSupernode;
  const vtkm::Id Round;
  const vtkm::Id HierarchicalTreeNumRounds;

}; // ComputeSuperarcDependentWeightsWorklet

} // namespace hierarchical_hyper_sweeper
} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
