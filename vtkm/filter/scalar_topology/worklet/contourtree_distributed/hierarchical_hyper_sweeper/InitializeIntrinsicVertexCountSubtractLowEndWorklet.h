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

#ifndef vtk_m_worklet_contourtree_distributed_hierarchical_hyper_sweeper_initialize_intrinsic_vertex_count_subtract_low_end_worklet_h
#define vtk_m_worklet_contourtree_distributed_hierarchical_hyper_sweeper_initialize_intrinsic_vertex_count_subtract_low_end_worklet_h

#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_hyper_sweeper
{

/// Worklet used in HierarchicalHyperSweeper.InitializeIntrinsicVertexCount(...) to
/// subtract out the low end from the superarc regular counts
class InitializeIntrinsicVertexCountSubtractLowEndWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn superparents, WholeArrayInOut superarcRegularCounts);
  using ExecutionSignature = void(InputIndex, _1, _2);
  using InputDomain = _1;

  // Default Constructor
  VTKM_EXEC_CONT
  InitializeIntrinsicVertexCountSubtractLowEndWorklet() {}

  template <typename InFieldPortalType, typename InOutFieldPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& vertex,
                            const InFieldPortalType superparentsPortal,
                            const InOutFieldPortalType superarcRegularCountsPortal) const
  {
    // per vertex
    // retrieve the superparent
    vtkm::Id superparent = superparentsPortal.Get(vertex);

    // if it's NSE, ignore (should never happen, but . . . )
    if (vtkm::worklet::contourtree_augmented::NoSuchElement(superparent))
    {
      return;
    }

    // if its the first element, always write
    if (vertex == 0)
    {
      superarcRegularCountsPortal.Set(superparent,
                                      superarcRegularCountsPortal.Get(superparent) - vertex);
    }
    // otherwise, only write if different from previous one
    else
    {
      if (superparentsPortal.Get(vertex - 1) != superparent)
      {
        superarcRegularCountsPortal.Set(superparent,
                                        superarcRegularCountsPortal.Get(superparent) - vertex);
      }
    }

    // In serial this worklet implements the following operation
    /*
    for (vtkm::Id vertex = 0; vertex < superparents.GetNumberOfValues(); vertex++)
    { // per vertex
      // retrieve the superparent
      vtkm::Id superparent = superparents[vertex];

      // if it's NSE, ignore (should never happen, but . . . )
      if (noSuchElement(superparent))
        continue;

      // if its the first element, always write
      if (vertex == 0)
        superarcRegularCounts[superparent] -= vertex;
      // otherwise, only write if different from previous one
      else
        if (superparents[vertex-1] != superparent)
          superarcRegularCounts[superparent] -= vertex;
    } // per vertex
    */
  } // operator()()

}; // InitializeIntrinsicVertexCountSubtractLowEndWorklet

} // namespace hierarchical_hyper_sweeper
} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
