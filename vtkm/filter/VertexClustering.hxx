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
inline VTKM_CONT vtkm::filter::Result VertexClustering::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& tag)
{
  // todo this code needs to obey the policy for what storage types
  // the output should use
  vtkm::worklet::VertexClustering clustering;

  //need to compute bounds first
  vtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds(
    typename DerivedPolicy::CoordinateTypeList(), typename DerivedPolicy::CoordinateStorageList());

  vtkm::cont::DataSet outDataSet =
    clustering.Run(vtkm::filter::ApplyPolicyUnstructured(input.GetCellSet(), policy),
                   vtkm::filter::ApplyPolicy(input.GetCoordinateSystem(), policy),
                   bounds,
                   this->GetNumberOfDivisions(),
                   tag);

  return vtkm::filter::Result(outDataSet);
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool VertexClustering::DoMapField(vtkm::filter::Result&,
                                                   const vtkm::cont::ArrayHandle<T, StorageType>&,
                                                   const vtkm::filter::FieldMetadata&,
                                                   const vtkm::filter::PolicyBase<DerivedPolicy>&,
                                                   const DeviceAdapter&)
{
  return false;
}
}
}
