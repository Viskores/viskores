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
// RegularToCriticalDown.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_regular_to_critical_down_h
#define vtkm_worklet_contourtree_regular_to_critical_down_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for doing regular to critical
class RegularToCriticalDown : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertexID,               // (input) candidate index
                                WholeArrayIn<IdType> mergeArcs,         // (input) merge arcs
                                WholeArrayIn<IdType> regularToCritical, // (input)
                                FieldOut<IdType> sortVector);           // (output) 
  typedef _4 ExecutionSignature(_1, _2, _3);
  typedef _1 InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  RegularToCriticalDown() {}

  template <typename InFieldPortalType>
  VTKM_EXEC
  vtkm::Id operator()(const vtkm::Id& vertexID,
                      const InFieldPortalType& mergeArcs,
                      const InFieldPortalType& regularToCritical) const
  {
    vtkm::Id sortVector;

    // copy the mergeArc ID
    vtkm::Id splitNeighbour = mergeArcs.Get(vertexID);
    // if it's the root vertex
    if (splitNeighbour == NO_VERTEX_ASSIGNED)
      // set it to the sentinel value
      sortVector = NO_VERTEX_ASSIGNED;
    else
      // otherwise convert to a candidate ID & save
      sortVector = regularToCritical.Get(splitNeighbour);
    return sortVector;
  }
}; // RegularToCriticalDown

}
}
}

#endif
