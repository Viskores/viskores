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

#ifndef vtk_m_worklet_contourtree_augmented_mesh_boundary_mesh_boundary_2d_h
#define vtk_m_worklet_contourtree_augmented_mesh_boundary_mesh_boundary_2d_h

#include <cstdlib>

#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem/MeshStructure2D.h>

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

  VTKM_EXEC_CONT
  const mesh_dem::MeshStructure2D<DeviceTag>& GetMeshStructure() const
  {
    return this->MeshStructure;
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


} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
