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
// JoinSuperArcFinder.h - functor that searches through the superarcs to assign each vertex
//
//=======================================================================================
//
// COMMENTS:
//
// After the core join tree is constructed, we need to assign each vertex to a join superarc
// This was previously done with a set of rocking iterations, which burned extra memory and
// work.  The OpenMP version was therefore updated so that each vertex looped until it found
// it's destination arc. 
//
// This functor implements that for use by a for_each call.
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_join_super_arc_finder_h
#define vtkm_worklet_contourtree_join_super_arc_finder_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include "vtkm/worklet/contourtree/Types.h"
#include "vtkm/worklet/contourtree/VertexValueComparator.h"

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for finding join superarc - expressed as a unary functor since it is not
// guaranteed to write back
// There will be no out-of-sequence writes, since:
// 1.  Critical points are already set and are simply skipped
// 2.  Regular points only read from critical points
// 3.  Regular points only write to critical points
//
template <typename T>
class JoinSuperArcFinder : public vtkm::worklet::WorkletMapField
{
public:
  struct TagType : vtkm::ListTagBase<T> {};

  typedef void ControlSignature(FieldIn<IdType> vertex,           // (input) index into sorted edges
                                WholeArrayIn<TagType> values,     // (input) data values
                                WholeArrayInOut<IdType> saddles,  // (in out) saddles
                                WholeArrayInOut<IdType> extrema); // (in out) maxima
  typedef void ExecutionSignature(_1, _2, _3, _4);
  typedef _1   InputDomain;

  bool isJoinTree;

  // Constructor
  VTKM_EXEC_CONT
  JoinSuperArcFinder(bool IsJoinTree) :	isJoinTree(IsJoinTree) {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& vertex,
                  const InFieldPortalType& values,
                  const OutFieldPortalType& saddles,
                  const OutFieldPortalType& extrema) const
  {
    VertexValueComparator<InFieldPortalType> lessThan(values);

    // local copies
    vtkm::Id saddle = saddles.Get(vertex);
    vtkm::Id extreme = extrema.Get(vertex);

    // now test for regular points
    if (saddle != extreme)
      return;
                
    // while loop   
    while (lessThan(saddle, vertex, isJoinTree))
    { //  saddle above vertex
      extreme = extrema.Get(saddle);
      saddle = saddles.Get(saddle);
      // if we're in the trunk, break immediately
      if (saddle == NO_VERTEX_ASSIGNED)
        break;
      } // saddle above vertex
                
      // when done, write back to the array
      extrema.Set(vertex, extreme);
      saddles.Set(vertex, saddle);
  }
}; // JoinSuperArcFinder

}
}
}

#endif
