//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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

#ifndef vtkm_worklet_contourtree_augmented_contourtree_mesh_inc_update_combined_neighbours_worklet_h
#define vtkm_worklet_contourtree_augmented_contourtree_mesh_inc_update_combined_neighbours_worklet_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem_contourtree_mesh_inc
{

class UpdateCombinedNeighboursWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(
    WholeArrayIn firstNeighbour, // (input) this->firstNerighbour or other.firstNeighbour
    WholeArrayIn neighbours,     // (input) this->neighbours or other.neighbours array
    WholeArrayIn
      toCombinedSortOrder, // (input) thisToCombinedSortOrder or otherToCombinedSortOrder array
    WholeArrayIn combinedFirstNeighbour, // (input) combinedFirstNeighbour array in both cases
    WholeArrayIn
      combinedOtherStartIndex,         // (input) const 0 array of length combinedOtherStartIndex for this and combinedOtherStartIndex for other loop
    WholeArrayOut combinedNeighbours); // (output) combinedNeighbours array in both cases
  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6);
  typedef _1 InputDomain;

  // Default Constructor
  VTKM_EXEC_CONT
  UpdateCombinedNeighboursWorklet() {}

  template <typename InFieldPortalType, typename InFieldPortalType2, typename OutFieldPortalType>
  VTKM_EXEC void operator()(
    const InFieldPortalType& firstNeighbourPortal,
    const vtkm::Id vtx,
    const InFieldPortalType& neighboursPortal,
    const InFieldPortalType& toCombinedSortOrderPortal,
    const InFieldPortalType& combinedFirstNeighbourPortal,
    const InFieldPortalType2&
      combinedOtherStartIndexPortal, // We need another InFieldPortalType here to allow us to hand in a smart array handle instead of a VTKM array
    const OutFieldPortalType& combinedNeighboursPortal) const
  {
    vtkm::Id totalNumNeighbours = neighboursPortal.GetNumberOfValues();
    vtkm::Id totalNumVertices = firstNeighbourPortal.GetNumberOfValues();
    vtkm::Id numNeighbours = (vtx < totalNumVertices - 1)
      ? firstNeighbourPortal.Get(vtx + 1) - firstNeighbourPortal.Get(vtx)
      : totalNumNeighbours - firstNeighbourPortal.Get(vtx);
    for (vtkm::Id nbrNo = 0; nbrNo < numNeighbours; ++nbrNo)
    {
      combinedNeighboursPortal.Set(
        combinedFirstNeighbourPortal.Get(toCombinedSortOrderPortal.Get(vtx)) +
          combinedOtherStartIndexPortal.Get(toCombinedSortOrderPortal.Get(vtx)) + nbrNo,
        toCombinedSortOrderPortal.Get(neighboursPortal.Get(firstNeighbourPortal.Get(vtx) + nbrNo)));
    }

    /*
      This worklet implemnts the following two loops from the original OpenMP code
      The two loops are the same but the arrays required are different

      #pragma omp parallel for
      for (indexVector::size_type vtx = 0; vtx < firstNeighbour.size(); ++vtx)
      {
        indexType numNeighbours = (vtx < GetNumberOfVertices() - 1) ? firstNeighbour[vtx+1] - firstNeighbour[vtx] : neighbours.size() - firstNeighbour[vtx];

        for (indexType nbrNo = 0; nbrNo < numNeighbours; ++nbrNo)
        {
            combinedNeighbours[combinedFirstNeighbour[thisToCombinedSortOrder[vtx]] + nbrNo] = thisToCombinedSortOrder[neighbours[firstNeighbour[vtx] + nbrNo]];
        }
      }

      #pragma omp parallel for
      for (indexVector::size_type vtx = 0; vtx < other.firstNeighbour.size(); ++vtx)
      {
        indexType numNeighbours = (vtx < other.GetNumberOfVertices() - 1) ? other.firstNeighbour[vtx+1] - other.firstNeighbour[vtx] : other.neighbours.size() - other.firstNeighbour[vtx];
        for (indexType nbrNo = 0; nbrNo < numNeighbours; ++nbrNo)
        {
          combinedNeighbours[combinedFirstNeighbour[otherToCombinedSortOrder[vtx]] + combinedOtherStartIndex[otherToCombinedSortOrder[vtx]] + nbrNo] = otherToCombinedSortOrder[other.neighbours[other.firstNeighbour[vtx] + nbrNo]];
        }
      }
      */
  }
}; //  AdditionAssignWorklet


} // namespace mesh_dem_contourtree_mesh_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
