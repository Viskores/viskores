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
// FindLeaves.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_find_leaves_h
#define vtkm_worklet_contourtree_find_leaves_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for doing regular to candidate
class FindLeaves : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> superID,                  // (input) super nodes
                                WholeArrayIn<IdType> updegree,            // (input)
                                WholeArrayIn<IdType> downdegree,          // (input)
                                WholeArrayIn<IdType> joinArc,             // (input)
                                WholeArrayIn<IdType> splitArc,            // (input)
                                WholeArrayInOut<IdType> superarc);        // (i/o)
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  FindLeaves() {}

  template <typename InPortalFieldType, typename OutPortalFieldType>
  VTKM_EXEC
  void operator()(const vtkm::Id& superID,
                  const InPortalFieldType& updegree,
                  const InPortalFieldType& downdegree,
                  const InPortalFieldType& joinArc,
                  const InPortalFieldType& splitArc,
                  const OutPortalFieldType& superarc) const
  {
    // omit previously processed vertices
    if (superarc.Get(superID) != NO_VERTEX_ASSIGNED)
      return;
    // Test for extremality - maxima first
    if ((updegree.Get(superID) == 0) && (downdegree.Get(superID) == 1))
    { // maximum
      superarc.Set(superID, joinArc.Get(superID));
    } // maximum
    // minima next
    else if ((updegree.Get(superID) == 1) && (downdegree.Get(superID) == 0))
    { // minimum
      superarc.Set(superID, splitArc.Get(superID));
    } // minimum
  }
}; // FindLeaves

}
}
}

#endif
