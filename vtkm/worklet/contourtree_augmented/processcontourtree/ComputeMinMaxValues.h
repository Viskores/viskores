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

#ifndef vtkm_worklet_contourtree_augmented_process_contourtree_inc_compute_min_max_values_h
#define vtkm_worklet_contourtree_augmented_process_contourtree_inc_compute_min_max_values_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

/*
 *
* This code is written by Petar Hristov in 09.2019
*
* This worklet computes a prefix min/max over the subtree of a rooted contour tree
* using the euler tour data structure.
*
* Given an euler tour and an array which has the index of the first and last occurence of every vertex,
* this worklet goes through that subarray of the euler tour to find the min/max value (in terms of regular node Id, or isovalue, does not matter which)
*
* This can be optimised with a vtkm parallel reduce operation
*
*/
namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{
class ComputeMinMaxValues : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn supernodes,
                                WholeArrayIn firstLast,
                                WholeArrayIn tourEdges,
                                WholeArrayOut output);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4);
  using InputDomain = _1;

  bool isMin = true;

  // Default Constructor
  VTKM_EXEC_CONT ComputeMinMaxValues(bool _isMin)
    : isMin(_isMin)
  {
  }

  template <typename SupernodesArrayPortalType,
            typename FirstLastArrayPortalType,
            typename TourEdgesArrayPortalType,
            typename OutputArrayPortalType>
  VTKM_EXEC void operator()(const vtkm::Id i,
                            const SupernodesArrayPortalType& supernodes,
                            const FirstLastArrayPortalType& firstLast,
                            const TourEdgesArrayPortalType& tourEdges,
                            const OutputArrayPortalType& output) const
  {
    Id optimal = tourEdges.Get(firstLast.Get(i).first)[1];

    for (Id j = firstLast.Get(i).first; j < firstLast.Get(i).second; j++)
    {
      Id vertex = tourEdges.Get(j)[1];

      Id vertexValue = maskedIndex(supernodes.Get(vertex));
      Id optimalValue = maskedIndex(supernodes.Get(optimal));

      if ((true == isMin && vertexValue < optimalValue) ||
          (false == isMin && vertexValue > optimalValue))
      {
        optimal = vertex;
      }
    }

    output.Set(i, optimal);
  }
}; // ComputeMinMaxValues
} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm


#endif
