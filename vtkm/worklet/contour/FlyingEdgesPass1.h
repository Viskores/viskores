
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================


#ifndef vtk_m_worklet_contour_flyingedges_pass1_h
#define vtk_m_worklet_contour_flyingedges_pass1_h

#include <vtkm/worklet/contour/FlyingEdgesHelpers.h>

namespace vtkm
{
namespace worklet
{
namespace flying_edges
{

/*
* Understanding Pass1 in general
*
* PASS 1: Process all of the voxel edges that compose each row. Determine the
* edges case classification, count the number of edge intersections, and
* figure out where intersections along the row begins and ends
* (i.e., gather information for computational trimming).
*
* So in general the algorithm selects a primary axis to stride ( X or Y).
* It does this by forming a plane along the other two axes and marching
* over the sum/primary axis.
*
* So for SumXAxis, this means that we form a YZ plane and march the
* X axis along each point. As we march we are looking at the X axis edge
* that is formed by the current and next point.
*
* So for SumYAxis, this means that we form a XZ plane and march the
* Y axis along each point. As we march we are looking at the Y axis edge
* that is formed by the current and next point.
*
*/
template <typename T, typename AxisToSum>
struct ComputePass1 : public vtkm::worklet::WorkletPointNeighborhood
{

  vtkm::Id NumberOfPoints = 0;
  T IsoValue;

  ComputePass1() {}
  ComputePass1(T value, const vtkm::Id3& pdims)
    : NumberOfPoints(compute_num_pts(AxisToSum{}, pdims[0], pdims[1]))
    , IsoValue(value)
  {
  }

  using ControlSignature = void(CellSetIn,
                                FieldOut axis_sum,
                                FieldOut axis_min,
                                FieldOut axis_max,
                                WholeArrayInOut edgeData,
                                WholeArrayIn data);
  using ExecutionSignature = void(Boundary, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  template <typename WholeEdgeField, typename WholeDataField>
  VTKM_EXEC void operator()(const vtkm::exec::BoundaryState& boundary,
                            vtkm::Id3& axis_sum,
                            vtkm::Id& axis_min,
                            vtkm::Id& axis_max,
                            WholeEdgeField& edges,
                            const WholeDataField& field) const
  {

    const vtkm::Id3 ijk = compute_ijk(AxisToSum{}, boundary.IJK);
    const vtkm::Id3 dims = compute_pdims(AxisToSum{}, boundary.PointDimensions, NumberOfPoints);
    const vtkm::Id startPos = compute_start(AxisToSum{}, ijk, dims);
    const vtkm::Id offset = compute_inc(AxisToSum{}, dims);

    const T value = this->IsoValue;
    axis_min = this->NumberOfPoints;
    axis_max = 0;
    T s1 = field.Get(startPos);
    T s0 = s1;
    axis_sum = { 0, 0, 0 };
    for (vtkm::Id i = 0; i < NumberOfPoints - 1; ++i)
    {
      s0 = s1;
      s1 = field.Get(startPos + (offset * (i + 1)));

      // We don't explicit write the Below case as that ruins performance.
      // It is better to initially fill everything as Below and only
      // write the exceptions
      vtkm::UInt8 edgeCase = FlyingEdges3D::Below;
      if (s0 >= value)
      {
        edgeCase = FlyingEdges3D::LeftAbove;
      }
      if (s1 >= value)
      {
        edgeCase |= FlyingEdges3D::RightAbove;
      }
      if (edgeCase != FlyingEdges3D::Below)
      {
        edges.Set(startPos + (offset * i), edgeCase);
      }

      if (edgeCase == FlyingEdges3D::LeftAbove || edgeCase == FlyingEdges3D::RightAbove)
      {
        axis_sum[AxisToSum::xindex] += 1; // increment number of intersections along axis
        axis_max = i + 1;
        if (axis_min == this->NumberOfPoints)
        {
          axis_min = i;
        }
      }
    }
  }
};
}
}
}


#endif
