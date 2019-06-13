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

#ifndef vtkm_worklet_contourtree_augmented_process_contourtree_inc_supernode_branch_comperator_h
#define vtkm_worklet_contourtree_augmented_process_contourtree_inc_supernode_branch_comperator_h

#include <vtkm/Pair.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{

template <typename DeviceAdapter>
class SuperNodeBranchComparatorImpl
{ // SuperNodeBranchComparatorImpl
public:
  using IdPortalType =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapter>::PortalConst;
  IdPortalType whichBranchPortal;
  IdPortalType supernodesPortal;

  // constructor
  SuperNodeBranchComparatorImpl(const IdArrayType& WhichBranch, const IdArrayType& Supernodes)
  { // constructor
    whichBranchPortal = WhichBranch.PrepareForInput(DeviceAdapter());
    supernodesPortal = Supernodes.PrepareForInput(DeviceAdapter());
  } // constructor

  // () operator - gets called to do comparison
  VTKM_EXEC
  bool operator()(const vtkm::Id& i, const vtkm::Id& j) const
  { // operator()
    // retrieve which branch the supernodes are on
    vtkm::Id branchI = maskedIndex(whichBranchPortal.Get(i));
    vtkm::Id branchJ = maskedIndex(whichBranchPortal.Get(j));

    // and test them
    if (branchI < branchJ)
      return true;
    if (branchJ < branchI)
      return false;

    // now fall back on regular ID
    vtkm::Id regularI = supernodesPortal.Get(i);
    vtkm::Id regularJ = supernodesPortal.Get(j);

    if (regularI < regularJ)
      return true;
    if (regularJ < regularI)
      return false;

    // fallback just in case
    return false;
  } // operator()
};  // SuperNodeBranchComparatorImpl

class SuperNodeBranchComparator : public vtkm::cont::ExecutionObjectBase
{ // SuperNodeBranchComparator
public:
  // constructor
  SuperNodeBranchComparator(const IdArrayType& whichBranch, const IdArrayType& supernodes)
    : WhichBranch(whichBranch)
    , Supernodes(supernodes)
  {
  }

  template <typename DeviceAdapter>
  VTKM_CONT SuperNodeBranchComparatorImpl<DeviceAdapter> PrepareForExecution(DeviceAdapter)
  {
    return SuperNodeBranchComparatorImpl<DeviceAdapter>(this->WhichBranch, this->Supernodes);
  }

private:
  IdArrayType WhichBranch;
  IdArrayType Supernodes;
}; // SuperNodeBranchComparator


} // namespace process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
