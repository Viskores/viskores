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
// Mesh2D_DEM_VertexAscender.h - a functor that computes a link mask & a regular ascent
//
//=======================================================================================
//
// COMMENTS:
//
// This functor replaces a parallel loop examining neighbours - again, for arbitrary
// meshes, it needs to be a reduction, but for regular meshes, it's faster this way.
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_mesh2d_dem_vertex_starter_h
#define vtkm_worklet_contourtree_mesh2d_dem_vertex_starter_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Mesh2D_DEM_Triangulation_Macros.h>
#include <vtkm/worklet/contourtree/VertexValueComparator.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
template<typename T>
class Mesh2D_DEM_VertexStarter : public vtkm::worklet::WorkletMapField
{
public:
  struct TagType : vtkm::ListTagBase<T> {};

  typedef void ControlSignature(FieldIn<IdType> vertex,          // (input) index of vertex
                                WholeArrayIn<TagType> values,    // (input) values within mesh
                                FieldOut<IdType> chain,          // (output) modify the chains
                                FieldOut<IdType> linkMask);      // (output) modify the mask
  typedef void ExecutionSignature(_1, _2, _3, _4);
  typedef _1   InputDomain;

  vtkm::Id nRows;     // (input) number of rows in 2D
  vtkm::Id nCols;     // (input) number of cols in 2D
  bool ascending;     // ascending or descending (join or split tree)

  // Constructor
  VTKM_EXEC_CONT
  Mesh2D_DEM_VertexStarter(vtkm::Id NRows,
                           vtkm::Id NCols,
                           bool Ascending) : nRows(NRows),
                                             nCols(NCols),
                                             ascending(Ascending) {}

  // Locate the next vertex in direction indicated
  template <typename InFieldPortalType>
  VTKM_EXEC
  void operator()(const vtkm::Id& vertex,
                  const InFieldPortalType& values,
                        vtkm::Id& chain,
                        vtkm::Id& linkMask) const
  {
    VertexValueComparator<InFieldPortalType> lessThan(values);
    vtkm::Id row = VERTEX_ROW(vertex, nCols);
    vtkm::Id col = VERTEX_COL(vertex, nCols);

    vtkm::Id destination = vertex;
    vtkm::Id mask = 0;

    bool isLeft = (col == 0);
    bool isRight = (col == nCols - 1);
    bool isTop = (row == 0);
    bool isBottom = (row  == nRows - 1);

    for (vtkm::Id edgeNo = 0; edgeNo < N_INCIDENT_EDGES; edgeNo++)
    { // per edge
      vtkm::Id nbr;

      switch (edgeNo)
      { 
      case 5:         // up   
        if (isTop)
          break;
        nbr = vertex - nCols;
        if (lessThan(vertex, nbr, ascending))
          break;
        mask |= 0x20;
        destination = nbr;
        break;

      case 4:         // up left      
        if (isLeft || isTop)
          break;
        nbr = vertex - nCols - 1;
        if (lessThan(vertex, nbr, ascending))
          break;
        mask |= 0x10;
        destination = nbr;
        break;

      case 3:         // left
        if (isLeft)
          break;
        nbr = vertex - 1;
        if (lessThan(vertex, nbr, ascending))
          break;
        mask |= 0x08;
        destination = nbr;
        break;

      case 2:         // down
        if (isBottom)
          break;
        nbr = vertex + nCols;
        if (lessThan(vertex, nbr, ascending))
          break;
        mask |= 0x04;
        destination = nbr;
        break;

      case 1:         // down right
        if (isBottom || isRight)
          break;
        nbr = vertex + nCols + 1;
        if (lessThan(vertex, nbr, ascending))
          break;
        mask |= 0x02;
        destination = nbr;
        break;

      case 0:         // right
        if (isRight)
          break;
        nbr = vertex + 1;
        if (lessThan(vertex, nbr, ascending))
          break;
        mask |= 0x01;
        destination = nbr;
        break;
      } // switch on edgeNo
    } // per edge

    linkMask = mask;
    chain = destination;
  } // operator()
}; // Mesh2D_DEM_VertexStarter

}
}
}

#endif
