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

// This header contains a collection of classes used to describe the boundary
// of a mesh, for each main mesh type (i.e., 2D, 3D, and ContourTreeMesh).
// For each mesh type, there are two classes, the actual boundary desriptor
// class and an ExectionObject class with the PrepareForInput function that
// VTKm expects to generate the object for the execution environment.

#ifndef vtk_m_worklet_contourtree_augmented_mesh_boundary_mesh_boundary_3d_h
#define vtk_m_worklet_contourtree_augmented_mesh_boundary_mesh_boundary_3d_h

#include <cstdlib>

#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/data_set_mesh/MeshStructure3D.h>

#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

// TODO/FIXME: Consider making marching cubes connectivity its own class. However
// at the moment making this a boolean template parameter makes it easuer to avoid
// code duplication. However, if we add more mesh types we should refactor.
template <bool MarchingCubesConnectivity>
class MeshBoundary3D
{
public:
  // Sort indicies types
  using SortIndicesPortalType = IdArrayType::ReadPortalType;

  VTKM_EXEC_CONT
  MeshBoundary3D()
    : MeshStructure(data_set_mesh::MeshStructure3D(vtkm::Id3{ 0, 0, 0 }))
  {
  }

  VTKM_CONT
  MeshBoundary3D(vtkm::Id3 meshSize,
                 const IdArrayType& inSortIndices,
                 vtkm::cont::DeviceAdapterId device,
                 vtkm::cont::Token& token)
    : MeshStructure(data_set_mesh::MeshStructure3D(meshSize))
  {
    this->SortIndicesPortal = inSortIndices.PrepareForInput(device, token);
  }

  VTKM_EXEC_CONT
  bool LiesOnBoundary(const vtkm::Id meshIndex) const
  {
    const vtkm::Id3 pos = this->MeshStructure.VertexPos(meshIndex);
    return (pos[0] == 0) || (pos[1] == 0) || (pos[2] == 0) ||
      (pos[0] == this->MeshStructure.MeshSize[0] - 1) ||
      (pos[1] == this->MeshStructure.MeshSize[1] - 1) ||
      (pos[2] == this->MeshStructure.MeshSize[2] - 1);
  }

