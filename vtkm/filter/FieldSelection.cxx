//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/FieldSelection.h>

#include <map>

namespace
{

struct FieldDescription
{
  std::string Name;
  vtkm::cont::Field::Association Association;
  FieldDescription() = default;
  FieldDescription(const std::string& name, vtkm::cont::Field::Association assoc)
    : Name(name)
    , Association(assoc)
  {
  }

  FieldDescription(const FieldDescription&) = default;
  FieldDescription& operator=(const FieldDescription&) = default;

  bool operator<(const FieldDescription& other) const
  {
    return (this->Association == other.Association) ? (this->Name < other.Name)
                                                    : (this->Association < other.Association);
  }
};

} // anonymous namespace

namespace vtkm
{
namespace filter
{

struct FieldSelection::InternalStruct
{
  Mode ModeType;

  std::map<FieldDescription, Mode> Fields;
};

FieldSelection::FieldSelection(Mode mode)
  : Internals(new InternalStruct)
{
  this->SetMode(mode);
}

FieldSelection::FieldSelection(const std::string& field, Mode mode)
  : FieldSelection(mode)
{
  this->AddField(field, vtkm::cont::Field::Association::Any);
}

FieldSelection::FieldSelection(const char* field, Mode mode)
  : FieldSelection(mode)
{
  this->AddField(field, vtkm::cont::Field::Association::Any);
}

FieldSelection::FieldSelection(const std::string& field,
                               vtkm::cont::Field::Association association,
                               Mode mode)
  : FieldSelection(mode)
{
  this->AddField(field, association);
}

FieldSelection::FieldSelection(std::initializer_list<std::string> fields, Mode mode)
  : FieldSelection(mode)
{
  for (const std::string& afield : fields)
  {
    this->AddField(afield, vtkm::cont::Field::Association::Any);
  }
}

FieldSelection::FieldSelection(
  std::initializer_list<std::pair<std::string, vtkm::cont::Field::Association>> fields,
  Mode mode)
  : FieldSelection(mode)
{
  for (const auto& item : fields)
  {
    this->AddField(item.first, item.second);
  }
}

FieldSelection::FieldSelection(
  std::initializer_list<vtkm::Pair<std::string, vtkm::cont::Field::Association>> fields,
  Mode mode)
  : FieldSelection(mode)
{
  for (const auto& item : fields)
  {
    this->AddField(item.first, item.second);
  }
}

FieldSelection::FieldSelection(const FieldSelection& src)
  : Internals(new InternalStruct(*src.Internals))
{
}

FieldSelection::FieldSelection(FieldSelection&&) = default;

FieldSelection& FieldSelection::operator=(const FieldSelection& src)
{
  *this->Internals = *src.Internals;
  return *this;
}

FieldSelection& FieldSelection::operator=(FieldSelection&&) = default;

FieldSelection::~FieldSelection() = default;

bool FieldSelection::IsFieldSelected(const std::string& name,
                                     vtkm::cont::Field::Association association) const
{
  switch (this->GetFieldMode(name, association))
  {
    case Mode::Select:
      return true;
    case Mode::Exclude:
      return false;
    default:
      switch (this->GetMode())
      {
        case Mode::None:
        case Mode::Select:
          // Fields are not selected unless explicitly set
          return false;
        case Mode::All:
        case Mode::Exclude:
          // Fields are selected unless explicitly excluded
          return true;
      }
  }
  VTKM_ASSERT(false && "Internal error. Unexpected mode");
  return false;
}

void FieldSelection::AddField(const std::string& fieldName,
                              vtkm::cont::Field::Association association,
                              Mode mode)
{
  this->Internals->Fields[FieldDescription(fieldName, association)] = mode;
}

FieldSelection::Mode FieldSelection::GetFieldMode(const std::string& fieldName,
                                                  vtkm::cont::Field::Association association) const
{
  auto iter = this->Internals->Fields.find(FieldDescription(fieldName, association));
  if (iter != this->Internals->Fields.end())
  {
    return iter->second;
  }

  // if not exact match, let's lookup for Association::Any.
  for (const auto& aField : this->Internals->Fields)
  {
    if (aField.first.Name == fieldName)
    {
      if (aField.first.Association == vtkm::cont::Field::Association::Any ||
          association == vtkm::cont::Field::Association::Any)
      {
        return aField.second;
      }
    }
  }

  return Mode::None;
}

void FieldSelection::ClearFields()
{
  this->Internals->Fields.clear();
}

FieldSelection::Mode FieldSelection::GetMode() const
{
  return this->Internals->ModeType;
}

void FieldSelection::SetMode(Mode val)
{
  switch (val)
  {
    case Mode::None:
      this->ClearFields();
      this->Internals->ModeType = Mode::Select;
      break;
    case Mode::All:
      this->ClearFields();
      this->Internals->ModeType = Mode::Exclude;
      break;
    case Mode::Select:
    case Mode::Exclude:
      this->Internals->ModeType = val;
      break;
  }
}

}
} // namespace vtkm::filter
