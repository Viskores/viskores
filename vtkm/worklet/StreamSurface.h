//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_streamsurface_h
#define vtk_m_worklet_streamsurface_h

#include <typeinfo>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace worklet
{

class StreamSurface
{
public:
  //Helper worklet to count various things in each polyline.
  class CountPolylines : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VTKM_CONT
    CountPolylines() {}

    using ControlSignature = void(CellSetIn, WholeArrayInOut invalidCell, FieldOut ptsPerPolyline);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    _2 isValid,
                                    _3 ptsPerPolyline);
    using InputDomain = _1;

    template <typename CellShapeTag, typename InValidType>
    VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const vtkm::IdComponent& numPoints,
                              InValidType& invalidCell,
                              vtkm::Id& ptsPerPolyline) const
    {
      // We only support polylines that contain 2 or more points.
      if (shapeType.Id == vtkm::CELL_SHAPE_POLY_LINE && numPoints > 1)
        ptsPerPolyline = numPoints;
      else
      {
        invalidCell.Set(0, 1);
        ptsPerPolyline = 0;
      }
    }

  private:
  };

  //Helper worklet to determine number of triangles for each pair of polylines
  class CountTriangleConn : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    CountTriangleConn() {}

    using ControlSignature = void(FieldIn numPts0, FieldIn numPts1, FieldOut outConnCount);
    using ExecutionSignature = void(_1 numPts0, _2 numPts1, _3 outConnCount);
    using InputDomain = _1;

    VTKM_EXEC void operator()(const vtkm::Id& numPts0,
                              const vtkm::Id& numPts1,
                              vtkm::Id& outConnCount) const
    {
      if (numPts0 == numPts1)
        outConnCount = (numPts0 - 1) * 2 * 3;
      else if (numPts1 < numPts0)
        outConnCount = (numPts0 - 1) * 2 * 3 + (numPts1 - numPts0) * 3;
      else
        outConnCount = (numPts1 - 1) * 2 * 3 + (numPts0 - numPts1) * 3;
    }

  private:
  };

  //Helper worklet to generate the stream surface cells
  class GenerateCells : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    GenerateCells() {}

    using ControlSignature = void(FieldIn numPts0,
                                  FieldIn numPts1,
                                  FieldIn offset0,
                                  FieldIn offset1,
                                  FieldIn connOffset,
                                  WholeArrayOut outConn);
    using ExecutionSignature =
      void(_1 numPts0, _2 numPts1, _3 ptOffset0, _4 offset1, _5 connOffset, _6 outConn);
    using InputDomain = _1;

    template <typename OutConnType>
    VTKM_EXEC void operator()(const vtkm::Id& numPts0,
                              const vtkm::Id& numPts1,
                              const vtkm::Id& offset0,
                              const vtkm::Id& offset1,
                              const vtkm::Id& connOffset,
                              OutConnType& outConn) const
    {
      vtkm::Id idx0 = 0, idx1 = 0;
      vtkm::Id nextToLastIdx0 = numPts0 - 1;
      vtkm::Id nextToLastIdx1 = numPts1 - 1;
      vtkm::Id outIdx = connOffset;

      //There could be different numbers of points in the pairs of polylines.
      //Create pairs of triangles as far as possible.

      /*        polyline0    polyline1
       *
       *  idx0 + 1  x----------- x  idx1 + 1
       *            | \          |
       *            |   \  Tri2  |
       *            |     \      |
       *            |       \    |
       *            |  Tri1   \  |
       *            |           \|
       *  idx0 + 0  x ---------- x  idx1 + 0
       *
      */
      while (idx0 < nextToLastIdx0 && idx1 < nextToLastIdx1)
      {
        //Tri 1
        outConn.Set(outIdx + 0, offset0 + idx0 + 0);
        outConn.Set(outIdx + 1, offset1 + idx1 + 0);
        outConn.Set(outIdx + 2, offset0 + idx0 + 1);

        //Tri 2
        outConn.Set(outIdx + 3, offset0 + idx0 + 1);
        outConn.Set(outIdx + 4, offset1 + idx1 + 0);
        outConn.Set(outIdx + 5, offset1 + idx1 + 1);

        idx0++;
        idx1++;
        outIdx += 6;
      }

      // Same number of points in both polylines. We are done.
      if (numPts0 == numPts1)
        return;

      //If we have more points in one polyline, create a triangle fan
      //to complete the triangulation.
      //polyline0 is at the end, polyline1 still has more points.
      /*        polyline0    polyline1
       *
       *                         x  idx1 + 1
       *                        /|
       *                      /  |
       *                    /    |
       *                  /      |
       *                /  Tri   |
       *              /          |
       *  idx0 + 0  x ---------- x  idx1 + 0
       *
       */
      if (idx0 == nextToLastIdx0 && idx1 < nextToLastIdx1)
      {
        while (idx1 < nextToLastIdx1)
        {
          outConn.Set(outIdx + 0, offset0 + idx0 + 0);
          outConn.Set(outIdx + 1, offset1 + idx1 + 0);
          outConn.Set(outIdx + 2, offset1 + idx1 + 1);
          idx1++;
          outIdx += 3;
        }
      }

      //polyline1 is at the end, polyline0 still has more points.
      /*        polyline0    polyline1
       *
       *   idx0 + 1  x
       *             | \
       *             |   \
       *             |     \
       *             |       \
       *             |  Tri    \
       *             |           \
       *   idx0 + 0  x ---------- x  idx1 + 0
       *
       */
      else
      {
        while (idx0 < nextToLastIdx0)
        {
          outConn.Set(outIdx + 0, offset0 + idx0 + 0);
          outConn.Set(outIdx + 1, offset1 + idx1 + 0);
          outConn.Set(outIdx + 2, offset0 + idx0 + 1);
          idx0++;
          outIdx += 3;
        }
      }
    }

  private:
  };

  VTKM_CONT
  StreamSurface() {}

  VTKM_CONT
  void Run(const vtkm::cont::CoordinateSystem& coords,
           const vtkm::cont::DynamicCellSet& cellset,
           vtkm::cont::ArrayHandle<vtkm::Vec3f>& newPoints,
           vtkm::cont::CellSetSingleType<>& newCells)
  {
    using ExplCoordsType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

    if (!(coords.GetData().IsType<ExplCoordsType>() &&
          (cellset.IsSameType(vtkm::cont::CellSetExplicit<>()) ||
           cellset.IsSameType(vtkm::cont::CellSetSingleType<>()))))
    {
      throw vtkm::cont::ErrorBadValue("Stream surface requires polyline data.");
    }

    //Count number of polylines and make sure we ONLY have polylines
    vtkm::cont::ArrayHandle<vtkm::Id> ptsPerPolyline, invalidCell;
    vtkm::worklet::DispatcherMapTopology<CountPolylines> countInvoker;

    //We only care if there are ANY non-polyline cells. So use a one element array.
    //Any non-polyline cell will set the value to 1. No need to worry about race conditions
    //as the outcasts will all set it to the same value.
    invalidCell.Allocate(1);
    invalidCell.GetPortalControl().Set(0, 0);
    countInvoker.Invoke(cellset, invalidCell, ptsPerPolyline);

    if (invalidCell.GetPortalConstControl().Get(0) == 1)
      throw vtkm::cont::ErrorBadValue("Stream surface requires only polyline data.");

    vtkm::Id numPolylines = cellset.GetNumberOfCells();

    //Compute polyline offsets
    vtkm::cont::ArrayHandle<vtkm::Id> polylineOffset;
    vtkm::cont::Algorithm::ScanExclusive(ptsPerPolyline, polylineOffset);

    auto ptsPerPolyline0 = vtkm::cont::make_ArrayHandleView(ptsPerPolyline, 0, numPolylines - 1);
    auto ptsPerPolyline1 = vtkm::cont::make_ArrayHandleView(ptsPerPolyline, 1, numPolylines - 1);

    //Count the number of triangles to be generated
    vtkm::cont::ArrayHandle<vtkm::Id> triangleConnCount, triangleConnOffset;
    vtkm::worklet::DispatcherMapField<CountTriangleConn> countTriInvoker;
    countTriInvoker.Invoke(ptsPerPolyline0, ptsPerPolyline1, triangleConnCount);
    vtkm::cont::Algorithm::ScanExclusive(triangleConnCount, triangleConnOffset);

    //Surface points are same as input points.
    newPoints = coords.GetData().Cast<ExplCoordsType>();

    //Create surface triangles
    vtkm::Id numConnIds = vtkm::cont::Algorithm::Reduce(triangleConnCount, vtkm::Id(0));
    vtkm::cont::ArrayHandle<vtkm::Id> newConnectivity;
    newConnectivity.Allocate(numConnIds);
    vtkm::worklet::DispatcherMapField<GenerateCells> genCellsDisp;

    genCellsDisp.Invoke(ptsPerPolyline0,
                        ptsPerPolyline1,
                        vtkm::cont::make_ArrayHandleView(polylineOffset, 0, numPolylines - 1),
                        vtkm::cont::make_ArrayHandleView(polylineOffset, 1, numPolylines - 1),
                        triangleConnOffset,
                        newConnectivity);
    newCells.Fill(numConnIds, vtkm::CELL_SHAPE_TRIANGLE, 3, newConnectivity);
  }

private:
};
}
}

#endif //  vtk_m_worklet_streamsurface_h
