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

#ifndef vtk_m_filter_scalar_topology_worklet_select_top_volume_contours_GetBranchVolumeWorklet_h
#define vtk_m_filter_scalar_topology_worklet_select_top_volume_contours_GetBranchVolumeWorklet_h

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace scalar_topology
{
namespace select_top_volume_contours
{

using IdArrayType = vtkm::worklet::contourtree_augmented::IdArrayType;

/// <summary>
/// worklet to check the direction of branch
/// return true if the branch inner superarc points to the senior-most node
/// return false if the branch inner superarc comes from the senior-most node
/// trick: if the branch is the main branch, we return true for computation later
/// </summary>
class GetBranchVolumeWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn lowerDirection, // (input) lower end superarc ID with direction information
    FieldIn lowerIntrinsic, // (input) lower end superarc intrisic volume
    FieldIn lowerDependent, // (input) lower end superarc dependent volume
    FieldIn upperDirection, // (input) upper end superarc ID with direction information
    FieldIn upperIntrinsic, // (input) upper end superarc intrisic volume
    FieldIn upperDependent, // (input) upper end superarc dependent volume
    FieldIn isLowerLeaf,    // (input) bool, whether the lower end is a leaf
    FieldIn isUpperLeaf,    // (input) bool, whether the upper end is a leaf
    FieldOut branchVolume   // (output) volume of the branch
  );
  using ExecutionSignature = _9(_1, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  /// Constructor
  VTKM_EXEC_CONT
  GetBranchVolumeWorklet(const vtkm::Id tVol)
    : totalVolume(tVol)
  {
  }

  /// The functor checks the direction of the branch
  VTKM_EXEC vtkm::Id operator()(const vtkm::Id& lowerDirection,
                                const vtkm::Id& lowerIntrinsic,
                                const vtkm::Id& lowerDependent,
                                const vtkm::Id& upperDirection,
                                const vtkm::Id& upperIntrinsic,
                                const vtkm::Id& upperDependent,
                                const bool& isLowerLeaf,
                                const bool& isUpperLeaf) const
  {
    if (isLowerLeaf && isUpperLeaf)
      return totalVolume;
    // if the branch is a minimum-saddle branch
    // if the upper end superarc direction is pointing up, then dependent; otherwise, reverse
    if (isLowerLeaf)
      return contourtree_augmented::IsAscending(upperDirection)
        ? upperDependent
        : totalVolume - upperDependent + upperIntrinsic;
    // if the branch is a maximum-saddle branch
    // if the lower end superarc direction is pointing down, then true; otherwise, false
    if (isUpperLeaf)
      return !contourtree_augmented::IsAscending(lowerDirection)
        ? lowerDependent
        : totalVolume - lowerDependent + lowerIntrinsic;

    // in case of fallout, should never reach
    return 0;
  }

private:
  const vtkm::Id totalVolume;
}; // GetBranchVolumeWorklet

} // namespace select_top_volume_contours
} // namespace scalar_topology
} // namespace worklet
} // namespace vtkm

#endif
