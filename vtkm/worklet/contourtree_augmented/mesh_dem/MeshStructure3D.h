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

#ifndef vtk_m_worklet_contourtree_augmented_mesh_dem_execution_object_mesh_3d_h
#define vtk_m_worklet_contourtree_augmented_mesh_dem_execution_object_mesh_3d_h

#include <vtkm/Types.h>


namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem
{

// Worklet for computing the sort indices from the sort order
template <typename DeviceAdapter>
class MeshStructure3D
{
public:
  VTKM_EXEC_CONT
  MeshStructure3D()
    : NumColumns(0)
    , NumRows(0)
    , NumSlices(0)
  {
  }

  VTKM_EXEC_CONT
  MeshStructure3D(vtkm::Id ncols, vtkm::Id nrows, vtkm::Id nslices)
    : NumColumns(ncols)
    , NumRows(nrows)
    , NumSlices(nslices)
  {
  }

  // number of mesh vertices
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfVertices() const
  {
    return (this->NumRows * this->NumColumns * this->NumSlices);
  }

  // vertex row - integer modulus by (NumRows&NumColumns) and integer divide by columns
  VTKM_EXEC
  vtkm::Id VertexRow(vtkm::Id v) const { return (v % (NumRows * NumColumns)) / NumColumns; }

  // vertex column -- integer modulus by columns
  VTKM_EXEC
  vtkm::Id VertexColumn(vtkm::Id v) const { return v % NumColumns; }

  // vertex slice -- integer divide by (NumRows*NumColumns)
  VTKM_EXEC
  vtkm::Id VertexSlice(vtkm::Id v) const { return v / (NumRows * NumColumns); }

  //vertex ID - row * ncols + col
  VTKM_EXEC
  vtkm::Id VertexId(vtkm::Id s, vtkm::Id r, vtkm::Id c) const
  {
    return (s * NumRows + r) * NumColumns + c;
  }

  vtkm::Id NumColumns, NumRows, NumSlices;

}; // Mesh_DEM_2D_ExecutionObject

} // namespace mesh_dem_triangulation_worklets
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
