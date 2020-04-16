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

#ifndef vtk_m_worklet_contourtree_augmented_process_contourtree_inc_hypersweep_worklets_h
#define vtk_m_worklet_contourtree_augmented_process_contourtree_inc_hypersweep_worklets_h

#include <vtkm/BinaryOperators.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

/**
 * Incorporates values of the parent of the current subtree in the subtree for the min and max hypersweeps
 */
namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{




template <typename Operator>
class IncorporateEdge : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn minParents,
                                WholeArrayIn supernodes,
                                WholeArrayOut minMaxValues);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _1;

  Operator op;
  VTKM_EXEC_CONT IncorporateEdge(Operator _op)
    : op(_op)
  {
  }

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayOutPortalType>
  VTKM_EXEC void operator()(const vtkm::Id currentId,
                            const IdWholeArrayInPortalType& parentsPortal,
                            const IdWholeArrayInPortalType& supernodesPortal,
                            const IdWholeArrayOutPortalType& minMaxValuesPortal) const
  {
    Id parent = MaskedIndex(parentsPortal.Get(currentId));
    Id subtreeValue = minMaxValuesPortal.Get(currentId);
    Id parentValue = MaskedIndex(supernodesPortal.Get(parent));
    minMaxValuesPortal.Set(currentId, op(parentValue, subtreeValue));
  }
}; // ComputeMinMaxValues




class GetOppositeValue : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn minParents,
                                WholeArrayIn maxParents,
                                WholeArrayIn minValues,
                                WholeArrayIn maxValues,
                                WholeArrayInOut arcs);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4, _5);
  using InputDomain = _1;

  vtkm::Id globalMinSortedIndex, globalMaxSortedIndex;
  VTKM_EXEC_CONT GetOppositeValue(vtkm::Id _globalMinSortedIndex, vtkm::Id _globalMaxSortedIndex)
    : globalMinSortedIndex(_globalMinSortedIndex)
    , globalMaxSortedIndex(_globalMaxSortedIndex)
  {
  }

  template <typename IdWholeArrayInPortalType, typename EdgeWholeArrayInOutPortal>
  VTKM_EXEC void operator()(const vtkm::Id currentId,
                            const IdWholeArrayInPortalType& minParents,
                            const IdWholeArrayInPortalType& maxParents,
                            const IdWholeArrayInPortalType& minValues,
                            const IdWholeArrayInPortalType& maxValues,
                            const EdgeWholeArrayInOutPortal& arcs) const
  {
    auto i = currentId;
    auto edge = arcs.Get(i);

    // Is it in the direction of the minRootedTree?
    if (MaskedIndex(minParents.ReadPortal().Get(edge.j)) == edge.i)
    {
      edge.subtreeMin = minValues.ReadPortal().Get(edge.j);
    }
    else
    {
      edge.subtreeMin = globalMinSortedIndex;
    }

    // Is it in the direction of the maxRootedTree?
    if (MaskedIndex(maxParents.ReadPortal().Get(edge.j)) == edge.i)
    {
      edge.subtreeMax = maxValues.ReadPortal().Get(edge.j);
    }
    else
    {
      edge.subtreeMax = globalMinSortedIndex;
    }

    arcs.Set(i, edge);
  }
}; // ComputeMinMaxValues




} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm


#endif
