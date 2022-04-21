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
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/mesh_info/MeshQuality.h>
#include <vtkm/filter/mesh_info/worklet/MeshQuality.h>

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
  { CellMetric::AspectGama, "aspectGamma" },
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
  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsFieldPoint())
  {
    throw vtkm::cont::ErrorBadValue("Active field for MeshQuality must be point coordinates. "
                                    "But the active field is not a point field.");
  }

  vtkm::cont::UnknownCellSet inputCellSet = input.GetCellSet();
  vtkm::worklet::MeshQuality qualityWorklet;

  if (this->MyMetric == CellMetric::RelativeSizeSquared ||
      this->MyMetric == CellMetric::ShapeAndSize)
  {
    vtkm::worklet::MeshQuality subWorklet;
    vtkm::FloatDefault totalArea;
    vtkm::FloatDefault totalVolume;

    auto resolveType = [&](const auto& concrete) {
      // use std::decay to remove const ref from the decltype of concrete.
      using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
      vtkm::cont::ArrayHandle<T> array;

      subWorklet.SetMetric(CellMetric::Area);
      this->Invoke(subWorklet, inputCellSet, concrete, array);
      totalArea = (vtkm::FloatDefault)vtkm::cont::Algorithm::Reduce(array, T{});

      subWorklet.SetMetric(CellMetric::Volume);
      this->Invoke(subWorklet, inputCellSet, concrete, array);
      totalVolume = (vtkm::FloatDefault)vtkm::cont::Algorithm::Reduce(array, T{});
    };
    this->CastAndCallVecField<3>(field, resolveType);

    vtkm::FloatDefault averageArea = 1.;
    vtkm::FloatDefault averageVolume = 1.;
    vtkm::Id numCells = inputCellSet.GetNumberOfCells();
    if (numCells > 0)
    {
      averageArea = totalArea / static_cast<vtkm::FloatDefault>(numCells);
      averageVolume = totalVolume / static_cast<vtkm::FloatDefault>(numCells);
    }
    qualityWorklet.SetAverageArea(averageArea);
    qualityWorklet.SetAverageVolume(averageVolume);
  }

  vtkm::cont::UnknownArrayHandle outArray;

  //Invoke the MeshQuality worklet
  auto resolveType = [&](const auto& concrete) {
    using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
    vtkm::cont::ArrayHandle<T> result;
    qualityWorklet.SetMetric(this->MyMetric);
    this->Invoke(qualityWorklet, inputCellSet, concrete, result);
    outArray = result;
  };
  this->CastAndCallVecField<3>(field, resolveType);

  return this->CreateResultFieldCell(input, this->GetOutputFieldName(), outArray);
}
} // namespace mesh_info
} // namespace filter
} // namespace vtkm

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED_SUPPRESS_BEGIN

vtkm::filter::mesh_info::CellMetric MeshQuality::ConvertCellMetric(
  vtkm::filter::CellMetric oldMetricEnum)
{
  switch (oldMetricEnum)
  {
    case vtkm::filter::CellMetric::AREA:
      return vtkm::filter::mesh_info::CellMetric::Area;
    case vtkm::filter::CellMetric::ASPECT_GAMMA:
      return vtkm::filter::mesh_info::CellMetric::AspectGama;
    case vtkm::filter::CellMetric::ASPECT_RATIO:
      return vtkm::filter::mesh_info::CellMetric::AspectRatio;
    case vtkm::filter::CellMetric::CONDITION:
      return vtkm::filter::mesh_info::CellMetric::Condition;
    case vtkm::filter::CellMetric::DIAGONAL_RATIO:
      return vtkm::filter::mesh_info::CellMetric::DiagonalRatio;
    case vtkm::filter::CellMetric::DIMENSION:
      return vtkm::filter::mesh_info::CellMetric::Dimension;
    case vtkm::filter::CellMetric::JACOBIAN:
      return vtkm::filter::mesh_info::CellMetric::Jacobian;
    case vtkm::filter::CellMetric::MAX_ANGLE:
      return vtkm::filter::mesh_info::CellMetric::MaxAngle;
    case vtkm::filter::CellMetric::MAX_DIAGONAL:
      return vtkm::filter::mesh_info::CellMetric::MaxDiagonal;
    case vtkm::filter::CellMetric::MIN_ANGLE:
      return vtkm::filter::mesh_info::CellMetric::MinAngle;
    case vtkm::filter::CellMetric::MIN_DIAGONAL:
      return vtkm::filter::mesh_info::CellMetric::MinDiagonal;
    case vtkm::filter::CellMetric::ODDY:
      return vtkm::filter::mesh_info::CellMetric::Oddy;
    case vtkm::filter::CellMetric::RELATIVE_SIZE_SQUARED:
      return vtkm::filter::mesh_info::CellMetric::RelativeSizeSquared;
    case vtkm::filter::CellMetric::SCALED_JACOBIAN:
      return vtkm::filter::mesh_info::CellMetric::ScaledJacobian;
    case vtkm::filter::CellMetric::SHAPE:
      return vtkm::filter::mesh_info::CellMetric::Shape;
    case vtkm::filter::CellMetric::SHAPE_AND_SIZE:
      return vtkm::filter::mesh_info::CellMetric::ShapeAndSize;
    case vtkm::filter::CellMetric::SHEAR:
      return vtkm::filter::mesh_info::CellMetric::Shear;
    case vtkm::filter::CellMetric::SKEW:
      return vtkm::filter::mesh_info::CellMetric::Skew;
    case vtkm::filter::CellMetric::STRETCH:
      return vtkm::filter::mesh_info::CellMetric::Stretch;
    case vtkm::filter::CellMetric::TAPER:
      return vtkm::filter::mesh_info::CellMetric::Taper;
    case vtkm::filter::CellMetric::VOLUME:
      return vtkm::filter::mesh_info::CellMetric::Volume;
    case vtkm::filter::CellMetric::WARPAGE:
      return vtkm::filter::mesh_info::CellMetric::Warpage;
    default:
      throw vtkm::cont::ErrorBadValue("Invalid mesh quality metric.");
  }
}

VTKM_DEPRECATED_SUPPRESS_END

} // namespace filter
} // namespace vtkm