  VTKM_EXEC_CONT
  vtkm::Id CountLinkComponentsIn2DSlice4Neighborhood(const vtkm::Id meshIndex,
                                                     const vtkm::Id2 strides) const
  {
    // IMPORTANT: We assume that function is called only for *interior* vertices (i.e.,
    // neither row nor col within slice is 0 and we do not need to check for boundary cases).
    vtkm::Id sortIndex = this->SortIndicesPortal.Get(meshIndex);
    bool prevWasInUpperLink = false;
    vtkm::Id numLinkTypeChanges = 0;

    for (vtkm::Id edgeNo = 0; edgeNo < 4 + 1; edgeNo++)
    {
      VTKM_ASSERT(meshIndex + strides[1] + strides[0] <
                  this->SortIndicesPortal.GetNumberOfValues());
      VTKM_ASSERT(meshIndex - strides[1] - strides[0] >= 0);
      vtkm::Id nbrSortIndex;
      switch (edgeNo % 4)
      {
        case 0:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex + strides[0]);
          break; // [1]    , [0] + 1
        case 1:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex + strides[1]);
          break; // [1] + 1, [0]
        case 2:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex - strides[0]);
          break; // [1]    , [0] - 1
        case 3:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex - strides[1]);
          break; // [1] - 1, [0]
        default:
          // Should not occur, since switch statement is over edgeNo % 4
          VTKM_ASSERT(false);
          // Initialize nbrSortIndex to something anyway to prevent compiler warning
          // Set to the sort index of the vertex itself since there is "no" edge so
          // that it contains a "sane" value if it should ever be reached.
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex);
          break;
      } // switch edgeNo

      bool currIsInUpperLink = (nbrSortIndex > sortIndex);
      numLinkTypeChanges += (edgeNo != 0 && currIsInUpperLink != prevWasInUpperLink) ? 1 : 0;
      prevWasInUpperLink = currIsInUpperLink;
    } // per edge
    return numLinkTypeChanges == 0 ? 1 : numLinkTypeChanges;
  }

  VTKM_EXEC_CONT
  vtkm::Id CountLinkComponentsIn2DSlice6Neighborhood(const vtkm::Id meshIndex,
                                                     const vtkm::Id2 strides) const
  {
    // IMPORTANT: We assume that function is called only for *interior* vertices (i.e.,
    // neither row nor col within slice is 0 and we do not need to check for boundary cases).
    vtkm::Id sortIndex = this->SortIndicesPortal.Get(meshIndex);
    bool prevWasInUpperLink = false;
    vtkm::Id numLinkTypeChanges = 0;

    for (vtkm::Id edgeNo = 0; edgeNo < 6 + 1; edgeNo++)
    {
      VTKM_ASSERT(meshIndex + strides[1] + strides[0] <
                  this->SortIndicesPortal.GetNumberOfValues());
      VTKM_ASSERT(meshIndex - strides[1] - strides[0] >= 0);
      vtkm::Id nbrSortIndex;
      switch (edgeNo % 6)
      {
        case 0:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex + strides[0]);
          break; // [1]    , [0] + 1
        case 1:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex + strides[1] + strides[0]);
          break; // [1] + 1, [0] + 1
        case 2:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex + strides[1]);
          break; // [1] + 1, [0]
        case 3:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex - strides[0]);
          break; // [1]    , [0] - 1
        case 4:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex - strides[1] - strides[0]);
          break; // [1] - 1, [0] - 1
        case 5:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex - strides[1]);
          break; // [1] - 1, [0]
        default:
          // Should not occur, since switch statement is over edgeNo % 6
          VTKM_ASSERT(false);
          // Initialize nbrSortIndex to something anyway to prevent compiler warning
          // Set to the sort index of the vertex itself since there is "no" edge so
          // that it contains a "sane" value if it should ever be reached.
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex);
          break;
      } // seitch edgeNo

      bool currIsInUpperLink = (nbrSortIndex > sortIndex);
      numLinkTypeChanges += (edgeNo != 0 && currIsInUpperLink != prevWasInUpperLink) ? 1 : 0;
      prevWasInUpperLink = currIsInUpperLink;
    } // per edge
    return numLinkTypeChanges == 0 ? 1 : numLinkTypeChanges;
  }

  VTKM_EXEC_CONT
  vtkm::Id CountLinkComponentsIn2DSlice8Neighborhood(const vtkm::Id meshIndex,
                                                     const vtkm::Id2 strides) const
  {
    // IMPORTANT: We assume that function is called only for *interior* vertices (i.e.,
    // neither row nor col within slice is 0 and we do not need to check for boundary cases).
    vtkm::Id sortIndex = this->SortIndicesPortal.Get(meshIndex);
    bool prevWasInUpperLink = false;
    vtkm::Id numLinkTypeChanges = 0;

    for (vtkm::Id edgeNo = 0; edgeNo < 8 + 1; edgeNo++)
    {
      VTKM_ASSERT(meshIndex + strides[1] + strides[0] <
                  this->SortIndicesPortal.GetNumberOfValues());
      VTKM_ASSERT(meshIndex - strides[1] - strides[0] >= 0);
      vtkm::Id nbrSortIndex;

      switch (edgeNo % 8)
      {
        case 0:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex + strides[0]);
          break; // [1]    , [0] + 1
        case 1:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex + strides[1] + strides[0]);
          break; // [1] + 1, [0] + 1
        case 2:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex + strides[1]);
          break; // [1] + 1, [0]
        case 3:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex + strides[1] - strides[0]);
          break; // [1] + 1, [0] -1
        case 4:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex - strides[0]);
          break; // [1]    , [0] - 1
        case 5:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex - strides[1] - strides[0]);
          break; // [1] - 1, [0] - 1
        case 6:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex - strides[1]);
          break; // [1] - 1, [0]
        case 7:
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex - strides[1] + strides[0]);
          break; // [1] - 1, [0] + 1
        default:
          // Should not occur, since switch statement is over edgeNo % 8
          VTKM_ASSERT(false);
          // Initialize nbrSortIndex to something anyway to prevent compiler warning
          // Set to the sort index of the vertex itself since there is "no" edge so
          // that it contains a "sane" value if it should ever be reached.
          nbrSortIndex = this->SortIndicesPortal.Get(meshIndex);
          break;
      } // switch edgeNo

      bool currIsInUpperLink = (nbrSortIndex > sortIndex);
      numLinkTypeChanges += (edgeNo != 0 && currIsInUpperLink != prevWasInUpperLink) ? 1 : 0;
      prevWasInUpperLink = currIsInUpperLink;
    } // per edge
    return numLinkTypeChanges == 0 ? 1 : numLinkTypeChanges;
  }

  VTKM_EXEC_CONT
  bool IsNecessary(vtkm::Id meshIndex) const
  {
    vtkm::Id sortIndex = this->SortIndicesPortal.Get(meshIndex);
    vtkm::Id3 pos{ this->MeshStructure.VertexPos(meshIndex) };
    vtkm::Id nPerSlice = this->MeshStructure.MeshSize[0] *
      this->MeshStructure.MeshSize[1]; // number of vertices on a [2]-perpendicular "slice"

    // Keep only when lying on boundary
    if ((pos[1] == 0) || (pos[0] == 0) || (pos[2] == 0) ||
        (pos[1] == this->MeshStructure.MeshSize[1] - 1) ||
        (pos[0] == this->MeshStructure.MeshSize[0] - 1) ||
        (pos[2] == this->MeshStructure.MeshSize[2] - 1))
    {
      // Keep data on corners
      bool atEndOfLine = (pos[0] == 0) || (pos[0] == this->MeshStructure.MeshSize[0] - 1);
      bool atQuadCorner = (pos[1] == 0 && atEndOfLine) ||
        (pos[1] == this->MeshStructure.MeshSize[1] - 1 && atEndOfLine);
      if ((pos[2] == 0 && atQuadCorner) ||
          (pos[2] == this->MeshStructure.MeshSize[2] - 1 && atQuadCorner))
      {
        return true;
      }
      else
      {
        // Check if vertex lies along one of the boundary edges, if so, keep
        // local extrema
        // Edges in [0] direction
        if ((pos[1] == 0 && pos[2] == 0) ||
            (pos[1] == 0 && pos[2] == this->MeshStructure.MeshSize[2] - 1) ||
            (pos[1] == this->MeshStructure.MeshSize[1] - 1 && pos[2] == 0) ||
            (pos[1] == this->MeshStructure.MeshSize[1] - 1 &&
             pos[2] == this->MeshStructure.MeshSize[2] - 1))
        {
          if (MarchingCubesConnectivity)
          {
            // TODO/FIXME: Be more selective about what points to include, but will likely
            // require an additional layer of "ghost cells"
            return true;
          }
          else
          {
            VTKM_ASSERT(meshIndex >= 1);
            vtkm::Id sp = this->SortIndicesPortal.Get(meshIndex - 1);
            VTKM_ASSERT(meshIndex + 1 < this->SortIndicesPortal.GetNumberOfValues());
            vtkm::Id sn = this->SortIndicesPortal.Get(meshIndex + 1);
            return (sortIndex < sp && sortIndex < sn) || (sortIndex > sp && sortIndex > sn);
          }
        }
        // Edges in [1] directtion
        else if ((pos[0] == 0 && pos[2] == 0) ||
                 (pos[0] == 0 && pos[2] == this->MeshStructure.MeshSize[2] - 1) ||
                 (pos[0] == this->MeshStructure.MeshSize[0] - 1 && pos[2] == 0) ||
                 (pos[0] == this->MeshStructure.MeshSize[0] - 1 &&
                  pos[2] == this->MeshStructure.MeshSize[2] - 1))
        {
          if (MarchingCubesConnectivity)
          {
            // TODO/FIXME: Be more selective about what points to include, but will likely
            // require an additional layer of "ghost cells"
            return true;
          }
          else
          {
            VTKM_ASSERT(pos[1] > 0 && pos[1] < this->MeshStructure.MeshSize[1] - 1);
            VTKM_ASSERT(meshIndex >= this->MeshStructure.MeshSize[0]);
            vtkm::Id sp = this->SortIndicesPortal.Get(meshIndex - this->MeshStructure.MeshSize[0]);
            VTKM_ASSERT(meshIndex + this->MeshStructure.MeshSize[0] <
                        this->SortIndicesPortal.GetNumberOfValues());
            vtkm::Id sn = this->SortIndicesPortal.Get(meshIndex + this->MeshStructure.MeshSize[0]);
            return (sortIndex < sp && sortIndex < sn) || (sortIndex > sp && sortIndex > sn);
          }
        }
        // Edges in [2] direction
        else if ((pos[1] == 0 && pos[0] == 0) ||
                 (pos[1] == 0 && pos[0] == this->MeshStructure.MeshSize[0] - 1) ||
                 (pos[1] == this->MeshStructure.MeshSize[1] - 1 && pos[0] == 0) ||
                 (pos[1] == this->MeshStructure.MeshSize[1] - 1 &&
                  pos[0] == this->MeshStructure.MeshSize[0] - 1))
        {
          if (MarchingCubesConnectivity)
          {
            // TODO/FIXME: Be more selective about what points to include, but will likely
            // require an additional layer of "ghost cells"
            return true;
          }
          else
          {
            VTKM_ASSERT(meshIndex >= nPerSlice);
            vtkm::Id sp = this->SortIndicesPortal.Get(meshIndex - nPerSlice);
            VTKM_ASSERT(meshIndex + nPerSlice < this->SortIndicesPortal.GetNumberOfValues());
            vtkm::Id sn = this->SortIndicesPortal.Get(meshIndex + nPerSlice);
            return (sortIndex < sp && sortIndex < sn) || (sortIndex > sp && sortIndex > sn);
          }
        }
        else
        {
          // On a face/slice
          if (pos[2] == 0 || pos[2] == this->MeshStructure.MeshSize[2] - 1)
          { // On [2]-perpendicular face
            VTKM_ASSERT(pos[0] != 0 && pos[0] != this->MeshStructure.MeshSize[0]);
            VTKM_ASSERT(pos[1] != 0 && pos[1] != this->MeshStructure.MeshSize[1]);
            if (MarchingCubesConnectivity)
            {
              return CountLinkComponentsIn2DSlice4Neighborhood(
                       meshIndex, vtkm::Id2(1, this->MeshStructure.MeshSize[0])) != 2 ||
                CountLinkComponentsIn2DSlice8Neighborhood(
                  meshIndex, vtkm::Id2(1, this->MeshStructure.MeshSize[0])) != 2;
            }
            else
            {
              return CountLinkComponentsIn2DSlice6Neighborhood(
                       meshIndex, vtkm::Id2(1, this->MeshStructure.MeshSize[0])) != 2;
            }
          }
          else if (pos[1] == 0 || pos[1] == this->MeshStructure.MeshSize[1] - 1)
          { // On [1]-perpendicular face
            VTKM_ASSERT(pos[0] != 0 && pos[0] != this->MeshStructure.MeshSize[0]);
            VTKM_ASSERT(pos[2] != 0 && pos[2] != this->MeshStructure.MeshSize[2]);
            if (MarchingCubesConnectivity)
            {
              return CountLinkComponentsIn2DSlice4Neighborhood(meshIndex,
                                                               vtkm::Id2(1, nPerSlice)) != 2 ||
                CountLinkComponentsIn2DSlice8Neighborhood(meshIndex, vtkm::Id2(1, nPerSlice)) != 2;
            }
            else
            {
              return CountLinkComponentsIn2DSlice6Neighborhood(meshIndex,
                                                               vtkm::Id2(1, nPerSlice)) != 2;
            }
          }
          else
          { // On [0]-perpendicular face
            VTKM_ASSERT(pos[0] == 0 || pos[0] == this->MeshStructure.MeshSize[0] - 1);
            VTKM_ASSERT(pos[1] != 0 && pos[1] != this->MeshStructure.MeshSize[1]);
            VTKM_ASSERT(pos[2] != 0 && pos[2] != this->MeshStructure.MeshSize[2]);
            if (MarchingCubesConnectivity)
            {
              return CountLinkComponentsIn2DSlice4Neighborhood(
                       meshIndex, vtkm::Id2(nPerSlice, this->MeshStructure.MeshSize[0])) != 2 ||
                CountLinkComponentsIn2DSlice8Neighborhood(
                  meshIndex, vtkm::Id2(nPerSlice, this->MeshStructure.MeshSize[0])) != 2;
            }
            else
            {
              return CountLinkComponentsIn2DSlice6Neighborhood(
                       meshIndex, vtkm::Id2(nPerSlice, this->MeshStructure.MeshSize[0])) != 2;
            }
          }
        }
      }
    }
    else
    {
      return false;
    }
  }

  VTKM_EXEC_CONT
  const data_set_mesh::MeshStructure3D& GetMeshStructure() const { return this->MeshStructure; }

protected:
  // 3D Mesh size parameters
  data_set_mesh::MeshStructure3D MeshStructure;
  SortIndicesPortalType SortIndicesPortal;
};


// TODO/FIXME: Consider making marching cubes connectivity its own class. However
// at the moment making this a boolean template parameter makes it easuer to avoid
// code duplication. However, if we add more mesh types we should refactor.
template <bool MarchingCubesConnectivity>
class MeshBoundary3DExec : public vtkm::cont::ExecutionObjectBase
{
public:
  VTKM_EXEC_CONT
  MeshBoundary3DExec(vtkm::Id3 meshSize, const IdArrayType& inSortIndices)
    : MeshSize(meshSize)
    , SortIndices(inSortIndices)
  {
  }

  VTKM_CONT MeshBoundary3D<MarchingCubesConnectivity> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    return MeshBoundary3D<MarchingCubesConnectivity>(
      this->MeshSize, this->SortIndices, device, token);
  }

protected:
  // 3D Mesh size parameters
  vtkm::Id3 MeshSize;
  const IdArrayType& SortIndices;
};


} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
