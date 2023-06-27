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
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/filter/contour/ContourMarchingCells.h>
#include <vtkm/filter/contour/worklet/ContourMarchingCells.h>


namespace vtkm
{
namespace filter
{
namespace contour
{
//-----------------------------------------------------------------------------
vtkm::cont::DataSet ContourMarchingCells::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::worklet::ContourMarchingCells worklet;
  worklet.SetMergeDuplicatePoints(this->GetMergeDuplicatePoints());

  if (!this->GetFieldFromDataSet(inDataSet).IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  if (this->IsoValues.empty())
  {
    throw vtkm::cont::ErrorFilterExecution("No iso-values provided.");
  }

  //get the inputCells and coordinates of the dataset
  const vtkm::cont::UnknownCellSet& inputCells = inDataSet.GetCellSet();
  const vtkm::cont::CoordinateSystem& inputCoords =
    inDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  using Vec3HandleType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  Vec3HandleType vertices;
  Vec3HandleType normals;

  vtkm::cont::CellSetSingleType<> outputCells;

  auto resolveFieldType = [&](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    std::vector<T> ivalues(this->IsoValues.size());
    for (std::size_t i = 0; i < ivalues.size(); ++i)
    {
      ivalues[i] = static_cast<T>(this->IsoValues[i]);
    }

    if (this->GenerateNormals && !this->GetComputeFastNormals())
    {
      outputCells = worklet.Run(ivalues, inputCells, inputCoords, concrete, vertices, normals);
    }
    else
    {
      outputCells = worklet.Run(ivalues, inputCells, inputCoords, concrete, vertices);
    }
  };

  this->CastAndCallScalarField(this->GetFieldFromDataSet(inDataSet), resolveFieldType);

  auto mapper = [&](auto& result, const auto& f) { this->DoMapField(result, f, worklet); };
  vtkm::cont::DataSet output = this->CreateResultCoordinateSystem(
    inDataSet, outputCells, inputCoords.GetName(), vertices, mapper);

  this->ExecuteGenerateNormals(output, normals);
  this->ExecuteAddInterpolationEdgeIds(output, worklet);


  return output;
}
} // namespace contour
} // namespace filter
} // namespace vtkm
