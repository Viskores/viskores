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
// DegreeSubrangeOffset.h - functor that performs conditional chain-doubling
//
//=======================================================================================
//
// COMMENTS:
//
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_degree_subrange_offset_h
#define vtkm_worklet_contourtree_degree_subrange_offset_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet 
class DegreeSubrangeOffset : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> sortID,             // (input) index into sort vector
                                WholeArrayIn<IdType> sortVector,    // (input)
                                WholeArrayOut<IdType> degree);      // (output)
  typedef void ExecutionSignature(_1, _2, _3);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  DegreeSubrangeOffset() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& sortID,
                  const InFieldPortalType& sortVector,
                  const OutFieldPortalType& degree) const
  {
    vtkm::Id superID = sortVector.Get(sortID);
    vtkm::Id prevSuper = sortVector.Get(sortID-1);

    // if they don't match, we've found a boundary
    if (superID != prevSuper)
      degree.Set(superID, sortID);
  }
}; // DegreeSubrangeOffset

}
}
}

#endif
