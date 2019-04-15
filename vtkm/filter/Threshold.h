//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Threshold_h
#define vtk_m_filter_Threshold_h

#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/worklet/Threshold.h>

namespace vtkm
{
namespace filter
{

/// \brief Extracts cells where scalar value in cell satisfies threshold criterion
///
/// Extracts all cells from any dataset type that
/// satisfy a threshold criterion. A cell satisfies the criterion if the
/// scalar value of every point or cell satisfies the criterion. The
/// criterion takes the form of between two values. The output of this
/// filter is an permutation of the input dataset.
///
/// You can threshold either on point or cell fields
class Threshold : public vtkm::filter::FilterDataSetWithField<Threshold>
{
public:
  VTKM_CONT
  Threshold();

  VTKM_CONT
  void SetLowerThreshold(vtkm::Float64 value) { this->LowerValue = value; }
  VTKM_CONT
  void SetUpperThreshold(vtkm::Float64 value) { this->UpperValue = value; }

  VTKM_CONT
  vtkm::Float64 GetLowerThreshold() const { return this->LowerValue; }
  VTKM_CONT
  vtkm::Float64 GetUpperThreshold() const { return this->UpperValue; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  double LowerValue;
  double UpperValue;
  vtkm::worklet::Threshold Worklet;
};

template <>
class FilterTraits<Threshold>
{ //currently the threshold filter only works on scalar data.
public:
  using InputFieldTypeList = TypeListTagScalarAll;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Threshold.hxx>

#endif // vtk_m_filter_Threshold_h
