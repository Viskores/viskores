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

#ifndef vtk_m_worklet_contourtree_augmented_process_contourtree_inc_hypersweep_worklets_h
#define vtk_m_worklet_contourtree_augmented_process_contourtree_inc_hypersweep_worklets_h

#include <vtkm/BinaryOperators.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

/**
 * Incorporates values of the parent of the current subtree in the subtree for the min and max hypersweeps
 */
namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{


class InitialiseArcs : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  vtkm::Id globalMinSortedIndex, globalMaxSortedIndex, rootSupernodeId;

  VTKM_EXEC_CONT InitialiseArcs(vtkm::Id _globalMinSortedIndex,
                                vtkm::Id _globalMaxSortedIndex,
                                vtkm::Id _rootSupernodeId)
    : globalMinSortedIndex(_globalMinSortedIndex)
    , globalMaxSortedIndex(_globalMaxSortedIndex)
    , rootSupernodeId(_rootSupernodeId)
  {
  }

  template <typename IdWholeArrayInPortalType, typename EdgeWholeArrayInOutPortal>
  VTKM_EXEC void operator()(const vtkm::Id currentId,
                            const IdWholeArrayInPortalType& minParentsPortal,
                            const IdWholeArrayInPortalType& maxParentsPortal,
                            const IdWholeArrayInPortalType& minValuesPortal,
                            const IdWholeArrayInPortalType& maxValuesPortal,
                            const IdWholeArrayInPortalType& superarcsPortal,
                            const EdgeWholeArrayInOutPortal& arcsPortal) const
  {
    Id i = currentId;
    Id parent = MaskedIndex(superarcsPortal.Get(i));
    if (parent == 0)
      return;

    EdgeData edge;
    edge.i = i;
    edge.j = parent;
    edge.upEdge = IsAscending((superarcsPortal.Get(i)));

    EdgeData oppositeEdge;
    oppositeEdge.i = parent;
    oppositeEdge.j = i;
    oppositeEdge.upEdge = !edge.upEdge;


    // Is it in the direction of the minRootedTree?
    if (MaskedIndex(minParentsPortal.Get(edge.j)) == edge.i)
    {
      edge.subtreeMin = minValuesPortal.Get(edge.j);
      oppositeEdge.subtreeMin = globalMinSortedIndex;
    }
    else
    {
      oppositeEdge.subtreeMin = minValuesPortal.Get(oppositeEdge.j);
      edge.subtreeMin = globalMinSortedIndex;
    }

    // Is it in the direction of the maxRootedTree?
    if (MaskedIndex(maxParentsPortal.Get(edge.j)) == edge.i)
    {
      edge.subtreeMax = maxValuesPortal.Get(edge.j);
      oppositeEdge.subtreeMax = globalMaxSortedIndex;
    }
    else
    {
      oppositeEdge.subtreeMax = maxValuesPortal.Get(oppositeEdge.j);
      edge.subtreeMax = globalMaxSortedIndex;
    }

    // Compensate for the missing edge where the root is
    if (i > rootSupernodeId)
    {
      i--;
    }

    // We cannot use i here because one of the vertices is skipped (the root one and we don't know where it is)
    arcsPortal.Set(i * 2, edge);
    arcsPortal.Set(i * 2 + 1, oppositeEdge);
  }
}; // ComputeMinMaxValues




class ComputeSubtreeHeight : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4);
  using InputDomain = _4;

  VTKM_EXEC_CONT ComputeSubtreeHeight() {}

  template <typename Float64WholeArrayInPortalType,
            typename IdWholeArrayInPortalType,
            typename EdgeWholeArrayInOutPortal>
  VTKM_EXEC void operator()(const vtkm::Id currentId,
                            const Float64WholeArrayInPortalType& fieldValuesPortal,
                            const IdWholeArrayInPortalType& ctSortOrderPortal,
                            const IdWholeArrayInPortalType& supernodesPortal,
                            const EdgeWholeArrayInOutPortal& arcsPortal) const
  {
    Id i = currentId;
    EdgeData edge = arcsPortal.Get(i);

    Float64 minIsoval = fieldValuesPortal.Get(ctSortOrderPortal.Get(edge.subtreeMin));
    Float64 maxIsoval = fieldValuesPortal.Get(ctSortOrderPortal.Get(edge.subtreeMax));
    Float64 vertexIsoval =
      fieldValuesPortal.Get(ctSortOrderPortal.Get(supernodesPortal.Get(edge.i)));

    // We need to incorporate the value of the vertex into the height of the tree (otherwise leafs edges have 0 persistence)
    minIsoval = vtkm::Minimum()(minIsoval, vertexIsoval);
    maxIsoval = vtkm::Maximum()(maxIsoval, vertexIsoval);

    edge.subtreeHeight = maxIsoval - minIsoval;

    arcsPortal.Set(i, edge);
  }
}; // ComputeMinMaxValues




