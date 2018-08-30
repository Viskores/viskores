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

#include <vtkm/filter/internal/CreateResult.h>
#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT PointElevation::PointElevation()
  : Worklet()
{
  this->SetOutputFieldName("elevation");
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointElevation::SetLowPoint(vtkm::Float64 x, vtkm::Float64 y, vtkm::Float64 z)
{
  this->Worklet.SetLowPoint(vtkm::make_Vec(x, y, z));
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointElevation::SetHighPoint(vtkm::Float64 x,
                                                   vtkm::Float64 y,
                                                   vtkm::Float64 z)
{
  this->Worklet.SetHighPoint(vtkm::make_Vec(x, y, z));
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointElevation::SetRange(vtkm::Float64 low, vtkm::Float64 high)
{
  this->Worklet.SetRange(low, high);
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet PointElevation::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter&)
{
  vtkm::cont::ArrayHandle<vtkm::Float64> outArray;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::PointElevation> dispatcher(this->Worklet);
  dispatcher.SetDevice(DeviceAdapter());

  //todo, we need to use the policy to determine the valid conversions
  //that the dispatcher should do
  dispatcher.Invoke(field, outArray);

  return internal::CreateResult(inDataSet,
                                outArray,
                                this->GetOutputFieldName(),
                                fieldMetadata.GetAssociation(),
                                fieldMetadata.GetCellSetName());
}
}
} // namespace vtkm::filter
