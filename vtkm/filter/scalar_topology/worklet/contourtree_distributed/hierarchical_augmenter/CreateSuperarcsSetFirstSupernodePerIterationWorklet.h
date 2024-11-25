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

#ifndef vtk_m_worklet_contourtree_distributed_hierarchical_augmenter_create_auperarcs_set_first_supernode_per_iteration_worklet_h
#define vtk_m_worklet_contourtree_distributed_hierarchical_augmenter_create_auperarcs_set_first_supernode_per_iteration_worklet_h

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
class CreateSuperarcsSetFirstSupernodePerIterationWorklet : public vtkm::worklet::WorkletMapField
{
public:
  /// Control signature for the worklet
  using ControlSignature = void(FieldIn supernodeIndex,
                                WholeArrayIn augmentedTreeWhichIteration,
                                WholeArrayInOut augmentedTreeFirstSupernodePerIteration);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  // Default Constructor
  VTKM_EXEC_CONT
  CreateSuperarcsSetFirstSupernodePerIterationWorklet(vtkm::Id numSupernodesAlready)
    : NumSupernodesAlready(numSupernodesAlready)
  {
  }

  template <typename InFieldPortalType, typename InOutFieldPortalType>
  VTKM_EXEC void operator()(
    const vtkm::Id& supernode, // index in supernodeSorter
    // const vtkm::Id& supernodeSetindex,  // supernodeSorter[supernode]
    const InFieldPortalType& augmentedTreeWhichIterationPortal,
    const InOutFieldPortalType& augmentedTreeFirstSupernodePerIterationPortal) const
  { // operator()()
    // per supernode in the set
    // retrieve the index from the sorting index array (Done on input)(NOT USED)
    // indexType supernodeSetIndex = supernodeSorter[supernode];

    // work out the new supernode ID
    vtkm::Id newSupernodeId = this->NumSupernodesAlready + supernode;

    // The 0th element sets the first element in the zeroth iteration
    if (supernode == 0)
    {
      augmentedTreeFirstSupernodePerIterationPortal.Set(0, newSupernodeId);
    }
    // otherwise, mismatch to the left identifies a new iteration
    else
    {
      if (vtkm::worklet::contourtree_augmented::MaskedIndex(
            augmentedTreeWhichIterationPortal.Get(newSupernodeId)) !=
          vtkm::worklet::contourtree_augmented::MaskedIndex(
            augmentedTreeWhichIterationPortal.Get(newSupernodeId - 1)))
      { // start of segment
        augmentedTreeFirstSupernodePerIterationPortal.Set(
          vtkm::worklet::contourtree_augmented::MaskedIndex(
            augmentedTreeWhichIterationPortal.Get(newSupernodeId)),
          newSupernodeId);
      } // start of segmen
    }

    /*
    #pragma omp parallel for
    for (indexType supernode = 0; supernode < supernodeSorter.size(); supernode++)
    { // per supernode in the set
      // retrieve the index from the sorting index array
      indexType supernodeSetIndex = supernodeSorter[supernode];

      // work out the new supernode ID
      indexType newSupernodeID = nSupernodesAlready + supernode;

      // The 0th element sets the first element in the zeroth iteration
      if (supernode == 0)
        augmentedTree->firstSupernodePerIteration[roundNo][0] = newSupernodeID;
      // otherwise, mismatch to the left identifies a new iteration
      else
      {
        if (augmentedTree->whichIteration[newSupernodeID] != augmentedTree->whichIteration[newSupernodeID-1])
          augmentedTree->firstSupernodePerIteration[roundNo][maskedIndex(augmentedTree->whichIteration[newSupernodeID])] = newSupernodeID;
      }
    } // per supernode in the set
    */

  } // operator()()

private:
  vtkm::Id NumSupernodesAlready;


}; // CreateSuperarcsSetFirstSupernodePerIterationWorklet

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
