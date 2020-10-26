
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

VTKM_EXEC inline constexpr vtkm::Id increment_cellId(SumXAxis, vtkm::Id cellId, vtkm::Id)
{
  return cellId + 1;
}
VTKM_EXEC inline constexpr vtkm::Id increment_cellId(SumYAxis,
                                                     vtkm::Id cellId,
                                                     vtkm::Id y_point_axis_inc)
{
  return cellId + (y_point_axis_inc - 1);
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
  vtkm::Id cellId;
  vtkm::Id axis_inc;
  vtkm::Vec<vtkm::UInt8, 3> boundaryStatus;
  bool hasWork = true;

  template <typename AxisToSum,
            typename ThreadIndices,
            typename WholeSumField,
            typename FieldInPointId,
            typename WholeEdgeField>
  VTKM_EXEC Pass4TrimState(AxisToSum,
                           const vtkm::Id3& pdims,
                           const ThreadIndices& threadIndices,
                           const WholeSumField& vtkmNotUsed(axis_sums),
                           const FieldInPointId& axis_mins,
                           const FieldInPointId& axis_maxs,
                           const WholeEdgeField& edges)
  {
    ijk = compute_ijk(AxisToSum{}, threadIndices.GetInputIndex3D());

    startPos = compute_neighbor_starts(AxisToSum{}, ijk, pdims);
    axis_inc = compute_inc(AxisToSum{}, pdims);

    // Compute the subset (start and end) of the row that we need
    // to iterate to generate triangles for the iso-surface
    hasWork = computeTrimBounds(
      pdims[AxisToSum::xindex] - 1, edges, axis_mins, axis_maxs, startPos, axis_inc, left, right);
    hasWork = hasWork && left != right;
    if (!hasWork)
    {
      return;
    }


    cellId = compute_start(AxisToSum{}, ijk, pdims - vtkm::Id3{ 1, 1, 1 });

    //update our ijk
    ijk[AxisToSum::xindex] = left;

    boundaryStatus[0] = FlyingEdges3D::Interior;
    boundaryStatus[1] = FlyingEdges3D::Interior;
    boundaryStatus[2] = FlyingEdges3D::Interior;

    if (ijk[AxisToSum::xindex] < 1)
    {
      boundaryStatus[AxisToSum::xindex] += FlyingEdges3D::MinBoundary;
    }
    if (ijk[AxisToSum::xindex] >= (pdims[AxisToSum::xindex] - 2))
    {
      boundaryStatus[AxisToSum::xindex] += FlyingEdges3D::MaxBoundary;
    }
    if (ijk[AxisToSum::yindex] < 1)
    {
      boundaryStatus[AxisToSum::yindex] += FlyingEdges3D::MinBoundary;
    }
    if (ijk[AxisToSum::yindex] >= (pdims[AxisToSum::yindex] - 2))
    {
      boundaryStatus[AxisToSum::yindex] += FlyingEdges3D::MaxBoundary;
    }
    if (ijk[AxisToSum::zindex] < 1)
    {
      boundaryStatus[AxisToSum::zindex] += FlyingEdges3D::MinBoundary;
    }
    if (ijk[AxisToSum::zindex] >= (pdims[AxisToSum::yindex] - 2))
    {
      boundaryStatus[AxisToSum::zindex] += FlyingEdges3D::MaxBoundary;
    }
  }

  template <typename AxisToSum>
  VTKM_EXEC inline void increment(AxisToSum, const vtkm::Id3& pdims)
  {
    //compute what the current cellId is
    cellId = increment_cellId(AxisToSum{}, cellId, axis_inc);

    //compute what the current ijk is
    ijk[AxisToSum::xindex]++;

    // compute what the current boundary state is
    // can never be on the MinBoundary after we increment
    if (ijk[AxisToSum::xindex] >= (pdims[AxisToSum::xindex] - 2))
    {
      boundaryStatus[AxisToSum::xindex] = FlyingEdges3D::MaxBoundary;
    }
    else
    {
      boundaryStatus[AxisToSum::xindex] = FlyingEdges3D::Interior;
    }
  }
};

// Helper function to state if a boundary object refers to a location that
// is fully inside ( not on a boundary )
//----------------------------------------------------------------------------
VTKM_EXEC inline bool fully_interior(const vtkm::Vec<vtkm::UInt8, 3>& boundaryStatus)
{
  return boundaryStatus[0] == FlyingEdges3D::Interior &&
    boundaryStatus[1] == FlyingEdges3D::Interior && boundaryStatus[2] == FlyingEdges3D::Interior;
}
}
}
}
#endif
