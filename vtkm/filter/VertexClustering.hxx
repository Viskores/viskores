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

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT VertexClustering::VertexClustering()
  : vtkm::filter::FilterDataSet<VertexClustering>()
  , NumberOfDivisions(256, 256, 256)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet VertexClustering::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& tag)
{
  // todo this code needs to obey the policy for what storage types
  // the output should use
  //need to compute bounds first
  vtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  vtkm::cont::DataSet outDataSet =
    this->Worklet.Run(vtkm::filter::ApplyPolicyUnstructured(input.GetCellSet(), policy),
                      input.GetCoordinateSystem(),
                      bounds,
                      this->GetNumberOfDivisions(),
                      tag);

  return outDataSet;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool VertexClustering::DoMapField(
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
}
