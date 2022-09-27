//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_DataSetWithFieldFilter_h
#define vtk_m_filter_DataSetWithFieldFilter_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/PartitionedDataSet.h>

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED_SUPPRESS_BEGIN
template <class Derived>
class VTKM_DEPRECATED(1.9, "Use vtkm::filter::NewFilterField.") FilterDataSetWithField
  : public vtkm::filter::Filter<Derived>
{
public:
  VTKM_CONT
  FilterDataSetWithField();

  VTKM_CONT
  ~FilterDataSetWithField();

  VTKM_CONT
  void SetActiveCoordinateSystem(vtkm::Id index) { this->CoordinateSystemIndex = index; }

  VTKM_CONT
  vtkm::Id GetActiveCoordinateSystemIndex() const { return this->CoordinateSystemIndex; }

  ///@{
  /// Choose the field to operate on. Note, if
  /// `this->UseCoordinateSystemAsField` is true, then the active field is not used.
  VTKM_CONT
  void SetActiveField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
  {
    this->ActiveFieldName = name;
    this->ActiveFieldAssociation = association;
  }

  VTKM_CONT const std::string& GetActiveFieldName() const { return this->ActiveFieldName; }
  VTKM_CONT vtkm::cont::Field::Association GetActiveFieldAssociation() const
  {
    return this->ActiveFieldAssociation;
  }
  ///@}

  ///@{
  /// To simply use the active coordinate system as the field to operate on, set
  /// UseCoordinateSystemAsField to true.
  VTKM_CONT
  void SetUseCoordinateSystemAsField(bool val) { this->UseCoordinateSystemAsField = val; }
  VTKM_CONT
  bool GetUseCoordinateSystemAsField() const { return this->UseCoordinateSystemAsField; }
  ///@}

  //From the field we can extract the association component
  // Association::Any -> unable to map
  // Association::WholeDataSet -> (I think this is points)
  // Association::Points -> map using point mapping
  // Association::Cells -> how do we map this?
  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(const vtkm::cont::DataSet& input,
                                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

protected:
  vtkm::filter::FilterDataSetWithField<Derived>& operator=(
    const vtkm::filter::FilterDataSetWithField<Derived>&) = default;

  VTKM_CONT
  void CopyStateFrom(const FilterDataSetWithField<Derived>* filter) { *this = *filter; }

private:
  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(const vtkm::cont::DataSet& input,
                                                    const vtkm::cont::Field& field,
                                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

  //How do we specify float/double coordinate types?
  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(const vtkm::cont::DataSet& input,
                                                    const vtkm::cont::CoordinateSystem& field,
                                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

  std::string OutputFieldName;
  vtkm::Id CoordinateSystemIndex;
  std::string ActiveFieldName;
  vtkm::cont::Field::Association ActiveFieldAssociation;
  bool UseCoordinateSystemAsField;

  friend class vtkm::filter::Filter<Derived>;
};
VTKM_DEPRECATED_SUPPRESS_END
}
} // namespace vtkm::filter

#include <vtkm/filter/FilterDataSetWithField.hxx>

#endif // vtk_m_filter_DataSetWithFieldFilter_h
