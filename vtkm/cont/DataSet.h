//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_DataSet_h
#define vtk_m_cont_DataSet_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/VariantArrayHandle.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT DataSet
{
public:
  VTKM_CONT void Clear();

  VTKM_CONT void AddField(const Field& field) { this->Fields.push_back(field); }

  VTKM_CONT
  const vtkm::cont::Field& GetField(vtkm::Id index) const;

  VTKM_CONT
  bool HasField(const std::string& name,
                vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::ANY) const
  {
    bool found;
    this->FindFieldIndex(name, assoc, found);
    return found;
  }

  VTKM_CONT
  vtkm::Id GetFieldIndex(
    const std::string& name,
    vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::ANY) const;

  VTKM_CONT
  const vtkm::cont::Field& GetField(
    const std::string& name,
    vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::ANY) const
  {
    return this->GetField(this->GetFieldIndex(name, assoc));
  }

  VTKM_CONT
  const vtkm::cont::Field& GetCellField(const std::string& name) const
  {
    return this->GetField(name, vtkm::cont::Field::Association::CELL_SET);
  }

  VTKM_CONT
  const vtkm::cont::Field& GetPointField(const std::string& name) const
  {
    return this->GetField(name, vtkm::cont::Field::Association::POINTS);
  }

  VTKM_CONT
  void AddCoordinateSystem(const vtkm::cont::CoordinateSystem& cs)
  {
    this->CoordSystems.push_back(cs);
  }

  VTKM_CONT
  const vtkm::cont::CoordinateSystem& GetCoordinateSystem(vtkm::Id index = 0) const;

  VTKM_CONT
  bool HasCoordinateSystem(const std::string& name) const
  {
    bool found;
    this->FindCoordinateSystemIndex(name, found);
    return found;
  }

  VTKM_CONT
  vtkm::Id GetCoordinateSystemIndex(const std::string& name) const;

  VTKM_CONT
  const vtkm::cont::CoordinateSystem& GetCoordinateSystem(const std::string& name) const
  {
    return this->GetCoordinateSystem(this->GetCoordinateSystemIndex(name));
  }

  VTKM_CONT
  void AddCellSet(const vtkm::cont::DynamicCellSet& cellSet) { this->CellSets.push_back(cellSet); }

  template <typename CellSetType>
  VTKM_CONT void AddCellSet(const CellSetType& cellSet)
  {
    VTKM_IS_CELL_SET(CellSetType);
    this->CellSets.push_back(vtkm::cont::DynamicCellSet(cellSet));
  }

  VTKM_CONT
  vtkm::cont::DynamicCellSet GetCellSet(vtkm::Id index = 0) const
  {
    VTKM_ASSERT((index >= 0) && (index < this->GetNumberOfCellSets()));
    return this->CellSets[static_cast<std::size_t>(index)];
  }

  VTKM_CONT
  bool HasCellSet(const std::string& name) const
  {
    bool found;
    this->FindCellSetIndex(name, found);
    return found;
  }

  VTKM_CONT
  vtkm::Id GetCellSetIndex(const std::string& name) const;

  VTKM_CONT
  vtkm::cont::DynamicCellSet GetCellSet(const std::string& name) const
  {
    return this->GetCellSet(this->GetCellSetIndex(name));
  }

  VTKM_CONT
  vtkm::IdComponent GetNumberOfCellSets() const
  {
    return static_cast<vtkm::IdComponent>(this->CellSets.size());
  }

  VTKM_CONT
  vtkm::IdComponent GetNumberOfFields() const
  {
    return static_cast<vtkm::IdComponent>(this->Fields.size());
  }

  VTKM_CONT
  vtkm::IdComponent GetNumberOfCoordinateSystems() const
  {
    return static_cast<vtkm::IdComponent>(this->CoordSystems.size());
  }

  /// Copies the structure i.e. coordinates systems and cellsets from the source
  /// dataset. The fields are left unchanged.
  VTKM_CONT
  void CopyStructure(const vtkm::cont::DataSet& source);

  VTKM_CONT
  void PrintSummary(std::ostream& out) const;

private:
  std::vector<vtkm::cont::CoordinateSystem> CoordSystems;
  std::vector<vtkm::cont::Field> Fields;
  std::vector<vtkm::cont::DynamicCellSet> CellSets;

  VTKM_CONT
  vtkm::Id FindFieldIndex(const std::string& name,
                          vtkm::cont::Field::Association association,
                          bool& found) const;

  VTKM_CONT
  vtkm::Id FindCoordinateSystemIndex(const std::string& name, bool& found) const;

  VTKM_CONT
  vtkm::Id FindCellSetIndex(const std::string& name, bool& found) const;
};

} // namespace cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes
namespace vtkm
{
namespace cont
{

template <typename FieldTypeList = VTKM_DEFAULT_TYPE_LIST_TAG,
          typename CellSetTypesList = VTKM_DEFAULT_CELL_SET_LIST_TAG>
struct SerializableDataSet
{
  SerializableDataSet() = default;

