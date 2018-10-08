//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef vtkm_worklet_contourtree_augmented_mesh_dem_triangulation_2D_freudenthal_execution_obect_mesh_structure_h
#define vtkm_worklet_contourtree_augmented_mesh_dem_triangulation_2D_freudenthal_execution_obect_mesh_structure_h

#include <vtkm/Pair.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem/ExecutionObject_MeshStructure_2D.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/freudenthal_2D/Types.h>



//Define namespace alias for the freudenthal types to make the code a bit more readable
namespace mesh_dem_inc_ns = vtkm::worklet::contourtree_augmented::mesh_dem_inc;
namespace cpp2_ns = vtkm::worklet::contourtree_augmented;

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem_2d_freudenthal_inc
{

// Worklet for computing the sort indices from the sort order
template <typename DeviceAdapter>
class ExecutionObject_MeshStructure
  : public mesh_dem_inc_ns::ExecutionObject_MeshStructure_2D<DeviceAdapter>
{
public:
  typedef typename cpp2_ns::IdArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst
    sortIndicesPortalType;
  typedef
    typename edgeBoundaryDetectionMasksType::template ExecutionTypes<DeviceAdapter>::PortalConst
      edgeBoundaryDetectionMasksPortalType;

  // Default constucture. Needed for the CUDA built to work
  VTKM_EXEC_CONT
  ExecutionObject_MeshStructure()
    : mesh_dem_inc_ns::ExecutionObject_MeshStructure_2D<DeviceAdapter>()
    , getMax(false)
    , nIncidentEdges(N_INCIDENT_EDGES)
  {
  }

  // Main constructor used in the code
  VTKM_EXEC_CONT
  ExecutionObject_MeshStructure(vtkm::Id nrows,
                                vtkm::Id ncols,
                                vtkm::Int32 nincident_edges,
                                bool getmax,
                                const cpp2_ns::IdArrayType& sortIndices,
                                const edgeBoundaryDetectionMasksType& edgeBoundaryDetectionMasksIn)
    : mesh_dem_inc_ns::ExecutionObject_MeshStructure_2D<DeviceAdapter>(nrows, ncols)
    , getMax(getmax)
    , nIncidentEdges(nincident_edges)
  {
    sortIndicesPortal = sortIndices.PrepareForInput(DeviceAdapter());
    edgeBoundaryDetectionMasksPortal =
      edgeBoundaryDetectionMasksIn.PrepareForInput(DeviceAdapter());
  }

  VTKM_EXEC
  constexpr vtkm::Id GetMaxNumberOfNeighbours() const { return N_INCIDENT_EDGES; }

  VTKM_EXEC
  inline vtkm::Id GetNeighbourIndex(vtkm::Id vertex, vtkm::Id edgeNo) const
  { // GetNeighbourIndex
    switch (edgeNo)
    {
      case 0:
        return vertex + 1;
      //break; // row    , col + 1
      case 1:
        return vertex + this->nCols + 1;
      //break; // row + 1, col + 1
      case 2:
        return vertex + this->nCols;
      //break; // row + 1, col
      case 3:
        return vertex - 1;
      //break; // row    , col - 1
      case 4:
        return vertex - this->nCols - 1;
      //break; // row - 1, col - 1
      case 5:
        return vertex - this->nCols;
      //break; // row - 1, col
      default:
        std::
          abort(); // TODO How to generate a meaningful error message from a device (in particular when using CUDA?)
        //default: std::cerr << "Internal error (invalid neighbour requested)"; std::abort();
    }
  } // GetNeighbourIndex


// Disable conversion warnings for Add, Subtract, Multiply, Divide on GCC only.
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang

  // sets outgoing paths for saddles
  VTKM_EXEC
  inline vtkm::Id GetExtremalNeighbour(vtkm::Id vertex) const
  { // operator()
    // convert to a sort index
    vtkm::Id sortIndex = sortIndicesPortal.Get(vertex);

    // get the row and column
    vtkm::Id row = this->vertexRow(vertex);
    vtkm::Id col = this->vertexColumn(vertex);
    vtkm::Int8 boundaryConfig = ((col == 0) ? leftBit : 0) |
      ((col == this->nCols - 1) ? rightBit : 0) | ((row == 0) ? topBit : 0) |
      ((row == this->nRows - 1) ? bottomBit : 0);

    // in what follows, the boundary conditions always reset wasAscent
    for (vtkm::Id edgeNo = 0; edgeNo < this->nIncidentEdges; edgeNo++)
    { // per edge
      // ignore if at edge of data
      if (!(boundaryConfig & edgeBoundaryDetectionMasksPortal.Get(edgeNo)))
      {
        // calculate neighbour's ID and sort order
        vtkm::Id nbr = GetNeighbourIndex(vertex, edgeNo);

        // retrieve the sort index
        vtkm::Id nbrIndex = sortIndicesPortal.Get(nbr);

        // if it's not a valid destination, ignore it
        if (getMax ? (nbrIndex > sortIndex) : (nbrIndex < sortIndex))
        {
          // and save the destination
          return nbrIndex;
        }
      }
    } // per edge

    return sortIndex | TERMINAL_ELEMENT;
  } // operator()


  // NOTE/FIXME: The following also iterates over all values and could be combined with GetExtremalNeighbour(). However, the
  // results are needed at different places and splitting the two functions leads to a cleaner design
  VTKM_EXEC
  inline vtkm::Pair<vtkm::Id, vtkm::Id> GetNeighbourComponentsMaskAndDegree(
    vtkm::Id vertex,
    bool getMaxComponents) const
  { // GetNeighbourComponentsMaskAndDegree()
    // get data portals
    // convert to a sort index
    vtkm::Id sortIndex = sortIndicesPortal.Get(vertex);

    // get the row and column
    vtkm::Id row = this->vertexRow(vertex);
    vtkm::Id col = this->vertexColumn(vertex);
    vtkm::Int8 boundaryConfig = ((col == 0) ? leftBit : 0) |
      ((col == this->nCols - 1) ? rightBit : 0) | ((row == 0) ? topBit : 0) |
      ((row == this->nRows - 1) ? bottomBit : 0);

    // and initialise the mask
    vtkm::Id neighbourhoodMask = 0;
    // in what follows, the boundary conditions always reset wasAscent
    for (vtkm::Id edgeNo = 0; edgeNo < N_INCIDENT_EDGES; edgeNo++)
    { // per edge
      // ignore if at edge of data
      if (!(boundaryConfig & edgeBoundaryDetectionMasksPortal.Get(edgeNo)))
      {
        // calculate neighbour's ID and sort order
        vtkm::Id nbr = GetNeighbourIndex(vertex, edgeNo);

        // retrieve the sort index
        vtkm::Id nbrIndex = sortIndicesPortal.Get(nbr);

        // if it's not a valid destination, ignore it
        if (getMaxComponents ? (nbrIndex > sortIndex) : (nbrIndex < sortIndex))
        {
          // now set the flag in the neighbourhoodMask
          neighbourhoodMask |= static_cast<vtkm::Id>(1) << edgeNo;
        }
      }
    } // per edge

    // we now know which edges are outbound, so we count to get the outdegree
    vtkm::Id outDegree = 0;
    vtkm::Id neighbourComponentMask = 0;
    // special case for local minimum
    if (neighbourhoodMask == 0x3F)
      outDegree = 1;
    else
    { // not a local minimum
      if ((neighbourhoodMask & 0x30) == 0x20)
      {
        ++outDegree;
        neighbourComponentMask |= static_cast<vtkm::Id>(1) << 5;
      }
      if ((neighbourhoodMask & 0x18) == 0x10)
      {
        ++outDegree;
        neighbourComponentMask |= static_cast<vtkm::Id>(1) << 4;
      }
      if ((neighbourhoodMask & 0x0C) == 0x08)
      {
        ++outDegree;
        neighbourComponentMask |= static_cast<vtkm::Id>(1) << 3;
      }
      if ((neighbourhoodMask & 0x06) == 0x04)
      {
        ++outDegree;
        neighbourComponentMask |= static_cast<vtkm::Id>(1) << 2;
      }
      if ((neighbourhoodMask & 0x03) == 0x02)
      {
        ++outDegree;
        neighbourComponentMask |= static_cast<vtkm::Id>(1) << 1;
      }
      if ((neighbourhoodMask & 0x21) == 0x01)
      {
        ++outDegree;
        neighbourComponentMask |= static_cast<vtkm::Id>(1) << 0;
      }
    } // not a local minimum

    return vtkm::make_Pair(neighbourComponentMask, outDegree);
  } // GetNeighbourComponentsMaskAndDegree()

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang

private:
  sortIndicesPortalType sortIndicesPortal;
  edgeBoundaryDetectionMasksPortalType edgeBoundaryDetectionMasksPortal;
  bool getMax;
  vtkm::Id nIncidentEdges;

}; // ExecutionObjec_MeshStructure_3Dt

} // namespace mesh_dem_2d_freudenthal_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
