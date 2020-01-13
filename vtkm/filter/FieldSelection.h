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

#include <initializer_list>
#include <set>
#include <vtkm/Pair.h>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace filter
{

/// A \c FieldSelection stores information about fields to map for input dataset to output
/// when a filter is executed. A \c FieldSelection object is passed to
/// `vtkm::filter::Filter::Execute` to execute the filter and map selected
/// fields. It is possible to easily construct \c FieldSelection that selects all or
/// none of the input fields.
class FieldSelection
{
public:
  enum ModeEnum
  {
    MODE_NONE,
    MODE_ALL,
    MODE_SELECT,
    MODE_EXCLUDE
  };

  VTKM_CONT
  FieldSelection(ModeEnum mode = MODE_SELECT)
    : Mode(mode)
  {
  }

  /// Use this constructor to create a field selection given a single field name
  /// \code{cpp}
  /// FieldSelection("field_name");
  /// \endcode
  VTKM_CONT
  FieldSelection(const std::string& field, ModeEnum mode = MODE_SELECT)
    : Mode(mode)
  {
    this->AddField(field, vtkm::cont::Field::Association::ANY);
  }

  /// Use this constructor to create a field selection given a single field name
  /// \code{cpp}
  /// FieldSelection("field_name");
  /// \endcode
  VTKM_CONT
  FieldSelection(const char* field, ModeEnum mode = MODE_SELECT)
    : Mode(mode)
  {
    this->AddField(field, vtkm::cont::Field::Association::ANY);
  }

  /// Use this constructor to create a field selection given a single name and association.
  /// \code{cpp}
  /// FieldSelection("field_name", vtkm::cont::Field::Association::POINTS)
  /// \endcode{cpp}
  VTKM_CONT
  FieldSelection(const std::string& field,
                 vtkm::cont::Field::Association association,
                 ModeEnum mode = MODE_SELECT)
    : Mode(mode)
  {
    this->AddField(field, association);
  }

  /// Use this constructor to create a field selection given the field names.
  /// \code{cpp}
  /// FieldSelection({"field_one", "field_two"});
  /// \endcode
  VTKM_CONT
  FieldSelection(std::initializer_list<std::string> fields, ModeEnum mode = MODE_SELECT)
    : Mode(mode)
  {
    for (const std::string& afield : fields)
    {
      this->AddField(afield, vtkm::cont::Field::Association::ANY);
    }
  }

  /// Use this constructor create a field selection given the field names and
  /// associations e.g.
  /// @code{cpp}
  /// using pair_type = std::pair<std::string, vtkm::cont::Field::Association>;
  /// FieldSelection({
  ///      pair_type{"field_one", vtkm::cont::Field::Association::POINTS},
  ///      pair_type{"field_two", vtkm::cont::Field::Association::CELL_SET} });
  /// @endcode
  VTKM_CONT
  FieldSelection(
    std::initializer_list<std::pair<std::string, vtkm::cont::Field::Association>> fields,
    ModeEnum mode = MODE_SELECT)
    : Mode(mode)
  {
    for (const auto& item : fields)
    {
      this->AddField(item.first, item.second);
    }
  }

  /// Use this constructor create a field selection given the field names and
  /// associations e.g.
  /// @code{cpp}
  /// using pair_type = vtkm::Pair<std::string, vtkm::cont::Field::Association>;
  /// FieldSelection({
  ///      pair_type{"field_one", vtkm::cont::Field::Association::POINTS},
  ///      pair_type{"field_two", vtkm::cont::Field::Association::CELL_SET} });
  /// @endcode
  VTKM_CONT
  FieldSelection(
    std::initializer_list<vtkm::Pair<std::string, vtkm::cont::Field::Association>> fields,
    ModeEnum mode = MODE_SELECT)
    : Mode(mode)
  {
    for (const auto& item : fields)
    {
      this->AddField(item.first, item.second);
    }
  }

  VTKM_CONT
  ~FieldSelection() {}

  /// Returns true if the input field should be mapped to the output
  /// dataset.
  VTKM_CONT
  bool IsFieldSelected(const vtkm::cont::Field& inputField) const
  {
    return this->IsFieldSelected(inputField.GetName(), inputField.GetAssociation());
  }

  bool IsFieldSelected(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY) const
  {
    switch (this->Mode)
    {
      case MODE_NONE:
        return false;

      case MODE_ALL:
        return true;

      case MODE_SELECT:
      default:
        return this->HasField(name, association);

      case MODE_EXCLUDE:
        return !this->HasField(name, association);
    }
  }

  //@{
  /// Add fields to map. Note, if Mode is not MODE_SELECT, then adding fields
  /// will have no impact of the fields that will be mapped.
  VTKM_CONT
  void AddField(const vtkm::cont::Field& inputField)
  {
    this->AddField(inputField.GetName(), inputField.GetAssociation());
  }

  VTKM_CONT
  void AddField(const std::string& fieldName,
                vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->Fields.insert(Field(fieldName, association));
  }
  //@}

  /// Returns true if the input field has been added to this selection.
  /// Note that depending on the mode of this selection, the result of HasField
  /// is not necessarily the same as IsFieldSelected. (If the mode is MODE_SELECT,
  /// then the result of the two will be the same.)
  VTKM_CONT
  bool HasField(const vtkm::cont::Field& inputField) const
  {
    return this->HasField(inputField.GetName(), inputField.GetAssociation());
  }

  bool HasField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY) const
  {
    if (this->Fields.find(Field(name, association)) != this->Fields.end())
    {
      return true;
    }
    // if not exact match, let's lookup for Association::ANY.
    for (const auto& aField : this->Fields)
    {
      if (aField.Name == name)
      {
        if (aField.Association == vtkm::cont::Field::Association::ANY ||
            association == vtkm::cont::Field::Association::ANY)
        {
          return true;
        }
      }
    }
    return false;
  }

  /// Clear all fields added using `AddField`.
  VTKM_CONT
  void ClearFields() { this->Fields.clear(); }

  VTKM_CONT
  ModeEnum GetMode() const { return this->Mode; }
  void SetMode(ModeEnum val) { this->Mode = val; }

private:
  ModeEnum Mode; ///< mode

  struct Field
  {
    std::string Name;
    vtkm::cont::Field::Association Association;
    Field() = default;
    Field(const std::string& name, vtkm::cont::Field::Association assoc)
      : Name(name)
      , Association(assoc)
    {
    }

    Field(const Field&) = default;
    Field& operator=(const Field&) = default;

    bool operator<(const Field& other) const
    {
      return (this->Association == other.Association) ? (this->Name < other.Name)
                                                      : (this->Association < other.Association);
    }
  };

  std::set<Field> Fields;
};
}
}


#endif // vtk_m_filter_FieldSelection_h
