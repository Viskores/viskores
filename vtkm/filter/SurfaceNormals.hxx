//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/internal/CreateResult.h>
#include <vtkm/worklet/SurfaceNormals.h>

namespace vtkm
{
namespace filter
{

namespace internal
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

inline SurfaceNormals::SurfaceNormals()
  : GenerateCellNormals(false)
  , NormalizeCellNormals(true)
  , GeneratePointNormals(true)
{
  this->SetUseCoordinateSystemAsField(true);
}

template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline vtkm::cont::DataSet SurfaceNormals::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& points,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& device)
{
  VTKM_ASSERT(fieldMeta.IsPointField());

  if (!this->GenerateCellNormals && !this->GeneratePointNormals)
  {
    throw vtkm::cont::ErrorFilterExecution("No normals selected.");
  }

  const auto& cellset = input.GetCellSet(this->GetActiveCellSetIndex());

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> faceNormals;
  vtkm::worklet::FacetedSurfaceNormals faceted;
  faceted.SetNormalize(this->NormalizeCellNormals);
  faceted.Run(vtkm::filter::ApplyPolicy(cellset, policy), points, faceNormals, device);

  vtkm::cont::DataSet result;
  if (this->GeneratePointNormals)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> pointNormals;
    vtkm::worklet::SmoothSurfaceNormals smooth;
    smooth.Run(vtkm::filter::ApplyPolicy(cellset, policy), faceNormals, pointNormals, device);

    result = internal::CreateResult(input,
                                    pointNormals,
                                    internal::ComputePointNormalsName(this),
                                    vtkm::cont::Field::Association::POINTS);
    if (this->GenerateCellNormals)
    {
      result.AddField(vtkm::cont::Field(internal::ComputeCellNormalsName(this),
                                        vtkm::cont::Field::Association::CELL_SET,
                                        cellset.GetName(),
                                        faceNormals));
    }
  }
  else
  {
    result = internal::CreateResult(input,
                                    faceNormals,
                                    internal::ComputeCellNormalsName(this),
                                    vtkm::cont::Field::Association::CELL_SET,
                                    cellset.GetName());
  }

  return result;
}
}
} // vtkm::filter
