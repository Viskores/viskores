//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/internal/FieldCollection.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

VTKM_CONT void FieldCollection::AddField(const Field& field)
{
  if (this->ValidAssoc.find(field.GetAssociation()) == this->ValidAssoc.end())
  {
    throw vtkm::cont::ErrorBadValue("Invalid association for field: " + field.GetName());
  }

  this->Fields[{ field.GetName(), field.GetAssociation() }] = field;
}

const vtkm::cont::Field& FieldCollection::GetField(vtkm::Id index) const
{
  VTKM_ASSERT((index >= 0) && (index < this->GetNumberOfFields()));
  auto it = this->Fields.cbegin();
  std::advance(it, index);
  return it->second;
}

vtkm::cont::Field& FieldCollection::GetField(vtkm::Id index)
{
  VTKM_ASSERT((index >= 0) && (index < this->GetNumberOfFields()));
  auto it = this->Fields.begin();
  std::advance(it, index);
  return it->second;
}

vtkm::Id FieldCollection::GetFieldIndex(const std::string& name,
                                        vtkm::cont::Field::Association assoc) const
{
  // Find the field with the given name and association. If the association is
  // `vtkm::cont::Field::Association::Any`, then the `Fields` object has a
  // special comparator that will match the field to any association.
  const auto it = this->Fields.find({ name, assoc });
  if (it != this->Fields.end())
  {
    return static_cast<vtkm::Id>(std::distance(this->Fields.begin(), it));
  }
  return -1;
}

const vtkm::cont::Field& FieldCollection::GetField(const std::string& name,
                                                   vtkm::cont::Field::Association assoc) const
{
  auto idx = this->GetFieldIndex(name, assoc);
  if (idx == -1)
  {
    throw vtkm::cont::ErrorBadValue("No field with requested name: " + name);
  }

  return this->GetField(idx);
}

vtkm::cont::Field& FieldCollection::GetField(const std::string& name,
                                             vtkm::cont::Field::Association assoc)
{
  auto idx = this->GetFieldIndex(name, assoc);
  if (idx == -1)
  {
    throw vtkm::cont::ErrorBadValue("No field with requested name: " + name);
  }

  return this->GetField(idx);
}

}
}
} //vtkm::cont::internal
