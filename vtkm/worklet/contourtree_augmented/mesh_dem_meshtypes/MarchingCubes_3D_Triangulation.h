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


#ifndef vtkm_worklet_contourtree_augmented_mesh_dem_triangulation_3d_marchingcubes_h
#define vtkm_worklet_contourtree_augmented_mesh_dem_triangulation_3d_marchingcubes_h

#include <cstdlib>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ExecutionObjectBase.h>

#include <vtkm/worklet/contourtree_augmented/Mesh_DEM_Triangulation.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/marchingcubes_3D/ExecutionObject_MeshStructure.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/marchingcubes_3D/Types.h>

//Define namespace alias for the marching cubes types to make the code a bit more readable
namespace m3d_marchingcubes_inc_ns =
  vtkm::worklet::contourtree_augmented::mesh_dem_3d_marchingcubes_inc;

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

template <typename T, typename StorageType>
class Mesh_DEM_Triangulation_3D_MarchingCubes : public Mesh_DEM_Triangulation_3D<T, StorageType>,
                                                public vtkm::cont::ExecutionObjectBase
{ // class Mesh_DEM_Triangulation
public:
  //Constants and case tables

  m3d_marchingcubes_inc_ns::edgeBoundaryDetectionMasksType edgeBoundaryDetectionMasks;
  m3d_marchingcubes_inc_ns::cubeVertexPermutationsType cubeVertexPermutations;
  m3d_marchingcubes_inc_ns::linkVertexConnectionsType linkVertexConnectionsSix;
  m3d_marchingcubes_inc_ns::linkVertexConnectionsType linkVertexConnectionsEighteen;
  m3d_marchingcubes_inc_ns::inCubeConnectionsType inCubeConnectionsSix;
  m3d_marchingcubes_inc_ns::inCubeConnectionsType inCubeConnectionsEighteen;

  // mesh depended helper functions
  void setPrepareForExecutionBehavior(bool getMax);

  template <typename DeviceTag>
  mesh_dem_3d_marchingcubes_inc::ExecutionObject_MeshStructure<DeviceTag> PrepareForExecution(
    DeviceTag) const;

  Mesh_DEM_Triangulation_3D_MarchingCubes(vtkm::Id nrows, vtkm::Id ncols, vtkm::Id nslices);

private:
  bool useGetMax; // Define the behavior ofr the PrepareForExecution function
};                // class Mesh_DEM_Triangulation

// creates input mesh
template <typename T, typename StorageType>
Mesh_DEM_Triangulation_3D_MarchingCubes<T, StorageType>::Mesh_DEM_Triangulation_3D_MarchingCubes(
  vtkm::Id nrows,
  vtkm::Id ncols,
  vtkm::Id nslices)
  : Mesh_DEM_Triangulation_3D<T, StorageType>(nrows, ncols, nslices)

{
  // Initialize the case tables in vtkm
  edgeBoundaryDetectionMasks =
    vtkm::cont::make_ArrayHandle(m3d_marchingcubes_inc_ns::edgeBoundaryDetectionMasks,
                                 m3d_marchingcubes_inc_ns::N_ALL_NEIGHBOURS);
  cubeVertexPermutations = vtkm::cont::make_ArrayHandleGroupVec<
    m3d_marchingcubes_inc_ns::
      cubeVertexPermutations_PermVecLength>( // create 2D array of vectors of lengths ...PermVecLength
    vtkm::cont::make_ArrayHandle(
      m3d_marchingcubes_inc_ns::cubeVertexPermutations, // the array to convert
      m3d_marchingcubes_inc_ns::cubeVertexPermutations_NumPermutations *
        m3d_marchingcubes_inc_ns::cubeVertexPermutations_PermVecLength // total number of elements
      ));
  linkVertexConnectionsSix = vtkm::cont::make_ArrayHandleGroupVec<
    m3d_marchingcubes_inc_ns::
      vertexConnections_VecLength>( // create 2D array of vectors o length ...VecLength
    vtkm::cont::make_ArrayHandle(
      m3d_marchingcubes_inc_ns::linkVertexConnectionsSix, // the array to convert
      m3d_marchingcubes_inc_ns::linkVertexConnectionsSix_NumPairs *
        m3d_marchingcubes_inc_ns::vertexConnections_VecLength // total number of elements
      ));
  linkVertexConnectionsEighteen = vtkm::cont::make_ArrayHandleGroupVec<
    m3d_marchingcubes_inc_ns::
      vertexConnections_VecLength>( // create 2D array of vectors o length ...VecLength
    vtkm::cont::make_ArrayHandle(
      m3d_marchingcubes_inc_ns::linkVertexConnectionsEighteen, // the array to convert
      m3d_marchingcubes_inc_ns::linkVertexConnectionsEighteen_NumPairs *
        m3d_marchingcubes_inc_ns::vertexConnections_VecLength // total number of elements
      ));
  inCubeConnectionsSix =
    vtkm::cont::make_ArrayHandle(m3d_marchingcubes_inc_ns::inCubeConnectionsSix,
                                 m3d_marchingcubes_inc_ns::inCubeConnectionsSix_NumElements);
  inCubeConnectionsEighteen =
    vtkm::cont::make_ArrayHandle(m3d_marchingcubes_inc_ns::inCubeConnectionsEighteen,
                                 m3d_marchingcubes_inc_ns::inCubeConnectionsEighteen_NumElements);
}

template <typename T, typename StorageType>
void Mesh_DEM_Triangulation_3D_MarchingCubes<T, StorageType>::setPrepareForExecutionBehavior(
  bool getMax)
{
  this->useGetMax = getMax;
}

// Get VTKM execution object that represents the structure of the mesh and provides the mesh helper functions on the device
template <typename T, typename StorageType>
template <typename DeviceTag>
mesh_dem_3d_marchingcubes_inc::ExecutionObject_MeshStructure<DeviceTag>
  Mesh_DEM_Triangulation_3D_MarchingCubes<T, StorageType>::PrepareForExecution(DeviceTag) const
{
  return mesh_dem_3d_marchingcubes_inc::ExecutionObject_MeshStructure<DeviceTag>(
    this->nRows,
    this->nCols,
    this->nSlices,
    this->useGetMax,
    this->sortIndices,
    edgeBoundaryDetectionMasks,
    cubeVertexPermutations,
    linkVertexConnectionsSix,
    linkVertexConnectionsEighteen,
    inCubeConnectionsSix,
    inCubeConnectionsEighteen);
}

} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
