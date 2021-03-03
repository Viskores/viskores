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
#include <vtkm/worklet/contourtree_augmented/data_set_mesh/MeshStructure2D.h>

#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{


class MeshBoundary2D
{
public:
  // Sort indicies types
  using SortIndicesPortalType = IdArrayType::ReadPortalType;

  VTKM_EXEC_CONT
  MeshBoundary2D()
    : MeshStructure({ 0, 0 })
  {
  }

  VTKM_CONT
  MeshBoundary2D(vtkm::Id2 meshSize,
                 const IdArrayType& sortIndices,
                 vtkm::cont::DeviceAdapterId device,
                 vtkm::cont::Token& token)
    : MeshStructure(meshSize)
  {
    this->SortIndicesPortal = sortIndices.PrepareForInput(device, token);
  }

  VTKM_EXEC_CONT
  bool LiesOnBoundary(const vtkm::Id meshIndex) const
  {
    const vtkm::Id2 pos{ this->MeshStructure.VertexPos(meshIndex) };
    return (pos[0] == 0) || (pos[1] == 0) || (pos[0] == this->MeshStructure.MeshSize[0] - 1) ||
      (pos[1] == this->MeshStructure.MeshSize[1] - 1);
  }

  VTKM_EXEC_CONT
  bool IsNecessary(const vtkm::Id meshIndex) const
  {
    vtkm::Id sortIndex = this->SortIndicesPortal.Get(meshIndex);
    const vtkm::Id2 pos{ this->MeshStructure.VertexPos(meshIndex) };

    // Keep only when lying on boundary
    if ((pos[0] == 0) || (pos[1] == 0) || (pos[0] == this->MeshStructure.MeshSize[0] - 1) ||
        (pos[1] == this->MeshStructure.MeshSize[1] - 1))
    {
      // If vertex lies on boundary, keep all corners in any case
      if (((pos[1] == 0) && ((pos[0] == 0) || (pos[0] == this->MeshStructure.MeshSize[0] - 1))) ||
          ((pos[1] == this->MeshStructure.MeshSize[1] - 1) &&
           ((pos[0] == 0) || (pos[0] == this->MeshStructure.MeshSize[0] - 1))))
      {
        return true;
      }
      else
      {
        // if not a corner, keep only vertices that are local extrema
        vtkm::Id sp, sn;
        if (pos[1] == 0 || pos[1] == this->MeshStructure.MeshSize[1] - 1)
        {
          assert(pos[0] > 0 && pos[0] < this->MeshStructure.MeshSize[0] - 1);
          assert(meshIndex >= 1);
          sp = this->SortIndicesPortal.Get(meshIndex - 1);
          assert(meshIndex + 1 < this->SortIndicesPortal.GetNumberOfValues());
          sn = this->SortIndicesPortal.Get(meshIndex + 1);
        }
        else if (pos[0] == 0 || pos[0] == this->MeshStructure.MeshSize[0] - 1)
        {
          assert(pos[1] > 0 && pos[1] < this->MeshStructure.MeshSize[1] - 1);
          assert(meshIndex >= this->MeshStructure.MeshSize[0]);
          sp = this->SortIndicesPortal.Get(meshIndex - this->MeshStructure.MeshSize[0]);
          assert(meshIndex + this->MeshStructure.MeshSize[0] <
                 this->SortIndicesPortal.GetNumberOfValues());
          sn = this->SortIndicesPortal.Get(meshIndex + this->MeshStructure.MeshSize[0]);
        }
        else
        {
          return false;
        }
        return (sortIndex < sp && sortIndex < sn) || (sortIndex > sp && sortIndex > sn);
      }
    }
    else
    {
      // Discard vertices in the interior
      return false;
    }
  }
  VTKM_EXEC_CONT
  const data_set_mesh::MeshStructure2D& GetMeshStructure() const { return this->MeshStructure; }

private:
  // 2D Mesh size parameters
  data_set_mesh::MeshStructure2D MeshStructure;
  SortIndicesPortalType SortIndicesPortal;
};

class MeshBoundary2DExec : public vtkm::cont::ExecutionObjectBase
{
public:
  VTKM_EXEC_CONT
  MeshBoundary2DExec(vtkm::Id2 inMeshSize, const IdArrayType& inSortIndices)
    : MeshSize(inMeshSize)
    , SortIndices(inSortIndices)
  {
  }

  VTKM_CONT
  MeshBoundary2D PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                     vtkm::cont::Token& token) const
  {
    return MeshBoundary2D(this->MeshSize, this->SortIndices, device, token);
  }

private:
  // 2D Mesh size parameters
  vtkm::Id2 MeshSize;
  const IdArrayType& SortIndices;
};


} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
