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
// COMMENTS:
//
// A comparator that sorts supernode pairs by:
//  1.  the superparent (ie the superarc into which an attachment point inserts)
//    note that this implicitly sorts on round of insertion as well
//  2.  data value
//  3.  global regular ID
//
//  The superparent is assumed to have a flag indicating ascending/descending, and this
//  needs to be used to get the correct inwards ordering along each superarc
//
//=======================================================================================


#ifndef vtk_m_worklet_contourtree_distributed_hierarchical_hyper_augmenter_attachment_and_supernode_comparator_h
#define vtk_m_worklet_contourtree_distributed_hierarchical_hyper_augmenter_attachment_and_supernode_comparator_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_augmenter
{


/// Compartor implementation used in HierarchicalAugmenter<FieldType>::ResizeArrays to sort this->SupernodeSorter
/// A comparator that sorts supernode pairs by:
///  1.  the superparent (ie the superarc into which an attachment point inserts)
///    note that this implicitly sorts on round of insertion as well
///  2.  data value
///  3.  global regular ID
///
///  The superparent is assumed to have a flag indicating ascending/descending, and this
///  needs to be used to get the correct inwards ordering along each superarc
template <typename FieldType>
class AttachmentAndSupernodeComparatorImpl
{
public:
  using IdArrayPortalType =
    typename vtkm::worklet::contourtree_augmented::IdArrayType::ReadPortalType;
  using FieldArrayPortalType = typename vtkm::cont::ArrayHandle<FieldType>::ReadPortalType;

  // constructor
  VTKM_CONT
  AttachmentAndSupernodeComparatorImpl(IdArrayPortalType superparentSetPortal,
                                       FieldArrayPortalType dataValueSetPortal,
                                       IdArrayPortalType globalRegularIdSetPortal)
    : SuperparentSetPortal(superparentSetPortal)
    , DataValueSetPortal(dataValueSetPortal)
    , GlobalRegularIdSetPortal(globalRegularIdSetPortal)
  { // constructor
  } // constructor

  // () operator - gets called to do comparison
  VTKM_EXEC
  bool operator()(const vtkm::Id& left, const vtkm::Id& right) const
  { // operator()
    // first comparison is on superparent WITHOUT ascending descending flag
    if (vtkm::worklet::contourtree_augmented::MaskedIndex(this->SuperparentSetPortal.Get(left)) <
        vtkm::worklet::contourtree_augmented::MaskedIndex(this->SuperparentSetPortal.Get(right)))
    {
      return true;
    }
    if (vtkm::worklet::contourtree_augmented::MaskedIndex(this->SuperparentSetPortal.Get(left)) >
        vtkm::worklet::contourtree_augmented::MaskedIndex(this->SuperparentSetPortal.Get(right)))
    {
      return false;
    }

    // second comparison is on data value
    if (this->DataValueSetPortal.Get(left) < this->DataValueSetPortal.Get(right))
    {
      return vtkm::worklet::contourtree_augmented::IsAscending(
        this->SuperparentSetPortal.Get(left));
    }
    if (this->DataValueSetPortal.Get(left) > this->DataValueSetPortal.Get(right))
    {
      return !vtkm::worklet::contourtree_augmented::IsAscending(
        this->SuperparentSetPortal.Get(left));
    }

    // third comparison is on global regular ID
    if (this->GlobalRegularIdSetPortal.Get(left) < this->GlobalRegularIdSetPortal.Get(right))
    {
      return vtkm::worklet::contourtree_augmented::IsAscending(
        this->SuperparentSetPortal.Get(left));
    }
    if (this->GlobalRegularIdSetPortal.Get(left) > this->GlobalRegularIdSetPortal.Get(right))
    {
      return !vtkm::worklet::contourtree_augmented::IsAscending(
        this->SuperparentSetPortal.Get(left));
    }

    // fall-through (should never happen)
    return false;
  } // operator()

private:
  IdArrayPortalType SuperparentSetPortal;
  FieldArrayPortalType DataValueSetPortal;
  IdArrayPortalType GlobalRegularIdSetPortal;
}; // AttachmentAndSupernodeComparatorImpl


/// Execution object for  Compartor used in HierarchicalAugmenter<FieldType>::ResizeArrays to sort this->SupernodeSorter
/// A comparator that sorts supernode pairs by:
///  1.  the superparent (ie the superarc into which an attachment point inserts)
///    note that this implicitly sorts on round of insertion as well
///  2.  data value
///  3.  global regular ID
///
///  The superparent is assumed to have a flag indicating ascending/descending, and this
///  needs to be used to get the correct inwards ordering along each superarc
template <typename FieldType>
class AttachmentAndSupernodeComparator : public vtkm::cont::ExecutionObjectBase
{
public:
  // constructor - takes vectors as parameters
  VTKM_CONT
  AttachmentAndSupernodeComparator(
    const vtkm::worklet::contourtree_augmented::IdArrayType& superparentSet,
    const vtkm::cont::ArrayHandle<FieldType>& dataValueSet,
    const vtkm::worklet::contourtree_augmented::IdArrayType& globalRegularIdSet)
    : SuperparentSet(superparentSet)
    , DataValueSet(dataValueSet)
    , GlobalRegularIdSet(globalRegularIdSet)
  { // constructor
  } // constructor

  /// Create a AttachmentAndSupernodeComparatorImpl object for use in the sort or worklet
  VTKM_CONT AttachmentAndSupernodeComparatorImpl<FieldType> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    return AttachmentAndSupernodeComparatorImpl<FieldType>(
      this->SuperparentSet.PrepareForInput(device, token),
      this->DataValueSet.PrepareForInput(device, token),
      this->GlobalRegularIdSet.PrepareForInput(device, token));
  }

private:
  /// the superparent Id
  vtkm::worklet::contourtree_augmented::IdArrayType SuperparentSet;
  /// the global rergular Id for tiebreak
  vtkm::cont::ArrayHandle<FieldType> DataValueSet;
  /// the supernode Id for tiebreak
  vtkm::worklet::contourtree_augmented::IdArrayType GlobalRegularIdSet;
}; // AttachmentAndSupernodeComparator

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
