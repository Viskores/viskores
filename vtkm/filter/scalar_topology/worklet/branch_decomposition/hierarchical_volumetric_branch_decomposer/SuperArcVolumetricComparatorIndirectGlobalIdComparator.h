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
// SuperArcVolumetricComparatorIndirectGlobalID.h - a comparator for sorting superarcs by volume
// Has to take a flag for high end vs. low end sorting
// Also, this version takes supernode IDs rather than global IDs, so has an extra indirection
//
//=======================================================================================
//
// COMMENTS:
//
// A comparator that sorts superarc pairs by:
// 1.   ID of low end vertex
// 2.   volumetric measure at low end
// 3.   global index of upper end, OR
//
// the same for the higher end.
//
// Notice that 2. only applies if two edges share a lower end and have the same volume.
// We then look at the index at the upper end to see which is "furthest" from the low end
//
//=======================================================================================

#ifndef vtk_m_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_SuperarcVolumetricComparatorIndirectGlobalIdComparator_h
#define vtk_m_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_SuperarcVolumetricComparatorIndirectGlobalIdComparator_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace vtkm
{
namespace worklet
{
namespace scalar_topology
{
namespace hierarchical_volumetric_branch_decomposer
{

/// Implementation of the comparator for the SuperArcVolumetricComparatorIndirectGlobalId ExecutionObject
class SuperArcVolumetricComparatorIndirectGlobalIdComparatorImpl
{
public:
  using IdArrayPortalType =
    typename vtkm::worklet::contourtree_augmented::IdArrayType::ReadPortalType;
  using EdgePairArrayPortalType =
    typename vtkm::worklet::contourtree_augmented::EdgePairArray::ReadPortalType;

  // constructor
  VTKM_CONT
  SuperArcVolumetricComparatorIndirectGlobalIdComparatorImpl(
    IdArrayPortalType weightPortal,
    EdgePairArrayPortalType superarcListPortal,
    IdArrayPortalType globalIdPortal,
    bool pairsAtLowEnd)
    : WeightPortal(weightPortal)
    , SuperarcListPortal(superarcListPortal)
    , GlobalIdPortal(globalIdPortal)
    , PairsAtLowEnd(pairsAtLowEnd)
  { // constructor
  } // constructor

  // () operator - gets called to do comparison
  VTKM_EXEC
  bool operator()(const vtkm::Id& left, const vtkm::Id& right) const
  { // operator()
    // get local references to the edge details
    vtkm::worklet::contourtree_augmented::EdgePair edgeLeft = this->SuperarcListPortal.Get(left);
    vtkm::worklet::contourtree_augmented::EdgePair edgeRight = this->SuperarcListPortal.Get(right);

    if (this->PairsAtLowEnd)
    { // pairs at low end
      // test by low end ID
      if (edgeLeft.first < edgeRight.first)
      {
        return true;
      }
      if (edgeLeft.first > edgeRight.first)
      {
        return false;
      }

      // test by volumetric measure
      vtkm::Id weightLeft = this->WeightPortal.Get(left);
      vtkm::Id weightRight = this->WeightPortal.Get(right);
      if (weightLeft < weightRight)
        return true;
      if (weightLeft > weightRight)
        return false;

      // test by the global ID - we were past a supernode ID, so there's an
      // extra level of indirection
      vtkm::Id globalIdLeftEdgeSecond = this->GlobalIdPortal.Get(edgeLeft.second);
      vtkm::Id globalIdRightEdgeSecond = this->GlobalIdPortal.Get(edgeRight.second);
      if (globalIdLeftEdgeSecond < globalIdRightEdgeSecond)
      {
        return true;
      }
      if (globalIdLeftEdgeSecond > globalIdRightEdgeSecond)
      {
        return false;
      }

      // fallback
      return false;
    } // pairs at low end
    else
    { // pairs at high end
      // test by high end ID
      if (edgeLeft.second < edgeRight.second)
      {
        return true;
      }
      if (edgeLeft.second > edgeRight.second)
      {
        return false;
      }

      // test by volumetric measure
      vtkm::Id weightLeft = this->WeightPortal.Get(left);
      vtkm::Id weightRight = this->WeightPortal.Get(right);
      if (weightLeft < weightRight)
      {
        return true;
      }
      if (weightLeft > weightRight)
      {
        return false;
      }

      // test by the global ID - we were past a supernode ID, so there's an
      // extra level of indirection
      // Note the reversal from above - we want the greatest difference, not
      // the greatest value
      vtkm::Id globalIdLeftEdgeFirst = this->GlobalIdPortal.Get(edgeLeft.first);
      vtkm::Id globalIdRightEdgeFirst = this->GlobalIdPortal.Get(edgeRight.first);
      if (globalIdLeftEdgeFirst > globalIdRightEdgeFirst)
      {
        return true;
      }
      if (globalIdLeftEdgeFirst < globalIdRightEdgeFirst)
      {
        return false;
      }

      // fallback
      return false;
    } // pairs at high end
  }   // operator()

private:
  IdArrayPortalType WeightPortal;
  EdgePairArrayPortalType SuperarcListPortal;
  IdArrayPortalType GlobalIdPortal;
  bool PairsAtLowEnd;
}; // SuperArcVolumetricComparatorIndirectGlobalIdComparatorImpl


/// Execution object for  Compartor used in HierarchicalVolumetricBranchDecomposer<FieldType>::LocalBestUpDownByVolume.
/// The comparator is used to sort superarc pairs by:
/// 1.  ID of low end vertex
/// 2.  volumetric measure at low end
/// 3.  global index of upper end, OR
/// the same for the higher end. Notice that 2. only applies if two edges share a lower end and have the same volume.
/// We then look at the index at the upper end to see which is "furthest" from the low end
class SuperArcVolumetricComparatorIndirectGlobalIdComparator
  : public vtkm::cont::ExecutionObjectBase
{
public:
  // constructor - takes vectors as parameters
  VTKM_CONT
  SuperArcVolumetricComparatorIndirectGlobalIdComparator(
    const vtkm::worklet::contourtree_augmented::IdArrayType& weight,
    const vtkm::worklet::contourtree_augmented::EdgePairArray& superarcList,
    const vtkm::worklet::contourtree_augmented::IdArrayType& globalId,
    bool pairsAtLowEnd)
    : Weight(weight)
    , SuperarcList(superarcList)
    , GlobalId(globalId)
    , PairsAtLowEnd(pairsAtLowEnd)
  { // constructor
  } // constructor

  /// Create a SuperArcVolumetricComparatorIndirectGlobalIdComparatorImpl object for use in the sort or worklet
  VTKM_CONT SuperArcVolumetricComparatorIndirectGlobalIdComparatorImpl
  PrepareForExecution(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token& token) const
  {
    return SuperArcVolumetricComparatorIndirectGlobalIdComparatorImpl(
      this->Weight.PrepareForInput(device, token),
      this->SuperarcList.PrepareForInput(device, token),
      this->GlobalId.PrepareForInput(device, token),
      this->PairsAtLowEnd);
  }

private:
  vtkm::worklet::contourtree_augmented::IdArrayType Weight;
  vtkm::worklet::contourtree_augmented::EdgePairArray SuperarcList;
  vtkm::worklet::contourtree_augmented::IdArrayType GlobalId;
  bool PairsAtLowEnd;
}; // SuperArcVolumetricComparatorIndirectGlobalIdComparator

} // namespace hierarchical_volumetric_branch_decomposer
} // namespace scalar_topology
} // namespace worklet
} // namespace vtkm

#endif
