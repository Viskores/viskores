//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_FieldFilter_h
#define vtk_m_filter_FieldFilter_h

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

template <class Derived>
class FilterField : public vtkm::filter::Filter<Derived>
{
public:
  VTKM_CONT
  FilterField();

  VTKM_CONT
  ~FilterField();

  VTKM_CONT
  void SetOutputFieldName(const std::string& name) { this->OutputFieldName = name; }

  VTKM_CONT
  const std::string& GetOutputFieldName() const { return this->OutputFieldName; }

  //@{
  /// Choose the field to operate on. Note, if
  /// `this->UseCoordinateSystemAsField` is true, then the active field is not used.
  VTKM_CONT
  void SetActiveField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->ActiveFieldName = name;
    this->ActiveFieldAssociation = association;
  }

  VTKM_CONT const std::string& GetActiveFieldName() const { return this->ActiveFieldName; }
  VTKM_CONT vtkm::cont::Field::Association GetActiveFieldAssociation() const
  {
    return this->ActiveFieldAssociation;
  }
  //@}

  //@{
  /// To simply use the active coordinate system as the field to operate on, set
  /// UseCoordinateSystemAsField to true.
  VTKM_CONT
  void SetUseCoordinateSystemAsField(bool val) { this->UseCoordinateSystemAsField = val; }
  VTKM_CONT
  bool GetUseCoordinateSystemAsField() const { return this->UseCoordinateSystemAsField; }
  //@}


  //@{
  /// Select the coordinate system index to make active to use when processing the input
  /// DataSet. This is used primarily by the Filter to select the coordinate system
  /// to use as a field when \c UseCoordinateSystemAsField is true.
  VTKM_CONT
  void SetActiveCoordinateSystem(vtkm::Id index) { this->CoordinateSystemIndex = index; }

  VTKM_CONT
  vtkm::Id GetActiveCoordinateSystemIndex() const { return this->CoordinateSystemIndex; }
  //@}

  //@{
  /// These are provided to satisfy the Filter API requirements.
  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(const vtkm::cont::DataSet& input,
                                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(const vtkm::cont::DataSet& input,
                                                    const vtkm::cont::Field& field,
                                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(const vtkm::cont::DataSet& input,
                                                    const vtkm::cont::CoordinateSystem& field,
                                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);
  //@}

protected:
private:
  std::string OutputFieldName;
  vtkm::Id CoordinateSystemIndex;
  std::string ActiveFieldName;
  vtkm::cont::Field::Association ActiveFieldAssociation;
  bool UseCoordinateSystemAsField;

  friend class vtkm::filter::Filter<Derived>;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/FilterField.hxx>

#endif // vtk_m_filter_FieldFilter_h