  explicit SerializableDataSet(const vtkm::cont::DataSet& dataset)
    : DataSet(dataset)
  {
  }

  vtkm::cont::DataSet DataSet;
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename FieldTypeList, typename CellSetTypesList>
struct Serialization<vtkm::cont::SerializableDataSet<FieldTypeList, CellSetTypesList>>
{
private:
  using Type = vtkm::cont::SerializableDataSet<FieldTypeList, CellSetTypesList>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& serializable)
  {
    const auto& dataset = serializable.DataSet;

    vtkm::IdComponent numberOfCoordinateSystems = dataset.GetNumberOfCoordinateSystems();
    vtkmdiy::save(bb, numberOfCoordinateSystems);
    for (vtkm::IdComponent i = 0; i < numberOfCoordinateSystems; ++i)
    {
      vtkmdiy::save(bb, dataset.GetCoordinateSystem(i));
    }

    vtkm::IdComponent numberOfCellSets = dataset.GetNumberOfCellSets();
    vtkmdiy::save(bb, numberOfCellSets);
    for (vtkm::IdComponent i = 0; i < numberOfCellSets; ++i)
    {
      vtkmdiy::save(bb, dataset.GetCellSet(i).ResetCellSetList(CellSetTypesList{}));
    }

    vtkm::IdComponent numberOfFields = dataset.GetNumberOfFields();
    vtkmdiy::save(bb, numberOfFields);
    for (vtkm::IdComponent i = 0; i < numberOfFields; ++i)
    {
      vtkmdiy::save(bb, vtkm::cont::SerializableField<FieldTypeList>(dataset.GetField(i)));
    }
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& serializable)
  {
    auto& dataset = serializable.DataSet;
    dataset = {}; // clear

    vtkm::IdComponent numberOfCoordinateSystems = 0;
    vtkmdiy::load(bb, numberOfCoordinateSystems);
    for (vtkm::IdComponent i = 0; i < numberOfCoordinateSystems; ++i)
    {
      vtkm::cont::CoordinateSystem coords;
      vtkmdiy::load(bb, coords);
      dataset.AddCoordinateSystem(coords);
    }

    vtkm::IdComponent numberOfCellSets = 0;
    vtkmdiy::load(bb, numberOfCellSets);
    for (vtkm::IdComponent i = 0; i < numberOfCellSets; ++i)
    {
      vtkm::cont::DynamicCellSetBase<CellSetTypesList> cells;
      vtkmdiy::load(bb, cells);
      dataset.AddCellSet(vtkm::cont::DynamicCellSet(cells));
    }

    vtkm::IdComponent numberOfFields = 0;
    vtkmdiy::load(bb, numberOfFields);
    for (vtkm::IdComponent i = 0; i < numberOfFields; ++i)
    {
      vtkm::cont::SerializableField<FieldTypeList> field;
      vtkmdiy::load(bb, field);
      dataset.AddField(field.Field);
    }
  }
};

} // diy

#endif //vtk_m_cont_DataSet_h
