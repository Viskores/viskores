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
// RegularPointTransferrer.h - iterator for transferring regular points
//
//=======================================================================================
//
// COMMENTS:
//
// This functor replaces a parallel loop through regular points, since more than one
// output needs to be set.
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_regular_point_transferrer_h
#define vtkm_worklet_contourtree_regular_point_transferrer_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include "vtkm/worklet/contourtree/Mesh2D_DEM_Triangulation_Macros.h"
#include "vtkm/worklet/contourtree/VertexValueComparator.h"
#include "vtkm/worklet/contourtree/Types.h"

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
template<typename T>
class RegularPointTransferrer : public vtkm::worklet::WorkletMapField
{
public:
  struct TagType : vtkm::ListTagBase<T> {};

  typedef void ControlSignature(FieldIn<IdType> vertexID,             // (input) vertex ID
                                WholeArrayIn<IdType> chainExtremum,   // (input) chain extremum
                                WholeArrayIn<TagType> values,         // (input) values array
                                WholeArrayIn<IdType> valueIndex,      // (input) index into value array
                                WholeArrayInOut<IdType> prunesTo,     // (i/o) where vertex is pruned to
                                WholeArrayOut<IdType> outdegree);     // (output) updegree of vertex
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  typedef _1   InputDomain;

  bool isJoinGraph;

  // Constructor
  VTKM_EXEC_CONT
  RegularPointTransferrer(bool IsJoinGraph) : isJoinGraph(IsJoinGraph) {}

  template <typename InFieldPortalType, typename InIndexPortalType, typename InOutFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& vertexID,
                  const InIndexPortalType& chainExtremum,
                  const InFieldPortalType &values,
                  const InIndexPortalType &valueIndex,
                  const InOutFieldPortalType &prunesTo,
                  const OutFieldPortalType &outdegree) const
  {
    VertexValueComparator<InFieldPortalType> lessThan(values);

    // ignore vertices which have already been labelled
    vtkm::Id saddleID = prunesTo.Get(vertexID);

    if (saddleID != NO_VERTEX_ASSIGNED) 
      return;

    // now, if the vertex is beyond the governing saddle, we need to label it
    // and arrange to get rid of it
    saddleID = prunesTo.Get(chainExtremum.Get(vertexID));

    if (lessThan(valueIndex.Get(saddleID), valueIndex.Get(vertexID), !isJoinGraph))
    {
      // set the merge extremum to the current chain extremum
      prunesTo.Set(vertexID, chainExtremum.Get(vertexID));
      // and reset the outdegree to zero
      outdegree.Set(vertexID, 0);
    } // regular point to be pruned

  }
}; // RegularPointTransferrer

}
}
}

#endif
