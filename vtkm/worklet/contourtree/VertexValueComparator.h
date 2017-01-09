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
// VertexValueComparator.h - a comparator for sorting the original array
//
//=======================================================================================
//
// COMMENTS:
//
// A comparator that sorts vertices by data value, falling back on index to implement
// simulation of simplicity
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_vertex_value_comparator_h
#define vtkm_worklet_contourtree_vertex_value_comparator_h

namespace vtkm {
namespace worklet {
namespace contourtree {

template <typename InFieldPortalType>
class VertexValueComparator
{
public:
  const InFieldPortalType& values;

  VTKM_CONT
  VertexValueComparator(const InFieldPortalType& Values) : values(Values) {}

  VTKM_EXEC_CONT
  bool operator () (const vtkm::Id &i, const vtkm::Id &j, bool ascending)
  {
    if (values.Get(i) < values.Get(j))
      return ascending ^ true;
    else if (values.Get(j) < values.Get(i))
      return ascending ^ false;
    else if (i < j)
      return ascending ^ true;
    else if (j < i)
      return ascending ^ false;
    // fall through to return false
      return false;
    }
};

}
}
}

#endif
