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

    if (dims > 0)
    {
      vtkm::ErrorCode ec{ vtkm::ErrorCode::Success };
      switch (this->Metric)
      {
        case vtkm::filter::mesh_info::CellMetric::AspectGamma:
          metricValue =
            vtkm::worklet::cellmetrics::CellAspectGammaMetric<OutType>(numPts, pts, tag, ec);
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
