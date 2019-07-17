//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_MeshQuality_h
#define vtk_m_worklet_MeshQuality_h

#include "vtkm/exec/CellMeasure.h"
#include "vtkm/exec/cellmetrics/CellDiagonalRatioMetric.h"
#include "vtkm/exec/cellmetrics/CellEdgeRatioMetric.h"
#include "vtkm/worklet/WorkletMapTopology.h"

namespace vtkm
{
namespace worklet
{

/**
  * Worklet that computes mesh quality metric values for each cell in
  * the input mesh. A metric is specified per cell type in the calling filter,
  * and this metric is invoked over all cells of that cell type. An array of
  * the computed metric values (one per cell) is returned as output.
  */
template <typename MetricTagType>
class MeshQuality : public vtkm::worklet::WorkletMapPointToCell
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                WholeArrayIn counts,
                                WholeArrayIn metrics,
                                FieldInPoint pointCoords,
                                FieldOutCell metricOut);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename CellShapeType,
            typename PointCoordVecType,
            typename CountsArrayType,
            typename MetricsArrayType,
            typename OutType>
  VTKM_EXEC void operator()(CellShapeType shape,
                            const vtkm::IdComponent& numPoints,
                            const CountsArrayType& counts,
                            const MetricsArrayType& metrics,
                            const PointCoordVecType& pts,
                            OutType& metricValue) const
  {
    printf("shape.Id: %u\n", shape.Id);
    vtkm::UInt8 thisId = shape.Id;
    if (shape.Id == vtkm::CELL_SHAPE_POLYGON)
    {
      if (numPoints == 3)
        thisId = vtkm::CELL_SHAPE_TRIANGLE;
      else if (numPoints == 4)
        thisId = vtkm::CELL_SHAPE_QUAD;
    }
    switch (thisId)
    {
      vtkmGenericCellShapeMacro(
        metricValue = this->ComputeMetric<OutType>(
          numPoints, pts, counts.Get(shape.Id), CellShapeTag(), metrics.Get(CellShapeTag().Id)));
      default:
        this->RaiseError("Asked for metric of unknown cell type.");
        metricValue = OutType(0.0);
    }
  }

protected:
  template <typename OutType,
            typename PointCoordVecType,
            typename CellShapeType,
            typename CellMetricType>
  VTKM_EXEC OutType ComputeMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  const vtkm::Id& numShapes,
                                  CellShapeType tag,
                                  CellMetricType metric) const
  {
    UNUSED(numShapes);
    constexpr vtkm::IdComponent dims = vtkm::CellTraits<CellShapeType>::TOPOLOGICAL_DIMENSIONS;

    //Only compute the metric for 2D and 3D shapes; return 0 otherwise
    OutType metricValue = OutType(0.0);
    if (dims > 0)
    {
      switch (metric)
      {
        case MetricTagType::DIAGONAL_RATIO:
          metricValue =
            vtkm::exec::cellmetrics::CellDiagonalRatioMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::EDGE_RATIO:
          metricValue =
            vtkm::exec::cellmetrics::CellEdgeRatioMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::VOLUME:
          metricValue = vtkm::exec::CellMeasure<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::EMPTY:
          break;
        default:
          //Only call metric function if a metric is specified for this shape type
          this->RaiseError("Asked for unknown metric.");
      }
    }

    return metricValue;
  }
};

} // namespace worklet
} // namespace vtkm

#endif // vtk_m_worklet_MeshQuality_h
