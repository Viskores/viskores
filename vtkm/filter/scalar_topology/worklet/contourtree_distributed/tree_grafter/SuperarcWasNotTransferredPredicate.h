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

#ifndef vtk_m_worklet_contourtree_distributed_tree_grafter_superarc_was_not_transferred_predicate_h
#define vtk_m_worklet_contourtree_distributed_tree_grafter_superarc_was_not_transferred_predicate_h


#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{
namespace tree_grafter
{

// Uniary predicate used in TreeGrafter.CompressActiveArrays to decide which active superarcs to keep
class SuperarcWasNotTransferredPredicateImpl
{
public:
  using IdArrayPortalType =
    typename vtkm::worklet::contourtree_augmented::IdArrayType::ReadPortalType;

  // Default Constructor
  VTKM_EXEC_CONT
  SuperarcWasNotTransferredPredicateImpl(const IdArrayPortalType& whenTransferredPortal)
    : WhenTransferredPortal(whenTransferredPortal)
  {
  }

  VTKM_EXEC bool operator()(const vtkm::worklet::contourtree_augmented::EdgePair& superarc) const
  { // operator ()
    // if either end is marked as transferred, the arc must be gone already
    return !((!vtkm::worklet::contourtree_augmented::NoSuchElement(
               this->WhenTransferredPortal.Get(superarc.first))) ||
             (!vtkm::worklet::contourtree_augmented::NoSuchElement(
               this->WhenTransferredPortal.Get(superarc.second))));

  } // operator ()

private:
  IdArrayPortalType WhenTransferredPortal;


}; // SuperarcWasNotTransferredPredicate


class SuperarcWasNotTransferredPredicate : public vtkm::cont::ExecutionObjectBase
{
public:
  VTKM_CONT
  SuperarcWasNotTransferredPredicate(
    const vtkm::worklet::contourtree_augmented::IdArrayType& whenTransferred)
    : WhenTransferred(whenTransferred)
  {
  }

  VTKM_CONT SuperarcWasNotTransferredPredicateImpl
  PrepareForExecution(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token& token) const
  {
    return SuperarcWasNotTransferredPredicateImpl(
      this->WhenTransferred.PrepareForInput(device, token));
  }

private:
  const vtkm::worklet::contourtree_augmented::IdArrayType& WhenTransferred;
};

} // namespace tree_grafter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
