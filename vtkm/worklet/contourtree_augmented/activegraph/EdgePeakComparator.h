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

#ifndef vtkm_worklet_contourtree_augmented_active_graph_inc_edge_peak_comparator_h
#define vtkm_worklet_contourtree_augmented_active_graph_inc_edge_peak_comparator_h

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
class EdgePeakComparatorImpl
{
public:
  using IdPortalType =
    typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<DeviceAdapter>::PortalConst;

  IdPortalType edgeFarPortal;
  IdPortalType edgeNearPortal;
  bool isJoinGraph;

  // constructor - takes vectors as parameters
  VTKM_CONT
  EdgePeakComparatorImpl(const IdArrayType& edgeFar, const IdArrayType& edgeNear, bool joinGraph)
    : isJoinGraph(joinGraph)
  { // constructor
    edgeFarPortal = edgeFar.PrepareForInput(DeviceAdapter());
    edgeNearPortal = edgeNear.PrepareForInput(DeviceAdapter());
  } // constructor

  // () operator - gets called to do comparison
  VTKM_EXEC
  bool operator()(const vtkm::Id& i, const vtkm::Id& j) const
  { // operator()
    // start by comparing the indices of the far end
    vtkm::Id farIndex1 = edgeFarPortal.Get(i);
    vtkm::Id farIndex2 = edgeFarPortal.Get(j);

    // first compare the far end
    if (farIndex1 < farIndex2)
    {
      return true ^ isJoinGraph;
    }
    if (farIndex2 < farIndex1)
    {
      return false ^ isJoinGraph;
    }

    // then compare the indices of the near end (which are guaranteed to be sorted!)
    vtkm::Id nearIndex1 = edgeNearPortal.Get(i);
    vtkm::Id nearIndex2 = edgeNearPortal.Get(j);

    if (nearIndex1 < nearIndex2)
    {
      return true ^ isJoinGraph;
    }
    if (nearIndex2 < nearIndex1)
    {
      return false ^ isJoinGraph;
    }

    // if the near indices match, compare the edge IDs
    if (i < j)
    {
      return false ^ isJoinGraph;
    }
    if (j < i)
    {
      return true ^ isJoinGraph;
    }

    // fallback can happen when multiple paths end at same extremum
    return false;
  } // operator()
};  // EdgePeakComparator

class EdgePeakComparator : public vtkm::cont::ExecutionObjectBase
{
public:
  // constructor - takes vectors as parameters
  VTKM_CONT
  EdgePeakComparator(const IdArrayType& edgeFar, const IdArrayType& edgeNear, bool joinGraph)
    : EdgeFar(edgeFar)
    , EdgeNear(edgeNear)
    , JoinGraph(joinGraph)
  {
  }

  template <typename DeviceAdapter>
  VTKM_CONT EdgePeakComparatorImpl<DeviceAdapter> PrepareForExecution(DeviceAdapter) const
  {
    return EdgePeakComparatorImpl<DeviceAdapter>(this->EdgeFar, this->EdgeNear, this->JoinGraph);
  }

private:
  IdArrayType EdgeFar;
  IdArrayType EdgeNear;
  bool JoinGraph;
}; // EdgePeakComparator

} // namespace active_graph_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
