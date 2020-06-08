
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================


#ifndef vtk_m_worklet_contour_flyingedges_pass4_common_h
#define vtk_m_worklet_contour_flyingedges_pass4_common_h

#include <vtkm/worklet/contour/FlyingEdgesHelpers.h>
#include <vtkm/worklet/contour/FlyingEdgesTables.h>

namespace vtkm
{
namespace worklet
{
namespace flying_edges
{

VTKM_EXEC inline vtkm::Id3 compute_incs3d(const vtkm::Id3& dims)
{
  return vtkm::Id3{ 1, dims[0], (dims[0] * dims[1]) };
}

VTKM_EXEC inline constexpr vtkm::Id compute_cell_Id(SumXAxis,
                                                    vtkm::Id startingCellId,
                                                    vtkm::Id,
                                                    vtkm::Id i)
{
  return startingCellId + i;
}
VTKM_EXEC inline constexpr vtkm::Id compute_cell_Id(SumYAxis,
                                                    vtkm::Id startingCellId,
                                                    vtkm::Id y_point_axis_inc,
                                                    vtkm::Id j)
{
  return startingCellId + ((y_point_axis_inc - 1) * j);
}

VTKM_EXEC inline bool case_includes_axes(vtkm::UInt8 const* const edgeUses)
{
  return (edgeUses[0] != 0 || edgeUses[4] != 0 || edgeUses[8] != 0);
}

template <typename WholeConnField, typename WholeCellIdField>
VTKM_EXEC inline void generate_tris(vtkm::Id inputCellId,
                                    vtkm::UInt8 edgeCase,
                                    vtkm::UInt8 numTris,
                                    vtkm::Id* edgeIds,
                                    vtkm::Id& triId,
                                    const WholeConnField& conn,
                                    const WholeCellIdField& cellIds)
{
  auto* edges = data::GetTriEdgeCases(edgeCase);
  vtkm::Id edgeIndex = 1;
  vtkm::Id index = static_cast<vtkm::Id>(triId) * 3;
  for (vtkm::UInt8 i = 0; i < numTris; ++i)
  {
    cellIds.Set(triId + i, inputCellId);

    //This keeps the same winding for the triangles that marching cells
    //produced. By keeping the winding the same we make sure
    //that 'fast' normals are consistent with the marching
    //cells version
    conn.Set(index, edgeIds[edges[edgeIndex]]);
    conn.Set(index + 1, edgeIds[edges[edgeIndex + 2]]);
    conn.Set(index + 2, edgeIds[edges[edgeIndex + 1]]);
    index += 3;
    edgeIndex += 3;
  }
  triId += numTris;
}


// Helper function to set up the point ids on voxel edges.
//----------------------------------------------------------------------------
template <typename AxisToSum, typename FieldInPointId3>
VTKM_EXEC inline void init_voxelIds(AxisToSum,
                                    vtkm::Id writeOffset,
                                    vtkm::UInt8 edgeCase,
                                    const FieldInPointId3& axis_sums,
                                    vtkm::Id* edgeIds)
{
  auto* edgeUses = data::GetEdgeUses(edgeCase);
  edgeIds[0] = writeOffset + axis_sums[0][AxisToSum::xindex]; // x-edges
  edgeIds[1] = writeOffset + axis_sums[1][AxisToSum::xindex];
  edgeIds[2] = writeOffset + axis_sums[3][AxisToSum::xindex];
  edgeIds[3] = writeOffset + axis_sums[2][AxisToSum::xindex];
  edgeIds[4] = writeOffset + axis_sums[0][AxisToSum::yindex]; // y-edges
  edgeIds[5] = edgeIds[4] + edgeUses[4];
  edgeIds[6] = writeOffset + axis_sums[3][AxisToSum::yindex];
  edgeIds[7] = edgeIds[6] + edgeUses[6];
  edgeIds[8] = writeOffset + axis_sums[0][AxisToSum::zindex]; // z-edges
  edgeIds[9] = edgeIds[8] + edgeUses[8];
  edgeIds[10] = writeOffset + axis_sums[1][AxisToSum::zindex];
  edgeIds[11] = edgeIds[10] + edgeUses[10];
}

// Helper function to advance the point ids along voxel rows.
//----------------------------------------------------------------------------
VTKM_EXEC inline void advance_voxelIds(vtkm::UInt8 const* const edgeUses, vtkm::Id* edgeIds)
{
  edgeIds[0] += edgeUses[0]; // x-edges
  edgeIds[1] += edgeUses[1];
  edgeIds[2] += edgeUses[2];
  edgeIds[3] += edgeUses[3];
  edgeIds[4] += edgeUses[4]; // y-edges
  edgeIds[5] = edgeIds[4] + edgeUses[5];
  edgeIds[6] += edgeUses[6];
  edgeIds[7] = edgeIds[6] + edgeUses[7];
  edgeIds[8] += edgeUses[8]; // z-edges
  edgeIds[9] = edgeIds[8] + edgeUses[9];
  edgeIds[10] += edgeUses[10];
  edgeIds[11] = edgeIds[10] + edgeUses[11];
}

//----------------------------------------------------------------------------
struct Pass4TrimState
{
  vtkm::Id left, right;
  vtkm::Id3 ijk;
  vtkm::Id4 startPos;
  vtkm::Id axis_inc;
  vtkm::UInt8 yzLoc;
  bool valid = true;

