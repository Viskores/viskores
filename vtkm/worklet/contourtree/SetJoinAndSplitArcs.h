//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

//=======================================================================================
// 
// Second Attempt to Compute Contour Tree in Data-Parallel Mode
//
// Started August 19, 2015
//
// Copyright Hamish Carr, University of Leeds
//
// SetJoinAndSplitArcs.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_set_join_and_split_arcs_h
#define vtkm_worklet_contourtree_set_join_and_split_arcs_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for doing regular to candidate
class SetJoinAndSplitArcs : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> regularID,              // (input)
                                WholeArrayIn<IdType> joinMergeArcs,     // (input)
                                WholeArrayIn<IdType> splitMergeArcs,    // (input)
                                WholeArrayIn<IdType> regularToCritical, // (input)
                                FieldOut<IdType> joinArc,               // (output)
                                FieldOut<IdType> splitArc);             // (output)
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  SetJoinAndSplitArcs() {}

  template <typename InFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& regularID,
                  const InFieldPortalType& joinMergeArcs,
                  const InFieldPortalType& splitMergeArcs,
                  const InFieldPortalType& regularToCritical,
                        vtkm::Id &joinArc,
                        vtkm::Id &splitArc) const
  {
    // use it to grab join arc target
    vtkm::Id joinTo = joinMergeArcs.Get(regularID);
    // and set the join arc
    if (joinTo == NO_VERTEX_ASSIGNED)
      joinArc = NO_VERTEX_ASSIGNED;
    else
      joinArc = regularToCritical.Get(joinTo);
    // now grab split arc target
    vtkm::Id splitTo = splitMergeArcs.Get(regularID);
    // and set the split arc
    if (splitTo == NO_VERTEX_ASSIGNED)
      splitArc = NO_VERTEX_ASSIGNED;
    else
      splitArc = regularToCritical.Get(splitTo);
  }
}; // SetJoinAndSplitArcs

}
}
}

#endif
