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

#ifndef vtk_m_worklet_contourtree_augmented_mesh_boundary_h
#define vtk_m_worklet_contourtree_augmented_mesh_boundary_h

#include <cstdlib>

#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem/MeshStructure2D.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem/MeshStructure3D.h>

#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{


template <typename DeviceTag>
class MeshBoundary2D
{
public:
  // Sort indicies types
  using SortOrderPortalType = typename IdArrayType::template ExecutionTypes<DeviceTag>::PortalConst;

  VTKM_EXEC_CONT
  MeshBoundary2D()
    : MeshStructure(mesh_dem::MeshStructure2D<DeviceTag>(0, 0))
  {
  }

  VTKM_CONT
  MeshBoundary2D(vtkm::Id nrows,
                 vtkm::Id ncols,
                 const IdArrayType& sortOrder,
                 vtkm::cont::Token& token)
    : MeshStructure(mesh_dem::MeshStructure2D<DeviceTag>(nrows, ncols))
  {
    this->SortOrderPortal = sortOrder.PrepareForInput(DeviceTag(), token);
  }

  VTKM_EXEC_CONT
  bool liesOnBoundary(const vtkm::Id index) const
  {
    vtkm::Id meshSortOrderValue = this->SortOrderPortal.Get(index);
    const vtkm::Id row = this->MeshStructure.VertexRow(meshSortOrderValue);
    const vtkm::Id col = this->MeshStructure.VertexColumn(meshSortOrderValue);

    return (row == 0) || (col == 0) || (row == this->MeshStructure.NumRows - 1) ||
      (col == this->MeshStructure.NumColumns - 1);
  }

private:
  // 2D Mesh size parameters
  mesh_dem::MeshStructure2D<DeviceTag> MeshStructure;
  SortOrderPortalType SortOrderPortal;
};

class MeshBoundary2DExec : public vtkm::cont::ExecutionObjectBase
{
public:
  VTKM_EXEC_CONT
  MeshBoundary2DExec(vtkm::Id nrows, vtkm::Id ncols, const IdArrayType& inSortOrder)
    : NumRows(nrows)
    , NumColumns(ncols)
    , SortOrder(inSortOrder)
  {
  }

  VTKM_CONT
  template <typename DeviceTag>
  MeshBoundary2D<DeviceTag> PrepareForExecution(DeviceTag, vtkm::cont::Token& token) const
  {
    return MeshBoundary2D<DeviceTag>(this->NumRows, this->NumColumns, this->SortOrder, token);
  }

private:
  // 2D Mesh size parameters
  vtkm::Id NumRows;
  vtkm::Id NumColumns;
  const IdArrayType& SortOrder;
};


template <typename DeviceTag>
class MeshBoundary3D : public vtkm::cont::ExecutionObjectBase
{
public:
  // Sort indicies types
  using SortOrderPortalType = typename IdArrayType::template ExecutionTypes<DeviceTag>::PortalConst;

  VTKM_EXEC_CONT
  MeshBoundary3D()
    : MeshStructure(mesh_dem::MeshStructure3D<DeviceTag>(0, 0, 0))
  {
  }

  VTKM_CONT
  MeshBoundary3D(vtkm::Id nrows,
                 vtkm::Id ncols,
                 vtkm::Id nslices,
                 const IdArrayType& sortOrder,
                 vtkm::cont::Token& token)
    : MeshStructure(mesh_dem::MeshStructure3D<DeviceTag>(nrows, ncols, nslices))
  {
    this->SortOrderPortal = sortOrder.PrepareForInput(DeviceTag(), token);
  }

  VTKM_EXEC_CONT
  bool liesOnBoundary(const vtkm::Id index) const
  {
    vtkm::Id meshSortOrderValue = this->SortOrderPortal.Get(index);
    const vtkm::Id row = this->MeshStructure.VertexRow(meshSortOrderValue);
    const vtkm::Id col = this->MeshStructure.VertexColumn(meshSortOrderValue);
    const vtkm::Id sli = this->MeshStructure.VertexSlice(meshSortOrderValue);
    return (row == 0) || (col == 0) || (sli == 0) || (row == this->MeshStructure.NumRows - 1) ||
      (col == this->MeshStructure.NumColumns - 1) || (sli == this->MeshStructure.NumSlices - 1);
  }

protected:
  // 3D Mesh size parameters
  mesh_dem::MeshStructure3D<DeviceTag> MeshStructure;
  SortOrderPortalType SortOrderPortal;
};


