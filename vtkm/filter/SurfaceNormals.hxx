//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/worklet/SurfaceNormals.h>

namespace vtkm
{
namespace filter
{

vtkm::filter::ResultField SurfaceNormals::Execute(const vtkm::cont::DataSet& input)
{
  return this->Execute(input, input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
}

template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
vtkm::filter::ResultField SurfaceNormals::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& points,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& device)
{
  VTKM_ASSERT(fieldMeta.IsPointField());

  const auto& cellset = input.GetCellSet(this->GetActiveCellSetIndex());

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> normals;
  vtkm::worklet::FacetedSurfaceNormals worklet;
  worklet.Run(vtkm::filter::ApplyPolicy(cellset, policy), points, normals, device);

  std::string outputName = this->GetOutputFieldName();
  if (outputName == "")
  {
    outputName = "normals";
  }

  return vtkm::filter::ResultField(
    input, normals, outputName, vtkm::cont::Field::ASSOC_CELL_SET, cellset.GetName());
}
}
} // vtkm::filter
