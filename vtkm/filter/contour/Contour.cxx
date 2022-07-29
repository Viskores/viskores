//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/filter/contour/worklet/Contour.h>
#include <vtkm/filter/vector_analysis/SurfaceNormals.h>

namespace vtkm
{
namespace filter
{

using SupportedTypes = vtkm::List<vtkm::UInt8, vtkm::Int8, vtkm::Float32, vtkm::Float64>;

namespace
{

inline bool IsCellSetStructured(const vtkm::cont::UnknownCellSet& cellset)
{
  if (cellset.template IsType<vtkm::cont::CellSetStructured<1>>() ||
      cellset.template IsType<vtkm::cont::CellSetStructured<2>>() ||
      cellset.template IsType<vtkm::cont::CellSetStructured<3>>())
  {
    return true;
  }
  return false;
}

VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          vtkm::worklet::Contour& worklet)
{
  if (field.IsFieldPoint())
  {
    auto functor = [&](const auto& concrete) {
      auto fieldArray = worklet.ProcessPointField(concrete);
      result.AddPointField(field.GetName(), fieldArray);
    };
    field.GetData()
      .CastAndCallForTypesWithFloatFallback<vtkm::TypeListField, VTKM_DEFAULT_STORAGE_LIST>(
        functor);
    return true;
  }
  else if (field.IsFieldCell())
  {
    // Use the precompiled field permutation function.
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = worklet.GetCellIdMap();
    return vtkm::filter::MapFieldPermutation(field, permutation, result);
  }
  else if (field.IsFieldGlobal())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}

} // anonymous namespace

namespace contour
{
//-----------------------------------------------------------------------------
void Contour::SetMergeDuplicatePoints(bool on)
{
  this->MergeDuplicatedPoints = on;
}

VTKM_CONT
bool Contour::GetMergeDuplicatePoints() const
{
  return MergeDuplicatedPoints;
}

//-----------------------------------------------------------------------------
vtkm::cont::DataSet Contour::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::worklet::Contour worklet;
  worklet.SetMergeDuplicatePoints(this->GetMergeDuplicatePoints());

  if (!this->GetFieldFromDataSet(inDataSet).IsFieldPoint())
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

  bool generateHighQualityNormals = IsCellSetStructured(inputCells)
    ? !this->ComputeFastNormalsForStructured
    : !this->ComputeFastNormalsForUnstructured;

  auto resolveFieldType = [&](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    std::vector<T> ivalues(this->IsoValues.size());
    for (std::size_t i = 0; i < ivalues.size(); ++i)
    {
      ivalues[i] = static_cast<T>(this->IsoValues[i]);
    }

    if (this->GenerateNormals && generateHighQualityNormals)
    {
      outputCells =
        worklet.Run(ivalues, inputCells, inputCoords.GetData(), concrete, vertices, normals);
    }
    else
    {
      outputCells = worklet.Run(ivalues, inputCells, inputCoords.GetData(), concrete, vertices);
    }
  };

  this->GetFieldFromDataSet(inDataSet)
    .GetData()
    .CastAndCallForTypesWithFloatFallback<SupportedTypes, VTKM_DEFAULT_STORAGE_LIST>(
      resolveFieldType);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  vtkm::cont::DataSet output = this->CreateResult(
    inDataSet, outputCells, vtkm::cont::CoordinateSystem{ "coordinates", vertices }, mapper);

  if (this->GenerateNormals)
  {
    if (!generateHighQualityNormals)
    {
      vtkm::filter::vector_analysis::SurfaceNormals surfaceNormals;
      surfaceNormals.SetPointNormalsName(this->NormalArrayName);
      surfaceNormals.SetGeneratePointNormals(true);
      output = surfaceNormals.Execute(output);
    }
    else
    {
      output.AddField(vtkm::cont::make_FieldPoint(this->NormalArrayName, normals));
    }
  }

  if (this->AddInterpolationEdgeIds)
  {
    vtkm::cont::Field interpolationEdgeIdsField(InterpolationEdgeIdsArrayName,
                                                vtkm::cont::Field::Association::Points,
                                                worklet.GetInterpolationEdgeIds());
    output.AddField(interpolationEdgeIdsField);
  }

  return output;
}
} // namespace contour
} // namespace filter
} // namespace vtkm
