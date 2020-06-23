//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

#ifndef vtk_m_worklet_contourtree_augmented_mesh_dem_MeshStructureMarchingCubes_h
#define vtk_m_worklet_contourtree_augmented_mesh_dem_MeshStructureMarchingCubes_h

#include <vtkm/Pair.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem/MeshStructure3D.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/marchingcubes_3D/Types.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

// Worklet for computing the sort indices from the sort order
template <typename DeviceAdapter>
class MeshStructureMarchingCubes : public mesh_dem::MeshStructure3D<DeviceAdapter>
{
public:
  // EdgeBoundaryDetectionMasks types
  using EdgeBoundaryDetectionMasksPortalType =
    typename m3d_marchingcubes::EdgeBoundaryDetectionMasksType::template ExecutionTypes<
      DeviceAdapter>::PortalConst;

  // Sort indicies types
  using SortIndicesPortalType =
    typename IdArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;

  // CubeVertexPermutations types
  using CubeVertexPermutationsPortalType =
    typename m3d_marchingcubes::CubeVertexPermutationsType::template ExecutionTypes<
      DeviceAdapter>::PortalConst;

  // linkVertexConnection types
  using LinkVertexConnectionsPortalType =
    typename m3d_marchingcubes::LinkVertexConnectionsType::template ExecutionTypes<
      DeviceAdapter>::PortalConst;
  // inCubeConnection types

  using InCubeConnectionsPortalType =
    typename m3d_marchingcubes::InCubeConnectionsType::template ExecutionTypes<
      DeviceAdapter>::PortalConst;

  // Default constructor needed to make the CUDA build work
  VTKM_EXEC_CONT
  MeshStructureMarchingCubes()
    : mesh_dem::MeshStructure3D<DeviceAdapter>()
    , GetMax(false)
  {
  }

  // Main constructore used in the code
  MeshStructureMarchingCubes(
    vtkm::Id ncols,
    vtkm::Id nrows,
    vtkm::Id nslices,
    bool getmax,
    const IdArrayType& sortIndices,
    const IdArrayType& sortOrder,
    const m3d_marchingcubes::EdgeBoundaryDetectionMasksType& EdgeBoundaryDetectionMasksIn,
    const m3d_marchingcubes::CubeVertexPermutationsType& CubeVertexPermutationsIn,
    const m3d_marchingcubes::LinkVertexConnectionsType& LinkVertexConnectionsSixIn,
    const m3d_marchingcubes::LinkVertexConnectionsType& LinkVertexConnectionsEighteenIn,
    const m3d_marchingcubes::InCubeConnectionsType& InCubeConnectionsSixIn,
    const m3d_marchingcubes::InCubeConnectionsType& InCubeConnectionsEighteenIn,
    vtkm::cont::Token& token)
    : mesh_dem::MeshStructure3D<DeviceAdapter>(ncols, nrows, nslices)
    , GetMax(getmax)
  {
    this->SortIndicesPortal = sortIndices.PrepareForInput(DeviceAdapter(), token);
    this->SortOrderPortal = sortOrder.PrepareForInput(DeviceAdapter(), token);
    this->EdgeBoundaryDetectionMasksPortal =
      EdgeBoundaryDetectionMasksIn.PrepareForInput(DeviceAdapter(), token);
    this->CubeVertexPermutationsPortal =
      CubeVertexPermutationsIn.PrepareForInput(DeviceAdapter(), token);
    this->LinkVertexConnectionsSixPortal =
      LinkVertexConnectionsSixIn.PrepareForInput(DeviceAdapter(), token);
    this->LinkVertexConnectionsEighteenPortal =
      LinkVertexConnectionsEighteenIn.PrepareForInput(DeviceAdapter(), token);
    this->InCubeConnectionsSixPortal =
      InCubeConnectionsSixIn.PrepareForInput(DeviceAdapter(), token);
    this->InCubeConnectionsEighteenPortal =
      InCubeConnectionsEighteenIn.PrepareForInput(DeviceAdapter(), token);
  }

  VTKM_EXEC
  constexpr vtkm::Id GetMaxNumberOfNeighbours() const
  {
    return m3d_marchingcubes::N_FACE_NEIGHBOURS;
  }

