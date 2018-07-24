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
#ifndef vtk_m_filter_OscillatorSource_h
#define vtk_m_filter_OscillatorSource_h

#include "vtkm/filter/FilterField.h"
#include "vtkm/worklet/OscillatorSource.h"

namespace vtkm
{
namespace filter
{

/**\brief An analytical, time-varying array-source filter.
  *
  * This filter will create a new array (named "oscillation" by default)
  * that evaluates to a sum of time-varying Gaussian exponentials
  * specified in its configuration.
  */
class OscillatorSource : public vtkm::filter::FilterField<OscillatorSource>
{
public:
  VTKM_CONT
  OscillatorSource();

  VTKM_CONT
  void SetTime(vtkm::Float64 time);

  VTKM_CONT
  void AddPeriodic(vtkm::Float64 x,
                   vtkm::Float64 y,
                   vtkm::Float64 z,
                   vtkm::Float64 radius,
                   vtkm::Float64 omega,
                   vtkm::Float64 zeta);

  VTKM_CONT
  void AddDamped(vtkm::Float64 x,
                 vtkm::Float64 y,
                 vtkm::Float64 z,
                 vtkm::Float64 radius,
                 vtkm::Float64 omega,
                 vtkm::Float64 zeta);

  VTKM_CONT
  void AddDecaying(vtkm::Float64 x,
                   vtkm::Float64 y,
                   vtkm::Float64 z,
                   vtkm::Float64 radius,
                   vtkm::Float64 omega,
                   vtkm::Float64 zeta);

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                          const DeviceAdapter& tag);

private:
  vtkm::worklet::OscillatorSource Worklet;
};

template <>
class FilterTraits<OscillatorSource>
{
public:
  //Point Oscillator can only convert Float and Double Vec3 arrays
  using InputFieldTypeList = vtkm::TypeListTagFieldVec3;
};
}
}

#include "vtkm/filter/OscillatorSource.hxx"

#endif // vtk_m_filter_OscillatorSource_h