class SetBestUpDown : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayInOut, WholeArrayInOut, WholeArrayIn);
  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _3;

  VTKM_EXEC_CONT SetBestUpDown() {}

  template <typename IdWholeArrayInPortalType, typename EdgeWholeArrayInOutPortal>
  VTKM_EXEC void operator()(const vtkm::Id currentId,
                            const IdWholeArrayInPortalType& bestUpwardPortal,
                            const IdWholeArrayInPortalType& bestDownwardPortal,
                            const EdgeWholeArrayInOutPortal& arcsPortal) const
  {
    vtkm::Id i = currentId;

    if (i == 0)
    {
      if (arcsPortal.Get(0).upEdge == 0)
      {
        bestDownwardPortal.Set(arcsPortal.Get(0).i, arcsPortal.Get(0).j);
      }
      else
      {
        bestUpwardPortal.Set(arcsPortal.Get(0).i, arcsPortal.Get(0).j);
      }
    }
    else
    {
      if (arcsPortal.Get(i).upEdge == 0 && arcsPortal.Get(i).i != arcsPortal.Get(i - 1).i)
      {
        bestDownwardPortal.Set(arcsPortal.Get(i).i, arcsPortal.Get(i).j);
      }

      if (arcsPortal.Get(i).upEdge == 1 &&
          (arcsPortal.Get(i).i != arcsPortal.Get(i - 1).i || arcsPortal.Get(i - 1).upEdge == 0))
      {
        bestUpwardPortal.Set(arcsPortal.Get(i).i, arcsPortal.Get(i).j);
      }
    }
  }
}; // ComputeMinMaxValues


class UnmaskArray : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1);
  using InputDomain = _1;


  // Default Constructor
  VTKM_EXEC_CONT UnmaskArray() {}

  template <typename IdWholeArrayInPortalType>
  VTKM_EXEC void operator()(const vtkm::Id currentId,
                            const IdWholeArrayInPortalType& maskedArrayPortal) const
  {
    const auto currentValue = maskedArrayPortal.Get(currentId);
    maskedArrayPortal.Set(currentId, MaskedIndex(currentValue));
  }
}; // ComputeMinMaxValues

class PropagateBestUpDown : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayOut);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _3;


  // Default Constructor
  VTKM_EXEC_CONT PropagateBestUpDown() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayOutPortalType>
  VTKM_EXEC void operator()(const vtkm::Id supernodeId,
                            const IdWholeArrayInPortalType& bestUpwardPortal,
                            const IdWholeArrayInPortalType& bestDownwardPortal,
                            const IdWholeArrayOutPortalType& whichBranchPortal) const
  {
    vtkm::Id bestUp = bestUpwardPortal.Get(supernodeId);
    if (NoSuchElement(bestUp))
    {
      // flag it as an upper leaf
      whichBranchPortal.Set(supernodeId, TERMINAL_ELEMENT | supernodeId);
    }
    else if (bestDownwardPortal.Get(bestUp) == supernodeId)
      whichBranchPortal.Set(supernodeId, bestUp);
    else
      whichBranchPortal.Set(supernodeId, TERMINAL_ELEMENT | supernodeId);
  }
}; // ComputeMinMaxValues

class WhichBranchNewId : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2);
  using InputDomain = _2;


  // Default Constructor
  VTKM_EXEC_CONT WhichBranchNewId() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VTKM_EXEC void operator()(const vtkm::Id supernode,
                            const IdWholeArrayInPortalType& chainToBranchPortal,
                            const IdWholeArrayInOutPortalType& whichBranchPortal) const
  {
    const auto currentValue = MaskedIndex(whichBranchPortal.Get(supernode));
    whichBranchPortal.Set(supernode, chainToBranchPortal.Get(currentValue));
  }
}; // ComputeMinMaxValues

class BranchMinMaxSet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayInOut, WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4);
  using InputDomain = _2;

  vtkm::Id nSupernodes;

  // Default Constructor
  VTKM_EXEC_CONT BranchMinMaxSet(vtkm::Id _nSupernodes)
    : nSupernodes(_nSupernodes)
  {
  }

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VTKM_EXEC void operator()(const vtkm::Id supernode,
                            const IdWholeArrayInPortalType& supernodeSorterPortal,
                            const IdWholeArrayInPortalType& whichBranchPortal,
                            const IdWholeArrayInOutPortalType& branchMinimumPortal,
                            const IdWholeArrayInOutPortalType& branchMaximumPortal) const
  {
    // retrieve supernode & branch IDs
    vtkm::Id supernodeID = supernodeSorterPortal.Get(supernode);
    vtkm::Id branchID = whichBranchPortal.Get(supernodeID);
    // save the branch ID as the owner
    // use LHE of segment to set branch minimum
    if (supernode == 0)
    { // sn = 0
      branchMinimumPortal.Set(branchID, supernodeID);
    } // sn = 0
    else if (branchID != whichBranchPortal.Get(supernodeSorterPortal.Get(supernode - 1)))
    { // LHE
      branchMinimumPortal.Set(branchID, supernodeID);
    } // LHE
    // use RHE of segment to set branch maximum
    if (supernode == nSupernodes - 1)
    { // sn = max
      branchMaximumPortal.Set(branchID, supernodeID);
    } // sn = max
    else if (branchID != whichBranchPortal.Get(supernodeSorterPortal.Get(supernode + 1)))
    { // RHE
      branchMaximumPortal.Set(branchID, supernodeID);
    } // RHE
  }
}; // ComputeMinMaxValues

