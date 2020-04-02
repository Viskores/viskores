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

#ifndef vtk_m_worklet_contourtree_augmented_mesh_dem_MeshStructureFreudenthal3D_h
#define vtk_m_worklet_contourtree_augmented_mesh_dem_MeshStructureFreudenthal3D_h

#include <vtkm/Pair.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem/MeshStructure3D.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/freudenthal_3D/Types.h>


namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

// Worklet for computing the sort indices from the sort order
template <typename DeviceAdapter>
class MeshStructureFreudenthal3D : public mesh_dem::MeshStructure3D<DeviceAdapter>
{
public:
  using SortIndicesPortalType =
    typename IdArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;

  using EdgeBoundaryDetectionMasksPortalType =
    typename m3d_freudenthal::EdgeBoundaryDetectionMasksType::template ExecutionTypes<
      DeviceAdapter>::PortalConst;

  using NeighbourOffsetsPortalType =
    typename m3d_freudenthal::NeighbourOffsetsType::template ExecutionTypes<
      DeviceAdapter>::PortalConst;

  using LinkComponentCaseTablePortalType =
    typename m3d_freudenthal::LinkComponentCaseTableType::template ExecutionTypes<
      DeviceAdapter>::PortalConst;

  // Default constructor needed to make the CUDA build work
  VTKM_EXEC_CONT
  MeshStructureFreudenthal3D()
    : mesh_dem::MeshStructure3D<DeviceAdapter>()
    , GetMax(false)
    , NumIncidentEdge(m3d_freudenthal::N_INCIDENT_EDGES)
  {
  }

  // Main constructore used in the code
  MeshStructureFreudenthal3D(
    vtkm::Id ncols,
    vtkm::Id nrows,
    vtkm::Id nslices,
    vtkm::Id nincident_edges,
    bool getmax,
    const IdArrayType& sortIndices,
    const IdArrayType& sortOrder,
    const m3d_freudenthal::EdgeBoundaryDetectionMasksType& edgeBoundaryDetectionMasksIn,
    const m3d_freudenthal::NeighbourOffsetsType& neighbourOffsetsIn,
    const m3d_freudenthal::LinkComponentCaseTableType& linkComponentCaseTableIn,
    vtkm::cont::Token& token)
    : mesh_dem::MeshStructure3D<DeviceAdapter>(ncols, nrows, nslices)
    , GetMax(getmax)
    , NumIncidentEdge(nincident_edges)
  {
    this->SortIndicesPortal = sortIndices.PrepareForInput(DeviceAdapter(), token);
    this->SortOrderPortal = sortOrder.PrepareForInput(DeviceAdapter(), token);
    this->EdgeBoundaryDetectionMasksPortal =
      edgeBoundaryDetectionMasksIn.PrepareForInput(DeviceAdapter(), token);
    this->NeighbourOffsetsPortal = neighbourOffsetsIn.PrepareForInput(DeviceAdapter(), token);
    this->LinkComponentCaseTablePortal =
      linkComponentCaseTableIn.PrepareForInput(DeviceAdapter(), token);
  }

  VTKM_EXEC
  constexpr vtkm::Id GetMaxNumberOfNeighbours() const { return m3d_freudenthal::N_INCIDENT_EDGES; }


  VTKM_EXEC
  inline vtkm::Id GetNeighbourIndex(vtkm::Id sortIndex, vtkm::Id edgeNo) const
  { // GetNeighbourIndex
    vtkm::Id meshIndex = SortOrderPortal.Get(sortIndex);
    return SortIndicesPortal.Get(meshIndex +
                                 (NeighbourOffsetsPortal.Get(edgeNo)[0] * this->NumRows +
                                  NeighbourOffsetsPortal.Get(edgeNo)[1]) *
                                   this->NumColumns +
                                 NeighbourOffsetsPortal.Get(edgeNo)[2]);
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

  // sets outgoing paths for saddles
  VTKM_EXEC
  inline vtkm::Id GetExtremalNeighbour(vtkm::Id sortIndex) const
  { //  GetExtremalNeighbour()
    // convert to a mesh index
    using namespace m3d_freudenthal;
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
    for (vtkm::Id nbrNo = 0; nbrNo < NumIncidentEdge; ++nbrNo)
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


  // NOTE/FIXME: The following also iterates over all values and could be combined with GetExtremalNeighbour(). However, the
  // results are needed at different places and splitting the two functions leads to a cleaner design
  VTKM_EXEC
  inline vtkm::Pair<vtkm::Id, vtkm::Id> GetNeighbourComponentsMaskAndDegree(
    vtkm::Id sortIndex,
    bool getMaxComponents) const
  { // GetNeighbourComponentsMaskAndDegree()
    // convert to a meshIndex
    using namespace m3d_freudenthal;
    vtkm::Id meshIndex = SortOrderPortal.Get(sortIndex);

    // get the row and column
    vtkm::Id slice = this->VertexSlice(meshIndex);
    vtkm::Id row = this->VertexRow(meshIndex);
    vtkm::Id col = this->VertexColumn(meshIndex);
    vtkm::Int8 boundaryConfig = ((slice == 0) ? FrontBit : 0) |
      ((slice == this->NumSlices - 1) ? BackBit : 0) | ((col == 0) ? LeftBit : 0) |
      ((col == this->NumColumns - 1) ? RightBit : 0) | ((row == 0) ? TopBit : 0) |
      ((row == this->NumRows - 1) ? BottomBit : 0);

    // Initialize "union find"
    vtkm::Id caseNo = 0;

    // Compute components of upper link
    for (int edgeNo = 0; edgeNo < N_INCIDENT_EDGES; ++edgeNo)
    {
      if (!(boundaryConfig & EdgeBoundaryDetectionMasksPortal.Get(edgeNo)))
      {
        vtkm::Id nbrSortIndex = GetNeighbourIndex(sortIndex, edgeNo);
        if (getMaxComponents ? (sortIndex < nbrSortIndex) : (sortIndex > nbrSortIndex))
        {
          caseNo |= vtkm::Id{ 1 } << edgeNo;
        }
      } // inside grid
    }   // for each edge

    // we now know which edges are ascents, so we count to get the updegree
    vtkm::Id outDegree = 0;
    vtkm::Id neighbourComponentMask = 0;

    for (int nbrNo = 0; nbrNo < N_INCIDENT_EDGES; ++nbrNo)
      if (LinkComponentCaseTablePortal.Get(caseNo) & (1 << nbrNo))
      {
        outDegree++;
        neighbourComponentMask |= vtkm::Id{ 1 } << nbrNo;
      }

    vtkm::Pair<vtkm::Id, vtkm::Id> re(neighbourComponentMask, outDegree);
    return re;
  } // GetNeighbourComponentsMaskAndDegree()


#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang


private:
  SortIndicesPortalType SortIndicesPortal;
  SortIndicesPortalType SortOrderPortal;
  EdgeBoundaryDetectionMasksPortalType EdgeBoundaryDetectionMasksPortal;
  NeighbourOffsetsPortalType NeighbourOffsetsPortal;
  LinkComponentCaseTablePortalType LinkComponentCaseTablePortal;
  bool GetMax;
  vtkm::Id NumIncidentEdge;

}; // ExecutionObjec_MeshStructure_3Dt

} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
