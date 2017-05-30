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

#ifndef vtk_m_filter_PointElevation_h
#define vtk_m_filter_PointElevation_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/PointElevation.h>

namespace vtkm
{
namespace filter
{

class PointElevation : public vtkm::filter::FilterField<PointElevation>
{
public:
  VTKM_CONT
  PointElevation();

  VTKM_CONT
  void SetLowPoint(vtkm::Float64 x, vtkm::Float64 y, vtkm::Float64 z);

  VTKM_CONT
  void SetHighPoint(vtkm::Float64 x, vtkm::Float64 y, vtkm::Float64 z);

  VTKM_CONT
  void SetRange(vtkm::Float64 low, vtkm::Float64 high);

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT vtkm::filter::ResultField DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<T, StorageType>& field,
    const vtkm::filter::FieldMetadata& fieldMeta,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
    const DeviceAdapter& tag);

private:
  vtkm::worklet::PointElevation Worklet;
};

template <>
class FilterTraits<PointElevation>
{
public:
  //Point Elevation can only convert Float and Double Vec3 arrays
  typedef vtkm::TypeListTagFieldVec3 InputFieldTypeList;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/PointElevation.hxx>

#endif // vtk_m_filter_PointElevation_h
