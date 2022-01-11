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

#include <vtkm/worklet/SurfaceNormals.h>

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
                          vtkm::worklet::Contour& Worklet)
{
  if (field.IsFieldPoint())
  {
    auto array = field.GetData();

    auto functor = [&](auto concrete) {
      auto fieldArray = Worklet.ProcessPointField(concrete);
      result.AddPointField(field.GetName(), fieldArray);
    };
    array.CastAndCallForTypesWithFloatFallback<SupportedTypes, VTKM_DEFAULT_STORAGE_LIST>(functor);
    return true;
  }
  else if (field.IsFieldCell())
  {
    // Use the precompiled field permutation function.
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = Worklet.GetCellIdMap();
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
  vtkm::worklet::Contour Worklet;
  Worklet.SetMergeDuplicatePoints(this->GetMergeDuplicatePoints());

  if (!this->GetFieldFromDataSet(inDataSet).IsFieldPoint())
  {
    throw vtkm::cont::ErrorFilterExecution("Point fieldArray expected.");
  }

  if (this->IsoValues.empty())
  {
    throw vtkm::cont::ErrorFilterExecution("No iso-values provided.");
  }

  // Check the fields of the dataset to see what kinds of fields are present so
  // we can free the mapping arrays that won't be needed. A point fieldArray must
  // exist for this algorithm, so just check cells.
  const vtkm::Id numFields = inDataSet.GetNumberOfFields();
  bool hasCellFields = false;
  for (vtkm::Id fieldIdx = 0; fieldIdx < numFields && !hasCellFields; ++fieldIdx)
  {
    const auto& f = inDataSet.GetField(fieldIdx);
    hasCellFields = f.IsFieldCell();
  }

  //get the cells and coordinates of the dataset
  const vtkm::cont::UnknownCellSet& cells = inDataSet.GetCellSet();

  const vtkm::cont::CoordinateSystem& coords =
    inDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  const auto& fieldArray = this->GetFieldFromDataSet(inDataSet).GetData();

  using Vec3HandleType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  Vec3HandleType vertices;
  Vec3HandleType normals;

  vtkm::cont::DataSet output;
  vtkm::cont::CellSetSingleType<> outputCells;

  bool generateHighQualityNormals = IsCellSetStructured(cells)
    ? !this->ComputeFastNormalsForStructured
    : !this->ComputeFastNormalsForUnstructured;

  auto ResolveFieldType = [&, this](auto concrete) {
    std::vector<typename decltype(concrete)::ValueType> ivalues(IsoValues.begin(), IsoValues.end());

    if (this->GenerateNormals && generateHighQualityNormals)
    {
      outputCells = Worklet.Run(ivalues, cells, coords.GetData(), concrete, vertices, normals);
    }
    else
    {
      outputCells = Worklet.Run(ivalues, cells, coords.GetData(), concrete, vertices);
    }
  };

  fieldArray.CastAndCallForTypesWithFloatFallback<SupportedTypes, VTKM_DEFAULT_STORAGE_LIST>(
    ResolveFieldType);

  if (this->GenerateNormals)
  {
    if (!generateHighQualityNormals)
    {
      Vec3HandleType faceNormals;
      vtkm::worklet::FacetedSurfaceNormals faceted;
      faceted.Run(outputCells, vertices, faceNormals);

      vtkm::worklet::SmoothSurfaceNormals smooth;
      smooth.Run(outputCells, faceNormals, normals);
    }

    output.AddField(vtkm::cont::make_FieldPoint(this->NormalArrayName, normals));
  }

  if (this->AddInterpolationEdgeIds)
  {
    vtkm::cont::Field interpolationEdgeIdsField(InterpolationEdgeIdsArrayName,
                                                vtkm::cont::Field::Association::POINTS,
                                                Worklet.GetInterpolationEdgeIds());
    output.AddField(interpolationEdgeIdsField);
  }

  //assign the connectivity to the cell set
  output.SetCellSet(outputCells);

  //add the coordinates to the output dataset
  vtkm::cont::CoordinateSystem outputCoords("coordinates", vertices);
  output.AddCoordinateSystem(outputCoords);

  if (!hasCellFields)
  {
    Worklet.ReleaseCellMapArrays();
  }

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, Worklet); };
  MapFieldsOntoOutput(inDataSet, output, mapper);

  return output;
}
} // namespace contour
} // namespace filter
} // namespace vtkm
