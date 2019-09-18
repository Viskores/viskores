//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/worklet/DispatcherMapField.h>

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
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet OscillatorSource::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  vtkm::cont::ArrayHandle<vtkm::Float64> outArray;
  //todo, we need to use the policy to determine the valid conversions
  //that the dispatcher should do
  this->Invoke(this->Worklet, field, outArray);

  return CreateResult(inDataSet, outArray, this->GetOutputFieldName(), fieldMetadata);
}
}
} // namespace vtkm::filter
