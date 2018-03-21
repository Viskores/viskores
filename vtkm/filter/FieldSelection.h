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
#ifndef vtk_m_filter_FieldSelection_h
#define vtk_m_filter_FieldSelection_h

#include <initializer_list>
#include <set>
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
    MODE_SELECTED
  };

  VTKM_CONT
  FieldSelection(ModeEnum mode = MODE_SELECTED)
    : Mode(mode)
  {
  }

  /// Use this constructor create a field selection given the field names e.g.
  /// `FieldSelection({"field_one", "field_two"})`.
  VTKM_CONT
  FieldSelection(std::initializer_list<std::string> fields)
    : Mode(MODE_SELECTED)
  {
    for (const std::string& afield : fields)
    {
      this->AddField(afield, vtkm::cont::Field::ASSOC_ANY);
    }
  }

  /// Use this constructor create a field selection given the field names and
  /// associations e.g.
  /// @code{cpp}
  /// FieldSelection({
  ///      {"field_one", vtkm::cont::Field::ASSOC_POINTS},
  ///      {"field_two", vtkm::cont::Field::ASSOC_CELL_SET} });
  /// @endcode
  VTKM_CONT
  FieldSelection(
    std::initializer_list<std::pair<std::string, vtkm::cont::Field::AssociationEnum>> fields)
    : Mode(MODE_SELECTED)
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
    vtkm::cont::Field::AssociationEnum association = vtkm::cont::Field::ASSOC_ANY) const
  {
    switch (this->Mode)
    {
      case MODE_NONE:
        return false;

      case MODE_ALL:
        return true;

      case MODE_SELECTED:
      default:
        if (this->Fields.find(Field(name, association)) != this->Fields.end())
        {
          return true;
        }
        // if not exact match, let's lookup for ASSOC_ANY.
        for (const auto& aField : this->Fields)
        {
          if (aField.Name == name)
          {
            if (aField.Association == vtkm::cont::Field::ASSOC_ANY ||
                association == vtkm::cont::Field::ASSOC_ANY)
            {
              return true;
            }
          }
        }
        return false;
    }
  }

  //@{
  /// Add fields to map. Note, if Mode is not MODE_SELECTED, then adding fields
  /// will have no impact of the fields that will be mapped.
  VTKM_CONT
  void AddField(const vtkm::cont::Field& inputField)
  {
    this->AddField(inputField.GetName(), inputField.GetAssociation());
  }

  VTKM_CONT
  void AddField(const std::string& fieldName,
                vtkm::cont::Field::AssociationEnum association = vtkm::cont::Field::ASSOC_ANY)
  {
    this->Fields.insert(Field(fieldName, association));
  }
  //@}

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
    vtkm::cont::Field::AssociationEnum Association;
    Field() = default;
    Field(const std::string& name, vtkm::cont::Field::AssociationEnum assoc)
      : Name(name)
      , Association(assoc)
    {
    }
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
