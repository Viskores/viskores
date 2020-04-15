
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================


#ifndef vtk_m_worklet_contour_flyingedges_pass4_h
#define vtk_m_worklet_contour_flyingedges_pass4_h


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

template <typename T, typename WholeField, typename EdgeField, typename WeightField>
VTKM_EXEC inline void interpolate_weight(T value,
                                         const vtkm::Id2& iEdge,
                                         vtkm::Id writeIndex,
                                         const WholeField& field,
                                         EdgeField& interpolatedEdgeIds,
                                         WeightField& weights)
{
  interpolatedEdgeIds.Set(writeIndex, iEdge);
  T s0 = field.Get(iEdge[0]);
  T s1 = field.Get(iEdge[1]);

  auto t = (value - s0) / (s1 - s0);
  weights.Set(writeIndex, static_cast<vtkm::FloatDefault>(t));
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

    //We use edgeIndex, edgeIndex+2, edgeIndex+1 to keep
    //the same winding for the triangles that marching celss
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

template <typename T, typename AxisToSum>
struct ComputePass4 : public vtkm::worklet::WorkletVisitCellsWithPoints
{

  vtkm::Id3 PointDims;
  T IsoValue;
  vtkm::Id CellWriteOffset;
  vtkm::Id PointWriteOffset;

  ComputePass4() {}
  ComputePass4(T value,
               const vtkm::Id3& pdims,
               vtkm::Id multiContourCellOffset,
               vtkm::Id multiContourPointOffset)
    : PointDims(pdims)
    , IsoValue(value)
    , CellWriteOffset(multiContourCellOffset)
    , PointWriteOffset(multiContourPointOffset)
  {
  }

  using ControlSignature = void(CellSetIn,
                                FieldInPoint axis_sums,
                                FieldInPoint axis_mins,
                                FieldInPoint axis_maxs,
                                WholeArrayIn cell_tri_count,
                                WholeArrayIn edgeData,
                                WholeArrayIn data,
                                WholeArrayOut connectivity,
                                WholeArrayOut edgeIds,
                                WholeArrayOut weights,
                                WholeArrayOut inputCellIds);
  using ExecutionSignature =
    void(ThreadIndices, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, WorkIndex);

  template <typename ThreadIndices,
            typename FieldInPointId3,
            typename FieldInPointId,
            typename WholeTriField,
            typename WholeEdgeField,
            typename WholeDataField,
            typename WholeConnField,
            typename WholeEdgeIdField,
            typename WholeWeightField,
            typename WholeCellIdField>
  VTKM_EXEC void operator()(const ThreadIndices& threadIndices,
                            const FieldInPointId3& axis_sums,
                            const FieldInPointId& axis_mins,
                            const FieldInPointId& axis_maxs,
                            const WholeTriField& cellTriCount,
                            const WholeEdgeField& edges,
                            const WholeDataField& field,
                            const WholeConnField& conn,
                            const WholeEdgeIdField& interpolatedEdgeIds,
                            const WholeWeightField& weights,
                            const WholeCellIdField& inputCellIds,
                            vtkm::Id oidx) const
  {
    //This works as cellTriCount was computed with ScanExtended
    //and therefore has one more entry than the number of cells
    vtkm::Id cell_tri_offset = cellTriCount.Get(oidx);
    vtkm::Id next_tri_offset = cellTriCount.Get(oidx + 1);
    if (cell_tri_offset == next_tri_offset)
    { //we produce nothing
      return;
    }
    cell_tri_offset += this->CellWriteOffset;

    // find adjusted trim values.
    vtkm::Id left = vtkm::Min(axis_mins[0], axis_mins[1]);
    left = vtkm::Min(left, axis_mins[2]);
    left = vtkm::Min(left, axis_mins[3]);

    vtkm::Id right = vtkm::Max(axis_maxs[0], axis_maxs[1]);
    right = vtkm::Max(right, axis_maxs[2]);
    right = vtkm::Max(right, axis_maxs[3]);

    vtkm::Id3 ijk = compute_ijk(AxisToSum{}, threadIndices.GetInputIndex3D());
    const vtkm::Id3 pdims = this->PointDims;
    const vtkm::Id4 startPos = compute_neighbor_starts(AxisToSum{}, ijk, pdims);
    const vtkm::Id axis_inc = compute_inc(AxisToSum{}, pdims);

    if (left == pdims[AxisToSum::xindex] && right == 0)
    {
      //verify that we have nothing to generate and early terminate.
      bool mins_same = (axis_mins[0] == axis_mins[1] && axis_mins[0] == axis_mins[2] &&
                        axis_mins[0] == axis_mins[3]);
      bool maxs_same = (axis_maxs[0] == axis_maxs[1] && axis_maxs[0] == axis_maxs[2] &&
                        axis_maxs[0] == axis_maxs[3]);
      if (mins_same && maxs_same)
      {
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
    const vtkm::UInt8 yzLoc = static_cast<vtkm::UInt8>((yLoc << 2) | (zLoc << 4));



    const vtkm::Id3 increments = compute_incs3d(pdims);
    vtkm::Id edgeIds[12];
    auto edgeCase = getEdgeCase(edges, startPos, (axis_inc * left));
    init_voxelIds(AxisToSum{}, this->PointWriteOffset, edgeCase, axis_sums, edgeIds);
    for (vtkm::Id i = left; i < right; ++i) // run along the trimmed voxels
    {
      ijk[AxisToSum::xindex] = i;
      edgeCase = getEdgeCase(edges, startPos, (axis_inc * i));
      vtkm::UInt8 numTris = data::GetNumberOfPrimitives(edgeCase);
      if (numTris > 0)
      {
        //compute what the current cellId is
        vtkm::Id cellId = compute_start(AxisToSum{}, ijk, pdims - vtkm::Id3{ 1, 1, 1 });

        // Start by generating triangles for this case
        generate_tris(cellId, edgeCase, numTris, edgeIds, cell_tri_offset, conn, inputCellIds);

        // Now generate edgeIds and weights along voxel axes if needed. Remember to take
        // boundary into account.
        vtkm::UInt8 loc = static_cast<vtkm::UInt8>(
          yzLoc | (i < 1 ? FlyingEdges3D::MinBoundary
                         : (i >= (pdims[AxisToSum::xindex] - 2) ? FlyingEdges3D::MaxBoundary
                                                                : FlyingEdges3D::Interior)));
        auto* edgeUses = data::GetEdgeUses(edgeCase);
        if (loc != FlyingEdges3D::Interior || case_includes_axes(edgeUses))
        {
          this->GenerateWeights(loc,
                                field,
                                interpolatedEdgeIds,
                                weights,
                                startPos,
                                increments,
                                (axis_inc * i),
                                edgeUses,
                                edgeIds);
        }
        advance_voxelIds(edgeUses, edgeIds);
      }
    }
  }

  //----------------------------------------------------------------------------
  template <typename WholeDataField, typename WholeIEdgeField, typename WholeWeightField>
  VTKM_EXEC inline void GenerateWeights(vtkm::UInt8 loc,
                                        const WholeDataField& field,
                                        const WholeIEdgeField& interpolatedEdgeIds,
                                        const WholeWeightField& weights,
                                        const vtkm::Id4& startPos,
                                        const vtkm::Id3& incs,
                                        vtkm::Id offset,
                                        vtkm::UInt8 const* const edgeUses,
                                        vtkm::Id* edgeIds) const
  {
    vtkm::Id2 pos(startPos[0] + offset, 0);
    //EdgesUses 0,4,8 work for Y axis
    if (edgeUses[0])
    { // edgesUses[0] == i axes edge
      pos[1] = startPos[0] + offset + incs[AxisToSum::xindex];
      interpolate_weight(this->IsoValue, pos, edgeIds[0], field, interpolatedEdgeIds, weights);
    }
    if (edgeUses[4])
    { // edgesUses[4] == j axes edge
      pos[1] = startPos[1] + offset;
      interpolate_weight(this->IsoValue, pos, edgeIds[4], field, interpolatedEdgeIds, weights);
    }
    if (edgeUses[8])
    { // edgesUses[8] == k axes edge
      pos[1] = startPos[2] + offset;
      interpolate_weight(this->IsoValue, pos, edgeIds[8], field, interpolatedEdgeIds, weights);
    }
    // On the boundary cells special work has to be done to cover the partial
    // cell axes. These are boundary situations where the voxel axes is not
    // fully formed. These situations occur on the +x,+y,+z volume
    // boundaries. (The other cases fall through the default: case which is
    // expected.)
    //
    // Note that loc is one of 27 regions in the volume, with (0,1,2)
    // indicating (interior, min, max) along coordinate axes.
    switch (loc)
    {
      case 2:
      case 6:
      case 18:
      case 22: //+x
        this->InterpolateEdge(
          pos[0], incs, 5, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 9, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        break;
      case 8:
      case 9:
      case 24:
      case 25: //+y
        this->InterpolateEdge(
          pos[0], incs, 1, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 10, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        break;
      case 32:
      case 33:
      case 36:
      case 37: //+z
        this->InterpolateEdge(
          pos[0], incs, 2, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 6, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        break;
      case 10:
      case 26: //+x +y
        this->InterpolateEdge(
          pos[0], incs, 1, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 5, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 9, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 10, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 11, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        break;
      case 34:
      case 38: //+x +z
        this->InterpolateEdge(
          pos[0], incs, 2, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 5, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 9, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 6, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 7, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        break;
      case 40:
      case 41: //+y +z
        this->InterpolateEdge(
          pos[0], incs, 1, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 2, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 3, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 6, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 10, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        break;
      case 42: //+x +y +z happens no more than once per volume
        this->InterpolateEdge(
          pos[0], incs, 1, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 2, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 3, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 5, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 9, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 10, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 11, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 6, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        this->InterpolateEdge(
          pos[0], incs, 7, edgeUses, edgeIds, field, interpolatedEdgeIds, weights);
        break;
      default: // interior, or -x,-y,-z boundaries
        return;
    }
  }

  // Indicate whether voxel axes need processing for this case.
  //----------------------------------------------------------------------------
  template <typename WholeField, typename WholeIEdgeField, typename WholeWeightField>
  VTKM_EXEC inline void InterpolateEdge(vtkm::Id currentIdx,
                                        const vtkm::Id3& incs,
                                        vtkm::Id edgeNum,
                                        vtkm::UInt8 const* const edgeUses,
                                        vtkm::Id* edgeIds,
                                        const WholeField& field,
                                        const WholeIEdgeField& interpolatedEdgeIds,
                                        const WholeWeightField& weights) const
  {
    // if this edge is not used then get out
    if (!edgeUses[edgeNum])
    {
      return;
    }
    const vtkm::Id writeIndex = edgeIds[edgeNum];
    vtkm::Id2 iEdge;

    // build the edge information
    vtkm::Vec<vtkm::UInt8, 2> verts = data::GetVertMap(edgeNum);

    vtkm::Id3 offsets = data::GetVertOffsets(AxisToSum{}, verts[0]);
    iEdge[0] = currentIdx + vtkm::Dot(offsets, incs);
    offsets = data::GetVertOffsets(AxisToSum{}, verts[1]);
    iEdge[1] = currentIdx + vtkm::Dot(offsets, incs);

    interpolate_weight(this->IsoValue, iEdge, writeIndex, field, interpolatedEdgeIds, weights);
  }
};
}
}
}
#endif
