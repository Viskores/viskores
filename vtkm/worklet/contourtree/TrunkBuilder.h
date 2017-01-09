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
// TrunkBuilder.h - functor that sets remaining active vertices to the trunk
//
//=======================================================================================
//
// COMMENTS:
//
// This functor is applied to all remaining active vertices. The remaining maximum is the
// chain maximum of all vertices, and is set to prune to NO_VERTEX_ASSIGNED (the global
// root). All others are set to prune to it.
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_trunk_builder_h
#define vtkm_worklet_contourtree_trunk_builder_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
class TrunkBuilder : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertexID,           // (input) index into active vertices
                                WholeArrayIn<IdType> chainExtremum, // (input) chain extemum for vertices
                                WholeArrayOut<IdType> prunesTo);    // (output) where a vertex prunes to
  typedef void ExecutionSignature(_1, _2, _3);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  TrunkBuilder() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& vertexID,
                  const InFieldPortalType& chainExtremum,
                  const OutFieldPortalType& prunesTo) const
  {
    // the chain max of everyone prunes to the global minimum
    vtkm::Id chainExt = chainExtremum.Get(vertexID);
    if (vertexID == chainExt)
      prunesTo.Set(vertexID, NO_VERTEX_ASSIGNED);
    else
      prunesTo.Set(vertexID, chainExt);		
  }
}; // TrunkBuilder

}
}
}

#endif
