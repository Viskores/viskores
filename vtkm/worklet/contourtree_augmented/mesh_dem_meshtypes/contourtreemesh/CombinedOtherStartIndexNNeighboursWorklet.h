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

#ifndef vtkm_worklet_contourtree_augmented_contourtree_mesh_inc_combined_other_start_index_nneighbours_worklet_worklet_h
#define vtkm_worklet_contourtree_augmented_contourtree_mesh_inc_combined_other_start_index_nneighbours_worklet_worklet_h

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

class CombinedOtherStartIndexNNeighboursWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(
    FieldIn nNeighbours,                    // (input) nNeighbours
    FieldIn otherToCombinedSortOrder,       // (input) otherToCombinedSortOrder
    WholeArrayInOut combinedNNeighbours,    // (input/output) combinedNNeighbours
    WholeArrayInOut combinedOtherStartIndex // (input/output) combinedOthertStartIndex
    );
  typedef void ExecutionSignature(_1, _2, _3, _4);
  typedef _1 InputDomain;

  // Default Constructor
  VTKM_EXEC_CONT
  CombinedOtherStartIndexNNeighboursWorklet() {}


  template <typename InOutPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& nneighboursVal,
                            const vtkm::Id& otherToCombinedSortOrderVal,
                            const InOutPortalType combinedNNeighboursPortal,
                            const InOutPortalType combinedOtherStartIndexPortal) const
  {
    combinedOtherStartIndexPortal.Set(otherToCombinedSortOrderVal,
                                      combinedNNeighboursPortal.Get(otherToCombinedSortOrderVal));
    combinedNNeighboursPortal.Set(otherToCombinedSortOrderVal,
                                  combinedNNeighboursPortal.Get(otherToCombinedSortOrderVal) +
                                    nneighboursVal);

    // Implements in reference code
    // #pragma omp parallel for
    // The following is save since each global index is only written by one entry
    // for (indexVector::size_type vtx = 0; vtx < nNeighbours.size(); ++vtx)
    // {
    //     combinedOtherStartIndex[otherToCombinedSortOrder[vtx]] =  combinedNNeighbours[otherToCombinedSortOrder[vtx]];
    //     combinedNNeighbours[otherToCombinedSortOrder[vtx]] += nNeighbours[vtx];
    // }
  }



}; // CombinedOtherStartIndexNNeighboursWorklet


} // namespace mesh_dem_contourtree_mesh_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
