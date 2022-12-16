//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//=========================================================================
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/mesh_info/MeshQuality.h>
#include <vtkm/filter/mesh_info/MeshQualityArea.h>
#include <vtkm/filter/mesh_info/MeshQualityAspectGamma.h>
#include <vtkm/filter/mesh_info/MeshQualityAspectRatio.h>
#include <vtkm/filter/mesh_info/MeshQualityCondition.h>
#include <vtkm/filter/mesh_info/MeshQualityDiagonalRatio.h>
#include <vtkm/filter/mesh_info/MeshQualityDimension.h>
#include <vtkm/filter/mesh_info/MeshQualityJacobian.h>
#include <vtkm/filter/mesh_info/MeshQualityMaxAngle.h>
#include <vtkm/filter/mesh_info/MeshQualityMaxDiagonal.h>
#include <vtkm/filter/mesh_info/MeshQualityMinAngle.h>
#include <vtkm/filter/mesh_info/MeshQualityMinDiagonal.h>
#include <vtkm/filter/mesh_info/MeshQualityOddy.h>
#include <vtkm/filter/mesh_info/MeshQualityRelativeSizeSquared.h>
#include <vtkm/filter/mesh_info/MeshQualityScaledJacobian.h>
#include <vtkm/filter/mesh_info/MeshQualityShape.h>
#include <vtkm/filter/mesh_info/MeshQualityShapeAndSize.h>
#include <vtkm/filter/mesh_info/MeshQualityShear.h>
#include <vtkm/filter/mesh_info/MeshQualitySkew.h>
#include <vtkm/filter/mesh_info/MeshQualityStretch.h>
#include <vtkm/filter/mesh_info/MeshQualityTaper.h>
#include <vtkm/filter/mesh_info/MeshQualityVolume.h>
#include <vtkm/filter/mesh_info/MeshQualityWarpage.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{
namespace
{
//Names of the available cell metrics, for use in
//the output dataset fields
const std::map<CellMetric, std::string> MetricNames = {
  { CellMetric::Area, "area" },
  { CellMetric::AspectGamma, "aspectGamma" },
  { CellMetric::AspectRatio, "aspectRatio" },
  { CellMetric::Condition, "condition" },
  { CellMetric::DiagonalRatio, "diagonalRatio" },
  { CellMetric::Dimension, "dimension" },
  { CellMetric::Jacobian, "jacobian" },
  { CellMetric::MaxAngle, "maxAngle" },
  { CellMetric::MaxDiagonal, "maxDiagonal" },
  { CellMetric::MinAngle, "minAngle" },
  { CellMetric::MinDiagonal, "minDiagonal" },
  { CellMetric::Oddy, "oddy" },
  { CellMetric::RelativeSizeSquared, "relativeSizeSquared" },
  { CellMetric::ScaledJacobian, "scaledJacobian" },
  { CellMetric::Shape, "shape" },
  { CellMetric::ShapeAndSize, "shapeAndSize" },
  { CellMetric::Shear, "shear" },
  { CellMetric::Skew, "skew" },
  { CellMetric::Stretch, "stretch" },
  { CellMetric::Taper, "taper" },
  { CellMetric::Volume, "volume" },
  { CellMetric::Warpage, "warpage" }
};
} // anonymous namespace

VTKM_CONT MeshQuality::MeshQuality(CellMetric metric)
  : MyMetric(metric)
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetOutputFieldName(MetricNames.at(this->MyMetric));
}

VTKM_CONT vtkm::cont::DataSet MeshQuality::DoExecute(const vtkm::cont::DataSet& input)
{
  std::unique_ptr<vtkm::filter::FilterField> implementation;
  switch (this->MyMetric)
  {
    case vtkm::filter::mesh_info::CellMetric::Area:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityArea);
      break;
    case vtkm::filter::mesh_info::CellMetric::AspectGamma:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityAspectGamma);
      break;
    case vtkm::filter::mesh_info::CellMetric::AspectRatio:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityAspectRatio);
      break;
    case vtkm::filter::mesh_info::CellMetric::Condition:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityCondition);
      break;
    case vtkm::filter::mesh_info::CellMetric::DiagonalRatio:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityDiagonalRatio);
      break;
    case vtkm::filter::mesh_info::CellMetric::Dimension:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityDimension);
      break;
    case vtkm::filter::mesh_info::CellMetric::Jacobian:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityJacobian);
      break;
    case vtkm::filter::mesh_info::CellMetric::MaxAngle:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityMaxAngle);
      break;
    case vtkm::filter::mesh_info::CellMetric::MaxDiagonal:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityMaxDiagonal);
      break;
    case vtkm::filter::mesh_info::CellMetric::MinAngle:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityMinAngle);
      break;
    case vtkm::filter::mesh_info::CellMetric::MinDiagonal:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityMinDiagonal);
      break;
    case vtkm::filter::mesh_info::CellMetric::Oddy:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityOddy);
      break;
    case vtkm::filter::mesh_info::CellMetric::RelativeSizeSquared:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityRelativeSizeSquared);
      break;
    case vtkm::filter::mesh_info::CellMetric::ScaledJacobian:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityScaledJacobian);
      break;
    case vtkm::filter::mesh_info::CellMetric::Shape:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityShape);
      break;
    case vtkm::filter::mesh_info::CellMetric::ShapeAndSize:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityShapeAndSize);
      break;
    case vtkm::filter::mesh_info::CellMetric::Shear:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityShear);
      break;
    case vtkm::filter::mesh_info::CellMetric::Skew:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualitySkew);
      break;
    case vtkm::filter::mesh_info::CellMetric::Stretch:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityStretch);
      break;
    case vtkm::filter::mesh_info::CellMetric::Taper:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityTaper);
      break;
    case vtkm::filter::mesh_info::CellMetric::Volume:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityVolume);
      break;
    case vtkm::filter::mesh_info::CellMetric::Warpage:
      implementation.reset(new vtkm::filter::mesh_info::MeshQualityWarpage);
      break;
    case vtkm::filter::mesh_info::CellMetric::None:
      // Nothing to do
      return input;
  }

  VTKM_ASSERT(implementation);

  implementation->SetOutputFieldName(this->GetOutputFieldName());
  implementation->SetActiveCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  return implementation->Execute(input);
}
} // namespace mesh_info
} // namespace filter
} // namespace vtkm
