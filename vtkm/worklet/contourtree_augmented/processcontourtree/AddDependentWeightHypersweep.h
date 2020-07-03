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

#ifndef vtk_m_worklet_contourtree_augmented_process_contourtree_inc_add_dependent_weight_hypersweep_h
#define vtk_m_worklet_contourtree_augmented_process_contourtree_inc_add_dependent_weight_hypersweep_h

#include <vtkm/BinaryOperators.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

/*
 *
* This code is written by Petar Hristov in 03.2020
*
* It does pointer doubling on an array.
*
*
*/
namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{




class InitialiseArcsVolume : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4);
  using InputDomain = _3;

  vtkm::Id totalVolume;

  VTKM_EXEC_CONT InitialiseArcsVolume(vtkm::Id _totalVolume)
    : totalVolume(_totalVolume)
  {
  }

  template <typename IdWholeArrayInPortalType, typename EdgeWholeArrayInOutPortal>
  VTKM_EXEC void operator()(const vtkm::Id currentId,
                            const IdWholeArrayInPortalType& hypersweepSumValuesPortal,
                            const IdWholeArrayInPortalType& superarcIntrinsicWeightPortal,
                            const IdWholeArrayInPortalType& superarcsPortal,
                            const EdgeWholeArrayInOutPortal& arcsPortal) const
  {
    Id i = currentId;
    Id parent = MaskedIndex(superarcsPortal.Get(i));
    if (parent == 0)
    {
      // We expect the root to the last vertex in the supernodes array
      assert(i != superarcsPortal.GetNumberOfValues() - 2);
      return;
    }

    EdgeDataVolume edge;
    edge.i = i;
    edge.j = parent;
    edge.upEdge = IsAscending((superarcsPortal.Get(i)));
    edge.subtreeVolume =
      (totalVolume - hypersweepSumValuesPortal.Get(i)) + (superarcIntrinsicWeightPortal.Get(i) - 1);

    EdgeDataVolume oppositeEdge;
    oppositeEdge.i = parent;
    oppositeEdge.j = i;
    oppositeEdge.upEdge = !edge.upEdge;
    oppositeEdge.subtreeVolume = hypersweepSumValuesPortal.Get(i);

    arcsPortal.Set(i * 2, edge);
    arcsPortal.Set(i * 2 + 1, oppositeEdge);
  }
};


class SetFirstVertexForSuperparent : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _1;

  VTKM_EXEC_CONT SetFirstVertexForSuperparent() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VTKM_EXEC void operator()(
    const vtkm::Id sortedNode,
    const IdWholeArrayInPortalType& nodesPortal,
    const IdWholeArrayInPortalType& superparentsPortal,
    const IdWholeArrayInOutPortalType& firstVertexForSuperparentPortal) const
  {
    vtkm::Id sortID = nodesPortal.Get(sortedNode);
    vtkm::Id superparent = superparentsPortal.Get(sortID);
    if (sortedNode == 0)
    {
      firstVertexForSuperparentPortal.Set(superparent, sortedNode);
    }
    else if (superparent != superparentsPortal.Get(nodesPortal.Get(sortedNode - 1)))
    {
      firstVertexForSuperparentPortal.Set(superparent, sortedNode);
    }
  }
};

class ComputeIntrinsicWeight : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4);
  using InputDomain = _2;

  VTKM_EXEC_CONT ComputeIntrinsicWeight() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VTKM_EXEC void operator()(const vtkm::Id superarc,
                            const IdWholeArrayInPortalType& arcsPortal,
                            const IdWholeArrayInPortalType& superarcsPortal,
                            const IdWholeArrayInPortalType& firstVertexForSuperparentPortal,
                            const IdWholeArrayInOutPortalType& superarcIntrinsicWeightPortal) const
  {
    if (superarc == superarcsPortal.GetNumberOfValues() - 1)
    {
      superarcIntrinsicWeightPortal.Set(
        superarc, arcsPortal.GetNumberOfValues() - firstVertexForSuperparentPortal.Get(superarc));
    }
    else
    {
      superarcIntrinsicWeightPortal.Set(superarc,
                                        firstVertexForSuperparentPortal.Get(superarc + 1) -
                                          firstVertexForSuperparentPortal.Get(superarc));
    }
  }
};


class SetFirstSupernodePerIteration : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2);
  using InputDomain = _1;

  VTKM_EXEC_CONT SetFirstSupernodePerIteration() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VTKM_EXEC void operator()(
    const vtkm::Id supernode,
    const IdWholeArrayInPortalType& whenTransferredPortal,
    const IdWholeArrayInOutPortalType& firstSupernodePerIterationPortal) const
  {
    vtkm::Id when = MaskedIndex(whenTransferredPortal.Get(supernode));
    if (supernode == 0)
    {
      firstSupernodePerIterationPortal.Set(when, supernode);
    }
    else if (when != MaskedIndex(whenTransferredPortal.Get(supernode - 1)))
    {
      firstSupernodePerIterationPortal.Set(when, supernode);
    }
  }
};




template <typename Operator>
class AddDependentWeightHypersweep : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn iterationHypernodes,
                                WholeArrayIn hypernodes,
                                WholeArrayIn hyperarcs,
                                WholeArrayIn howManyUsed,
                                AtomicArrayInOut minMaxIndex);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4, _5);
  using InputDomain = _1;

  Operator op;

  // Default Constructor
  VTKM_EXEC_CONT AddDependentWeightHypersweep(Operator _op)
    : op(_op)
  {
  }

  template <typename IdWholeArrayHandleCountingIn,
            typename IdWholeArrayIn,
            typename IdWholeArrayInOut>
  VTKM_EXEC void operator()(const vtkm::Id hyperarcId,
                            const IdWholeArrayHandleCountingIn& iterationHypernodesPortal,
                            const IdWholeArrayIn& hypernodesPortal,
                            const IdWholeArrayIn& hyperarcsPortal,
                            const IdWholeArrayIn& howManyUsedPortal,
                            const IdWholeArrayInOut& minMaxIndexPortal) const
  {
    Id i = iterationHypernodesPortal.Get(hyperarcId);

    // If it's the last hyperacs (there's nothing to do it's just the root)
    if (i >= hypernodesPortal.GetNumberOfValues() - 1)
    {
      return;
    }

    //
    // The value of the prefix scan is now accumulated in the last supernode of the hyperarc. Transfer is to the target
    //
    vtkm::Id lastSupernode = MaskedIndex(hypernodesPortal.Get(i + 1)) - howManyUsedPortal.Get(i);

    //
    // Transfer the accumulated value to the target supernode
    //
    Id vertex = lastSupernode - 1;
    Id parent = MaskedIndex(hyperarcsPortal.Get(i));

    Id vertexValue = minMaxIndexPortal.Get(vertex);
    Id parentValue = minMaxIndexPortal.Get(parent);

    //Id writeValue = op(vertexValue, parentValue);

    vtkm::Int32 cur = minMaxIndexPortal.Get(parent); // Load the current value at idx
    vtkm::Int32 newVal;                              // will hold the result of the multiplication
    vtkm::Int32 expect; // will hold the expected value before multiplication

    do
    {
      expect = cur;                  // Used to ensure the value hasn't changed since reading
      newVal = op(cur, vertexValue); // the actual multiplication
    } while ((cur = minMaxIndexPortal.CompareAndSwap(parent, newVal, expect)) != expect);

    //minMaxIndexPortal.Set(parent, writeValue);
  }
}; // ComputeMinMaxValues
} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm


#endif
