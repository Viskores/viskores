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

#ifndef vtkm_worklet_contourtree_augmented_process_contourtree_inc_compute_best_up_down_h
#define vtkm_worklet_contourtree_augmented_process_contourtree_inc_compute_best_up_down_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>


/*
* This code is written by Petar Hristov in 09.2019
*
* This worklet is part of the Contour Tree Height Based Simplification Code.
* It selects the bestUp and the bestDown for every supernode in the contour tree.
* The best up and best down are used to construct the branches.
*
* This worklet receives a 1D array of edges, sorter by their first vertex and then by their second vertex.
* Each invocation of the worklet goes through the neighbours of one vertex and looks for the bestUp and bestDown.
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
class ComputeBestUpDown : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn _1,
                                WholeArrayIn _2,
                                WholeArrayIn _3,
                                WholeArrayIn _4,
                                WholeArrayIn _5,
                                WholeArrayIn _6,
                                WholeArrayIn _7,
                                WholeArrayIn _8,
                                WholeArrayIn _9,
                                WholeArrayIn _10,
                                WholeArrayOut _11,
                                WholeArrayOut _12);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12);
  using InputDomain = _3;

  // Default Constructor
  VTKM_EXEC_CONT ComputeBestUpDown() {}

  template <typename FirstArrayPortalType,
            typename NodesArrayPortalType,
            typename SupernodesArrayPortalType,
            typename MinValuesArrayPortalType,
            typename MinParentsArrayPortalType,
            typename MaxValuesArrayPortalType,
            typename MaxParentsArrayPortalType,
            typename SortOrderArrayPortalType,
            typename EdgesLinearArrayPortalType,
            typename FieldValuesArrayPortalType,
            typename OutputArrayPortalType>

  VTKM_EXEC void operator()(const vtkm::Id i,
                            const FirstArrayPortalType& first,
                            const NodesArrayPortalType& nodes,
                            const SupernodesArrayPortalType& supernodes,
                            const MinValuesArrayPortalType& minValues,
                            const MinParentsArrayPortalType& minParents,
                            const MaxValuesArrayPortalType& maxValues,
                            const MaxParentsArrayPortalType& maxParents,
                            const SortOrderArrayPortalType& ctSortOrder,
                            const EdgesLinearArrayPortalType& edgesLinear,
                            const FieldValuesArrayPortalType& fieldValues,
                            const OutputArrayPortalType& bestUp,  // output
                            const OutputArrayPortalType& bestDown // output
                            ) const
  {
    Id k = first.Get(i);
    Float64 maxUpSubtreeHeight = 0;
    Float64 maxDownSubtreeHeight = 0;

    while (k < edgesLinear.GetNumberOfValues() && edgesLinear.Get(k)[0] == i)
    {
      Id j = edgesLinear.Get(k++)[1];

      Id regularVertexValueI = maskedIndex(supernodes.Get(i));
      Id regularVertexValueJ = maskedIndex(supernodes.Get(j));

      //
      // Get the minimum of subtree T(j) \cup {i}
      //

      // If the arc is pointed the right way (according to the rooting of the tree) use the subtree min value
      // This is the minimum of T(j)
      Id minValueInSubtree = maskedIndex(supernodes.Get(minValues.Get(j)));

      // See if the vertex i has a smaller value,
      // This means find the minimum of T(j) \cup {i}
      if (minValueInSubtree > regularVertexValueI)
      {
        minValueInSubtree = maskedIndex(supernodes.Get(i));
      }

      // If the dirrection of the arc is not according to the rooting of the tree,
      // then the minimum on that subtree must be the global minimum
      if (j == minParents.Get(i))
      {
        minValueInSubtree = 0;
      }

      //
      // Get the maximum of subtree T(j) \cup {i}
      //

      // See if the vertex i has a bigger value,
      // This means find the maximum of T(j) \cup {i}
      Id maxValueInSubtree = maskedIndex(supernodes.Get(maxValues.Get(j)));

      // Include the current vertex along with the subtree it points at
      if (maxValueInSubtree < regularVertexValueI)
      {
        maxValueInSubtree = maskedIndex(supernodes.Get(i));
      }

      // If the dirrection of the arc is not according to the rooting of the tree,
      // then the maximum on that subtree must be the global maximum
      if (j == maxParents.Get(i))
      {
        maxValueInSubtree = nodes.GetNumberOfValues() - 1;
      }

      // Afte having found the min and the max in T(j) \cup {i} we compute their height difference
      Float64 minValue = fieldValues.Get(ctSortOrder.Get(minValueInSubtree));
      Float64 maxValue = fieldValues.Get(ctSortOrder.Get(maxValueInSubtree));
      Float64 subtreeHeight = maxValue - minValue;

      // Downward Edge
      if (regularVertexValueI > regularVertexValueJ)
      {
        if (subtreeHeight > maxDownSubtreeHeight)
        {
          maxDownSubtreeHeight = subtreeHeight;
          bestDown.Set(i, j);
        }
      }

      // UpwardsEdge
      else
      {
        if (subtreeHeight > maxUpSubtreeHeight)
        {
          maxUpSubtreeHeight = subtreeHeight;
          bestUp.Set(i, j);
        }
      }
    }

    // Make sure at least one of these was set
    assert(false == noSuchElement(bestUp.Get(i)) || false == noSuchElement(bestDown.Get(i)));
  }
}; // ComputeBestUpDown
} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm


#endif
