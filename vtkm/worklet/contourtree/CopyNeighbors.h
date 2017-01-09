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
// CopyNeighbors.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_copy_neighbors_h
#define vtkm_worklet_contourtree_copy_neighbors_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet
class CopyNeighbors : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> activeSupernode,       // (input) index into supernodes
                                WholeArrayIn<IdType> activeSupernodes, // (input) active supernode vertex IDs
                                WholeArrayIn<IdType> arcs,             // (input) merge tree arcs
                                FieldOut<IdType> sortVector);          // (output) neighbors for active edge
  typedef _4   ExecutionSignature(_1, _2, _3);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  CopyNeighbors() {}

  template <typename InFieldPortalType>
  VTKM_EXEC
  vtkm::Id operator()(const vtkm::Id& activeSupernode,
                      const InFieldPortalType& activeSupernodes,
                      const InFieldPortalType& arcs) const
  {
    vtkm::Id sortVector;
    vtkm::Id superID = activeSupernodes.Get(activeSupernode);
    vtkm::Id neighbour = arcs.Get(superID);
    sortVector = neighbour;
    return sortVector;
  }
}; // CopyNeighbors

}
}
}

#endif
