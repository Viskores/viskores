
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================


#ifndef vtk_m_worklet_contour_flyingedges_helpers_h
#define vtk_m_worklet_contour_flyingedges_helpers_h

#include <vtkm/Types.h>

namespace vtkm
{
namespace worklet
{
namespace flying_edges
{

struct FlyingEdges3D
{
public:
  // Edge case table values.
  enum EdgeClass
  {
    Below = 0,      // below isovalue
    Above = 1,      // above isovalue
    LeftAbove = 1,  // left vertex is above isovalue
    RightAbove = 2, // right vertex is above isovalue
    BothAbove = 3   // entire edge is above isovalue
  };
  enum CellClass
  {
    Interior = 0,
    MinBoundary = 1,
    MaxBoundary = 2
  };
};

struct SumXAxis
{
  static constexpr vtkm::Id xindex = 0;
  static constexpr vtkm::Id yindex = 1;
  static constexpr vtkm::Id zindex = 2;
};
struct SumYAxis
{
  static constexpr vtkm::Id xindex = 1;
  static constexpr vtkm::Id yindex = 0;
  static constexpr vtkm::Id zindex = 2;
};

template <typename Device>
struct select_AxisToSum
{
  using type = SumXAxis;
};

template <>
struct select_AxisToSum<vtkm::cont::DeviceAdapterTagCuda>
{
  using type = SumYAxis;
};
template <>
struct select_AxisToSum<vtkm::cont::DeviceAdapterTagKokkos>
{
  using type = SumYAxis;
};

inline vtkm::cont::CellSetStructured<2> make_metaDataMesh2D(SumXAxis, const vtkm::Id3& pdims)
{
  vtkm::cont::CellSetStructured<2> metaDataMesh;
  metaDataMesh.SetPointDimensions(vtkm::Id2{ pdims[1], pdims[2] });
  return metaDataMesh;
}

inline vtkm::cont::CellSetStructured<2> make_metaDataMesh2D(SumYAxis, const vtkm::Id3& pdims)
{
  vtkm::cont::CellSetStructured<2> metaDataMesh;
  metaDataMesh.SetPointDimensions(vtkm::Id2{ pdims[0], pdims[2] });
  return metaDataMesh;
}


VTKM_EXEC inline vtkm::Id3 compute_ijk(SumXAxis, const vtkm::Id3& executionSpaceIJK)
{
  return vtkm::Id3{ 0, executionSpaceIJK[0], executionSpaceIJK[1] };
}
VTKM_EXEC inline vtkm::Id3 compute_ijk(SumYAxis, const vtkm::Id3& executionSpaceIJK)
{
  return vtkm::Id3{ executionSpaceIJK[0], 0, executionSpaceIJK[1] };
}


VTKM_EXEC inline vtkm::Id3 compute_cdims(SumXAxis,
                                         const vtkm::Id3& executionSpacePDims,
                                         vtkm::Id numOfXPoints)
{
  return vtkm::Id3{ numOfXPoints - 1, executionSpacePDims[0] - 1, executionSpacePDims[1] - 1 };
}
VTKM_EXEC inline vtkm::Id3 compute_cdims(SumYAxis,
                                         const vtkm::Id3& executionSpacePDims,
                                         vtkm::Id numOfYPoints)
{
  return vtkm::Id3{ executionSpacePDims[0] - 1, numOfYPoints - 1, executionSpacePDims[1] - 1 };
}
VTKM_EXEC inline vtkm::Id3 compute_pdims(SumXAxis,
                                         const vtkm::Id3& executionSpacePDims,
                                         vtkm::Id numOfXPoints)
{
  return vtkm::Id3{ numOfXPoints, executionSpacePDims[0], executionSpacePDims[1] };
}
VTKM_EXEC inline vtkm::Id3 compute_pdims(SumYAxis,
                                         const vtkm::Id3& executionSpacePDims,
                                         vtkm::Id numOfYPoints)
{
  return vtkm::Id3{ executionSpacePDims[0], numOfYPoints, executionSpacePDims[1] };
}

VTKM_EXEC inline vtkm::Id compute_start(SumXAxis, const vtkm::Id3& ijk, const vtkm::Id3& dims)
{
  return (dims[0] * ijk[1]) + ((dims[0] * dims[1]) * ijk[2]);
}
VTKM_EXEC inline vtkm::Id compute_start(SumYAxis, const vtkm::Id3& ijk, const vtkm::Id3& dims)
{
  return ijk[0] + ((dims[0] * dims[1]) * ijk[2]);
}

VTKM_EXEC inline vtkm::Id4 compute_neighbor_starts(SumXAxis,
                                                   const vtkm::Id3& ijk,
                                                   const vtkm::Id3& pdims)
{
  //Optimized form of
  // return vtkm::Id4 { compute_start(sx, ijk, pdims),
  //                  compute_start(sx, ijk + vtkm::Id3{ 0, 1, 0 }, pdims),
  //                  compute_start(sx, ijk + vtkm::Id3{ 0, 0, 1 }, pdims),
  //                  compute_start(sx, ijk + vtkm::Id3{ 0, 1, 1 }, pdims) };
  const auto sliceSize = (pdims[0] * pdims[1]);
  const auto rowPos = (pdims[0] * ijk[1]);
  return vtkm::Id4{ rowPos + (sliceSize * ijk[2]),
                    rowPos + pdims[0] + (sliceSize * ijk[2]),
                    rowPos + (sliceSize * (ijk[2] + 1)),
                    rowPos + pdims[0] + (sliceSize * (ijk[2] + 1)) };
}
VTKM_EXEC inline vtkm::Id4 compute_neighbor_starts(SumYAxis,
                                                   const vtkm::Id3& ijk,
                                                   const vtkm::Id3& pdims)
{
  //Optimized form of
  // return vtkm::Id4{ compute_start(sy, ijk, pdims),
  //                   compute_start(sy, ijk + vtkm::Id3{ 1, 0, 0 }, pdims),
  //                   compute_start(sy, ijk + vtkm::Id3{ 0, 0, 1 }, pdims),
  //                   compute_start(sy, ijk + vtkm::Id3{ 1, 0, 1 }, pdims) };
  const auto sliceSize = (pdims[0] * pdims[1]);
  return vtkm::Id4{ ijk[0] + (sliceSize * ijk[2]),
                    ijk[0] + 1 + (sliceSize * ijk[2]),
                    ijk[0] + (sliceSize * (ijk[2] + 1)),
                    ijk[0] + 1 + (sliceSize * (ijk[2] + 1)) };
}



VTKM_EXEC inline vtkm::Id compute_inc(SumXAxis, const vtkm::Id3&)
{
  return 1;
}
VTKM_EXEC inline vtkm::Id compute_inc(SumYAxis, const vtkm::Id3& dims)
{
  return dims[0];
}

//----------------------------------------------------------------------------
template <typename WholeEdgeField>
VTKM_EXEC inline vtkm::UInt8 getEdgeCase(const WholeEdgeField& edges,
                                         const vtkm::Id4& startPos,
                                         vtkm::Id inc)
{
  vtkm::UInt8 e0 = edges.Get(startPos[0] + inc);
  vtkm::UInt8 e1 = edges.Get(startPos[1] + inc);
  vtkm::UInt8 e2 = edges.Get(startPos[2] + inc);
  vtkm::UInt8 e3 = edges.Get(startPos[3] + inc);
  return static_cast<vtkm::UInt8>(e0 | (e1 << 2) | (e2 << 4) | (e3 << 6));
}


//----------------------------------------------------------------------------

template <typename WholeEdgeField, typename FieldInPointId>
VTKM_EXEC inline bool computeTrimBounds(vtkm::Id rightMax,
                                        const WholeEdgeField& edges,
                                        const FieldInPointId& axis_mins,
                                        const FieldInPointId& axis_maxs,
                                        const vtkm::Id4& startPos,
                                        vtkm::Id inc,
                                        vtkm::Id& left,
                                        vtkm::Id& right)
{
  // find adjusted trim values.
  left = vtkm::Min(axis_mins[0], axis_mins[1]);
  left = vtkm::Min(left, axis_mins[2]);
  left = vtkm::Min(left, axis_mins[3]);

  right = vtkm::Max(axis_maxs[0], axis_maxs[1]);
  right = vtkm::Max(right, axis_maxs[2]);
  right = vtkm::Max(right, axis_maxs[3]);

  // The trim edges may need adjustment if the contour travels between rows
  // of edges (without intersecting these edges). This means checking
  // whether the trim faces at (left,right) made up of the edges intersect
  // the contour.
  if (left > rightMax && right == 0)
  {
    //verify that we have nothing to generate and early terminate.
    bool mins_same = (axis_mins[0] == axis_mins[1] && axis_mins[0] == axis_mins[2] &&
                      axis_mins[0] == axis_mins[3]);
    bool maxs_same = (axis_maxs[0] == axis_maxs[1] && axis_maxs[0] == axis_maxs[2] &&
                      axis_maxs[0] == axis_maxs[3]);

    left = 0;
    right = rightMax;
    if (mins_same && maxs_same)
    {
      vtkm::UInt8 e0 = edges.Get(startPos[0]);
      vtkm::UInt8 e1 = edges.Get(startPos[1]);
      vtkm::UInt8 e2 = edges.Get(startPos[2]);
      vtkm::UInt8 e3 = edges.Get(startPos[3]);
      if (e0 == e1 && e1 == e2 && e2 == e3)
      {
        //We have nothing to process in this row
        return false;
      }
    }
  }
  else
  {

    vtkm::UInt8 e0 = edges.Get(startPos[0] + (left * inc));
    vtkm::UInt8 e1 = edges.Get(startPos[1] + (left * inc));
    vtkm::UInt8 e2 = edges.Get(startPos[2] + (left * inc));
    vtkm::UInt8 e3 = edges.Get(startPos[3] + (left * inc));
    if ((e0 & 0x1) != (e1 & 0x1) || (e1 & 0x1) != (e2 & 0x1) || (e2 & 0x1) != (e3 & 0x1))
    {
      left = 0;
    }

    e0 = edges.Get(startPos[0] + (right * inc));
    e1 = edges.Get(startPos[1] + (right * inc));
    e2 = edges.Get(startPos[2] + (right * inc));
    e3 = edges.Get(startPos[3] + (right * inc));
    if ((e0 & 0x2) != (e1 & 0x2) || (e1 & 0x2) != (e2 & 0x2) || (e2 & 0x2) != (e3 & 0x2))
    {
      right = rightMax;
    }
  }

  return true;
}
}
}
}
#endif
