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
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/filter/internal/CreateResult.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT OscillatorSource::OscillatorSource()
  : Worklet()
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetOutputFieldName("oscillation");
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void OscillatorSource::SetTime(vtkm::Float64 time)
{
  this->Worklet.SetTime(time);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void OscillatorSource::AddPeriodic(vtkm::Float64 x,
                                                    vtkm::Float64 y,
                                                    vtkm::Float64 z,
                                                    vtkm::Float64 radius,
                                                    vtkm::Float64 omega,
                                                    vtkm::Float64 zeta)
{
  this->Worklet.AddPeriodic(x, y, z, radius, omega, zeta);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void OscillatorSource::AddDamped(vtkm::Float64 x,
                                                  vtkm::Float64 y,
                                                  vtkm::Float64 z,
                                                  vtkm::Float64 radius,
                                                  vtkm::Float64 omega,
                                                  vtkm::Float64 zeta)
{
  this->Worklet.AddDamped(x, y, z, radius, omega, zeta);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void OscillatorSource::AddDecaying(vtkm::Float64 x,
                                                    vtkm::Float64 y,
                                                    vtkm::Float64 z,
                                                    vtkm::Float64 radius,
                                                    vtkm::Float64 omega,
                                                    vtkm::Float64 zeta)
{
  this->Worklet.AddDecaying(x, y, z, radius, omega, zeta);
}


//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet OscillatorSource::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter&)
{
  vtkm::cont::ArrayHandle<vtkm::Float64> outArray;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::OscillatorSource, DeviceAdapter> dispatcher(
    this->Worklet);

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
