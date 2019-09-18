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

#ifndef vtkm_worklet_contourtree_augmented_contourtree_mesh_inc_find_start_index_worklet_h
#define vtkm_worklet_contourtree_augmented_contourtree_mesh_inc_find_start_index_worklet_h

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


class FindStartIndexWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn neighbours,       // (input) neighbours
                                WholeArrayIn arcs,             // (input) arcs
                                WholeArrayOut firstNeighbour); // (output) firstNeighbours
  typedef void ExecutionSignature(_1, InputIndex, _2, _3);
  typedef _1 InputDomain;

  // Default Constructor
  VTKM_EXEC_CONT
  FindStartIndexWorklet() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC void operator()(const InFieldPortalType& neighboursPortal,
                            vtkm::Id sortedArcNo,
                            const InFieldPortalType& arcsPortal,
                            const OutFieldPortalType& firstNeighbourPortal) const
  {
    if (sortedArcNo > 0)
    {
      vtkm::Id prevFrom = (neighboursPortal.Get(sortedArcNo - 1) % 2 == 0)
        ? neighboursPortal.Get(sortedArcNo - 1) / 2
        : maskedIndex(arcsPortal.Get(neighboursPortal.Get(sortedArcNo - 1) / 2));
      vtkm::Id currFrom = (neighboursPortal.Get(sortedArcNo) % 2 == 0)
        ? neighboursPortal.Get(sortedArcNo) / 2
        : maskedIndex(arcsPortal.Get(neighboursPortal.Get(sortedArcNo) / 2));
      if (currFrom != prevFrom)
      {
        firstNeighbourPortal.Set(currFrom, sortedArcNo);
      }
    }
    else // sortedArcNo == 0
    {
      firstNeighbourPortal.Set(0, 0);
    }

    // In serial this worklet implements the following operation
    // #pragma omp parallel for
    // for (indexVector::size_type sortedArcNo = 1; sortedArcNo < neighbours.size(); ++sortedArcNo)
    //   {
    //      indexType prevFrom = (neighbours[sortedArcNo-1] % 2 == 0) ? neighbours[sortedArcNo-1]/2 : maskedIndex(arcs[neighbours[sortedArcNo-1]/2]);
    //      indexType currFrom = (neighbours[sortedArcNo  ] % 2 == 0) ? neighbours[sortedArcNo  ]/2 : maskedIndex(arcs[neighbours[sortedArcNo  ]/2]);
    //      if (currFrom != prevFrom)
    //       {
    //          assert(currFrom < firstNeighbour.size());
    //          firstNeighbour[currFrom] = sortedArcNo;
    //        }
    //   }
  }


}; //  ComputeMaxNeighboursWorklet


} // namespace mesh_dem_contourtree_mesh_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
