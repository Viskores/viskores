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

#include "vtkm/worklet/CellMeasure.h"
#include "vtkm/worklet/WorkletMapTopology.h"
#include "vtkm/worklet/cellmetrics/CellAspectGammaMetric.h"
#include "vtkm/worklet/cellmetrics/CellAspectRatioMetric.h"
#include "vtkm/worklet/cellmetrics/CellConditionMetric.h"
#include "vtkm/worklet/cellmetrics/CellDiagonalRatioMetric.h"
#include "vtkm/worklet/cellmetrics/CellDimensionMetric.h"
#include "vtkm/worklet/cellmetrics/CellJacobianMetric.h"
#include "vtkm/worklet/cellmetrics/CellMaxAngleMetric.h"
#include "vtkm/worklet/cellmetrics/CellMaxDiagonalMetric.h"
#include "vtkm/worklet/cellmetrics/CellMinAngleMetric.h"
#include "vtkm/worklet/cellmetrics/CellMinDiagonalMetric.h"
#include "vtkm/worklet/cellmetrics/CellOddyMetric.h"
#include "vtkm/worklet/cellmetrics/CellRelativeSizeSquaredMetric.h"
#include "vtkm/worklet/cellmetrics/CellScaledJacobianMetric.h"
#include "vtkm/worklet/cellmetrics/CellShapeAndSizeMetric.h"
#include "vtkm/worklet/cellmetrics/CellShapeMetric.h"
#include "vtkm/worklet/cellmetrics/CellShearMetric.h"
#include "vtkm/worklet/cellmetrics/CellSkewMetric.h"
#include "vtkm/worklet/cellmetrics/CellStretchMetric.h"
#include "vtkm/worklet/cellmetrics/CellTaperMetric.h"
#include "vtkm/worklet/cellmetrics/CellWarpageMetric.h"

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
class MeshQuality : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint pointCoords,
                                FieldOutCell metricOut);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3);
  using InputDomain = _1;

  void SetMetric(MetricTagType m) { this->Metric = m; }
  void SetAverageArea(vtkm::FloatDefault a) { this->AverageArea = a; };
  void SetAverageVolume(vtkm::FloatDefault v) { this->AverageVolume = v; };

  template <typename CellShapeType, typename PointCoordVecType, typename OutType>
  VTKM_EXEC void operator()(CellShapeType shape,
                            const vtkm::IdComponent& numPoints,
                            //const CountsArrayType& counts,
                            //const MetricsArrayType& metrics,
                            //MetricTagType metric,
                            const PointCoordVecType& pts,
                            OutType& metricValue) const
  {
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
      vtkmGenericCellShapeMacro(metricValue =
                                  this->ComputeMetric<OutType>(numPoints, pts, CellShapeTag()));
      default:
        this->RaiseError("Asked for metric of unknown cell type.");
        metricValue = OutType(0.0);
    }
  }

protected:
  // data member
  MetricTagType Metric;
  vtkm::FloatDefault AverageArea;
  vtkm::FloatDefault AverageVolume;

  template <typename OutType, typename PointCoordVecType, typename CellShapeType>
  VTKM_EXEC OutType ComputeMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  CellShapeType tag) const
  {
    constexpr vtkm::IdComponent dims = vtkm::CellTraits<CellShapeType>::TOPOLOGICAL_DIMENSIONS;

    //Only compute the metric for 2D and 3D shapes; return 0 otherwise
    OutType metricValue = OutType(0.0);
    vtkm::FloatDefault average = (dims == 2 ? this->AverageArea : this->AverageVolume);

    if (dims > 0)
    {
      switch (this->Metric)
      {
        case MetricTagType::AREA:
          metricValue = vtkm::exec::CellMeasure<OutType>(numPts, pts, tag, *this);
          if (dims != 2)
            metricValue = 0.;
          break;
        case MetricTagType::ASPECT_GAMMA:
          metricValue =
            vtkm::worklet::cellmetrics::CellAspectGammaMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::ASPECT_RATIO:
          metricValue =
            vtkm::worklet::cellmetrics::CellAspectRatioMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::CONDITION:
          metricValue =
            vtkm::worklet::cellmetrics::CellConditionMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::DIAGONAL_RATIO:
          metricValue =
            vtkm::worklet::cellmetrics::CellDiagonalRatioMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::DIMENSION:
          metricValue =
            vtkm::worklet::cellmetrics::CellDimensionMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::JACOBIAN:
          metricValue =
            vtkm::worklet::cellmetrics::CellJacobianMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::MAX_ANGLE:
          metricValue =
            vtkm::worklet::cellmetrics::CellMaxAngleMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::MAX_DIAGONAL:
          metricValue =
            vtkm::worklet::cellmetrics::CellMaxDiagonalMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::MIN_ANGLE:
          metricValue =
            vtkm::worklet::cellmetrics::CellMinAngleMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::MIN_DIAGONAL:
          metricValue =
            vtkm::worklet::cellmetrics::CellMinDiagonalMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::ODDY:
          metricValue =
            vtkm::worklet::cellmetrics::CellOddyMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::RELATIVE_SIZE_SQUARED:
          metricValue = vtkm::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
            numPts, pts, static_cast<OutType>(average), tag, *this);
          break;
        case MetricTagType::SHAPE_AND_SIZE:
          metricValue = vtkm::worklet::cellmetrics::CellShapeAndSizeMetric<OutType>(
            numPts, pts, static_cast<OutType>(average), tag, *this);
          break;
        case MetricTagType::SCALED_JACOBIAN:
          metricValue =
            vtkm::worklet::cellmetrics::CellScaledJacobianMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::SHAPE:
          metricValue =
            vtkm::worklet::cellmetrics::CellShapeMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::SHEAR:
          metricValue =
            vtkm::worklet::cellmetrics::CellShearMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::SKEW:
          metricValue =
            vtkm::worklet::cellmetrics::CellSkewMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::STRETCH:
          metricValue =
            vtkm::worklet::cellmetrics::CellStretchMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::TAPER:
          metricValue =
            vtkm::worklet::cellmetrics::CellTaperMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::VOLUME:
          metricValue = vtkm::exec::CellMeasure<OutType>(numPts, pts, tag, *this);
          if (dims != 3)
            metricValue = 0.;
          break;
        case MetricTagType::WARPAGE:
          metricValue =
            vtkm::worklet::cellmetrics::CellWarpageMetric<OutType>(numPts, pts, tag, *this);
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