  template <typename AxisToSum,
            typename ThreadIndices,
            typename FieldInPointId,
            typename WholeEdgeField>
  VTKM_EXEC Pass4TrimState(AxisToSum,
                           const vtkm::Id3& pdims,
                           const ThreadIndices& threadIndices,
                           const FieldInPointId& axis_mins,
                           const FieldInPointId& axis_maxs,
                           const WholeEdgeField& edges)
  {
    // find adjusted trim values.
    left = vtkm::Min(axis_mins[0], axis_mins[1]);
    left = vtkm::Min(left, axis_mins[2]);
    left = vtkm::Min(left, axis_mins[3]);

    right = vtkm::Max(axis_maxs[0], axis_maxs[1]);
    right = vtkm::Max(right, axis_maxs[2]);
    right = vtkm::Max(right, axis_maxs[3]);

    ijk = compute_ijk(AxisToSum{}, threadIndices.GetInputIndex3D());

    startPos = compute_neighbor_starts(AxisToSum{}, ijk, pdims);
    axis_inc = compute_inc(AxisToSum{}, pdims);

    if (left == pdims[AxisToSum::xindex] && right == 0)
    {
      //verify that we have nothing to generate and early terminate.
      bool mins_same = (axis_mins[0] == axis_mins[1] && axis_mins[0] == axis_mins[2] &&
                        axis_mins[0] == axis_mins[3]);
      bool maxs_same = (axis_maxs[0] == axis_maxs[1] && axis_maxs[0] == axis_maxs[2] &&
                        axis_maxs[0] == axis_maxs[3]);
      if (mins_same && maxs_same)
      {
        valid = false;
        return;
      }
      else
      {
        left = 0;
        right = pdims[AxisToSum::xindex] - 1;
      }
    }

    // The trim edges may need adjustment if the contour travels between rows
    // of edges (without intersecting these edges). This means checking
    // whether the trim faces at (left,right) made up of the edges intersect
    // the contour.
    adjustTrimBounds(pdims[AxisToSum::xindex] - 1, edges, startPos, axis_inc, left, right);
    if (left == right)
    {
      valid = false;
      return;
    }

    const vtkm::UInt8 yLoc =
      (ijk[AxisToSum::yindex] < 1
         ? FlyingEdges3D::MinBoundary
         : (ijk[AxisToSum::yindex] >= (pdims[AxisToSum::yindex] - 2) ? FlyingEdges3D::MaxBoundary
                                                                     : FlyingEdges3D::Interior));
    const vtkm::UInt8 zLoc =
      (ijk[AxisToSum::zindex] < 1
         ? FlyingEdges3D::MinBoundary
         : (ijk[AxisToSum::zindex] >= (pdims[AxisToSum::zindex] - 2) ? FlyingEdges3D::MaxBoundary
                                                                     : FlyingEdges3D::Interior));
    yzLoc = static_cast<vtkm::UInt8>((yLoc << 2) | (zLoc << 4));
  }
};
}
}
}
#endif
