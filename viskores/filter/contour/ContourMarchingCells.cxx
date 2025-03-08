//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/UnknownCellSet.h>

#include <viskores/filter/contour/ContourMarchingCells.h>
#include <viskores/filter/contour/worklet/ContourMarchingCells.h>


namespace viskores
{
namespace filter
{
namespace contour
{
//-----------------------------------------------------------------------------
viskores::cont::DataSet ContourMarchingCells::DoExecute(const viskores::cont::DataSet& inDataSet)
{
  viskores::worklet::ContourMarchingCells worklet;
  worklet.SetMergeDuplicatePoints(this->GetMergeDuplicatePoints());

  if (!this->GetFieldFromDataSet(inDataSet).IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution("Point field expected.");
  }

  if (this->IsoValues.empty())
  {
    throw viskores::cont::ErrorFilterExecution("No iso-values provided.");
  }

  //get the inputCells and coordinates of the dataset
  const viskores::cont::UnknownCellSet& inputCells = inDataSet.GetCellSet();
  const viskores::cont::CoordinateSystem& inputCoords =
    inDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  using Vec3HandleType = viskores::cont::ArrayHandle<viskores::Vec3f>;
  Vec3HandleType vertices;
  Vec3HandleType normals;

  viskores::cont::CellSetSingleType<> outputCells;

  auto resolveFieldType = [&](const auto& concrete)
  {
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
  viskores::cont::DataSet output = this->CreateResultCoordinateSystem(
    inDataSet, outputCells, inputCoords.GetName(), vertices, mapper);

  this->ExecuteGenerateNormals(output, normals);
  this->ExecuteAddInterpolationEdgeIds(output, worklet);


  return output;
}
} // namespace contour
} // namespace filter
} // namespace viskores
