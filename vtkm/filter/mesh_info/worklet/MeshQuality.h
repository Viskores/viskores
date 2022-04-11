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

#include <vtkm/ErrorCode.h>
#include <vtkm/exec/CellMeasure.h>
#include <vtkm/filter/mesh_info/MeshQuality.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellAspectGammaMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellAspectRatioMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellConditionMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellDiagonalRatioMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellDimensionMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellJacobianMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellMaxAngleMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellMaxDiagonalMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellMinAngleMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellMinDiagonalMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellOddyMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellRelativeSizeSquaredMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellScaledJacobianMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellShapeAndSizeMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellShapeMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellShearMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellSkewMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellStretchMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellTaperMetric.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellWarpageMetric.h>
#include <vtkm/worklet/WorkletMapTopology.h>

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
class MeshQuality : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint pointCoords,
                                FieldOutCell metricOut);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3);
  using InputDomain = _1;

  void SetMetric(vtkm::filter::mesh_info::CellMetric m) { this->Metric = m; }
  void SetAverageArea(vtkm::FloatDefault a) { this->AverageArea = a; };
  void SetAverageVolume(vtkm::FloatDefault v) { this->AverageVolume = v; };

  template <typename CellShapeType, typename PointCoordVecType, typename OutType>
  VTKM_EXEC void operator()(CellShapeType shape,
                            const vtkm::IdComponent& numPoints,
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
        this->RaiseError(vtkm::ErrorString(vtkm::ErrorCode::CellNotFound));
        metricValue = OutType(0.0);
    }
  }

private:
  // data member
  vtkm::filter::mesh_info::CellMetric Metric{ vtkm::filter::mesh_info::CellMetric::None };
  vtkm::FloatDefault AverageArea{};
  vtkm::FloatDefault AverageVolume{};

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
      vtkm::ErrorCode ec{ vtkm::ErrorCode::Success };
      switch (this->Metric)
      {
        case vtkm::filter::mesh_info::CellMetric::Area:
          metricValue = vtkm::exec::CellMeasure<OutType>(numPts, pts, tag, ec);
          if (dims != 2)
            metricValue = 0.;
          break;
        case vtkm::filter::mesh_info::CellMetric::AspectGama:
          metricValue =
            vtkm::worklet::cellmetrics::CellAspectGammaMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::AspectRatio:
          metricValue =
            vtkm::worklet::cellmetrics::CellAspectRatioMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::Condition:
          metricValue =
            vtkm::worklet::cellmetrics::CellConditionMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::DiagonalRatio:
          metricValue =
            vtkm::worklet::cellmetrics::CellDiagonalRatioMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::Dimension:
          metricValue =
            vtkm::worklet::cellmetrics::CellDimensionMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::Jacobian:
          metricValue =
            vtkm::worklet::cellmetrics::CellJacobianMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::MaxAngle:
          metricValue =
            vtkm::worklet::cellmetrics::CellMaxAngleMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::MaxDiagonal:
          metricValue =
            vtkm::worklet::cellmetrics::CellMaxDiagonalMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::MinAngle:
          metricValue =
            vtkm::worklet::cellmetrics::CellMinAngleMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::MinDiagonal:
          metricValue =
            vtkm::worklet::cellmetrics::CellMinDiagonalMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::Oddy:
          metricValue = vtkm::worklet::cellmetrics::CellOddyMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::RelativeSizeSquared:
          metricValue = vtkm::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
            numPts, pts, static_cast<OutType>(average), tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::ShapeAndSize:
          metricValue = vtkm::worklet::cellmetrics::CellShapeAndSizeMetric<OutType>(
            numPts, pts, static_cast<OutType>(average), tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::ScaledJacobian:
          metricValue =
            vtkm::worklet::cellmetrics::CellScaledJacobianMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::Shape:
          metricValue = vtkm::worklet::cellmetrics::CellShapeMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::Shear:
          metricValue = vtkm::worklet::cellmetrics::CellShearMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::Skew:
          metricValue = vtkm::worklet::cellmetrics::CellSkewMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::Stretch:
          metricValue =
            vtkm::worklet::cellmetrics::CellStretchMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::Taper:
          metricValue = vtkm::worklet::cellmetrics::CellTaperMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::Volume:
          metricValue = vtkm::exec::CellMeasure<OutType>(numPts, pts, tag, ec);
          if (dims != 3)
            metricValue = 0.;
          break;
        case vtkm::filter::mesh_info::CellMetric::Warpage:
          metricValue =
            vtkm::worklet::cellmetrics::CellWarpageMetric<OutType>(numPts, pts, tag, ec);
          break;
        case vtkm::filter::mesh_info::CellMetric::None:
          break;
        default:
          //Only call metric function if a metric is specified for this shape type
          ec = vtkm::ErrorCode::InvalidCellMetric;
      }

      if (ec != vtkm::ErrorCode::Success)
      {
        this->RaiseError(vtkm::ErrorString(ec));
      }
    }

    return metricValue;
  }
};

} // namespace worklet
} // namespace vtkm

#endif // vtk_m_worklet_MeshQuality_h
