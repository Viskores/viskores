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

#ifndef vtk_m_filter_scalar_topology_worklet_extract_top_volume_contours_get_edges_in_cell_worklet_h
#define vtk_m_filter_scalar_topology_worklet_extract_top_volume_contours_get_edges_in_cell_worklet_h

#include <vtkm/filter/scalar_topology/worklet/extract_top_volume_contours/CopyConstArraysWorklet.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace scalar_topology
{
namespace extract_top_volume_contours
{

constexpr vtkm::IdComponent MAX_MARCHING_CUBE_TRIANGLES = static_cast<vtkm::IdComponent>(5);
constexpr vtkm::IdComponent MAX_LINEAR_INTERPOLATION_TRIANGLES = static_cast<vtkm::IdComponent>(12);

/// Worklet for calculating the edges to be drawn in the cell
/// NOTE: this worklet can only work on 2D and 3D data
template <typename ValueType>
class GetEdgesInCellWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn edgeOffset,          // (input) offset of output edge in the output array
    FieldIn caseCell,            // (input) the marching cube case of the cell
    WholeArrayIn localIds,       // (array input) local ids of points
    WholeArrayIn dataValues,     // (array input) data values within block
    WholeArrayIn globalIds,      // (array input) global regular ids within block
    WholeArrayIn vertexOffset,   // (array input) vertex offset look-up table
    WholeArrayIn edgeTable,      // (array input) edge-in-cell look-up table
    WholeArrayIn numBoundTable,  // (array input) number of boundaries look-up table
    WholeArrayIn boundaryTable,  // (array input) edge-of-boundary look-up table
    WholeArrayIn labelEdgeTable, // (array input) label edge (only for 3D) look-up table
    WholeArrayOut edgesFrom,     // (array output) array of start-points of edges on the isosurface
    WholeArrayOut edgesTo,       // (array output) array of end-points of edges on the isosurface
    WholeArrayOut
      isValidEdges, // (array output) whether the edge plan to draw belongs to the branch
    ExecObject
      findSuperarcForNode // (execution object) detector for the superarc of interpolated nodes
  );
  using ExecutionSignature =
    void(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14);
  using InputDomain = _1;

  using IdArrayReadPortalType = typename vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType;
  using IdArrayWritePortalType = typename vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType;
  using ValueArrayPortalType = typename vtkm::cont::ArrayHandle<ValueType>::ReadPortalType;
  using EdgePointArrayPortalType =
    typename vtkm::cont::ArrayHandle<vtkm::Vec3f_64>::WritePortalType;

  /// Constructor
  /// ptDimensions: dimension of points in the grid
  /// branchSuperarc: the superarc on the given branch intersecting the isosurface
  /// isoValue: isovalue for the isosurface to extract
  VTKM_EXEC_CONT
  GetEdgesInCellWorklet(const vtkm::Id3 ptDimensions,
                        const vtkm::Id3 globalPointIndexStart,
                        const ValueType isoValue,
                        const vtkm::Id globalRegularId,
                        const vtkm::Id branchSuperarc,
                        const vtkm::Id branchSaddleEpsilon,
                        const vtkm::Id totNumPoints,
                        const bool marchingCubes,
                        const bool contourByValue)
    : PointDimensions(ptDimensions)
    , GlobalPointIndexStart(globalPointIndexStart)
    , IsoValue(isoValue)
    , GlobalRegularId(globalRegularId)
    , BranchSuperarc(branchSuperarc)
    , BranchSaddleEpsilon(branchSaddleEpsilon)
    , TotalNumPoints(totNumPoints)
    , IsMarchingCubes(marchingCubes)
    , IsContourByValue(contourByValue)
  {
    CellDimensions[0] = ptDimensions[0] - 1;
    CellDimensions[1] = ptDimensions[1] - 1;
    CellDimensions[2] = ptDimensions[2] - 1;
  }

  // cell index: the point index within the local cell
  VTKM_EXEC vtkm::Id CellIndexToNodeIndex2D(const vtkm::Id2& localPt,
                                            const vtkm::Id cellIndex,
                                            const IdArrayReadPortalType& vertOffset) const
  {
    return vertOffset.Get(cellIndex * 2) + localPt[0] +
      (vertOffset.Get(cellIndex * 2 + 1) + localPt[1]) * PointDimensions[0];
  }

  // cell index: the point index within the local cell
  VTKM_EXEC vtkm::Vec3f_64 CellIndexToNodeCoord2D(const vtkm::Id2& localPt,
                                                  const vtkm::Id cellIndex,
                                                  const IdArrayReadPortalType& vertOffset) const
  {
    return vtkm::Vec3f_64(
      static_cast<vtkm::Float64>(vertOffset.Get(cellIndex * 2) + localPt[0]),
      static_cast<vtkm::Float64>(vertOffset.Get(cellIndex * 2 + 1) + localPt[1]),
      static_cast<vtkm::Float64>(0));
  }

  // cell index: the point index within the local cell
  VTKM_EXEC vtkm::Id CellIndexToNodeIndex3D(const vtkm::Id3& localPt,
                                            const vtkm::Id cellIndex,
                                            const IdArrayReadPortalType& vertOffset) const
  {
    return vertOffset.Get(cellIndex * 3) + localPt[0] +
      (vertOffset.Get(cellIndex * 3 + 1) + localPt[1]) * PointDimensions[0] +
      (vertOffset.Get(cellIndex * 3 + 2) + localPt[2]) * (PointDimensions[0] * PointDimensions[1]);
  }

  // cell index: the point index within the local cell
  VTKM_EXEC vtkm::Vec3f_64 CellIndexToNodeCoord3D(const vtkm::Id3& localPt,
                                                  const vtkm::Id cellIndex,
                                                  const IdArrayReadPortalType& vertOffset) const
  {
    return vtkm::Vec3f_64(
      static_cast<vtkm::Float64>(vertOffset.Get(cellIndex * 3) + localPt[0]),
      static_cast<vtkm::Float64>(vertOffset.Get(cellIndex * 3 + 1) + localPt[1]),
      static_cast<vtkm::Float64>(vertOffset.Get(cellIndex * 3 + 2) + localPt[2]));
  }

  // Implementation to draw isosurface edges
  // all hard-coded numbers in this function depends on the dimension of the data
  // The number of vertices/lines/faces of a cell is fixed for a certain dimension
  // The number of cases for the marching cube algorithm are also hard-coded
  // Check MarchingCubesDataTables.h for more details
  template <typename FindSuperarcExecType>
  VTKM_EXEC void operator()(
    const vtkm::Id localIndex, // refers to the index in the grid
    const vtkm::Id edgeOffset,
    const vtkm::Id caseCell,
    const IdArrayReadPortalType& localIdsPortal, // refers to the index in (superarc etc.) arrays
    const ValueArrayPortalType& dataValuesPortal,
    const IdArrayReadPortalType& globalIdsPortal,
    const IdArrayReadPortalType& vertexOffset,
    const IdArrayReadPortalType& edgeTable,
    const IdArrayReadPortalType& numBoundTable,
    const IdArrayReadPortalType& boundaryTable,
    const IdArrayReadPortalType& labelEdgeTable,
    EdgePointArrayPortalType& edgesFromPortal,
    EdgePointArrayPortalType& edgesToPortal,
    IdArrayWritePortalType& isValidEdgesPortal,
    const FindSuperarcExecType& findSuperarcForNode) const
  {
    const vtkm::Id nPoints = PointDimensions[0] * PointDimensions[1] * PointDimensions[2];
    // 2D
    if (CellDimensions[2] <= 0)
    {
      const vtkm::Id2 localPt(localIndex % CellDimensions[0], localIndex / CellDimensions[0]);

      VTKM_ASSERT(localIdsPortal.GetNumberOfValues() == nPoints);
      VTKM_ASSERT(dataValuesPortal.GetNumberOfValues() == nPoints);

      const vtkm::Id numEdges = numBoundTable.Get(caseCell);
      if (numEdges < 1)
        return;
      for (vtkm::Id edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
      {
        const vtkm::Id lineForCaseOffset = caseCell * nLineTableElemSize2d; // 8;
        const vtkm::Id lineOffset = lineForCaseOffset + edgeIndex * 2;
        // lineFrom and lineTo are two edges where the isosurface edge intersects
        const vtkm::Id lineFrom = boundaryTable.Get(lineOffset);
        const vtkm::Id lineTo = boundaryTable.Get(lineOffset + 1);

        // We need to assure that both lineFrom and lineTo belong to the branch
        // all 0 and 1 in the variables below refer to the two vertices of the line
        const vtkm::Id lineFromVert0 =
          CellIndexToNodeIndex2D(localPt, edgeTable.Get(lineFrom * 2), vertexOffset);
        const vtkm::Id lineFromVert1 =
          CellIndexToNodeIndex2D(localPt, edgeTable.Get(lineFrom * 2 + 1), vertexOffset);
        VTKM_ASSERT(lineFromVert0 < nPoints);
        VTKM_ASSERT(lineFromVert1 < nPoints);
        const vtkm::Id lineFromVert0LocalId = localIdsPortal.Get(lineFromVert0);
        const vtkm::Id lineFromVert1LocalId = localIdsPortal.Get(lineFromVert1);
        const ValueType lineFromVert0Value = dataValuesPortal.Get(lineFromVert0);
        const ValueType lineFromVert1Value = dataValuesPortal.Get(lineFromVert1);
        const vtkm::Id lineFromVert0GlobalId = globalIdsPortal.Get(lineFromVert0);
        const vtkm::Id lineFromVert1GlobalId = globalIdsPortal.Get(lineFromVert1);
        // due to simulation of simplicity
        // vert0 < vert1 if their values are equal
        const vtkm::Id lowVertFrom =
          lineFromVert0Value <= lineFromVert1Value ? lineFromVert0LocalId : lineFromVert1LocalId;
        const vtkm::Id highVertFrom =
          lineFromVert0Value > lineFromVert1Value ? lineFromVert0LocalId : lineFromVert1LocalId;

        const vtkm::Id lineToVert0 =
          CellIndexToNodeIndex2D(localPt, edgeTable.Get(lineTo * 2), vertexOffset);
        const vtkm::Id lineToVert1 =
          CellIndexToNodeIndex2D(localPt, edgeTable.Get(lineTo * 2 + 1), vertexOffset);
        VTKM_ASSERT(lineToVert0 < nPoints);
        VTKM_ASSERT(lineToVert1 < nPoints);
        const vtkm::Id lineToVert0LocalId = localIdsPortal.Get(lineToVert0);
        const vtkm::Id lineToVert1LocalId = localIdsPortal.Get(lineToVert1);
        const ValueType lineToVert0Value = dataValuesPortal.Get(lineToVert0);
        const ValueType lineToVert1Value = dataValuesPortal.Get(lineToVert1);
        const vtkm::Id lineToVert0GlobalId = globalIdsPortal.Get(lineToVert0);
        const vtkm::Id lineToVert1GlobalId = globalIdsPortal.Get(lineToVert1);
        // due to simulation of simplicity
        // vert0 < vert1 if their values are equal
        const vtkm::Id lowVertTo =
          lineToVert0Value <= lineToVert1Value ? lineToVert0LocalId : lineToVert1LocalId;
        const vtkm::Id highVertTo =
          lineToVert0Value > lineToVert1Value ? lineToVert0LocalId : lineToVert1LocalId;

        vtkm::Id lineFromSuperarc = -1;
        vtkm::Id lineToSuperarc = -1;

        // We always extract the isosurface above/below the isovalue by 0+
        // If we extract contours by value (i.e., ignore simulation of simplicity),
        // the global regular ID of the contour should be inf small or large;
        // otherwise, it is +/-1 by the global regular id of the saddle end of the branch.
        VTKM_ASSERT(BranchSaddleEpsilon != 0);
        vtkm::Id contourGRId = IsContourByValue ? (BranchSaddleEpsilon > 0 ? TotalNumPoints : -1)
                                                : GlobalRegularId + BranchSaddleEpsilon;
        lineFromSuperarc = findSuperarcForNode.FindSuperArcForUnknownNode(
          contourGRId, IsoValue, highVertFrom, lowVertFrom);
        lineToSuperarc = findSuperarcForNode.FindSuperArcForUnknownNode(
          contourGRId, IsoValue, highVertTo, lowVertTo);

        // we only draw the line if both lineFrom and lineTo belongs to the branch of query
        if (lineFromSuperarc != BranchSuperarc || lineToSuperarc != BranchSuperarc)
        {
          isValidEdgesPortal.Set(edgeOffset + edgeIndex, 0);
          continue;
        }
        isValidEdgesPortal.Set(edgeOffset + edgeIndex, 1);

        // Now let's draw the line
        vtkm::Vec3f_64 lineFromVert0Coord =
          CellIndexToNodeCoord2D(localPt, edgeTable.Get(lineFrom * 2), vertexOffset);
        vtkm::Vec3f_64 lineFromVert1Coord =
          CellIndexToNodeCoord2D(localPt, edgeTable.Get(lineFrom * 2 + 1), vertexOffset);
        vtkm::Vec3f_64 lineToVert0Coord =
          CellIndexToNodeCoord2D(localPt, edgeTable.Get(lineTo * 2), vertexOffset);
        vtkm::Vec3f_64 lineToVert1Coord =
          CellIndexToNodeCoord2D(localPt, edgeTable.Get(lineTo * 2 + 1), vertexOffset);

        vtkm::Vec3f_64 fromPt(lineFromVert0Coord);
        vtkm::Vec3f_64 toPt(lineToVert0Coord);

        // when values of two vertices in the cell are equal, we rely on the simulation of simplicity
        vtkm::Float64 fromRatio = lineFromVert1Value == lineFromVert0Value
          ? vtkm::Float64(GlobalRegularId - lineFromVert0GlobalId) /
            (lineFromVert1GlobalId - lineFromVert0GlobalId)
          : vtkm::Float64(IsoValue - lineFromVert0Value) /
            (lineFromVert1Value - lineFromVert0Value);
        vtkm::Float64 toRatio = lineToVert1Value == lineToVert0Value
          ? vtkm::Float64(GlobalRegularId - lineToVert0GlobalId) /
            (lineToVert1GlobalId - lineToVert0GlobalId)
          : vtkm::Float64(IsoValue - lineToVert0Value) / (lineToVert1Value - lineToVert0Value);

        VTKM_ASSERT(fromRatio >= 0.0 && fromRatio <= 1.0);
        VTKM_ASSERT(toRatio >= 0.0 && toRatio <= 1.0);

        fromPt += (lineFromVert1Coord - lineFromVert0Coord) * fromRatio;
        toPt += (lineToVert1Coord - lineToVert0Coord) * toRatio;

        edgesFromPortal.Set(edgeOffset + edgeIndex, fromPt + GlobalPointIndexStart);
        edgesToPortal.Set(edgeOffset + edgeIndex, toPt + GlobalPointIndexStart);
      }
    }
    else // 3D
    {
      vtkm::Id3 localPt(localIndex % CellDimensions[0],
                        (localIndex / CellDimensions[0]) % CellDimensions[1],
                        localIndex / (CellDimensions[0] * CellDimensions[1]));

      const vtkm::Id numTriangles = numBoundTable.Get(caseCell);
      if (numTriangles < 1)
        return;

      // we check a specific edge to know the superarc of the triangle
      // the edge label of the triangle is stored in labelEdgeTable in MarchingCubesDataTables.h
      // there are at most 5 triangles to draw in each 3D cell (for marching cubes)
      // for linear interpolation, there are at most 12 triangles
      if (IsMarchingCubes)
        VTKM_ASSERT(numTriangles <= MAX_MARCHING_CUBE_TRIANGLES);
      else
        VTKM_ASSERT(numTriangles <= MAX_LINEAR_INTERPOLATION_TRIANGLES);
      vtkm::Id triangleSuperarc[MAX_LINEAR_INTERPOLATION_TRIANGLES + 1];

      vtkm::Id triangleLabelIdx = 0;
      const vtkm::Id nLabelEdgeElemSize =
        IsMarchingCubes ? nLabelEdgeTableMC3dElemSize : nLabelEdgeTableLT3dElemSize;
      vtkm::Id labelPtr = caseCell * nLabelEdgeElemSize;
      while (labelEdgeTable.Get(labelPtr) != -1)
      {
        vtkm::Id labelCount = labelEdgeTable.Get(labelPtr++);
        vtkm::Id labelEdge = labelEdgeTable.Get(labelPtr++);

        // compute the superarc of the labelEdge belong to the branch
        const vtkm::Id labelEdgeVert0 =
          CellIndexToNodeIndex3D(localPt, edgeTable.Get(labelEdge * 2), vertexOffset);
        const vtkm::Id labelEdgeVert1 =
          CellIndexToNodeIndex3D(localPt, edgeTable.Get(labelEdge * 2 + 1), vertexOffset);
        VTKM_ASSERT(labelEdgeVert0 < nPoints);
        VTKM_ASSERT(labelEdgeVert1 < nPoints);

        const vtkm::Id labelEdgeVert0LocalId = localIdsPortal.Get(labelEdgeVert0);
        const vtkm::Id labelEdgeVert1LocalId = localIdsPortal.Get(labelEdgeVert1);
        const ValueType labelEdgeVert0Value = dataValuesPortal.Get(labelEdgeVert0);
        const ValueType labelEdgeVert1Value = dataValuesPortal.Get(labelEdgeVert1);
        // due to simulation of simplicity
        // vert0 < vert1 if their values are equal
        const vtkm::Id lowVert = labelEdgeVert0Value <= labelEdgeVert1Value ? labelEdgeVert0LocalId
                                                                            : labelEdgeVert1LocalId;
        const vtkm::Id highVert =
          labelEdgeVert0Value > labelEdgeVert1Value ? labelEdgeVert0LocalId : labelEdgeVert1LocalId;

        vtkm::Id labelEdgeSuperarc = -1;

        // We always extract the isosurface above/below the isovalue.globalRegularId by 0+
        // If we extract contours by value (i.e., ignore simulation of simplicity),
        // the global regular ID of the contour should be inf small or large;
        // otherwise, it is +/-1 by the global regular id of the saddle end of the branch.
        VTKM_ASSERT(BranchSaddleEpsilon != 0);
        vtkm::Id contourGRId = IsContourByValue ? (BranchSaddleEpsilon > 0 ? TotalNumPoints : -1)
                                                : GlobalRegularId + BranchSaddleEpsilon;

        labelEdgeSuperarc =
          findSuperarcForNode.FindSuperArcForUnknownNode(contourGRId, IsoValue, highVert, lowVert);
        for (vtkm::Id i = 0; i < labelCount; i++)
          triangleSuperarc[triangleLabelIdx++] = labelEdgeSuperarc;
      }

      VTKM_ASSERT(triangleLabelIdx == numTriangles);

      const vtkm::Id nTriTableElemSize =
        IsMarchingCubes ? nTriTableMC3dElemSize : nTriTableLT3dElemSize;
      for (vtkm::Id triIndex = 0; triIndex < numTriangles; triIndex++)
      {
        const vtkm::Id lineFroms[3] = {
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3),
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3 + 1),
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3 + 2)
        };
        const vtkm::Id lineTos[3] = {
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3 + 1),
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3 + 2),
          boundaryTable.Get(caseCell * nTriTableElemSize + triIndex * 3)
        };

        const vtkm::Id labelEdgeSuperarc = triangleSuperarc[triIndex];
        // we only draw the triangle if the triangle lies on the branch of query
        if (labelEdgeSuperarc != BranchSuperarc)
        {
          isValidEdgesPortal.Set(edgeOffset + triIndex * 3, 0);
          isValidEdgesPortal.Set(edgeOffset + triIndex * 3 + 1, 0);
          isValidEdgesPortal.Set(edgeOffset + triIndex * 3 + 2, 0);
          continue;
        }
        isValidEdgesPortal.Set(edgeOffset + triIndex * 3, 1);
        isValidEdgesPortal.Set(edgeOffset + triIndex * 3 + 1, 1);
        isValidEdgesPortal.Set(edgeOffset + triIndex * 3 + 2, 1);

        for (vtkm::Id edgeIndex = 0; edgeIndex < 3; edgeIndex++)
        {
          // lineFrom and lineTo are two edges where the edge of the triangle intersects
          const vtkm::Id lineFrom = lineFroms[edgeIndex];
          const vtkm::Id lineTo = lineTos[edgeIndex];

          // Now let's draw the line
          vtkm::Vec3f_64 lineFromVert0Coord =
            CellIndexToNodeCoord3D(localPt, edgeTable.Get(lineFrom * 2), vertexOffset);
          vtkm::Vec3f_64 lineFromVert1Coord =
            CellIndexToNodeCoord3D(localPt, edgeTable.Get(lineFrom * 2 + 1), vertexOffset);
          vtkm::Vec3f_64 lineToVert0Coord =
            CellIndexToNodeCoord3D(localPt, edgeTable.Get(lineTo * 2), vertexOffset);
          vtkm::Vec3f_64 lineToVert1Coord =
            CellIndexToNodeCoord3D(localPt, edgeTable.Get(lineTo * 2 + 1), vertexOffset);

          vtkm::Vec3f_64 fromPt(lineFromVert0Coord);
          vtkm::Vec3f_64 toPt(lineToVert0Coord);

          const vtkm::Id lineFromVert0 =
            CellIndexToNodeIndex3D(localPt, edgeTable.Get(lineFrom * 2), vertexOffset);
          const vtkm::Id lineFromVert1 =
            CellIndexToNodeIndex3D(localPt, edgeTable.Get(lineFrom * 2 + 1), vertexOffset);
          VTKM_ASSERT(lineFromVert0 < nPoints);
          VTKM_ASSERT(lineFromVert1 < nPoints);
          const ValueType lineFromVert0Value = dataValuesPortal.Get(lineFromVert0);
          const ValueType lineFromVert1Value = dataValuesPortal.Get(lineFromVert1);
          const vtkm::Id lineFromVert0GlobalId = globalIdsPortal.Get(lineFromVert0);
          const vtkm::Id lineFromVert1GlobalId = globalIdsPortal.Get(lineFromVert1);

          const vtkm::Id lineToVert0 =
            CellIndexToNodeIndex3D(localPt, edgeTable.Get(lineTo * 2), vertexOffset);
          const vtkm::Id lineToVert1 =
            CellIndexToNodeIndex3D(localPt, edgeTable.Get(lineTo * 2 + 1), vertexOffset);
          VTKM_ASSERT(lineToVert0 < nPoints);
          VTKM_ASSERT(lineToVert1 < nPoints);
          const ValueType lineToVert0Value = dataValuesPortal.Get(lineToVert0);
          const ValueType lineToVert1Value = dataValuesPortal.Get(lineToVert1);
          const vtkm::Id lineToVert0GlobalId = globalIdsPortal.Get(lineToVert0);
          const vtkm::Id lineToVert1GlobalId = globalIdsPortal.Get(lineToVert1);

          vtkm::Float64 fromRatio = lineFromVert1Value == lineFromVert0Value
            ? vtkm::Float64(GlobalRegularId - lineFromVert0GlobalId) /
              (lineFromVert1GlobalId - lineFromVert0GlobalId)
            : vtkm::Float64(IsoValue - lineFromVert0Value) /
              (lineFromVert1Value - lineFromVert0Value);
          vtkm::Float64 toRatio = lineToVert1Value == lineToVert0Value
            ? vtkm::Float64(GlobalRegularId - lineToVert0GlobalId) /
              (lineToVert1GlobalId - lineToVert0GlobalId)
            : vtkm::Float64(IsoValue - lineToVert0Value) / (lineToVert1Value - lineToVert0Value);
          VTKM_ASSERT(fromRatio >= 0.0 && fromRatio <= 1.0);
          VTKM_ASSERT(toRatio >= 0.0 && toRatio <= 1.0);

          fromPt += (lineFromVert1Coord - lineFromVert0Coord) * fromRatio;
          toPt += (lineToVert1Coord - lineToVert0Coord) * toRatio;

          edgesFromPortal.Set(edgeOffset + triIndex * 3 + edgeIndex,
                              fromPt + GlobalPointIndexStart);
          edgesToPortal.Set(edgeOffset + triIndex * 3 + edgeIndex, toPt + GlobalPointIndexStart);
        }
      }
    }
  }

private:
  vtkm::Id3 PointDimensions;
  vtkm::Id3 GlobalPointIndexStart;
  ValueType IsoValue;
  vtkm::Id GlobalRegularId;
  vtkm::Id BranchSuperarc;
  vtkm::Id BranchSaddleEpsilon;
  vtkm::Id TotalNumPoints;
  bool IsMarchingCubes;
  bool IsContourByValue;
  vtkm::Id3 CellDimensions;

}; // GetEdgesInCellWorklet

} // namespace extract_top_volume_contours
} // namespace scalar_topology
} // namespace worklet
} // namespace vtkm

#endif
