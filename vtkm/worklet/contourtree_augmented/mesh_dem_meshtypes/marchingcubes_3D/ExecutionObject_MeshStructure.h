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

#ifndef vtkm_worklet_contourtree_augmented_mesh_dem_triangulation_3D_marchingcubes_execution_obect_mesh_structure_h
#define vtkm_worklet_contourtree_augmented_mesh_dem_triangulation_3D_marchingcubes_execution_obect_mesh_structure_h

#include <vtkm/Pair.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem/ExecutionObject_MeshStructure_3D.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/marchingcubes_3D/Types.h>


//Define namespace alias for the marching cubes types to make the code a bit more readable
namespace m3d_marchingcubes_inc_ns =
  vtkm::worklet::contourtree_augmented::mesh_dem_3d_marchingcubes_inc;
namespace mesh_dem_inc_ns = vtkm::worklet::contourtree_augmented::mesh_dem_inc;
namespace cpp2_ns = vtkm::worklet::contourtree_augmented;

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem_3d_marchingcubes_inc
{

// Worklet for computing the sort indices from the sort order
template <typename DeviceAdapter>
class ExecutionObject_MeshStructure
  : public mesh_dem_inc_ns::ExecutionObject_MeshStructure_3D<DeviceAdapter>
{
public:
  // edgeBoundaryDetectionMasks types
  typedef
    typename edgeBoundaryDetectionMasksType::template ExecutionTypes<DeviceAdapter>::PortalConst
      edgeBoundaryDetectionMasksPortalType;
  // Sort indices types
  typedef typename cpp2_ns::IdArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst
    sortIndicesPortalType;
  // cubeVertexPermutations types
  typedef typename cubeVertexPermutationsType::template ExecutionTypes<DeviceAdapter>::PortalConst
    cubeVertexPermutationsPortalType;
  // linkVertexConnection types
  typedef typename linkVertexConnectionsType::template ExecutionTypes<DeviceAdapter>::PortalConst
    linkVertexConnectionsPortalType;
  // inCubeConnection types
  typedef typename inCubeConnectionsType::template ExecutionTypes<DeviceAdapter>::PortalConst
    inCubeConnectionsPortalType;



  // Default constructor needed to make the CUDA build work
  VTKM_EXEC_CONT
  ExecutionObject_MeshStructure()
    : mesh_dem_inc_ns::ExecutionObject_MeshStructure_3D<DeviceAdapter>()
    , getMax(false)
  {
  }

  // Main constructore used in the code
  VTKM_EXEC_CONT
  ExecutionObject_MeshStructure(vtkm::Id nrows,
                                vtkm::Id ncols,
                                vtkm::Id nslices,
                                bool getmax,
                                const cpp2_ns::IdArrayType& sortIndices,
                                const edgeBoundaryDetectionMasksType& edgeBoundaryDetectionMasksIn,
                                const cubeVertexPermutationsType& cubeVertexPermutationsIn,
                                const linkVertexConnectionsType& linkVertexConnectionsSixIn,
                                const linkVertexConnectionsType& linkVertexConnectionsEighteenIn,
                                const inCubeConnectionsType& inCubeConnectionsSixIn,
                                const inCubeConnectionsType& inCubeConnectionsEighteenIn)
    : mesh_dem_inc_ns::ExecutionObject_MeshStructure_3D<DeviceAdapter>(nrows, ncols, nslices)
    , getMax(getmax)
  {
    sortIndicesPortal = sortIndices.PrepareForInput(DeviceAdapter());
    edgeBoundaryDetectionMasksPortal =
      edgeBoundaryDetectionMasksIn.PrepareForInput(DeviceAdapter());
    cubeVertexPermutationsPortal = cubeVertexPermutationsIn.PrepareForInput(DeviceAdapter());
    linkVertexConnectionsSixPortal = linkVertexConnectionsSixIn.PrepareForInput(DeviceAdapter());
    linkVertexConnectionsEighteenPortal =
      linkVertexConnectionsEighteenIn.PrepareForInput(DeviceAdapter());
    inCubeConnectionsSixPortal = inCubeConnectionsSixIn.PrepareForInput(DeviceAdapter());
    inCubeConnectionsEighteenPortal = inCubeConnectionsEighteenIn.PrepareForInput(DeviceAdapter());
  }

  VTKM_EXEC
  constexpr vtkm::Id GetMaxNumberOfNeighbours() const { return N_FACE_NEIGHBOURS; }

  VTKM_EXEC
  inline vtkm::Id GetNeighbourIndex(vtkm::Id vertex, vtkm::Id nbrNo) const
  { // GetNeighbourIndex
    switch (nbrNo)
    {
      // Edge connected neighbours
      case 0:
        return vertex - (this->nRows * this->nCols); // { -1,  0,  0 }
      case 1:
        return vertex - this->nCols; // {  0, -1,  0 }
      case 2:
        return vertex - 1; // {  0,  0, -1 }
      case 3:
        return vertex + 1; // {  0,  0,  1 }
      case 4:
        return vertex + this->nCols; // {  0,  1,  0 }
      // Face connected neighbours
      case 5:
        return vertex + (this->nRows * this->nCols); // {  1,  0,  0 }
      case 6:
        return vertex - (this->nRows * this->nCols) - this->nCols; // { -1, -1,  0 }
      case 7:
        return vertex - (this->nRows * this->nCols) - 1; // { -1,  0, -1 }
      case 8:
        return vertex - (this->nRows * this->nCols) + 1; // { -1,  0,  1 }
      case 9:
        return vertex - (this->nRows * this->nCols) + this->nCols; // { -1,  1,  0 }
      case 10:
        return vertex - this->nCols - 1; // {  0, -1, -1 }
      case 11:
        return vertex - this->nCols + 1; // {  0, -1,  1 }
      case 12:
        return vertex + this->nCols - 1; // {  0,  1, -1 }
      case 13:
        return vertex + this->nCols + 1; // {  0,  1,  1 }
      case 14:
        return vertex + (this->nRows * this->nCols) - this->nCols; // {  1, -1,  0 }
      case 15:
        return vertex + (this->nRows * this->nCols) - 1; // {  1,  0, -1 }
      case 16:
        return vertex + (this->nRows * this->nCols) + 1; // {  1,  0,  1 }
      case 17:
        return vertex + (this->nRows * this->nCols) + this->nCols; // {  1,  1,  0 }
      // Diagonal connected neighbours
      case 18:
        return vertex - (this->nRows * this->nCols) - this->nCols - 1; // { -1, -1, -1 }
      case 19:
        return vertex - (this->nRows * this->nCols) - this->nCols + 1; // { -1, -1,  1 }
      case 20:
        return vertex - (this->nRows * this->nCols) + this->nCols - 1; // { -1,  1, -1 }
      case 21:
        return vertex - (this->nRows * this->nCols) + this->nCols + 1; // { -1,  1,  1 }
      case 22:
        return vertex + (this->nRows * this->nCols) - this->nCols - 1; // {  1, -1, -1 }
      case 23:
        return vertex + (this->nRows * this->nCols) - this->nCols + 1; // {  1, -1,  1 }
      case 24:
        return vertex + (this->nRows * this->nCols) + this->nCols - 1; // {  1,  1, -1 }
      case 25:
        return vertex + (this->nRows * this->nCols) + this->nCols + 1; // {  1,  1,  1 }
      default:
        std::
          abort(); // TODO How to generate a meaningful error message from a device (in particular when using CUDA?)
        //default: std::cerr << "Internal error!"; abort();
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

  VTKM_EXEC
  inline vtkm::Id GetExtremalNeighbour(vtkm::Id vertex) const
  { // GetExtremalNeighbour()
    // convert to a sort index
    vtkm::Id sortIndex = sortIndicesPortal.Get(vertex);

    vtkm::Id slice = this->vertexSlice(vertex);
    vtkm::Id row = this->vertexRow(vertex);
    vtkm::Id col = this->vertexColumn(vertex);
    vtkm::Int8 boundaryConfig = ((slice == 0) ? frontBit : 0) |
      ((slice == this->nSlices - 1) ? backBit : 0) | ((col == 0) ? leftBit : 0) |
      ((col == this->nCols - 1) ? rightBit : 0) | ((row == 0) ? topBit : 0) |
      ((row == this->nRows - 1) ? bottomBit : 0);

    // in what follows, the boundary conditions always reset wasAscent
    // loop downwards so that we pick the same edges as previous versions
    const int nNeighbours = (!getMax ? N_FACE_NEIGHBOURS : N_EDGE_NEIGHBOURS);
    for (vtkm::Id nbrNo = 0; nbrNo < nNeighbours; ++nbrNo)
    {
      // only consider valid edges
      if (!(boundaryConfig & edgeBoundaryDetectionMasksPortal.Get(nbrNo)))
      {
        vtkm::Id nbrIndex = sortIndicesPortal.Get(GetNeighbourIndex(vertex, nbrNo));
        // explicit test allows reversal between join and split trees
        if (getMax ? (nbrIndex > sortIndex) : (nbrIndex < sortIndex))
        { // valid edge and outbound
          return nbrIndex;
        } // valid edge and outbound
      }
    } // per edge

    return sortIndex | TERMINAL_ELEMENT;
  } // GetExtremalNeighbour()

  VTKM_EXEC
  inline vtkm::Pair<vtkm::Id, vtkm::Id> GetNeighbourComponentsMaskAndDegree(
    vtkm::Id vertex,
    bool getMaxComponents) const
  { // GetNeighbourComponentsMaskAndDegree()
    // convert to a sort index
    vtkm::Id sortIndex = sortIndicesPortal.Get(vertex);

    vtkm::Id slice = this->vertexSlice(vertex);
    vtkm::Id row = this->vertexRow(vertex);
    vtkm::Id col = this->vertexColumn(vertex);
    vtkm::Int8 boundaryConfig = ((slice == 0) ? frontBit : 0) |
      ((slice == this->nSlices - 1) ? backBit : 0) | ((col == 0) ? leftBit : 0) |
      ((col == this->nCols - 1) ? rightBit : 0) | ((row == 0) ? topBit : 0) |
      ((row == this->nRows - 1) ? bottomBit : 0);

    // Initialize "union find"
    int parentId[N_ALL_NEIGHBOURS];

    // Compute components of upper link
    for (int edgeNo = 0; edgeNo < N_ALL_NEIGHBOURS; ++edgeNo)
    {
      if (!(boundaryConfig & edgeBoundaryDetectionMasksPortal.Get(edgeNo)))
      {
        vtkm::Id nbrIndex = sortIndicesPortal.Get(GetNeighbourIndex(vertex, edgeNo));

        if (getMaxComponents ? (sortIndex < nbrIndex) : (sortIndex > nbrIndex))
        {
          parentId[edgeNo] = edgeNo;
        }
        else
        {
          parentId[edgeNo] = -1;
        }
      } // inside grid
      else
      {
        parentId[edgeNo] = -1;
      }
    } // for each edge

    for (vtkm::UInt8 permIndex = 0; permIndex < cubeVertexPermutations_NumPermutations; permIndex++)
    {
      // Combpute connection configuration in each of the eight cubes
      // surrounding a vertex
      vtkm::UInt8 caseNo = 0;
      for (int vtxNo = 0; vtxNo < 7; ++vtxNo)
      {
        if (parentId[cubeVertexPermutationsPortal.Get(permIndex)[vtxNo]] != -1)
        {
          caseNo |= (vtkm::UInt8)(1 << vtxNo);
        }
      }
      if (getMaxComponents)
      {
        for (int edgeNo = 0; edgeNo < 3; ++edgeNo)
        {
          if (inCubeConnectionsSixPortal.Get(caseNo) & (static_cast<vtkm::Id>(1) << edgeNo))
          {
            int root0 = cubeVertexPermutationsPortal.Get(
              permIndex)[linkVertexConnectionsSixPortal.Get(edgeNo)[0]];
            while (parentId[root0] != root0)
              root0 = parentId[root0];
            int root1 = cubeVertexPermutationsPortal.Get(
              permIndex)[linkVertexConnectionsSixPortal.Get(edgeNo)[1]];
            while (parentId[root1] != root1)
              root1 = parentId[root1];
            if (root0 != root1)
              parentId[root1] = root0;
          }
        }
      }
      else
      {
        for (int edgeNo = 0; edgeNo < 15; ++edgeNo)
        {
          if (inCubeConnectionsEighteenPortal.Get(caseNo) & (static_cast<vtkm::Id>(1) << edgeNo))
          {
            int root0 = cubeVertexPermutationsPortal.Get(
              permIndex)[linkVertexConnectionsEighteenPortal.Get(edgeNo)[0]];
            while (parentId[root0] != root0)
              root0 = parentId[root0];
            int root1 = cubeVertexPermutationsPortal.Get(
              permIndex)[linkVertexConnectionsEighteenPortal.Get(edgeNo)[1]];
            while (parentId[root1] != root1)
              root1 = parentId[root1];
            if (root0 != root1)
              parentId[root1] = root0;
          }
        }
      }
    }
    // we now know which edges are ascents, so we count to get the updegree
    vtkm::Id outDegree = 0;
    vtkm::Id neighbourComponentMask = 0;

    // Find one representaant for each connected compomnent in "link"
    const int nNeighbours = getMaxComponents ? 6 : 18;
    for (int nbrNo = 0; nbrNo < nNeighbours; ++nbrNo)
    {
      if (parentId[nbrNo] == nbrNo)
      {
        outDegree++;
        neighbourComponentMask |= static_cast<vtkm::Id>(1) << nbrNo;
      }
    }

    return vtkm::make_Pair(neighbourComponentMask, outDegree);
  } // GetNeighbourComponentsMaskAndDegree()

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang



private:
  sortIndicesPortalType sortIndicesPortal;
  edgeBoundaryDetectionMasksPortalType edgeBoundaryDetectionMasksPortal;
  cubeVertexPermutationsPortalType cubeVertexPermutationsPortal;
  linkVertexConnectionsPortalType linkVertexConnectionsSixPortal;
  linkVertexConnectionsPortalType linkVertexConnectionsEighteenPortal;
  inCubeConnectionsPortalType inCubeConnectionsSixPortal;
  inCubeConnectionsPortalType inCubeConnectionsEighteenPortal;
  bool getMax;


}; // ExecutionObjec_MeshStructure_3Dt

} // namespace mesh_dem_3d_marchingcubes_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
