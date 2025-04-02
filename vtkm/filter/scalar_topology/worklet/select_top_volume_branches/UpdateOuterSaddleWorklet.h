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

#ifndef vtk_m_filter_scalar_topology_worklet_select_top_volume_branches_UpdateOuterSaddleWorklet_h
#define vtk_m_filter_scalar_topology_worklet_select_top_volume_branches_UpdateOuterSaddleWorklet_h

#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace scalar_topology
{
namespace select_top_volume_branches
{

/// <summary>
/// worklet to update the value of outer saddles for parent branches
/// </summary>
template <bool isMaximum>
class UpdateOuterSaddle : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn branchOrder,    // (input) the order of the (top-volume) branch by volume
    FieldInOut branchValue, // (input/output) the isovalue to extract
    FieldInOut
      branchSaddleGRId, // (input/output) the global regular ID come along with the isovalue
    WholeArrayIn incomingOrders, // (array input) (sorted) orders of branches from the other block
    WholeArrayIn
      incomingValues, // (array input) isovalues to extract on branches from the other block
    WholeArrayIn incomingSaddleGRIds // (array input) saddle global regular IDs from the other block
  );
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  using IdArrayPortalType = typename IdArrayType::ReadPortalType;

  /// Constructor
  VTKM_EXEC_CONT
  UpdateOuterSaddle() {}

  template <typename ValueType, typename ValuePortalType>
  VTKM_EXEC void operator()(const vtkm::Id& branchOrder,
                            ValueType& branchValue,
                            vtkm::Id& branchSaddleGRId,
                            const IdArrayPortalType& incomingOrders,
                            const ValuePortalType& incomingValues,
                            const IdArrayPortalType& incomingSaddleGRIds) const
  {
    vtkm::Id head = 0;
    vtkm::Id tail = incomingOrders.GetNumberOfValues() - 1;
    while (head <= tail)
    {
      vtkm::Id mid = (head + tail) >> 1;
      vtkm::Id midOrder = incomingOrders.Get(mid);
      if (midOrder == branchOrder)
      {
        const ValueType midValue = incomingValues.Get(mid);
        const vtkm::Id midSaddleGRId = incomingSaddleGRIds.Get(mid);
        if (isMaximum &&
            (midValue > branchValue ||
             (midValue == branchValue && midSaddleGRId > branchSaddleGRId)))
        {
          branchSaddleGRId = midSaddleGRId;
          branchValue = midValue;
        }
        else if (!isMaximum &&
                 (midValue < branchValue ||
                  (midValue == branchValue && midSaddleGRId < branchSaddleGRId)))
        {
          branchSaddleGRId = midSaddleGRId;
          branchValue = midValue;
        }
        return;
      }
      else if (midOrder > branchOrder)
        tail = mid - 1;
      else
        head = mid + 1;
    }
  }
}; // UpdateOuterSaddle

} // namespace select_top_volume_branches
} // namespace scalar_topology
} // namespace worklet
} // namespace vtkm

#endif
