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

#include <vtkm/filter/vtkm_filter_export.h>

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
class VTKM_ALWAYS_EXPORT Threshold : public vtkm::filter::FilterDataSetWithField<Threshold>
{
public:
  using SupportedTypes = vtkm::TypeListScalarAll;

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
                            vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    if (fieldMeta.IsPointField())
    {
      //we copy the input handle to the result dataset, reusing the metadata
      result.AddField(fieldMeta.AsField(input));
      return true;
    }
    else if (fieldMeta.IsCellField())
    {
      vtkm::cont::ArrayHandle<T> out = this->Worklet.ProcessCellField(input);
      result.AddField(fieldMeta.AsField(out));
      return true;
    }
    else
    {
      return false;
    }
  }

private:
  double LowerValue = 0;
  double UpperValue = 0;
  vtkm::worklet::Threshold Worklet;
};

#ifndef vtkm_filter_Threshold_cxx
VTKM_FILTER_EXPORT_EXECUTE_METHOD(Threshold);
#endif
}
} // namespace vtkm::filter

#include <vtkm/filter/Threshold.hxx>

#endif // vtk_m_filter_Threshold_h
