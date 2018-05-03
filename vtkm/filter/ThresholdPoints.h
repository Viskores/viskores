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

#ifndef vtk_m_filter_ThresholdPoints_h
#define vtk_m_filter_ThresholdPoints_h

#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/worklet/ThresholdPoints.h>

namespace vtkm
{
namespace filter
{

class ThresholdPoints : public vtkm::filter::FilterDataSetWithField<ThresholdPoints>
{
public:
  VTKM_CONT
  ThresholdPoints();

  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the unused elements
  VTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  VTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  VTKM_CONT
  vtkm::Float64 GetLowerThreshold() const { return this->LowerValue; }
  VTKM_CONT
  void SetLowerThreshold(vtkm::Float64 value) { this->LowerValue = value; }

  VTKM_CONT
  vtkm::Float64 GetUpperThreshold() const { return this->UpperValue; }
  VTKM_CONT
  void SetUpperThreshold(vtkm::Float64 value) { this->UpperValue = value; }

  VTKM_CONT
  void SetThresholdBelow(const vtkm::Float64 value);
  VTKM_CONT
  void SetThresholdAbove(const vtkm::Float64 value);
  VTKM_CONT
  void SetThresholdBetween(const vtkm::Float64 value1, const vtkm::Float64 value2);

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                          const DeviceAdapter& tag);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                            const DeviceAdapter& tag);

private:
  double LowerValue;
  double UpperValue;
  int ThresholdType;

  bool CompactPoints;
  vtkm::filter::CleanGrid Compactor;
};

template <>
class FilterTraits<ThresholdPoints>
{ //currently the threshold filter only works on scalar data.
public:
  using InputFieldTypeList = TypeListTagScalarAll;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ThresholdPoints.hxx>

#endif // vtk_m_filter_ThresholdPoints_h
