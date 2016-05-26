//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

namespace vtkm {
namespace filter {

namespace
{
  template<typename DerivedPolicy,
           typename DeviceAdapter>
  vtkm::Bounds compute_bounds(const vtkm::cont::CoordinateSystem& coords,
                              const vtkm::filter::PolicyBase<DerivedPolicy>&,
                              const DeviceAdapter& tag)
{
  typedef typename DerivedPolicy::CoordinateTypeList TypeList;
  typedef typename DerivedPolicy::CoordinateStorageList StorageList;
  return coords.GetBounds(tag, TypeList(), StorageList());
}

}

//-----------------------------------------------------------------------------
VertexClustering::VertexClustering():
  vtkm::filter::FilterDataSet<VertexClustering>(),
  NumberOfDivisions(256, 256, 256)
{

}

//-----------------------------------------------------------------------------
template<typename DerivedPolicy,
         typename DeviceAdapter>
vtkm::filter::ResultDataSet VertexClustering::DoExecute(const vtkm::cont::DataSet& input,
                                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                                        const DeviceAdapter& tag)
{
  // todo this code needs to obey the policy for what storage types
  // the output should use
  vtkm::worklet::VertexClustering clustering;

  //need to compute bounds first
  vtkm::Bounds bounds =
      compute_bounds(input.GetCoordinateSystem(), policy, tag);

  vtkm::cont::DataSet outDataSet = clustering.Run(vtkm::filter::ApplyPolicyUnstructured(input.GetCellSet(), policy),
                                                  vtkm::filter::ApplyPolicy(input.GetCoordinateSystem(), policy),
                                                  bounds,
                                                  this->GetNumberOfDivisions(),
                                                  tag);

  return vtkm::filter::ResultDataSet(outDataSet);
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
bool VertexClustering::DoMapField(vtkm::filter::ResultDataSet&,
                               const vtkm::cont::ArrayHandle<T, StorageType>&,
                               const vtkm::filter::FieldMetadata&,
                               const vtkm::filter::PolicyBase<DerivedPolicy>&,
                               const DeviceAdapter&)
{
  return false;
}

}
}
