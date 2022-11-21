//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_FieldSelection_h
#define vtk_m_filter_FieldSelection_h

#include <vtkm/Pair.h>
#include <vtkm/cont/Field.h>

#include <vtkm/filter/vtkm_filter_core_export.h>

#include <initializer_list>
#include <memory>

namespace vtkm
{
namespace filter
{

/// A \c FieldSelection stores information about fields to map for input dataset to output
/// when a filter is executed. A \c FieldSelection object is passed to
/// `vtkm::filter::Filter::Execute` to execute the filter and map selected
/// fields. It is possible to easily construct \c FieldSelection that selects all or
/// none of the input fields.
class VTKM_FILTER_CORE_EXPORT FieldSelection
{
public:
  enum struct Mode
  {
    None,
    All,
    Select,
    Exclude
  };

  VTKM_CONT FieldSelection(Mode mode = Mode::Select);

  /// Use this constructor to create a field selection given a single field name
  /// \code{cpp}
  /// FieldSelection("field_name");
  /// \endcode
  VTKM_CONT FieldSelection(const std::string& field, Mode mode = Mode::Select);

  /// Use this constructor to create a field selection given a single field name
  /// \code{cpp}
  /// FieldSelection("field_name");
  /// \endcode
  VTKM_CONT FieldSelection(const char* field, Mode mode = Mode::Select);

  /// Use this constructor to create a field selection given a single name and association.
  /// \code{cpp}
  /// FieldSelection("field_name", vtkm::cont::Field::Association::Points)
  /// \endcode{cpp}
  VTKM_CONT FieldSelection(const std::string& field,
                           vtkm::cont::Field::Association association,
                           Mode mode = Mode::Select);

  /// Use this constructor to create a field selection given the field names.
  /// \code{cpp}
  /// FieldSelection({"field_one", "field_two"});
  /// \endcode
  VTKM_CONT FieldSelection(std::initializer_list<std::string> fields, Mode mode = Mode::Select);

  /// Use this constructor create a field selection given the field names and
  /// associations e.g.
  /// @code{cpp}
  /// using pair_type = std::pair<std::string, vtkm::cont::Field::Association>;
  /// FieldSelection({
  ///      pair_type{"field_one", vtkm::cont::Field::Association::Points},
  ///      pair_type{"field_two", vtkm::cont::Field::Association::Cells} });
  /// @endcode
  VTKM_CONT FieldSelection(
    std::initializer_list<std::pair<std::string, vtkm::cont::Field::Association>> fields,
    Mode mode = Mode::Select);

  /// Use this constructor create a field selection given the field names and
  /// associations e.g.
  /// @code{cpp}
  /// using pair_type = vtkm::Pair<std::string, vtkm::cont::Field::Association>;
  /// FieldSelection({
  ///      pair_type{"field_one", vtkm::cont::Field::Association::Points},
  ///      pair_type{"field_two", vtkm::cont::Field::Association::Cells} });
  /// @endcode
  VTKM_CONT FieldSelection(
    std::initializer_list<vtkm::Pair<std::string, vtkm::cont::Field::Association>> fields,
    Mode mode = Mode::Select);

  VTKM_CONT FieldSelection(const FieldSelection& src);
  VTKM_CONT FieldSelection(FieldSelection&& rhs);
  VTKM_CONT FieldSelection& operator=(const FieldSelection& src);
  VTKM_CONT FieldSelection& operator=(FieldSelection&& rhs);

  VTKM_CONT ~FieldSelection();

  /// Returns true if the input field should be mapped to the output
  /// dataset.
  VTKM_CONT
  bool IsFieldSelected(const vtkm::cont::Field& inputField) const
  {
    return this->IsFieldSelected(inputField.GetName(), inputField.GetAssociation());
  }

  VTKM_CONT bool IsFieldSelected(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any) const;

  ///@{
  /// Add fields to select or exclude. If no mode is specified, then the mode will follow
  /// that of `GetMode()`.
  VTKM_CONT void AddField(const vtkm::cont::Field& inputField)
  {
    this->AddField(inputField.GetName(), inputField.GetAssociation(), this->GetMode());
  }

  VTKM_CONT void AddField(const vtkm::cont::Field& inputField, Mode mode)
  {
    this->AddField(inputField.GetName(), inputField.GetAssociation(), mode);
  }

  VTKM_CONT
  void AddField(const std::string& fieldName,
                vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
  {
    this->AddField(fieldName, association, this->GetMode());
  }

  VTKM_CONT void AddField(const std::string& fieldName, Mode mode)
  {
    this->AddField(fieldName, vtkm::cont::Field::Association::Any, mode);
  }

  VTKM_CONT void AddField(const std::string& fieldName,
                          vtkm::cont::Field::Association association,
                          Mode mode);
  ///@}

  ///@{
  /// Returns the mode for a particular field. If the field as been added with `AddField`
  /// (or another means), then this will return `Select` or `Exclude`. If the field has
  /// not been added, `None` will be returned.
  VTKM_CONT Mode GetFieldMode(const vtkm::cont::Field& inputField) const
  {
    return this->GetFieldMode(inputField.GetName(), inputField.GetAssociation());
  }

  VTKM_CONT Mode GetFieldMode(
    const std::string& fieldName,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any) const;
  ///@}

  /// Returns true if the input field has been added to this selection.
  /// Note that depending on the mode of this selection, the result of HasField
  /// is not necessarily the same as IsFieldSelected. (If the mode is MODE_SELECT,
  /// then the result of the two will be the same.)
  VTKM_CONT bool HasField(const vtkm::cont::Field& inputField) const
  {
    return this->HasField(inputField.GetName(), inputField.GetAssociation());
  }

  VTKM_CONT bool HasField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any) const
  {
    return (this->GetFieldMode(name, association) != Mode::None);
  }

  /// Clear all fields added using `AddField`.
  VTKM_CONT void ClearFields();

  /// Gets the mode of the field selection. If `Select` mode is on, then only fields that have a
  /// `Select` mode are considered as selected. (All others are considered unselected.) Calling
  /// `AddField` in this mode will mark it as `Select`. If `Exclude` mode is on, then all fields
  /// are considered selected except those fields with an `Exclude` mode. Calling `AddField` in
  /// this mode will mark it as `Exclude`.
  VTKM_CONT Mode GetMode() const;

  /// Sets the mode of the field selection. If `Select` mode is on, then only fields that have a
  /// `Select` mode are considered as selected. (All others are considered unselected.) Calling
  /// `AddField` in this mode will mark it as `Select`. If `Exclude` mode is on, then all fields
  /// are considered selected except those fields with an `Exclude` mode. Calling `AddField` in
  /// this mode will mark it as `Exclude`.
  ///
  /// If the mode is set to `None`, then the field modes are cleared and the overall mode is set to
  /// `Select` (meaning none of the fields are initially selected). If the mode is set to `All`,
  /// then the field modes are cleared and the overall mode is set to `Exclude` (meaning all of the
  /// fields are initially selected).
  VTKM_CONT void SetMode(Mode val);

private:
  struct InternalStruct;
  std::unique_ptr<InternalStruct> Internals;
};

}
} // namespace vtkm::filter

#endif // vtk_m_filter_FieldSelection_h
