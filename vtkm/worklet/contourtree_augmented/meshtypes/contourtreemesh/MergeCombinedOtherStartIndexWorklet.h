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

#ifndef vtk_m_worklet_contourtree_augmented_contourtree_mesh_inc_merge_combined_other_start_index_worklet_h
#define vtk_m_worklet_contourtree_augmented_contourtree_mesh_inc_merge_combined_other_start_index_worklet_h

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

// STL
#include <algorithm>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem_contourtree_mesh_inc
{

class MergeCombinedOtherStartIndexWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(
    WholeArrayInOut combinedOtherStartIndex, // (input, output and input domain)
    WholeArrayInOut combinedNeighbours,      // (input, output)
    WholeArrayIn combinedFirstNeighbour      // (input)
  );
  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  typedef _1 InputDomain;

  // Default Constructor
  VTKM_EXEC_CONT
  MergeCombinedOtherStartIndexWorklet() {}


  template <typename InOutFieldPortalType, typename InFieldPortalType>
  VTKM_EXEC void operator()(const vtkm::Id vtx,
                            const InOutFieldPortalType combinedOtherStartIndexPortal,
                            const InOutFieldPortalType combinedNeighboursPortal,
                            const InFieldPortalType combinedFirstNeighbourPortal) const
  {
    if (combinedOtherStartIndexPortal.Get(vtx)) // Needs merge
    {
      auto myNeighboursBeginIdx = combinedFirstNeighbourPortal.Get(vtx);
      auto otherNeighboursBeginIdx = myNeighboursBeginIdx + combinedOtherStartIndexPortal.Get(vtx);
      auto neighboursEndIdx = (vtx < combinedFirstNeighbourPortal.GetNumberOfValues() - 1)
        ? combinedFirstNeighbourPortal.Get(vtx + 1) - 1
        : combinedNeighboursPortal.GetNumberOfValues() - 1;

      // Merge two sorted neighbours lists from myNeighboursBeginIdx through
      // otherNeighboursBeginIdx - 1 and from otherNeighboursBeginIdx - 1 to
      // neighboursEndIdx
      auto arr0_end = otherNeighboursBeginIdx - 1;
      auto curr = neighboursEndIdx;
      while (curr >= otherNeighboursBeginIdx)
      {
        auto x = combinedNeighboursPortal.Get(curr);
        if (x < combinedNeighboursPortal.Get(arr0_end))
        {
          combinedNeighboursPortal.Set(curr, combinedNeighboursPortal.Get(arr0_end));

          auto pos = arr0_end - 1;
          while (pos >= myNeighboursBeginIdx && (combinedNeighboursPortal.Get(pos) > x))
          {
            combinedNeighboursPortal.Set(pos + 1, combinedNeighboursPortal.Get(pos));
            --pos;
          }
          combinedNeighboursPortal.Set(pos + 1, x);
        }
        --curr;
      }

      // Remove duplicates
      vtkm::Id prevNeighbour = combinedNeighboursPortal.Get(myNeighboursBeginIdx);
      vtkm::Id currPos = myNeighboursBeginIdx + 1;
      for (vtkm::Id i = myNeighboursBeginIdx + 1; i <= neighboursEndIdx; ++i)
      {
        auto currNeighbour = combinedNeighboursPortal.Get(i);
        if (currNeighbour != prevNeighbour)
        {
          combinedNeighboursPortal.Set(currPos++, currNeighbour);
          prevNeighbour = currNeighbour;
        }
      }

      // Record number of elements in neighbour list for subsequent compression
      combinedOtherStartIndexPortal.Set(vtx, neighboursEndIdx + 1 - currPos);

      // Fill remainder with NO_SUCH_ELEMENT so that it can be easily discarded
      while (currPos != neighboursEndIdx + 1)
      {
        combinedNeighboursPortal.Set(currPos++, (vtkm::Id)NO_SUCH_ELEMENT);
      }
    }

    /* Reference code implemented by this worklet
       for (indexVector::size_type vtx = 0; vtx < combinedFirstNeighbour.size(); ++vtx)
       {
         if (combinedOtherStartIndex[vtx]) // Needs merge
         {
           indexVector::iterator neighboursBegin = combinedNeighbours.begin() + combinedFirstNeighbour[vtx];
           indexVector::iterator neighboursEnd = (vtx < combinedFirstNeighbour.size() - 1) ? combinedNeighbours.begin() + combinedFirstNeighbour[vtx+1] : combinedNeighbours.end();
           std::inplace_merge(neighboursBegin, neighboursBegin + combinedOtherStartIndex[vtx], neighboursEnd);
           indexVector::iterator it = std::unique(neighboursBegin, neighboursEnd);
           combinedOtherStartIndex[vtx] = neighboursEnd - it;
           while (it != neighboursEnd) *(it++) = NO_SUCH_ELEMENT;
         }
       }
    */
  }
}; //  MergeCombinedOtherStartIndexWorklet


} // namespace mesh_dem_contourtree_mesh_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
