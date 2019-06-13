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

#ifndef vtkm_worklet_contourtree_augmented_active_graph_inc_super_arc_node_comparator_h
#define vtkm_worklet_contourtree_augmented_active_graph_inc_super_arc_node_comparator_h

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
namespace active_graph_inc
{


// comparator used for initial sort of data values
template <typename DeviceAdapter>
class SuperArcNodeComparatorImpl
{
public:
  using IdPortalType =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapter>::PortalConst;

  IdPortalType superparentsPortal;
  bool isJoinSweep;

  // constructor - takes vectors as parameters
  VTKM_CONT
  SuperArcNodeComparatorImpl(const IdArrayType& superparents, bool joinSweep)
    : isJoinSweep(joinSweep)
  { // constructor
    superparentsPortal = superparents.PrepareForInput(DeviceAdapter());
  } // constructor

  // () operator - gets called to do comparison
  VTKM_EXEC
  bool operator()(const vtkm::Id& i, const vtkm::Id& j) const
  { // operator()
    // first make sure we have the "top" end set correctly
    vtkm::Id superarcI = superparentsPortal.Get(i);
    vtkm::Id superarcJ = superparentsPortal.Get(j);

    // now test on that
    if (superarcI < superarcJ)
      return false ^ isJoinSweep;
    if (superarcJ < superarcI)
      return true ^ isJoinSweep;

    // if that fails, we share the hyperarc, and sort on supernode index
    // since that's guaranteed to be pre-sorted
    if (i < j)
      return false ^ isJoinSweep;
    if (j < i)
      return true ^ isJoinSweep;

    // fallback just in case
    return false;
  } // operator()
};  // SuperArcNodeComparatorImpl

class SuperArcNodeComparator : public vtkm::cont::ExecutionObjectBase
{
public:
  // constructor - takes vectors as parameters
  VTKM_CONT
  SuperArcNodeComparator(const IdArrayType& superparents, bool joinSweep)
    : Superparents(superparents)
    , JoinSweep(joinSweep)
  {
  }

  template <typename DeviceAdapter>
  VTKM_CONT SuperArcNodeComparatorImpl<DeviceAdapter> PrepareForExecution(DeviceAdapter) const
  {
    return SuperArcNodeComparatorImpl<DeviceAdapter>(this->Superparents, this->JoinSweep);
  }

private:
  IdArrayType Superparents;
  bool JoinSweep;
}; // SuperArcNodeComparator

} // namespace active_graph_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
