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
//============================================================================

#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/SurfaceNormals.h>

namespace vtkm
{
namespace filter
{

namespace
{

template <typename CellSetList>
bool IsCellSetStructured(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellset)
{
  if (cellset.template IsType<vtkm::cont::CellSetStructured<1>>() ||
      cellset.template IsType<vtkm::cont::CellSetStructured<2>>() ||
      cellset.template IsType<vtkm::cont::CellSetStructured<3>>())
  {
    return true;
  }
  return false;
}
} // anonymous namespace

//-----------------------------------------------------------------------------
inline VTKM_CONT MarchingCubes::MarchingCubes()
  : vtkm::filter::FilterDataSetWithField<MarchingCubes>()
  , IsoValues()
  , GenerateNormals(false)
  , ComputeFastNormalsForStructured(false)
  , ComputeFastNormalsForUnstructured(true)
  , NormalArrayName("normals")
  , Worklet()
{
  // todo: keep an instance of marching cubes worklet as a member variable
}

//-----------------------------------------------------------------------------
inline void MarchingCubes::SetNumberOfIsoValues(vtkm::Id num)
{
  if (num >= 0)
  {
    this->IsoValues.resize(static_cast<std::size_t>(num));
  }
}

//-----------------------------------------------------------------------------
inline vtkm::Id MarchingCubes::GetNumberOfIsoValues() const
{
  return static_cast<vtkm::Id>(this->IsoValues.size());
}

//-----------------------------------------------------------------------------
inline void MarchingCubes::SetIsoValue(vtkm::Id index, vtkm::Float64 v)
{
  std::size_t i = static_cast<std::size_t>(index);
  if (i >= this->IsoValues.size())
  {
    this->IsoValues.resize(i + 1);
  }
  this->IsoValues[i] = v;
}

//-----------------------------------------------------------------------------
inline void MarchingCubes::SetIsoValues(const std::vector<vtkm::Float64>& values)
{
  this->IsoValues = values;
}

//-----------------------------------------------------------------------------
inline vtkm::Float64 MarchingCubes::GetIsoValue(vtkm::Id index) const
{
  return this->IsoValues[static_cast<std::size_t>(index)];
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet MarchingCubes::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& device)
{
  if (fieldMeta.IsPointField() == false)
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  if (this->IsoValues.size() == 0)
  {
    throw vtkm::cont::ErrorFilterExecution("No iso-values provided.");
  }

  // Check the fields of the dataset to see what kinds of fields are present so
  // we can free the mapping arrays that won't be needed. A point field must
  // exist for this algorithm, so just check cells.
  const vtkm::Id numFields = input.GetNumberOfFields();
  bool hasCellFields = false;
  for (vtkm::Id fieldIdx = 0; fieldIdx < numFields && !hasCellFields; ++fieldIdx)
  {
    auto f = input.GetField(fieldIdx);
    if (f.GetAssociation() == vtkm::cont::Field::Association::CELL_SET)
    {
      hasCellFields = true;
    }
  }

  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());

  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  using Vec3HandleType = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>;
  Vec3HandleType vertices;
  Vec3HandleType normals;

  vtkm::cont::DataSet output;
  vtkm::cont::CellSetSingleType<> outputCells;

  std::vector<T> ivalues(this->IsoValues.size());
  for (std::size_t i = 0; i < ivalues.size(); ++i)
  {
    ivalues[i] = static_cast<T>(this->IsoValues[i]);
  }

  //not sold on this as we have to generate more signatures for the
  //worklet with the design
  //But I think we should get this to compile before we tinker with
  //a more efficient api

  bool generateHighQualityNormals = IsCellSetStructured(cells)
    ? !this->ComputeFastNormalsForStructured
    : !this->ComputeFastNormalsForUnstructured;
  if (this->GenerateNormals && generateHighQualityNormals)
  {
    outputCells = this->Worklet.Run(&ivalues[0],
                                    static_cast<vtkm::Id>(ivalues.size()),
                                    vtkm::filter::ApplyPolicy(cells, policy),
                                    coords.GetData(),
                                    field,
                                    vertices,
                                    normals,
                                    device);
  }
  else
  {
    outputCells = this->Worklet.Run(&ivalues[0],
                                    static_cast<vtkm::Id>(ivalues.size()),
                                    vtkm::filter::ApplyPolicy(cells, policy),
                                    coords.GetData(),
                                    field,
                                    vertices,
                                    device);
  }

  if (this->GenerateNormals)
  {
    if (!generateHighQualityNormals)
    {
      Vec3HandleType faceNormals;
      vtkm::worklet::FacetedSurfaceNormals faceted;
      faceted.Run(outputCells, vertices, faceNormals, device);

      vtkm::worklet::SmoothSurfaceNormals smooth;
      smooth.Run(outputCells, faceNormals, normals, device);
    }

    vtkm::cont::Field normalField(
      this->NormalArrayName, vtkm::cont::Field::Association::POINTS, normals);
    output.AddField(normalField);
  }

  //assign the connectivity to the cell set
  output.AddCellSet(outputCells);

  //add the coordinates to the output dataset
  vtkm::cont::CoordinateSystem outputCoords("coordinates", vertices);
  output.AddCoordinateSystem(outputCoords);

  if (!hasCellFields)
  {
    this->Worklet.ReleaseCellMapArrays();
  }

  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool MarchingCubes::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
{
  vtkm::cont::ArrayHandle<T> fieldArray;

  if (fieldMeta.IsPointField())
  {
    fieldArray = this->Worklet.ProcessPointField(input, device);
  }
  else if (fieldMeta.IsCellField())
  {
    fieldArray = this->Worklet.ProcessCellField(input, device);
  }
  else
  {
    return false;
  }

  //use the same meta data as the input so we get the same field name, etc.
  result.AddField(fieldMeta.AsField(fieldArray));
  return true;
}
}
} // namespace vtkm::filter
