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
// Mesh2D_DEM_VertexOutdegreeStarter.h - computes how many unique starts per vertex
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

#ifndef vtkm_worklet_contourtree_mesh2d_dem_vertex_outdegree_starter_h
#define vtkm_worklet_contourtree_mesh2d_dem_vertex_outdegree_starter_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Mesh2D_DEM_Triangulation_Macros.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
class Mesh2D_DEM_VertexOutdegreeStarter : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertex,           // (input) index into active vertices
                                FieldIn<IdType> nbrMask,          // (input) neighbor mask
                                WholeArrayIn<IdType> arcArray,    // (input) chain extrema
                                FieldOut<IdType> outdegree,       // (output) outdegree
                                FieldOut<IdType> isCritical);     // (output) whether critical
  typedef void ExecutionSignature(_1, _2, _3, _4, _5/*, _6*/);
  typedef _1   InputDomain;

  vtkm::Id nRows;                                  // (input) number of rows in 2D
  vtkm::Id nCols;                                  // (input) number of cols in 2D
  bool ascending;                                  // (input) ascending or descending (join or split tree)

  // Constructor
  VTKM_EXEC_CONT
  Mesh2D_DEM_VertexOutdegreeStarter(vtkm::Id NRows,
                                    vtkm::Id NCols,
                                    bool Ascending) : nRows(NRows),
                                                      nCols(NCols),
                                                      ascending(Ascending) {}

  //template<typename InFieldPortalType>
  template<typename InFieldPortalType/*, typename InOutFieldPortalType*/>
  VTKM_EXEC
  void operator()(const vtkm::Id& vertex,
                  const vtkm::Id& nbrMask,
                  const InFieldPortalType& arcArray,
                        vtkm::Id& outdegree,
                        vtkm::Id& isCritical) const
  {
    // get the row and column
    vtkm::Id row = VERTEX_ROW(vertex, nCols);
    vtkm::Id col = VERTEX_COL(vertex, nCols);

    // we know which edges are outbound, so we count to get the outdegree
    vtkm::Id outDegree = 0;

    vtkm::Id farEnds[MAX_OUTDEGREE];

    // special case for local extremum
    if (nbrMask == 0x3F) {
      outDegree = 1;
    }
    else { // not a local minimum
      if ((nbrMask & 0x30) == 0x20)
        farEnds[outDegree++] = arcArray.Get(VERTEX_ID(row-1, col,   nCols));
      if ((nbrMask & 0x18) == 0x10)
        farEnds[outDegree++] = arcArray.Get(VERTEX_ID(row-1, col-1, nCols));
      if ((nbrMask & 0x0C) == 0x08)
        farEnds[outDegree++] = arcArray.Get(VERTEX_ID(row,   col-1, nCols));
      if ((nbrMask & 0x06) == 0x04)
        farEnds[outDegree++] = arcArray.Get(VERTEX_ID(row+1, col,   nCols));
      if ((nbrMask & 0x03) == 0x02) 
        farEnds[outDegree++] = arcArray.Get(VERTEX_ID(row+1, col+1, nCols));
      if ((nbrMask & 0x21) == 0x01) 
        farEnds[outDegree++] = arcArray.Get(VERTEX_ID(row,   col+1, nCols));
    } // not a local minimum

    // now we check them against each other
    if ((outDegree == 2) && (farEnds[0] == farEnds[1]))
    { // outDegree 2 & both match
      // treat as a regular point
      outDegree = 1;
    } // outDegree 2 & both match
    else if (outDegree == 3)
    { // outDegree 3
      if (farEnds[0] == farEnds[1])
      { // first two match
        if (farEnds[0] == farEnds[2])
        { // triple match
          // all match - treat as regular point
          outDegree = 1;
        } // triple match
        else
        { // first two match, but not third
          // copy third down one place
          farEnds[1] = farEnds[2];
          // and reset the count
          outDegree = 2;
        } //
      } // first two match
      else if ((farEnds[0] == farEnds[2]) || (farEnds[1] == farEnds[2]))
      { // second one matches either of the first two
        // decrease the count, keeping 0 & 1
        outDegree = 2;
      } // second one matches either of the first two
    } // outDegree 3

    // now store the outDegree
    outdegree = outDegree;

    // and set the initial inverse index to a flag
    isCritical = (outDegree != 1) ? 1 : 0;
  }
}; // Mesh2D_DEM_VertexStarter

} // namespace contourtree
} // namespace worklet
} // namespace vtkm

#endif
