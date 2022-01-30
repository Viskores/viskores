//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/vector_calculus/SurfaceNormals.h>
#include <vtkm/filter/vector_calculus/worklet/OrientNormals.h>
#include <vtkm/filter/vector_calculus/worklet/SurfaceNormals.h>
#include <vtkm/worklet/TriangleWinding.h>

namespace vtkm
{
namespace filter
{

namespace vector_calculus
{
namespace
{
inline std::string ComputePointNormalsName(const SurfaceNormals* filter)
{
  if (!filter->GetPointNormalsName().empty())
  {
    return filter->GetPointNormalsName();
  }
  else if (!filter->GetOutputFieldName().empty())
  {
    return filter->GetOutputFieldName();
  }
  else
  {
    return "Normals";
  }
}

inline std::string ComputeCellNormalsName(const SurfaceNormals* filter)
{
  if (!filter->GetCellNormalsName().empty())
  {
    return filter->GetCellNormalsName();
  }
  else if (!filter->GetGeneratePointNormals() && !filter->GetOutputFieldName().empty())
  {
    return filter->GetOutputFieldName();
  }
  else
  {
    return "Normals";
  }
}

} // internal

SurfaceNormals::SurfaceNormals()
{
  this->SetUseCoordinateSystemAsField(true);
}

vtkm::cont::DataSet SurfaceNormals::DoExecute(const vtkm::cont::DataSet& inputDataSet)
{
  auto field = this->GetFieldFromDataSet(inputDataSet);
  if (!field.IsFieldPoint())
  {
    // TODO: why only PointField?
    //  VTKM_ASSERT(fieldMeta.IsPointField());
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  if (!this->GenerateCellNormals && !this->GeneratePointNormals)
  {
    throw vtkm::cont::ErrorFilterExecution("No normals selected.");
  }

  const auto& inputCellSet = inputDataSet.GetCellSet();
  const auto& coords =
    inputDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()).GetDataAsMultiplexer();

  vtkm::cont::ArrayHandle<vtkm::Vec3f> faceNormals;
  vtkm::worklet::FacetedSurfaceNormals faceted;
  faceted.SetNormalize(this->NormalizeCellNormals);
  faceted.Run(inputCellSet,
              field.GetData().ResetTypes<SupportedTypes, VTKM_DEFAULT_STORAGE_LIST>(),
              faceNormals);

  vtkm::cont::DataSet outputDataSet;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> pointNormals;
  if (this->GeneratePointNormals)
  {
    vtkm::worklet::SmoothSurfaceNormals smooth;
    smooth.Run(inputCellSet, faceNormals, pointNormals);
    outputDataSet =
      this->CreateResultFieldPoint(inputDataSet, ComputePointNormalsName(this), pointNormals);
    if (this->GenerateCellNormals)
    {
      outputDataSet.AddField(vtkm::cont::make_FieldCell(ComputeCellNormalsName(this), faceNormals));
    }
  }
  else
  {
    outputDataSet =
      this->CreateResultFieldCell(inputDataSet, ComputeCellNormalsName(this), faceNormals);
  }

  if (this->AutoOrientNormals)
  {
    using Orient = vtkm::worklet::OrientNormals;

    if (this->GenerateCellNormals && this->GeneratePointNormals)
    {
      Orient::RunPointAndCellNormals(inputCellSet, coords, pointNormals, faceNormals);
    }
    else if (this->GenerateCellNormals)
    {
      Orient::RunCellNormals(inputCellSet, coords, faceNormals);
    }
    else if (this->GeneratePointNormals)
    {
      Orient::RunPointNormals(inputCellSet, coords, pointNormals);
    }

    if (this->FlipNormals)
    {
      if (this->GenerateCellNormals)
      {
        Orient::RunFlipNormals(faceNormals);
      }
      if (this->GeneratePointNormals)
      {
        Orient::RunFlipNormals(pointNormals);
      }
    }
  }

  if (this->Consistency && this->GenerateCellNormals)
  {
    auto newCells = vtkm::worklet::TriangleWinding::Run(inputCellSet, coords, faceNormals);
    outputDataSet.SetCellSet(newCells); // Overwrite the inputCellSet in the outputDataSet
  }

  return outputDataSet;
}
} // namespace vector_calculus
} // namespace filter
} // namespace vtkm
