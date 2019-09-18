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

#ifndef vtkm_worklet_contourtree_augmented_contourtree_mesh_inc_compute_max_neighbour_worklet_h
#define vtkm_worklet_contourtree_augmented_contourtree_mesh_inc_compute_max_neighbour_worklet_h

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem_contourtree_mesh_inc
{


// Worklet to update all of the edges so that the far end resets to the result of the ascent in the previous step
class ComputeMaxNeighboursWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn firstNeighbour, // (input) firstNeighbour
                                WholeArrayOut nNeighbours);  // (output)
  typedef void ExecutionSignature(_1, InputIndex, _2);
  typedef _1 InputDomain;

  // Default Constructor
  VTKM_EXEC_CONT
  ComputeMaxNeighboursWorklet(const vtkm::Id neighboursSize)
    : NeighboursSize(neighboursSize)
  {
  }

  template <typename OutFieldPortalType, typename InFieldPortalType>
  VTKM_EXEC void operator()(const InFieldPortalType& firstNeighbourPortal,
                            vtkm::Id startVtxNo,
                            const OutFieldPortalType& nNeighboursPortal) const
  {
    if (startVtxNo < firstNeighbourPortal.GetNumberOfValues() - 1)
    {
      nNeighboursPortal.Set(startVtxNo,
                            firstNeighbourPortal.Get(startVtxNo + 1) -
                              firstNeighbourPortal.Get(startVtxNo));
    }
    else
    {
      nNeighboursPortal.Set(startVtxNo,
                            NeighboursSize -
                              firstNeighbourPortal.Get(nNeighboursPortal.GetNumberOfValues() - 1));
    }

    // In serial this worklet implements the following operation
    // #pragma omp parallel for
    // for (indexVector::size_type startVtxNo = 0; startVtxNo < firstNeighbour.size()-1; ++startVtxNo)
    //   {
    //     nNeighbours[startVtxNo] = firstNeighbour[startVtxNo+1] - firstNeighbour[startVtxNo];
    //   }
    //  nNeighbours[nNeighbours.size() - 1] = neighbours.size() - firstNeighbour[nNeighbours.size() - 1];
    //
    // // NOTE: In the above we change the loop to run for the full length of the array and instead
    // //       then do a conditional assign for the last element directly within the loop, rather
    // //       than shortcutting the loop and doing a special assigne after the loop. This allows
    // //       us to process all elements on the device in parallel rather than having to pull
    // //       data back into the control area to do the last assignement
  }

private:
  vtkm::Id NeighboursSize;


}; //  ComputeMaxNeighboursWorklet


} // namespace mesh_dem_contourtree_mesh_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
