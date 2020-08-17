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

#ifndef vtk_m_worklet_contourtree_distributed_bract_maker_set_up_and_down_neighbours_worklet_h
#define vtk_m_worklet_contourtree_distributed_bract_maker_set_up_and_down_neighbours_worklet_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{
namespace bract_maker
{

/// Worklet to transfer the dependent counts for hyperarcs
/// Part of the BoundaryRestrictedAugmentedContourTree.PropagateBoundaryCounts function
class SetUpAndDownNeighboursWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn bractVertexSuperset, // input
                                FieldIn bractSuperarcs,           // input
                                WholeArrayIn meshSortIndex,       // input
                                WholeArrayOut upNeighbour,        // output
                                WholeArrayOut downNeighbour       // output
  );
  using ExecutionSignature = void(InputIndex, _2, _1, _3, _4, _5);
  using InputDomain = _1;

  // Default Constructor
  VTKM_EXEC_CONT
  SetUpAndDownNeighboursWorklet() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& from,
                            vtkm::Id& to,
                            InFieldPortalType bractVertexSupersetPortal,
                            InFieldPortalType meshSortIndexPortal,
                            OutFieldPortalType upNeighbourPortal,
                            OutFieldPortalType downNeighbourPortal)
  {
    // per vertex
    // ignore the last terminating edge
    if (vtkm::worklet::contourtree_augmented::NoSuchElement(to))
    {
      return;
    }

    // now find the sort index of the from and to
    vtkm::Id fromSort = meshSortIndexPortal.Get(bractVertexSupersetPortal.Get(from));
    vtkm::Id toSort = meshSortIndexPortal.Get(bractVertexSupersetPortal.Get(to));

    // use this to identify direction of edge
    if (fromSort < toSort)
    { // from is lower
      upNeighbourPortal.Set(from, to);
      downNeighbourPortal.Set(to, from);
    } // from is lower
    else
    { // to is lower
      upNeighbourPortal.Set(to, from);
      downNeighbourPortal.Set(from, to);
    } // to is lower
    // In serial this worklet implements the following operation
    /*
    //  a.  Loop through all of the superarcs in the return tree, retrieving the two ends
    for (indexType from = 0; from < bractVertexSuperset.size(); from++)
    { // per vertex
      indexType to = bract->superarcs[from];
      // ignore the last terminating edge
      if (noSuchElement(to))
        continue;
      // now find the sort index of the from and to
      indexType fromSort = mesh->SortIndex(bractVertexSuperset[from]);
      indexType toSort = mesh->SortIndex(bractVertexSuperset[to]);

      // use this to identify direction of edge
      if (fromSort < toSort)
        { // from is lower
        upNeighbour[from] = to;
        downNeighbour[to] = from;
        } // from is lower
      else
        { // to is lower
        upNeighbour[to] = from;
        downNeighbour[from] = to;
        } // to is lower
    } // per vertex
    */
  }

}; // SetUpAndDownNeighboursWorklet


} // namespace bract_maker
} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
