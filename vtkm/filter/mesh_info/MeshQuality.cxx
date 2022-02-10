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
  { CellMetric::AREA, "area" },
  { CellMetric::ASPECT_GAMMA, "aspectGamma" },
  { CellMetric::ASPECT_RATIO, "aspectRatio" },
  { CellMetric::CONDITION, "condition" },
  { CellMetric::DIAGONAL_RATIO, "diagonalRatio" },
  { CellMetric::DIMENSION, "dimension" },
  { CellMetric::JACOBIAN, "jacobian" },
  { CellMetric::MAX_ANGLE, "maxAngle" },
  { CellMetric::MAX_DIAGONAL, "maxDiagonal" },
  { CellMetric::MIN_ANGLE, "minAngle" },
  { CellMetric::MIN_DIAGONAL, "minDiagonal" },
  { CellMetric::ODDY, "oddy" },
  { CellMetric::RELATIVE_SIZE_SQUARED, "relativeSizeSquared" },
  { CellMetric::SCALED_JACOBIAN, "scaledJacobian" },
  { CellMetric::SHAPE, "shape" },
  { CellMetric::SHAPE_AND_SIZE, "shapeAndSize" },
  { CellMetric::SHEAR, "shear" },
  { CellMetric::SKEW, "skew" },
  { CellMetric::STRETCH, "stretch" },
  { CellMetric::TAPER, "taper" },
  { CellMetric::VOLUME, "volume" },
  { CellMetric::WARPAGE, "warpage" }
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

  if (this->MyMetric == CellMetric::RELATIVE_SIZE_SQUARED ||
      this->MyMetric == CellMetric::SHAPE_AND_SIZE)
  {
    vtkm::worklet::MeshQuality subWorklet;
    vtkm::FloatDefault totalArea;
    vtkm::FloatDefault totalVolume;

    auto resolveType = [&](const auto& concrete) {
      // use std::decay to remove const ref from the decltype of concrete.
      using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
      vtkm::cont::ArrayHandle<T> array;

      subWorklet.SetMetric(CellMetric::AREA);
      this->Invoke(subWorklet, inputCellSet, concrete, array);
      totalArea = (vtkm::FloatDefault)vtkm::cont::Algorithm::Reduce(array, T{});

      subWorklet.SetMetric(CellMetric::VOLUME);
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