class BranchSaddleParentSet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayInOut,
                                WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _2;

  // Default Constructor
  VTKM_EXEC_CONT BranchSaddleParentSet() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VTKM_EXEC void operator()(const vtkm::Id branchID,
                            const IdWholeArrayInPortalType& whichBranchPortal,
                            const IdWholeArrayInPortalType& branchMinimumPortal,
                            const IdWholeArrayInPortalType& branchMaximumPortal,
                            const IdWholeArrayInPortalType& bestDownwardPortal,
                            const IdWholeArrayInPortalType& bestUpwardPortal,
                            const IdWholeArrayInOutPortalType& branchSaddlePortal,
                            const IdWholeArrayInOutPortalType& branchParentPortal) const
  {
    vtkm::Id branchMax = branchMaximumPortal.Get(branchID);
    // check whether the maximum is NOT a leaf
    if (!NoSuchElement(bestUpwardPortal.Get(branchMax)))
    { // points to a saddle
      branchSaddlePortal.Set(branchID, MaskedIndex(bestUpwardPortal.Get(branchMax)));
      // if not, then the bestUp points to a saddle vertex at which we join the parent
      branchParentPortal.Set(branchID, whichBranchPortal.Get(bestUpwardPortal.Get(branchMax)));
    } // points to a saddle
    // now do the same with the branch minimum
    vtkm::Id branchMin = branchMinimumPortal.Get(branchID);
    // test whether NOT a lower leaf
    if (!NoSuchElement(bestDownwardPortal.Get(branchMin)))
    { // points to a saddle
      branchSaddlePortal.Set(branchID, MaskedIndex(bestDownwardPortal.Get(branchMin)));
      // if not, then the bestUp points to a saddle vertex at which we join the parent
      branchParentPortal.Set(branchID, whichBranchPortal.Get(bestDownwardPortal.Get(branchMin)));
    } // points to a saddle
  }
}; // ComputeMinMaxValues



class PrepareChainToBranch : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2);
  using InputDomain = _1;


  // Default Constructor
  VTKM_EXEC_CONT PrepareChainToBranch() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VTKM_EXEC void operator()(const vtkm::Id supernode,
                            const IdWholeArrayInPortalType& whichBranchPortal,
                            const IdWholeArrayInOutPortalType& chainToBranchPortal) const
  {
    // test whether the supernode points to itself to find the top ends
    if (MaskedIndex(whichBranchPortal.Get(supernode)) == supernode)
    {
      chainToBranchPortal.Set(supernode, 1);
    }
  }
}; // ComputeMinMaxValues


class FinaliseChainToBranch : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2);
  using InputDomain = _1;

  VTKM_EXEC_CONT FinaliseChainToBranch() {}

  template <typename IdWholeArrayInPortalType, typename IdWholeArrayInOutPortalType>
  VTKM_EXEC void operator()(const vtkm::Id supernode,
                            const IdWholeArrayInPortalType& whichBranchPortal,
                            const IdWholeArrayInOutPortalType& chainToBranchPortal) const
  {
    // test whether the supernode points to itself to find the top ends
    if (MaskedIndex(whichBranchPortal.Get(supernode)) == supernode)
    {
      const auto value = chainToBranchPortal.Get(supernode);
      chainToBranchPortal.Set(supernode, value - 1);
    }
    else
    {
      chainToBranchPortal.Set(supernode, NO_SUCH_ELEMENT);
    }
  }
}; // ComputeMinMaxValues




template <typename Operator>
class IncorporateParent : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn, WholeArrayIn, WholeArrayInOut);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _1;

  Operator op;

  // Default Constructor
  VTKM_EXEC_CONT IncorporateParent(Operator _op)
    : op(_op)
  {
  }

  template <typename IdWholeArrayIn, typename IdWholeArrayInOut>
  VTKM_EXEC void operator()(const vtkm::Id superarcId,
                            const IdWholeArrayIn& parentsPortal,
                            const IdWholeArrayIn& supernodesPortal,
                            const IdWholeArrayInOut& hypersweepValuesPortal) const
  {
    Id i = superarcId;

    Id parent = MaskedIndex(parentsPortal.Get(i));

    Id subtreeValue = hypersweepValuesPortal.Get(i);
    Id parentValue = MaskedIndex(supernodesPortal.Get(parent));

    hypersweepValuesPortal.Set(i, op(subtreeValue, parentValue));
  }
}; // ComputeMinMaxValues




} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm


#endif
