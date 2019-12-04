//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Contour_hxx
#define vtk_m_filter_Contour_hxx

#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

#include <vtkm/worklet/SurfaceNormals.h>

namespace vtkm
{
namespace filter
{

namespace
{

template <typename CellSetList>
inline bool IsCellSetStructured(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellset)
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
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
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
    hasCellFields = f.IsFieldCell();
  }

  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  using Vec3HandleType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
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
                                    vtkm::filter::ApplyPolicyCellSet(cells, policy),
                                    coords.GetData(),
                                    field,
                                    vertices,
                                    normals);
  }
  else
  {
    outputCells = this->Worklet.Run(&ivalues[0],
                                    static_cast<vtkm::Id>(ivalues.size()),
                                    vtkm::filter::ApplyPolicyCellSet(cells, policy),
                                    coords.GetData(),
                                    field,
                                    vertices);
  }

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
                                                this->Worklet.GetInterpolationEdgeIds());
    output.AddField(interpolationEdgeIdsField);
  }

  //assign the connectivity to the cell set
  output.SetCellSet(outputCells);

  //add the coordinates to the output dataset
  vtkm::cont::CoordinateSystem outputCoords("coordinates", vertices);
  output.AddCoordinateSystem(outputCoords);

  if (!hasCellFields)
  {
    this->Worklet.ReleaseCellMapArrays();
  }

  return output;
}
}
} // namespace vtkm::filter

#endif
