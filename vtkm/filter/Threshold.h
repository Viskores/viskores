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

#ifndef vtk_m_filter_Threshold_h
#define vtk_m_filter_Threshold_h

#include <vtkm/filter/DataSetWithFieldFilter.h>
#include <vtkm/worklet/Threshold.h>

namespace vtkm {
namespace filter {

class Threshold : public vtkm::filter::DataSetWithFieldFilter<Threshold>
{
public:
  VTKM_CONT_EXPORT
  Threshold();

  VTKM_CONT_EXPORT
  void SetThresholdValue(vtkm::Float64 value){ this->ThresholdValue = value; }

  VTKM_CONT_EXPORT
  vtkm::Float64 GetThresholdValue() const    { return this->ThresholdValue; }

  template<typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  vtkm::filter::DataSetResult DoExecute(const vtkm::cont::DataSet& input,
                                        const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                        const vtkm::filter::FieldMetadata& fieldMeta,
                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                        const DeviceAdapter& tag);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template<typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  bool DoMapField(vtkm::filter::DataSetResult& result,
                  const vtkm::cont::ArrayHandle<T, StorageType>& input,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                  const DeviceAdapter& tag);


private:
  double ThresholdValue;
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};

}
} // namespace vtkm::filter


#include <vtkm/filter/Threshold.hxx>

#endif // vtk_m_filter_Threshold_h