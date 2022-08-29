//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_FieldCollection_h
#define vtk_m_cont_internal_FieldCollection_h

#include <set>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

class VTKM_CONT_EXPORT FieldCollection
{
public:
  VTKM_CONT
  FieldCollection(std::initializer_list<vtkm::cont::Field::Association> validAssoc)
  {
    auto it = this->ValidAssoc.begin();
    for (const auto& item : validAssoc)
      it = this->ValidAssoc.insert(it, item);
  }

  VTKM_CONT
  FieldCollection(std::set<vtkm::cont::Field::Association>&& validAssoc)
    : ValidAssoc(std::move(validAssoc))
  {
  }

  VTKM_CONT
  void Clear() { this->Fields.clear(); }

  VTKM_CONT
  vtkm::IdComponent GetNumberOfFields() const
  {
    return static_cast<vtkm::IdComponent>(this->Fields.size());
  }

  VTKM_CONT void AddField(const Field& field);

  VTKM_CONT
  const vtkm::cont::Field& GetField(vtkm::Id index) const;

  VTKM_CONT
  vtkm::cont::Field& GetField(vtkm::Id index);

  VTKM_CONT
  bool HasField(const std::string& name,
                vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any) const
  {
    return (this->GetFieldIndex(name, assoc) != -1);
  }

  VTKM_CONT
  vtkm::Id GetFieldIndex(
    const std::string& name,
    vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any) const;

  VTKM_CONT
  const vtkm::cont::Field& GetField(
    const std::string& name,
    vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any) const;

  VTKM_CONT
  vtkm::cont::Field& GetField(
    const std::string& name,
    vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any);

private:
  struct FieldCompare
  {
    using Key = std::pair<std::string, vtkm::cont::Field::Association>;

    template <typename T>
    bool operator()(const T& a, const T& b) const
    {
      if (a.first == b.first)
        return a.second < b.second && a.second != vtkm::cont::Field::Association::Any &&
          b.second != vtkm::cont::Field::Association::Any;

      return a.first < b.first;
    }
  };

  std::map<FieldCompare::Key, vtkm::cont::Field, FieldCompare> Fields;
  std::set<vtkm::cont::Field::Association> ValidAssoc;
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_FieldCollection_h
