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

#ifndef vtk_m_filter_scalar_topology_worklet_branch_decomposition_select_top_volume_contours_BranchVolumeComparator_h
#define vtk_m_filter_scalar_topology_worklet_branch_decomposition_select_top_volume_contours_BranchVolumeComparator_h

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

// Implementation of BranchVolumeComparator
class BranchVolumeComparatorImpl
{
public:
  using IdPortalType = typename IdArrayType::ReadPortalType;

  // constructor
  VTKM_CONT
  BranchVolumeComparatorImpl(const IdArrayType& branchRoots,
                             const IdArrayType& branchVolume,
                             vtkm::cont::DeviceAdapterId device,
                             vtkm::cont::Token& token)
    : branchRootsPortal(branchRoots.PrepareForInput(device, token))
    , branchVolumePortal(branchVolume.PrepareForInput(device, token))
  { // constructor
  } // constructor

  // () operator - gets called to do comparison
  VTKM_EXEC
  bool operator()(const vtkm::Id& i, const vtkm::Id& j) const
  { // operator()
    vtkm::Id volumeI = this->branchVolumePortal.Get(i);
    vtkm::Id volumeJ = this->branchVolumePortal.Get(j);

    // primary sort on branch volume
    if (volumeI > volumeJ)
      return true;
    if (volumeI < volumeJ)
      return false;

    vtkm::Id branchI =
      vtkm::worklet::contourtree_augmented::MaskedIndex(this->branchRootsPortal.Get(i));
    vtkm::Id branchJ =
      vtkm::worklet::contourtree_augmented::MaskedIndex(this->branchRootsPortal.Get(j));

    // secondary sort on branch ID
    return (branchI < branchJ);
  } // operator()

private:
  IdPortalType branchRootsPortal;
  IdPortalType branchVolumePortal;

}; // BranchVolumeComparatorImpl

/// <summary>
/// Comparator of branch volume. Higher volume comes first
/// </summary>
class BranchVolumeComparator : public vtkm::cont::ExecutionObjectBase
{

public:
  // constructor
  VTKM_CONT
  BranchVolumeComparator(const IdArrayType& branchRoots, const IdArrayType& branchVolume)
    : BranchRoots(branchRoots)
    , BranchVolume(branchVolume)
  {
  }

  VTKM_CONT BranchVolumeComparatorImpl PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                           vtkm::cont::Token& token) const
  {
    return BranchVolumeComparatorImpl(this->BranchRoots, this->BranchVolume, device, token);
  }

private:
  IdArrayType BranchRoots;
  IdArrayType BranchVolume;
}; // BranchVolumeComparator


} // namespace select_top_volume_contours
} // namespace scalar_topology
} // namespace worklet
} // namespace vtkm

#endif
