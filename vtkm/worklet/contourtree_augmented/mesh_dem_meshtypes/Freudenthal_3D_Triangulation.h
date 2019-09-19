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


#ifndef vtkm_worklet_contourtree_augmented_mesh_dem_triangulation_3d_freudenthal_h
#define vtkm_worklet_contourtree_augmented_mesh_dem_triangulation_3d_freudenthal_h

#include <cstdlib>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>

#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/worklet/contourtree_augmented/Mesh_DEM_Triangulation.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/MeshBoundary.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/MeshStructureFreudenthal3D.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/freudenthal_3D/Types.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

template <typename T, typename StorageType>
class Mesh_DEM_Triangulation_3D_Freudenthal : public Mesh_DEM_Triangulation_3D<T, StorageType>,
                                              public vtkm::cont::ExecutionObjectBase
{ // class Mesh_DEM_Triangulation
public:
  // Constants and case tables
  m3d_freudenthal::edgeBoundaryDetectionMasksType edgeBoundaryDetectionMasks;
  m3d_freudenthal::neighbourOffsetsType neighbourOffsets;
  m3d_freudenthal::linkComponentCaseTableType linkComponentCaseTable;

  // Mesh helper functions
  void setPrepareForExecutionBehavior(bool getMax);

  template <typename DeviceTag>
  MeshStructureFreudenthal3D<DeviceTag> PrepareForExecution(DeviceTag) const;

  Mesh_DEM_Triangulation_3D_Freudenthal(vtkm::Id ncols, vtkm::Id nrows, vtkm::Id nslices);

  MeshBoundary3DExec GetMeshBoundaryExecutionObject() const;

private:
  bool useGetMax; // Define the behavior ofr the PrepareForExecution function
};                // class Mesh_DEM_Triangulation

// creates input mesh
template <typename T, typename StorageType>
Mesh_DEM_Triangulation_3D_Freudenthal<T, StorageType>::Mesh_DEM_Triangulation_3D_Freudenthal(
  vtkm::Id ncols,
  vtkm::Id nrows,
  vtkm::Id nslices)
  : Mesh_DEM_Triangulation_3D<T, StorageType>(ncols, nrows, nslices)

{
  // Initialize the case tables in vtkm
  edgeBoundaryDetectionMasks = vtkm::cont::make_ArrayHandle(
    m3d_freudenthal::edgeBoundaryDetectionMasks, m3d_freudenthal::N_INCIDENT_EDGES);
  neighbourOffsets = vtkm::cont::make_ArrayHandleGroupVec<3>(vtkm::cont::make_ArrayHandle(
    m3d_freudenthal::neighbourOffsets, m3d_freudenthal::N_INCIDENT_EDGES * 3));
  linkComponentCaseTable = vtkm::cont::make_ArrayHandle(m3d_freudenthal::linkComponentCaseTable,
                                                        m3d_freudenthal::LINK_COMPONENT_CASES);
}


template <typename T, typename StorageType>
void Mesh_DEM_Triangulation_3D_Freudenthal<T, StorageType>::setPrepareForExecutionBehavior(
  bool getMax)
{
  this->useGetMax = getMax;
}

// Get VTKM execution object that represents the structure of the mesh and provides the mesh helper functions on the device
template <typename T, typename StorageType>
template <typename DeviceTag>
MeshStructureFreudenthal3D<DeviceTag>
  Mesh_DEM_Triangulation_3D_Freudenthal<T, StorageType>::PrepareForExecution(DeviceTag) const
{
  return MeshStructureFreudenthal3D<DeviceTag>(this->nCols,
                                               this->nRows,
                                               this->nSlices,
                                               m3d_freudenthal::N_INCIDENT_EDGES,
                                               this->useGetMax,
                                               this->sortIndices,
                                               this->sortOrder,
                                               edgeBoundaryDetectionMasks,
                                               neighbourOffsets,
                                               linkComponentCaseTable);
}


template <typename T, typename StorageType>
MeshBoundary3DExec
Mesh_DEM_Triangulation_3D_Freudenthal<T, StorageType>::GetMeshBoundaryExecutionObject() const
{
  return MeshBoundary3DExec(this->nCols, this->nRows, this->nSlices, this->sortOrder);
}

} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
