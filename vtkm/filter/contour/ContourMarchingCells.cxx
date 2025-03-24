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
#include <vtkm/filter/multi_block/MergeDataSets.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
//-----------------------------------------------------------------------------
vtkm::cont::DataSet ContourMarchingCells::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  switch (this->GetInputCellDimension())
  {
    case vtkm::filter::contour::ContourDimension::Auto:
    {
      vtkm::cont::DataSet output = this->DoExecuteDimension<3>(inDataSet);
      if (output.GetNumberOfCells() > 0)
      {
        return output;
      }
      output = this->DoExecuteDimension<2>(inDataSet);
      if (output.GetNumberOfCells() > 0)
      {
        return output;
      }
      output = this->DoExecuteDimension<1>(inDataSet);
      return output;
    }
    case vtkm::filter::contour::ContourDimension::All:
    {
      vtkm::cont::PartitionedDataSet allData;
      vtkm::cont::DataSet output = this->DoExecuteDimension<3>(inDataSet);
      if (output.GetNumberOfCells() > 0)
      {
        allData.AppendPartition(output);
      }
      output = this->DoExecuteDimension<2>(inDataSet);
      if (output.GetNumberOfCells() > 0)
      {
        allData.AppendPartition(output);
      }
      output = this->DoExecuteDimension<1>(inDataSet);
      if (output.GetNumberOfCells() > 0)
      {
        allData.AppendPartition(output);
      }
      if (allData.GetNumberOfPartitions() > 1)
      {
        vtkm::filter::multi_block::MergeDataSets merge;
        return merge.Execute(allData).GetPartition(0);
      }
      else if (allData.GetNumberOfPartitions() == 1)
      {
        return allData.GetPartition(0);
      }
      else
      {
        return output;
      }
    }
    case vtkm::filter::contour::ContourDimension::Polyhedra:
      return this->DoExecuteDimension<3>(inDataSet);
    case vtkm::filter::contour::ContourDimension::Polygons:
      return this->DoExecuteDimension<2>(inDataSet);
    case vtkm::filter::contour::ContourDimension::Lines:
      return this->DoExecuteDimension<1>(inDataSet);
    default:
      throw vtkm::cont::ErrorBadValue("Invalid value for ContourDimension.");
  }
}


template <vtkm::UInt8 Dims>
vtkm::cont::DataSet ContourMarchingCells::DoExecuteDimension(const vtkm::cont::DataSet& inDataSet)
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
      outputCells =
        worklet.Run<Dims>(ivalues, inputCells, inputCoords, concrete, vertices, normals);
    }
    else
    {
      outputCells = worklet.Run<Dims>(ivalues, inputCells, inputCoords, concrete, vertices);
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
