//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/filter/contour/ContourFlyingEdges.h>
#include <vtkm/filter/contour/worklet/ContourFlyingEdges.h>

namespace vtkm
{
namespace filter
{

using SupportedTypes = vtkm::List<vtkm::UInt8, vtkm::Int8, vtkm::Float32, vtkm::Float64>;

namespace contour
{
//-----------------------------------------------------------------------------
vtkm::cont::DataSet ContourFlyingEdges::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::worklet::ContourFlyingEdges worklet;
  worklet.SetMergeDuplicatePoints(this->GetMergeDuplicatePoints());

  if (!this->GetFieldFromDataSet(inDataSet).IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  if (this->IsoValues.empty())
  {
    throw vtkm::cont::ErrorFilterExecution("No iso-values provided.");
  }

  vtkm::cont::UnknownCellSet inCellSet = inDataSet.GetCellSet();
  const vtkm::cont::CoordinateSystem& inCoords =
    inDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  if (!inCellSet.template IsType<vtkm::cont::CellSetStructured<3>>())
  {
    throw vtkm::cont::ErrorFilterExecution("This filter is only available for 3-Dimensional "
                                           "Structured Cell Sets");
  }

  // Get the CellSet's known dynamic type
  const vtkm::cont::CellSetStructured<3>& inputCells =
    inDataSet.GetCellSet().AsCellSet<vtkm::cont::CellSetStructured<3>>();

  using Vec3HandleType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  Vec3HandleType vertices;
  Vec3HandleType normals;

  vtkm::cont::CellSetSingleType<> outputCells;

  auto resolveFieldType = [&](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    using IVType = std::conditional_t<(sizeof(T) > 4), vtkm::Float64, vtkm::FloatDefault>;
    std::vector<IVType> ivalues(this->IsoValues.size());
    for (std::size_t i = 0; i < ivalues.size(); ++i)
    {
      ivalues[i] = static_cast<IVType>(this->IsoValues[i]);
    }

    if (this->GenerateNormals && !this->GetComputeFastNormals())
    {
      outputCells = worklet.Run(ivalues, inputCells, inCoords, concrete, vertices, normals);
    }
    else
    {
      outputCells = worklet.Run(ivalues, inputCells, inCoords, concrete, vertices);
    }
  };

  this->GetFieldFromDataSet(inDataSet)
    .GetData()
    .CastAndCallForTypesWithFloatFallback<SupportedTypes, VTKM_DEFAULT_STORAGE_LIST>(
      resolveFieldType);

  auto mapper = [&](auto& result, const auto& f) { this->DoMapField(result, f, worklet); };
  vtkm::cont::DataSet output = this->CreateResultCoordinateSystem(
    inDataSet, outputCells, inCoords.GetName(), vertices, mapper);

  this->ExecuteGenerateNormals(output, normals);
  this->ExecuteAddInterpolationEdgeIds(output, worklet);

  return output;
}
} // namespace contour
} // namespace filter
} // namespace vtkm
