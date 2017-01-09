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
// Mesh3D_DEM_VertexAscender.h - a functor that computes a link mask & a regular ascent
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

#ifndef vtkm_worklet_contourtree_mesh3d_dem_vertex_starter_h
#define vtkm_worklet_contourtree_mesh3d_dem_vertex_starter_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Mesh3D_DEM_Triangulation_Macros.h>
#include <vtkm/worklet/contourtree/VertexValueComparator.h>
#include <iostream>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
template<typename T>
class Mesh3D_DEM_VertexStarter : public vtkm::worklet::WorkletMapField
{
public:
  struct TagType : vtkm::ListTagBase<T> {};

  typedef void ControlSignature(FieldIn<IdType> vertex,          // (input) index of vertex
                                WholeArrayIn<TagType> values,    // (input) values within mesh
                                FieldOut<IdType> chain,          // (output) modify the chains
                                FieldOut<IdType> linkMask);      // (output) modify the mask
  typedef void ExecutionSignature(_1, _2, _3, _4);
  typedef _1   InputDomain;

  vtkm::Id nRows;     // (input) number of rows in 3D
  vtkm::Id nCols;     // (input) number of cols in 3D
  vtkm::Id nSlices;   // (input) number of cols in 3D
  bool ascending;     // ascending or descending (join or split tree)

  // Constructor
  VTKM_EXEC_CONT
  Mesh3D_DEM_VertexStarter(vtkm::Id NRows,
                           vtkm::Id NCols,
                           vtkm::Id NSlices,
                           bool Ascending) : nRows(NRows),
                                             nCols(NCols),
                                             nSlices(NSlices),
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
    vtkm::Id row = VERTEX_ROW_3D(vertex, nRows, nCols);
    vtkm::Id col = VERTEX_COL_3D(vertex, nRows, nCols);
    vtkm::Id slice = VERTEX_SLICE_3D(vertex, nRows, nCols);

    vtkm::Id destination = vertex;
    vtkm::Id mask = 0;

    bool isLeft = (col == 0);
    bool isRight = (col == nCols - 1);
    bool isTop = (row == 0);
    bool isBottom = (row  == nRows - 1);
    bool isFront = (slice == 0);
    bool isBack = (slice  == nSlices - 1);

    // This order of processing must be maintained to match the LinkComponentCaseTables
    // and to return the correct destination extremum
    for (vtkm::Id edgeNo = (N_INCIDENT_EDGES_3D - 1); edgeNo >= 0; edgeNo--)
    {
      vtkm::Id nbr;

      switch (edgeNo)
      { 
      ////////////////////////////////////////////////////////
      case 13:        // down right back   
        if (isBack || isRight || isBottom) break;
        nbr = vertex + (nRows * nCols) + nCols + 1;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x2000;
        destination = nbr;
        break;

      case 12:        // down       back   
        if (isBack || isBottom) break;
        nbr = vertex + (nRows * nCols) + nCols;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x1000;
        destination = nbr;
        break;

      case 11:        //      right back   
        if (isBack || isRight) break;
        nbr = vertex + (nRows * nCols) + 1;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x800;
        destination = nbr;
        break;

      case 10:        //            back   
        if (isBack) break;
        nbr = vertex + (nRows * nCols);
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x400;
        destination = nbr;
        break;

      case  9:        // down right   
        if (isBottom || isRight) break;
        nbr = vertex + nCols + 1;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x200;
        destination = nbr;
        break;

      case  8:        // down   
        if (isBottom) break;
        nbr = vertex + nCols;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x100;
        destination = nbr;
        break;

      case  7:        //      right   
        if (isRight) break;
        nbr = vertex + 1;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x80;
        destination = nbr;
        break;

      case  6:        // up left   
        if (isLeft || isTop) break;
        nbr = vertex - nCols - 1;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x40;
        destination = nbr;
        break;

      case  5:        //    left   
        if (isLeft) break;
        nbr = vertex - 1;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x20;
        destination = nbr;
        break;

      case  4:        //    left front   
        if (isLeft || isFront) break;
        nbr = vertex - (nRows * nCols) - 1;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x10;
        destination = nbr;
        break;

      case  3:        //         front   
        if (isFront) break;
        nbr = vertex - (nRows * nCols);
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x08;
        destination = nbr;
        break;

      case  2:        // up      front   
        if (isTop || isFront) break;
        nbr = vertex - (nRows * nCols) - nCols;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x04;
        destination = nbr;
        break;

      case  1:        // up   
        if (isTop) break;
        nbr = vertex - nCols;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x02;
        destination = nbr;
        break;

      case  0:        // up left front   
        if (isTop || isLeft || isFront) break;
        nbr = vertex - (nRows * nCols) - nCols - 1;
        if (lessThan(vertex, nbr, ascending)) break;
        mask |= 0x01;
        destination = nbr;
        break;
      } // switch on edgeNo
    } // per edge

    linkMask = mask;
    chain = destination;
  } // operator()
}; // Mesh3D_DEM_VertexStarter

}
}
}

#endif
