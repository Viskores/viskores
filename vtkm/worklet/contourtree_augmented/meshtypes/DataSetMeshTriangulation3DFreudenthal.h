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


#ifndef vtk_m_worklet_contourtree_augmented_data_set_mesh_triangulation_3d_freudenthal_h
#define vtk_m_worklet_contourtree_augmented_data_set_mesh_triangulation_3d_freudenthal_h

#include <cstdlib>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>

#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/worklet/contourtree_augmented/DataSetMesh.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/MeshStructureFreudenthal3D.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/freudenthal_3D/Types.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/mesh_boundary/ComputeMeshBoundary3D.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/mesh_boundary/MeshBoundary3D.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

class DataSetMeshTriangulation3DFreudenthal
  : public DataSetMesh
  , public vtkm::cont::ExecutionObjectBase
{ // class DataSetMeshTriangulation3DFreudenthal
public:
  // Constants and case tables
  m3d_freudenthal::EdgeBoundaryDetectionMasksType EdgeBoundaryDetectionMasks;
  m3d_freudenthal::NeighbourOffsetsType NeighbourOffsets;
  m3d_freudenthal::LinkComponentCaseTableType LinkComponentCaseTable;
  static constexpr int MAX_OUTDEGREE = 6; // True for Freudenthal and Marching Cubes

  // Mesh helper functions
  void SetPrepareForExecutionBehavior(bool getMax);

  MeshStructureFreudenthal3D PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                 vtkm::cont::Token& token) const;

  DataSetMeshTriangulation3DFreudenthal(vtkm::Id3 meshSize);

  MeshBoundary3DExec GetMeshBoundaryExecutionObject() const;

  void GetBoundaryVertices(IdArrayType& boundaryVertexArray,    // output
                           IdArrayType& boundarySortIndexArray, // output
                           MeshBoundary3DExec* meshBoundaryExecObj =
                             NULL // optional input, included for consistency with ContourTreeMesh
  ) const;

private:
  bool UseGetMax; // Define the behavior ofr the PrepareForExecution function
};                // class DataSetMeshTriangulation

// creates input mesh
inline DataSetMeshTriangulation3DFreudenthal::DataSetMeshTriangulation3DFreudenthal(
  vtkm::Id3 meshSize)
  : DataSetMesh(meshSize)

{
  // Initialize the case tables in vtkm
  this->EdgeBoundaryDetectionMasks =
    vtkm::cont::make_ArrayHandle(m3d_freudenthal::EdgeBoundaryDetectionMasks,
                                 m3d_freudenthal::N_INCIDENT_EDGES,
                                 vtkm::CopyFlag::Off);
  this->NeighbourOffsets = vtkm::cont::make_ArrayHandleGroupVec<3>(vtkm::cont::make_ArrayHandle(
    m3d_freudenthal::NeighbourOffsets, m3d_freudenthal::N_INCIDENT_EDGES * 3, vtkm::CopyFlag::Off));
  this->LinkComponentCaseTable =
    vtkm::cont::make_ArrayHandle(m3d_freudenthal::LinkComponentCaseTable,
                                 m3d_freudenthal::LINK_COMPONENT_CASES,
                                 vtkm::CopyFlag::Off);
}

inline void DataSetMeshTriangulation3DFreudenthal::SetPrepareForExecutionBehavior(bool getMax)
{
  this->UseGetMax = getMax;
}

// Get VTKM execution object that represents the structure of the mesh and provides the mesh helper functions on the device
inline MeshStructureFreudenthal3D DataSetMeshTriangulation3DFreudenthal::PrepareForExecution(
  vtkm::cont::DeviceAdapterId device,
  vtkm::cont::Token& token) const
{
  return MeshStructureFreudenthal3D(this->MeshSize,
                                    m3d_freudenthal::N_INCIDENT_EDGES,
                                    this->UseGetMax,
                                    this->SortIndices,
                                    this->SortOrder,
                                    this->EdgeBoundaryDetectionMasks,
                                    this->NeighbourOffsets,
                                    this->LinkComponentCaseTable,
                                    device,
                                    token);
}

inline MeshBoundary3DExec DataSetMeshTriangulation3DFreudenthal::GetMeshBoundaryExecutionObject()
  const
{
  return MeshBoundary3DExec(this->MeshSize, this->SortIndices);
}

inline void DataSetMeshTriangulation3DFreudenthal::GetBoundaryVertices(
  IdArrayType& boundaryVertexArray,       // output
  IdArrayType& boundarySortIndexArray,    // output
  MeshBoundary3DExec* meshBoundaryExecObj // input
) const
{
  vtkm::Id numBoundary = 2 * this->MeshSize[1] * this->MeshSize[0] // xy faces
    + 2 * this->MeshSize[1] * (this->MeshSize[2] - 2)        // yz faces - excluding vertices on xy
    + 2 * (this->MeshSize[0] - 2) * (this->MeshSize[2] - 2); // xz face interiors
  auto boundaryId = vtkm::cont::ArrayHandleIndex(numBoundary);
  ComputeMeshBoundary3D computeMeshBoundary3dWorklet;
  vtkm::cont::Invoker invoke;
  invoke(computeMeshBoundary3dWorklet,
         boundaryId,        // input
         this->SortIndices, // input
         (meshBoundaryExecObj == NULL) ? this->GetMeshBoundaryExecutionObject()
                                       : *meshBoundaryExecObj, // input
         boundaryVertexArray,                                  // output
         boundarySortIndexArray                                // output
  );
}

} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
