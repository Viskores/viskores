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

#ifndef vtkm_worklet_contourtree_augmented_mesh_dem_MeshStructureFreudenthal3D_h
#define vtkm_worklet_contourtree_augmented_mesh_dem_MeshStructureFreudenthal3D_h

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
  using sortIndicesPortalType =
    typename IdArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;

  using edgeBoundaryDetectionMasksPortalType =
    typename m3d_freudenthal::edgeBoundaryDetectionMasksType::template ExecutionTypes<
      DeviceAdapter>::PortalConst;

  using neighbourOffsetsPortalType =
    typename m3d_freudenthal::neighbourOffsetsType::template ExecutionTypes<
      DeviceAdapter>::PortalConst;

  using linkComponentCaseTablePortalType =
    typename m3d_freudenthal::linkComponentCaseTableType::template ExecutionTypes<
      DeviceAdapter>::PortalConst;

  // Default constructor needed to make the CUDA build work
  VTKM_EXEC_CONT
  MeshStructureFreudenthal3D()
    : mesh_dem::MeshStructure3D<DeviceAdapter>()
    , getMax(false)
    , nIncidentEdges(m3d_freudenthal::N_INCIDENT_EDGES)
  {
  }

  // Main constructore used in the code
  MeshStructureFreudenthal3D(
    vtkm::Id nrows,
    vtkm::Id ncols,
    vtkm::Id nslices,
    vtkm::Id nincident_edges,
    bool getmax,
    const IdArrayType& sortIndices,
    const m3d_freudenthal::edgeBoundaryDetectionMasksType& edgeBoundaryDetectionMasksIn,
    const m3d_freudenthal::neighbourOffsetsType& neighbourOffsetsIn,
    const m3d_freudenthal::linkComponentCaseTableType& linkComponentCaseTableIn)
    : mesh_dem::MeshStructure3D<DeviceAdapter>(nrows, ncols, nslices)
    , getMax(getmax)
    , nIncidentEdges(nincident_edges)
  {
    sortIndicesPortal = sortIndices.PrepareForInput(DeviceAdapter());
    edgeBoundaryDetectionMasksPortal =
      edgeBoundaryDetectionMasksIn.PrepareForInput(DeviceAdapter());
    neighbourOffsetsPortal = neighbourOffsetsIn.PrepareForInput(DeviceAdapter());
    linkComponentCaseTablePortal = linkComponentCaseTableIn.PrepareForInput(DeviceAdapter());
  }

  VTKM_EXEC
  constexpr vtkm::Id GetMaxNumberOfNeighbours() const { return m3d_freudenthal::N_INCIDENT_EDGES; }


  VTKM_EXEC
  inline vtkm::Id GetNeighbourIndex(vtkm::Id vertex, vtkm::Id edgeNo) const
  { // GetNeighbourIndex
    return vertex +
      (neighbourOffsetsPortal.Get(edgeNo)[0] * this->nRows +
       neighbourOffsetsPortal.Get(edgeNo)[1]) *
      this->nCols +
      neighbourOffsetsPortal.Get(edgeNo)[2];
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
  inline vtkm::Id GetExtremalNeighbour(vtkm::Id vertex) const
  {
    using namespace m3d_freudenthal;
    // GetExtremalNeighbour()
    // convert to a sort index
    vtkm::Id sortIndex = sortIndicesPortal.Get(vertex);

    // get the row and column
    vtkm::Id slice = this->vertexSlice(vertex);
    vtkm::Id row = this->vertexRow(vertex);
    vtkm::Id col = this->vertexColumn(vertex);
    vtkm::Int8 boundaryConfig = ((slice == 0) ? frontBit : 0) |
      ((slice == this->nSlices - 1) ? backBit : 0) | ((col == 0) ? leftBit : 0) |
      ((col == this->nCols - 1) ? rightBit : 0) | ((row == 0) ? topBit : 0) |
      ((row == this->nRows - 1) ? bottomBit : 0);

    // in what follows, the boundary conditions always reset wasAscent
    // loop downwards so that we pick the same edges as previous versions
    for (vtkm::Id nbrNo = 0; nbrNo < nIncidentEdges; ++nbrNo)
    {
      // only consider valid edges
      if (!(boundaryConfig & edgeBoundaryDetectionMasksPortal.Get(nbrNo)))
      {
        vtkm::Id nbrIndex = sortIndicesPortal.Get(GetNeighbourIndex(vertex, nbrNo));
        // explicit test allows reversal between join and split trees
        if (getMax ? (nbrIndex > sortIndex) : (nbrIndex < sortIndex))
        { // valid edge and outbound
          return nbrIndex;
        } // valid edge and outbound
      }
    } // per edge

    return sortIndex | TERMINAL_ELEMENT;

  } // GetExtremalNeighbour()

  // NOTE/FIXME: The following also iterates over all values and could be combined with GetExtremalNeighbour(). However, the
  // results are needed at different places and splitting the two functions leads to a cleaner design
  VTKM_EXEC
  inline vtkm::Pair<vtkm::Id, vtkm::Id> GetNeighbourComponentsMaskAndDegree(
    vtkm::Id vertex,
    bool getMaxComponents) const
  {
    using namespace m3d_freudenthal;
    // GetNeighbourComponentsMaskAndDegree()
    // convert to a sort index
    vtkm::Id sortIndex = sortIndicesPortal.Get(vertex);

    // get the row and column
    vtkm::Id slice = this->vertexSlice(vertex);
    vtkm::Id row = this->vertexRow(vertex);
    vtkm::Id col = this->vertexColumn(vertex);
    vtkm::Int8 boundaryConfig = ((slice == 0) ? frontBit : 0) |
      ((slice == this->nSlices - 1) ? backBit : 0) | ((col == 0) ? leftBit : 0) |
      ((col == this->nCols - 1) ? rightBit : 0) | ((row == 0) ? topBit : 0) |
      ((row == this->nRows - 1) ? bottomBit : 0);

    // Initialize "union find"
    vtkm::Id caseNo = 0;

    // Compute components of upper link
    for (int edgeNo = 0; edgeNo < N_INCIDENT_EDGES; ++edgeNo)
    {
      if (!(boundaryConfig & edgeBoundaryDetectionMasksPortal.Get(edgeNo)))
      {
        vtkm::Id nbrIndex = sortIndicesPortal.Get(this->GetNeighbourIndex(vertex, edgeNo));

        if (getMaxComponents ? (sortIndex < nbrIndex) : (sortIndex > nbrIndex))
        {
          caseNo |= static_cast<vtkm::Id>(1) << edgeNo;
        }
      } // inside grid
    }   // for each edge

    // we now know which edges are ascents, so we count to get the updegree
    vtkm::Id outDegree = 0;
    vtkm::Id neighbourComponentMask = 0;

    for (int nbrNo = 0; nbrNo < N_INCIDENT_EDGES; ++nbrNo)
      if (linkComponentCaseTablePortal.Get(caseNo) & (static_cast<vtkm::Id>(1) << nbrNo))
      {
        outDegree++;
        neighbourComponentMask |= static_cast<vtkm::Id>(1) << nbrNo;
      }

    return vtkm::make_Pair(neighbourComponentMask, outDegree);
  } // GetNeighbourComponentsMaskAndDegree()

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang


private:
  sortIndicesPortalType sortIndicesPortal;
  edgeBoundaryDetectionMasksPortalType edgeBoundaryDetectionMasksPortal;
  neighbourOffsetsPortalType neighbourOffsetsPortal;
  linkComponentCaseTablePortalType linkComponentCaseTablePortal;
  bool getMax;
  vtkm::Id nIncidentEdges;

}; // ExecutionObjec_MeshStructure_3Dt

} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