class MeshBoundary3DExec : public vtkm::cont::ExecutionObjectBase
{
public:
  VTKM_EXEC_CONT
  MeshBoundary3DExec(vtkm::Id nrows,
                     vtkm::Id ncols,
                     vtkm::Id nslices,
                     const IdArrayType& inSortOrder)
    : NumRows(nrows)
    , NumColumns(ncols)
    , NumSlices(nslices)
    , SortOrder(inSortOrder)
  {
  }

  VTKM_CONT
  template <typename DeviceTag>
  MeshBoundary3D<DeviceTag> PrepareForExecution(DeviceTag, vtkm::cont::Token& token) const
  {
    return MeshBoundary3D<DeviceTag>(
      this->NumRows, this->NumColumns, this->NumSlices, this->SortOrder, token);
  }

protected:
  // 3D Mesh size parameters
  vtkm::Id NumRows;
  vtkm::Id NumColumns;
  vtkm::Id NumSlices;
  const IdArrayType& SortOrder;
};


template <typename DeviceTag>
class MeshBoundaryContourTreeMesh
{
public:
  using IndicesPortalType = typename IdArrayType::template ExecutionTypes<DeviceTag>::PortalConst;

  VTKM_EXEC_CONT
  MeshBoundaryContourTreeMesh() {}

  VTKM_CONT
  MeshBoundaryContourTreeMesh(const IdArrayType& globalMeshIndex,
                              vtkm::Id totalNRows,
                              vtkm::Id totalNCols,
                              vtkm::Id3 minIdx,
                              vtkm::Id3 maxIdx,
                              vtkm::cont::Token& token)
    : TotalNRows(totalNRows)
    , TotalNCols(totalNCols)
    , MinIdx(minIdx)
    , MaxIdx(maxIdx)
  {
    assert(this->TotalNRows > 0 && this->TotalNCols > 0);
    this->GlobalMeshIndexPortal = globalMeshIndex.PrepareForInput(DeviceTag(), token);
  }

  VTKM_EXEC_CONT
  bool liesOnBoundary(const vtkm::Id index) const
  {
    vtkm::Id idx = this->GlobalMeshIndexPortal.Get(index);
    vtkm::Id3 rcs;
    rcs[0] = vtkm::Id((idx % (this->TotalNRows * this->TotalNCols)) / this->TotalNCols);
    rcs[1] = vtkm::Id(idx % this->TotalNCols);
    rcs[2] = vtkm::Id(idx / (this->TotalNRows * this->TotalNCols));
    for (int d = 0; d < 3; ++d)
    {
      if (this->MinIdx[d] != this->MaxIdx[d] &&
          (rcs[d] == this->MinIdx[d] || rcs[d] == this->MaxIdx[d]))
      {
        return true;
      }
    }
    return false;
  }

private:
  // mesh block parameters
  vtkm::Id TotalNRows;
  vtkm::Id TotalNCols;
  vtkm::Id3 MinIdx;
  vtkm::Id3 MaxIdx;
  IndicesPortalType GlobalMeshIndexPortal;
};


class MeshBoundaryContourTreeMeshExec : public vtkm::cont::ExecutionObjectBase
{
public:
  VTKM_EXEC_CONT
  MeshBoundaryContourTreeMeshExec(const IdArrayType& globalMeshIndex,
                                  vtkm::Id totalNRows,
                                  vtkm::Id totalNCols,
                                  vtkm::Id3 minIdx,
                                  vtkm::Id3 maxIdx)
    : GlobalMeshIndex(globalMeshIndex)
    , TotalNRows(totalNRows)
    , TotalNCols(totalNCols)
    , MinIdx(minIdx)
    , MaxIdx(maxIdx)
  {
  }

  VTKM_CONT
  template <typename DeviceTag>
  MeshBoundaryContourTreeMesh<DeviceTag> PrepareForExecution(DeviceTag,
                                                             vtkm::cont::Token& token) const
  {
    return MeshBoundaryContourTreeMesh<DeviceTag>(
      this->GlobalMeshIndex, this->TotalNRows, this->TotalNCols, this->MinIdx, this->MaxIdx, token);
  }

private:
  const IdArrayType& GlobalMeshIndex;
  vtkm::Id TotalNRows;
  vtkm::Id TotalNCols;
  vtkm::Id3 MinIdx;
  vtkm::Id3 MaxIdx;
};


} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