  VTKM_EXEC
  inline vtkm::Id GetNeighbourIndex(vtkm::Id sortIndex, vtkm::Id nbrNo) const
  {
    using namespace m3d_marchingcubes;
    vtkm::Id meshIndex = this->SortOrderPortal.Get(sortIndex);
    // GetNeighbourIndex
    switch (nbrNo)
    {
      // Edge connected neighbours
      case 0:
        return SortIndicesPortal.Get(meshIndex -
                                     (this->NumRows * this->NumColumns)); // { -1,  0,  0 }
      case 1:
        return SortIndicesPortal.Get(meshIndex - this->NumColumns); // {  0, -1,  0 }
      case 2:
        return SortIndicesPortal.Get(meshIndex - 1); // {  0,  0, -1 }
      case 3:
        return SortIndicesPortal.Get(meshIndex + 1); // {  0,  0,  1 }
      case 4:
        return SortIndicesPortal.Get(meshIndex + this->NumColumns); // {  0,  1,  0 }
      // Face connected neighbours
      case 5:
        return SortIndicesPortal.Get(meshIndex +
                                     (this->NumRows * this->NumColumns)); // {  1,  0,  0 }
      case 6:
        return SortIndicesPortal.Get(meshIndex - (this->NumRows * this->NumColumns) -
                                     this->NumColumns); // { -1, -1,  0 }
      case 7:
        return SortIndicesPortal.Get(meshIndex - (this->NumRows * this->NumColumns) -
                                     1); // { -1,  0, -1 }
      case 8:
        return SortIndicesPortal.Get(meshIndex - (this->NumRows * this->NumColumns) +
                                     1); // { -1,  0,  1 }
      case 9:
        return SortIndicesPortal.Get(meshIndex - (this->NumRows * this->NumColumns) +
                                     this->NumColumns); // { -1,  1,  0 }
      case 10:
        return SortIndicesPortal.Get(meshIndex - this->NumColumns - 1); // {  0, -1, -1 }
      case 11:
        return SortIndicesPortal.Get(meshIndex - this->NumColumns + 1); // {  0, -1,  1 }
      case 12:
        return SortIndicesPortal.Get(meshIndex + this->NumColumns - 1); // {  0,  1, -1 }
      case 13:
        return SortIndicesPortal.Get(meshIndex + this->NumColumns + 1); // {  0,  1,  1 }
      case 14:
        return SortIndicesPortal.Get(meshIndex + (this->NumRows * this->NumColumns) -
                                     this->NumColumns); // {  1, -1,  0 }
      case 15:
        return SortIndicesPortal.Get(meshIndex + (this->NumRows * this->NumColumns) -
                                     1); // {  1,  0, -1 }
      case 16:
        return SortIndicesPortal.Get(meshIndex + (this->NumRows * this->NumColumns) +
                                     1); // {  1,  0,  1 }
      case 17:
        return SortIndicesPortal.Get(meshIndex + (this->NumRows * this->NumColumns) +
                                     this->NumColumns); // {  1,  1,  0 }
      // Diagonal connected neighbours
      case 18:
        return SortIndicesPortal.Get(meshIndex - (this->NumRows * this->NumColumns) -
                                     this->NumColumns - 1); // { -1, -1, -1 }
      case 19:
        return SortIndicesPortal.Get(meshIndex - (this->NumRows * this->NumColumns) -
                                     this->NumColumns + 1); // { -1, -1,  1 }
      case 20:
        return SortIndicesPortal.Get(meshIndex - (this->NumRows * this->NumColumns) +
                                     this->NumColumns - 1); // { -1,  1, -1 }
      case 21:
        return SortIndicesPortal.Get(meshIndex - (this->NumRows * this->NumColumns) +
                                     this->NumColumns + 1); // { -1,  1,  1 }
      case 22:
        return SortIndicesPortal.Get(meshIndex + (this->NumRows * this->NumColumns) -
                                     this->NumColumns - 1); // {  1, -1, -1 }
      case 23:
        return SortIndicesPortal.Get(meshIndex + (this->NumRows * this->NumColumns) -
                                     this->NumColumns + 1); // {  1, -1,  1 }
      case 24:
        return SortIndicesPortal.Get(meshIndex + (this->NumRows * this->NumColumns) +
                                     this->NumColumns - 1); // {  1,  1, -1 }
      case 25:
        return SortIndicesPortal.Get(meshIndex + (this->NumRows * this->NumColumns) +
                                     this->NumColumns + 1); // {  1,  1,  1 }
      default:
        assert(false);
        return meshIndex; // Need to error out here
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
  inline vtkm::Id GetExtremalNeighbour(vtkm::Id sortIndex) const
  {
    using namespace m3d_marchingcubes;
    // GetExtremalNeighbour()
    // convert to a sort index
    vtkm::Id meshIndex = SortOrderPortal.Get(sortIndex);

    vtkm::Id slice = this->VertexSlice(meshIndex);
    vtkm::Id row = this->VertexRow(meshIndex);
    vtkm::Id col = this->VertexColumn(meshIndex);
    vtkm::Int8 boundaryConfig = ((slice == 0) ? FrontBit : 0) |
      ((slice == this->NumSlices - 1) ? BackBit : 0) | ((col == 0) ? LeftBit : 0) |
      ((col == this->NumColumns - 1) ? RightBit : 0) | ((row == 0) ? TopBit : 0) |
      ((row == this->NumRows - 1) ? BottomBit : 0);

    // in what follows, the boundary conditions always reset wasAscent
    // loop downwards so that we pick the same edges as previous versions
    const int nNeighbours = (!GetMax ? N_FACE_NEIGHBOURS : N_EDGE_NEIGHBOURS);
    for (vtkm::Id nbrNo = 0; nbrNo < nNeighbours; ++nbrNo)
    {
      // only consider valid edges
      if (!(boundaryConfig & EdgeBoundaryDetectionMasksPortal.Get(nbrNo)))
      {
        vtkm::Id nbrSortIndex = GetNeighbourIndex(sortIndex, nbrNo);
        // explicit test allows reversal between join and split trees
        if (GetMax ? (nbrSortIndex > sortIndex) : (nbrSortIndex < sortIndex))
        { // valid edge and outbound
          return nbrSortIndex;
        } // valid edge and outbound
      }
    } // per edge

    return sortIndex | TERMINAL_ELEMENT;
  } // GetExtremalNeighbour()

  VTKM_EXEC
  inline vtkm::Pair<vtkm::Id, vtkm::Id> GetNeighbourComponentsMaskAndDegree(
    vtkm::Id sortIndex,
    bool getMaxComponents) const
  {
    using namespace m3d_marchingcubes;
    // GetNeighbourComponentsMaskAndDegree()
    // convert to a sort index
    vtkm::Id meshIndex = SortOrderPortal.Get(sortIndex);

    vtkm::Id slice = this->VertexSlice(meshIndex);
    vtkm::Id row = this->VertexRow(meshIndex);
    vtkm::Id col = this->VertexColumn(meshIndex);
    vtkm::Int8 boundaryConfig = ((slice == 0) ? FrontBit : 0) |
      ((slice == this->NumSlices - 1) ? BackBit : 0) | ((col == 0) ? LeftBit : 0) |
      ((col == this->NumColumns - 1) ? RightBit : 0) | ((row == 0) ? TopBit : 0) |
      ((row == this->NumRows - 1) ? BottomBit : 0);

    // Initialize "union find"
    int parentId[N_ALL_NEIGHBOURS];

    // Compute components of upper link
    for (int edgeNo = 0; edgeNo < N_ALL_NEIGHBOURS; ++edgeNo)
    {
      if (!(boundaryConfig & EdgeBoundaryDetectionMasksPortal.Get(edgeNo)))
      {
        vtkm::Id nbrSortIndex = GetNeighbourIndex(sortIndex, edgeNo);

        if (getMaxComponents ? (sortIndex < nbrSortIndex) : (sortIndex > nbrSortIndex))
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

    for (vtkm::UInt8 permIndex = 0; permIndex < CubeVertexPermutations_NumPermutations; permIndex++)
    {
      // Combpute connection configuration in each of the eight cubes
      // surrounding a vertex
      vtkm::UInt8 caseNo = 0;
      for (int vtxNo = 0; vtxNo < 7; ++vtxNo)
      {
        if (parentId[CubeVertexPermutationsPortal.Get(permIndex)[vtxNo]] != -1)
        {
          caseNo |= (vtkm::UInt8)(1 << vtxNo);
        }
      }
      if (getMaxComponents)
      {
        for (int edgeNo = 0; edgeNo < 3; ++edgeNo)
        {
          if (InCubeConnectionsSixPortal.Get(caseNo) & (static_cast<vtkm::Id>(1) << edgeNo))
          {
            int root0 = CubeVertexPermutationsPortal.Get(
              permIndex)[LinkVertexConnectionsSixPortal.Get(edgeNo)[0]];
            while (parentId[root0] != root0)
              root0 = parentId[root0];
            int root1 = CubeVertexPermutationsPortal.Get(
              permIndex)[LinkVertexConnectionsSixPortal.Get(edgeNo)[1]];
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
          if (InCubeConnectionsEighteenPortal.Get(caseNo) & (static_cast<vtkm::Id>(1) << edgeNo))
          {
            int root0 = CubeVertexPermutationsPortal.Get(
              permIndex)[LinkVertexConnectionsEighteenPortal.Get(edgeNo)[0]];
            while (parentId[root0] != root0)
              root0 = parentId[root0];
            int root1 = CubeVertexPermutationsPortal.Get(
              permIndex)[LinkVertexConnectionsEighteenPortal.Get(edgeNo)[1]];
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
  SortIndicesPortalType SortIndicesPortal;
  SortIndicesPortalType SortOrderPortal;
  EdgeBoundaryDetectionMasksPortalType EdgeBoundaryDetectionMasksPortal;
  CubeVertexPermutationsPortalType CubeVertexPermutationsPortal;
  LinkVertexConnectionsPortalType LinkVertexConnectionsSixPortal;
  LinkVertexConnectionsPortalType LinkVertexConnectionsEighteenPortal;
  InCubeConnectionsPortalType InCubeConnectionsSixPortal;
  InCubeConnectionsPortalType InCubeConnectionsEighteenPortal;
  bool GetMax;


}; // MeshStructureMarchingCubes

} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
